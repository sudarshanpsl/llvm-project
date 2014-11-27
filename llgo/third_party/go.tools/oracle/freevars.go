// Copyright 2013 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package oracle

import (
	"bytes"
	"go/ast"
	"go/printer"
	"go/token"
	"sort"

	"llvm.org/llgo/third_party/go.tools/go/types"
	"llvm.org/llgo/third_party/go.tools/oracle/serial"
)

// freevars displays the lexical (not package-level) free variables of
// the selection.
//
// It treats A.B.C as a separate variable from A to reveal the parts
// of an aggregate type that are actually needed.
// This aids refactoring.
//
// TODO(adonovan): optionally display the free references to
// file/package scope objects, and to objects from other packages.
// Depending on where the resulting function abstraction will go,
// these might be interesting.  Perhaps group the results into three
// bands.
//
func freevars(o *Oracle, qpos *QueryPos) (queryResult, error) {
	file := qpos.path[len(qpos.path)-1] // the enclosing file
	fileScope := qpos.info.Scopes[file]
	pkgScope := fileScope.Parent()

	// The id and sel functions return non-nil if they denote an
	// object o or selection o.x.y that is referenced by the
	// selection but defined neither within the selection nor at
	// file scope, i.e. it is in the lexical environment.
	var id func(n *ast.Ident) types.Object
	var sel func(n *ast.SelectorExpr) types.Object

	sel = func(n *ast.SelectorExpr) types.Object {
		switch x := unparen(n.X).(type) {
		case *ast.SelectorExpr:
			return sel(x)
		case *ast.Ident:
			return id(x)
		}
		return nil
	}

	id = func(n *ast.Ident) types.Object {
		obj := qpos.info.Uses[n]
		if obj == nil {
			return nil // not a reference
		}
		if _, ok := obj.(*types.PkgName); ok {
			return nil // imported package
		}
		if !(file.Pos() <= obj.Pos() && obj.Pos() <= file.End()) {
			return nil // not defined in this file
		}
		scope := obj.Parent()
		if scope == nil {
			return nil // e.g. interface method, struct field
		}
		if scope == fileScope || scope == pkgScope {
			return nil // defined at file or package scope
		}
		if qpos.start <= obj.Pos() && obj.Pos() <= qpos.end {
			return nil // defined within selection => not free
		}
		return obj
	}

	// Maps each reference that is free in the selection
	// to the object it refers to.
	// The map de-duplicates repeated references.
	refsMap := make(map[string]freevarsRef)

	// Visit all the identifiers in the selected ASTs.
	ast.Inspect(qpos.path[0], func(n ast.Node) bool {
		if n == nil {
			return true // popping DFS stack
		}

		// Is this node contained within the selection?
		// (freevars permits inexact selections,
		// like two stmts in a block.)
		if qpos.start <= n.Pos() && n.End() <= qpos.end {
			var obj types.Object
			var prune bool
			switch n := n.(type) {
			case *ast.Ident:
				obj = id(n)

			case *ast.SelectorExpr:
				obj = sel(n)
				prune = true
			}

			if obj != nil {
				var kind string
				switch obj.(type) {
				case *types.Var:
					kind = "var"
				case *types.Func:
					kind = "func"
				case *types.TypeName:
					kind = "type"
				case *types.Const:
					kind = "const"
				case *types.Label:
					kind = "label"
				default:
					panic(obj)
				}

				typ := qpos.info.TypeOf(n.(ast.Expr))
				ref := freevarsRef{kind, printNode(o.fset, n), typ, obj}
				refsMap[ref.ref] = ref

				if prune {
					return false // don't descend
				}
			}
		}

		return true // descend
	})

	refs := make([]freevarsRef, 0, len(refsMap))
	for _, ref := range refsMap {
		refs = append(refs, ref)
	}
	sort.Sort(byRef(refs))

	return &freevarsResult{
		qpos: qpos,
		refs: refs,
	}, nil
}

type freevarsResult struct {
	qpos *QueryPos
	refs []freevarsRef
}

type freevarsRef struct {
	kind string
	ref  string
	typ  types.Type
	obj  types.Object
}

func (r *freevarsResult) display(printf printfFunc) {
	if len(r.refs) == 0 {
		printf(r.qpos, "No free identifiers.")
	} else {
		printf(r.qpos, "Free identifiers:")
		for _, ref := range r.refs {
			// Avoid printing "type T T".
			var typstr string
			if ref.kind != "type" {
				typstr = " " + types.TypeString(r.qpos.info.Pkg, ref.typ)
			}
			printf(ref.obj, "%s %s%s", ref.kind, ref.ref, typstr)
		}
	}
}

func (r *freevarsResult) toSerial(res *serial.Result, fset *token.FileSet) {
	var refs []*serial.FreeVar
	for _, ref := range r.refs {
		refs = append(refs,
			&serial.FreeVar{
				Pos:  fset.Position(ref.obj.Pos()).String(),
				Kind: ref.kind,
				Ref:  ref.ref,
				Type: ref.typ.String(),
			})
	}
	res.Freevars = refs
}

// -------- utils --------

type byRef []freevarsRef

func (p byRef) Len() int           { return len(p) }
func (p byRef) Less(i, j int) bool { return p[i].ref < p[j].ref }
func (p byRef) Swap(i, j int)      { p[i], p[j] = p[j], p[i] }

// printNode returns the pretty-printed syntax of n.
func printNode(fset *token.FileSet, n ast.Node) string {
	var buf bytes.Buffer
	printer.Fprint(&buf, fset, n)
	return buf.String()
}
