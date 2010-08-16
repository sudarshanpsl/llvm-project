//===-- ClangResultSynthesizer.h --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ClangResultSynthesizer_h_
#define liblldb_ClangResultSynthesizer_h_

#include "clang/Sema/SemaConsumer.h"
#include "lldb/Core/ClangForward.h"

namespace lldb_private {

//----------------------------------------------------------------------
/// @class ClangResultSynthesizer ClangResultSynthesizer.h "lldb/Expression/ClangResultSynthesizer.h"
/// @brief Adds a result variable declaration to the ASTs for an expression.
///
/// Users expect the expression "i + 3" to return a result, even if a result
/// variable wasn't specifically declared.  To fulfil this requirement, LLDB adds
/// a result variable to the expression, transforming it to 
/// "int ___clang_expr_result = i + 3."  The IR transformers ensure that the
/// resulting variable is mapped to the right piece of memory.
/// ClangResultSynthesizer's job is to add the variable and its initialization to
/// the ASTs for the expression, and it does so by acting as a SemaConsumer for
/// Clang.
//----------------------------------------------------------------------
class ClangResultSynthesizer : public clang::SemaConsumer
{
public:
    //----------------------------------------------------------------------
    /// Constructor
    ///
    /// @param[in] passthrough
    ///     Since the ASTs must typically go through to the Clang code generator
    ///     in order to produce LLVM IR, this SemaConsumer must allow them to
    ///     pass to the next step in the chain after processing.  Passthrough is
    ///     the next ASTConsumer, or NULL if none is required.
    //----------------------------------------------------------------------
    ClangResultSynthesizer(clang::ASTConsumer *passthrough);
    
    //----------------------------------------------------------------------
    /// Destructor
    //----------------------------------------------------------------------
    ~ClangResultSynthesizer();
    
    //----------------------------------------------------------------------
    /// Link this consumer with a particular AST context
    ///
    /// @param[in] Context
    ///     This AST context will be used for types and identifiers, and also
    ///     forwarded to the passthrough consumer, if one exists.
    //----------------------------------------------------------------------
    void Initialize(clang::ASTContext &Context);
    
    //----------------------------------------------------------------------
    /// Examine a list of Decls to find the function ___clang_expr and 
    /// transform its code
    ///
    /// @param[in] D
    ///     The list of Decls to search.  These may contain LinkageSpecDecls,
    ///     which need to be searched recursively.  That job falls to
    ///     TransformTopLevelDecl.
    //----------------------------------------------------------------------
    void HandleTopLevelDecl(clang::DeclGroupRef D);
    
    //----------------------------------------------------------------------
    /// Passthrough stub
    //----------------------------------------------------------------------
    void HandleTranslationUnit(clang::ASTContext &Ctx);
    
    //----------------------------------------------------------------------
    /// Passthrough stub
    //----------------------------------------------------------------------
    void HandleTagDeclDefinition(clang::TagDecl *D);
    
    //----------------------------------------------------------------------
    /// Passthrough stub
    //----------------------------------------------------------------------
    void CompleteTentativeDefinition(clang::VarDecl *D);
    
    //----------------------------------------------------------------------
    /// Passthrough stub
    //----------------------------------------------------------------------
    void HandleVTable(clang::CXXRecordDecl *RD, bool DefinitionRequired);
    
    //----------------------------------------------------------------------
    /// Passthrough stub
    //----------------------------------------------------------------------
    void PrintStats();
    
    //----------------------------------------------------------------------
    /// Set the Sema object to use when performing transforms, and pass it on
    ///
    /// @param[in] S
    ///     The Sema to use.  Because Sema isn't externally visible, this class
    ///     casts it to an Action for actual use.
    //----------------------------------------------------------------------
    void InitializeSema(clang::Sema &S);
    
    //----------------------------------------------------------------------
    /// Reset the Sema to NULL now that transformations are done
    //----------------------------------------------------------------------
    void ForgetSema();
private:
    //----------------------------------------------------------------------
    /// Hunt the given Decl for FunctionDecls named ___clang_expr, recursing
    /// as necessary through LinkageSpecDecls, and calling SynthesizeResult on
    /// anything that was found
    ///
    /// @param[in] D
    ///     The Decl to hunt.
    //----------------------------------------------------------------------
    void TransformTopLevelDecl(clang::Decl *D);
    
    //----------------------------------------------------------------------
    /// Process a function and produce the result variable and initialization
    ///
    /// @param[in] FunDecl
    ///     The function to process.
    //----------------------------------------------------------------------
    bool SynthesizeResult(clang::FunctionDecl *FunDecl);
    
    clang::ASTContext *m_ast_context;           ///< The AST context to use for identifiers and types.
    clang::ASTConsumer *m_passthrough;          ///< The ASTConsumer down the chain, for passthrough.  NULL if it's a SemaConsumer.
    clang::SemaConsumer *m_passthrough_sema;    ///< The SemaConsumer down the chain, for passthrough.  NULL if it's an ASTConsumer.
    clang::Sema *m_sema;                        ///< The Sema to use.
    clang::Action *m_action;                    ///< The Sema to use, cast to an Action so it's usable.
};

}

#endif