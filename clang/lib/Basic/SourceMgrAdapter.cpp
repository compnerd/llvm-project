//===- SourecMgrAdapter - SourceMgr/SourceManager Adapter -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/SourceMgrAdapter.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"

namespace clang {
SourceMgrAdapter::SourceMgrAdapter(SourceManager &SM, DiagnosticsEngine &Diag,
                                   unsigned ErrorDiagID, unsigned WarningDiagID,
                                   unsigned NoteDiagID,
                                   const FileEntry *DefaultFile)
    : SM(SM), Diag(Diag), ErrorDiagID(ErrorDiagID),
      WarningDiagID(WarningDiagID), NotesDiagID(NoteDiagID),
      DefaultFile(DefaultFile) {}

SourceMgrAdapter::~SourceMgrAdapter() {}

clang::SourceLocation SourceMgrAdapter::map(const llvm::SourceMgr &SrcMgr,
                                            llvm::SMLoc SMLoc) {
  if (!SMLoc.isValid())
    return SourceLocation();

  // Find the buffer containing the location.
  unsigned BufferID = SrcMgr.FindBufferContainingLoc(SMLoc);
  if (!BufferID)
    return SourceLocation();

  // If we haven't seen this buffer before, copy it over.
  auto Buffer = SrcMgr.getMemoryBuffer(BufferID);
  auto Mapping = Mappings.find(std::make_pair(&SrcMgr, BufferID));
  if (Mapping == Mappings.end()) {
    FileID FID;
    if (DefaultFile) {
      // Map to the default file.
      FID = SM.createFileID(DefaultFile, SourceLocation(), SrcMgr::C_User);
      // Only do this once.
      DefaultFile = nullptr;
    } else {
      // Make a copy of the memory buffer.
      llvm::StringRef Name = Buffer->getBufferIdentifier();
      auto BufferCopy = std::unique_ptr<llvm::MemoryBuffer>(
          llvm::MemoryBuffer::getMemBufferCopy(Buffer->getBuffer(), Name));

      // Add this memory buffer to the clang Source Manager.
      FID = SM.createFileID(std::move(BufferCopy));
    }

    // Save the mapping.
    Mapping =
        Mappings.insert(std::make_pair(std::make_pair(&SrcMgr, BufferID), FID))
            .first;
  }

  // Translate the offset into the file.
  unsigned Offset = SMLoc.getPointer() - Buffer->getBufferStart();
  return SM.getLocForStartOfFile(Mapping->second).getLocWithOffset(Offset);
}

clang::SourceRange SourceMgrAdapter::map(const llvm::SourceMgr &SrcMgr,
                                         llvm::SMRange SMRange) {
  if (!SMRange.isValid())
    return SourceRange();
  return SourceRange(map(SrcMgr, SMRange.Start), map(SrcMgr, SMRange.End));
}

void SourceMgrAdapter::Emit(const llvm::SMDiagnostic &D) {
  clang::SourceLocation SLoc;
  if (auto *SrcMgr = D.getSourceMgr())
    SLoc = map(*SrcMgr, D.getLoc());

  llvm::StringRef Message = D.getMessage();

  unsigned DiagID;
  switch (D.getKind()) {
  case llvm::SourceMgr::DK_Error:
    DiagID = ErrorDiagID;
    break;
  case llvm::SourceMgr::DK_Warning:
    DiagID = WarningDiagID;
    break;
  case llvm::SourceMgr::DK_Remark:
    llvm_unreachable("remark support not implemented");
  case llvm::SourceMgr::DK_Note:
    DiagID = NotesDiagID;
    break;
  }

  DiagnosticBuilder DB = Diag.Report(SLoc, DiagID) << Message;
  if (auto *SrcMgr = D.getSourceMgr()) {
    SourceLocation Begin = SLoc.getLocWithOffset(-D.getColumnNo());
    for (auto R : D.getRanges())
      DB << SourceRange(Begin.getLocWithOffset(R.first),
                        Begin.getLocWithOffset(R.second));

    for (const llvm::SMFixIt &FI : D.getFixIts())
      DB << FixItHint::CreateReplacement(
          CharSourceRange(map(*SrcMgr, FI.getRange()), false), FI.getText());
  }
}
} // namespace clang
