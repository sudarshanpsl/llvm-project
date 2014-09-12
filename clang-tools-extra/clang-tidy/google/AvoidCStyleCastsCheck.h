//===--- AvoidCStyleCastsCheck.h - clang-tidy -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_AVOID_C_STYLE_CASTS_CHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_AVOID_C_STYLE_CASTS_CHECK_H

#include "../ClangTidy.h"

namespace clang {
namespace tidy {
namespace readability {

/// \brief Finds usages of C-style casts.
///
/// http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml?showone=Casting#Casting
/// Corresponding cpplint.py check name: 'readability/casting'.
///
/// This check is similar to -Wold-style-cast, but it will suggest automated
/// fixes eventually. The reported locations should not be different from the
/// ones generated by -Wold-style-cast.
class AvoidCStyleCastsCheck : public ClangTidyCheck {
public:
  AvoidCStyleCastsCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace readability
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_GOOGLE_AVOID_C_STYLE_CASTS_CHECK_H
