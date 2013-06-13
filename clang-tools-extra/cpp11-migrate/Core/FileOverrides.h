//===-- Core/FileOverrides.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides types and functionality for dealing with source
/// and header file content overrides.
///
//===----------------------------------------------------------------------===//
#ifndef CPP11_MIGRATE_FILE_OVERRIDES_H
#define CPP11_MIGRATE_FILE_OVERRIDES_H

#include <map>
#include <string>

// Forward Declarations
namespace clang {
class SourceManager;
class FileManager;
} // namespace clang


/// \brief Container storing the file content overrides for a source file.
struct SourceOverrides {
  SourceOverrides(const char *MainFileName)
      : MainFileName(MainFileName) {}

  /// \brief Convenience function for applying this source's overrides to
  /// the given SourceManager.
  void applyOverrides(clang::SourceManager &SM, clang::FileManager &FM) const;

  std::string MainFileName;
  std::string MainFileOverride;
};

/// \brief Maps source file names to content override information.
typedef std::map<std::string, SourceOverrides> FileOverrides;

#endif // CPP11_MIGRATE_FILE_OVERRIDES_H
