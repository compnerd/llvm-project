//===-- APINotesManager.h - API Notes Manager -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APINOTES_MANAGER_H
#define LLVM_CLANG_APINOTES_MANAGER_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/VersionTuple.h"

namespace clang {
class DirectoryEntry;
class FileEntry;
class LangOptions;
class Module;
class SourceManager;

namespace api_notes {
class APINotesReader;

/// The API Notes Manager helps find API notes associated with declarations.
///
/// API notes are externally-provided annotations for declarations that can
/// introduce new attributes (covering availability, nullability of parameters
/// and results, etc) for specific declarations without directly modifying the
/// headers containing the declarations.
///
/// The API Notes Manager is responsible for finding and loading the external
/// API notes files that correspond to a given header.  Its primary operation is
/// \c findAPINotes(), which finds the API notes reader that provides the
/// information about the declarations at that location.
class APINotesManager {
  using ReaderEntry =
        llvm::PointerUnion<const DirectoryEntry *, APINotesReader *>;

  SourceManager &SM;

  /// Whether or implicitly search for API notes files based on the source file
  //from which an entity was declared.
  bool ImplicitAPINotes;

  /// The Swift version to use when interpreting versioned API notes.
  llvm::VersionTuple SwiftVersion;

  /// API notes readers for the current module.
  ///
  /// There can be up to two of these, one for public headers and one for
  /// private headers.
  enum { Public, Private, CurrentReaders };
  APINotesReader *CurrentModuleReaders[CurrentReaders] = { nullptr, nullptr };

  /// A mapping from header file directories to the API notes reader for that
  /// directory, or a redirection to another directory entry that may have more
  /// information, or \c NULL to indicate that there is no API notes reader for
  /// this directory.
  llvm::DenseMap<const DirectoryEntry *, ReaderEntry> Readers;

  /// Load the API notes associated with the given file, whether it is the
  /// binary or source form of the API notes.
  ///
  /// \returns the API notes reader for this file, or \c nullptr if there is a
  /// failure.
  std::unique_ptr<APINotesReader> loadAPINotes(const FileEntry *FE);

  /// Load the given API notes file for the given header directory.
  ///
  /// \param DE the directory at which we can find the header.
  /// \param FE the API notes file entry in the directory.
  ///
  /// \returns true if an error occurred.
  bool loadAPINotes(const DirectoryEntry *DE, const FileEntry *FE);

  /// Look for API notes in the given directory.
  ///
  /// This might find either a binary or source API notes file.
  const FileEntry *findAPINotesFile(const DirectoryEntry *Directory, StringRef
                                    FileName, bool PrivateNotes = false);

public:
  APINotesManager(SourceManager &SM, const LangOptions &LO);
  ~APINotesManager();

  /// Set the Swift version to use when filtering API notes.
  void setSwiftVersion(llvm::VersionTuple Version) {
    SwiftVersion = Version;
  }

  /// Load the API notes for the current module.
  ///
  /// \param M the current module.
  /// \param ModuleRelative whether to look inside the module itself.
  /// \param SearchPaths the patsh in which we should search for API notes for
  ///        the current module.
  ///
  /// \returns true if API notes were successfully loaded, \c false otherwise.
  bool loadCurrentModuleAPINotes(clang::Module *M, bool ModuleRelative,
                                 llvm::ArrayRef<std::string> SearchPaths);

  /// Retrieve the set of API notes readers for the current module.
  ArrayRef<APINotesReader *> getCurrentModuleReaders() const {
    unsigned Readers = 0;
    for (const auto *R : CurrentModuleReaders) {
      if (R == nullptr)
        break;
      ++Readers;
    }
    return llvm::makeArrayRef(CurrentModuleReaders).slice(0, Readers);
  }

  /// Find the APINotes reader that corresponds to the given source location.
  llvm::SmallVector<APINotesReader *, 2> findAPINotes(SourceLocation SLoc);
};
}
}

#endif
