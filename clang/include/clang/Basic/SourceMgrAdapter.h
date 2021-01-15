//===-- SourceMgrAdapter.h - SourceMgr to SourceManager Adapter -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_SOURCE_MGR_ADAPTER_H
#define LLVM_CLANG_BASIC_SOURCE_MGR_ADAPTER_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/SourceMgr.h"

#include <utility>

namespace clang {
class DiagnosticsEngine;
class FileEntry;
class SourceManager;

/// An adapter that can be used to translate diagnostics from one or more
/// llvm::SourceMgr instances to a clang::SourceManager instance.
class SourceMgrAdapter {
  /// Destination clang Source Manager.
  SourceManager &SM;

  /// clang Diagnostics Engine.
  DiagnosticsEngine &Diag;

  /// Diagnostic IDs for errors, warnings, and notes.
  unsigned ErrorDiagID, WarningDiagID, NotesDiagID;

  /// The default file to use when mapping buffers.
  const FileEntry *DefaultFile;

  /// Mappings from (LLVM Source Manager, Buffer ID) pairs to the corresponding
  /// FileID within the clang Source Manager.
  llvm::DenseMap<std::pair<const llvm::SourceMgr *, unsigned>, FileID> Mappings;

  SourceLocation map(const llvm::SourceMgr &SrcMgr, llvm::SMLoc SMLoc);
  SourceRange map(const llvm::SourceMgr &SrcMgr, llvm::SMRange SMRange);

public:
  SourceMgrAdapter(SourceManager &SM, DiagnosticsEngine &Diag,
                   unsigned ErrorDiagID, unsigned WarningDiagID,
                   unsigned NoteDiagID, const FileEntry *DefaultFile = nullptr);
  ~SourceMgrAdapter();

  void Emit(const llvm::SMDiagnostic &D);
};
} // namespace clang

#endif
