//===--- LLVMTidyModule.h - clang-tidy --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_LLVM_TIDY_MODULE_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_LLVM_TIDY_MODULE_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {

/// \brief Checks the correct order of \c #includes.
///
/// see: http://llvm.org/docs/CodingStandards.html#include-style
class IncludeOrderCheck : public ClangTidyCheck {
public:
  void registerPPCallbacks(CompilerInstance &Compiler) override;
};

/// \brief Checks that long namespaces have a closing comment.
///
/// see: http://llvm.org/docs/CodingStandards.html#namespace-indentation
class NamespaceCommentCheck : public ClangTidyCheck {
public:
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_TIDY_LLVM_MODULE_H
