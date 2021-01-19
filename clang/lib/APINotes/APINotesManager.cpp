//===-- APINotesManager.cpp - API Notes Manager -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/APINotes/APINotesManager.h"
#include "clang/APINotes/APINotesReader.h"
#include "clang/APINotes/APINotesYAMLCompiler.h"
#include "clang/APINotes/Types.h"
#include "clang/Basic/FileEntry.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/Module.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/SourceMgrAdapter.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/PrettyStackTrace.h"

namespace {
class APINotesPrettyStackTrace : public llvm::PrettyStackTraceEntry {
  llvm::StringRef First, Second;

public:
  APINotesPrettyStackTrace(llvm::StringRef first, llvm::StringRef second)
      : First(first), Second(second) {}

  void print(llvm::raw_ostream &OS) const override { OS << First << Second; }
};
} // namespace

namespace clang {
namespace api_notes {
APINotesManager::APINotesManager(SourceManager &SM, const LangOptions &LO)
    : SM(SM), ImplicitAPINotes(LO.APINotes) {}

APINotesManager::~APINotesManager() {
  for (const auto &RE : Readers)
    if (auto R = RE.second.dyn_cast<APINotesReader *>())
      delete R;

  for (auto &Reader : CurrentModuleReaders)
    delete Reader;
}

std::unique_ptr<APINotesReader>
APINotesManager::loadAPINotes(const FileEntry *FE) {
  APINotesPrettyStackTrace PST("Loading API Notes from ", FE->getName());

  // Open the Source File.
  auto FID = SM.createFileID(FE, SourceLocation(), SrcMgr::C_User);
  auto SourceBuffer = SM.getBufferOrNone(FID);
  if (!SourceBuffer)
    return nullptr;

  // Compile the API Notes source into a buffer.
  //
  // FIXME: Either propagate OSType through or, better yet, improve the binary
  // APINotes format to maintain complete availability information.
  //
  // FIXME: We don't even really need to go through the binary format at all;
  // we're going to immediately deserialize it again.
  llvm::SmallVector<char, 1024> APINotesBuffer;
  std::unique_ptr<llvm::MemoryBuffer> CompiledBuffer;
  {
    SourceMgrAdapter Adapter(SM, SM.getDiagnostics(),
                             diag::err_apinotes_message,
                             diag::warn_apinotes_message,
                             diag::note_apinotes_message, FE);
    llvm::raw_svector_ostream OS(APINotesBuffer);

    llvm::SourceMgr::DiagHandlerTy DiagHandler = [](const llvm::SMDiagnostic &D,
                                                    void *Context) {
      reinterpret_cast<SourceMgrAdapter *>(Context)->Emit(D);
    };
    if (api_notes::compileAPINotes(SourceBuffer->getBuffer(),
                                   SM.getFileEntryForID(FID), OS, DiagHandler,
                                   &Adapter))
      return nullptr;

    // Make a copy of the compiled form into the buffer.
    llvm::StringRef MB{APINotesBuffer.data(), APINotesBuffer.size()};
    CompiledBuffer = llvm::MemoryBuffer::getMemBufferCopy(MB);
  }

  auto Reader = APINotesReader::create(std::move(CompiledBuffer), SwiftVersion);
  assert(Reader && "Unable to load the compiled API Notes?");
  return Reader;
}

bool APINotesManager::loadAPINotes(const DirectoryEntry *DE,
                                   const FileEntry *FE) {
  assert(Readers.find(DE) == Readers.end());
  if (auto Reader = loadAPINotes(FE)) {
    Readers[DE] = Reader.release();
    return false;
  }
  Readers[DE] = nullptr;
  return true;
}

namespace {
bool HasPrivateSubmodules(const clang::Module *M) {
  return llvm::any_of(M->submodules(), [](const clang::Module *SM) {
    return SM->ModuleMapIsPrivate;
  });
}

void CheckPrivateAPINotesName(clang::DiagnosticsEngine &Diag,
                              const FileEntry *FE, const clang::Module *M) {
  llvm::StringRef Path = FE->tryGetRealPathName();

  if (Path.empty())
    return;

  llvm::StringRef FileName = llvm::sys::path::filename(Path);
  llvm::StringRef Stem = llvm::sys::path::stem(FileName);
  if (Stem.endswith("_private"))
    return;

  unsigned DiagID = M->IsSystem
                        ? diag::warn_invalid_private_system_apinotes_path
                        : diag::warn_invalid_private_apinotes_path;
  Diag.Report(SourceLocation(), DiagID) << M->Name << FileName;
}
} // namespace

bool APINotesManager::loadCurrentModuleAPINotes(clang::Module *M,
                                                bool ModuleRelative,
                                                ArrayRef<std::string> Paths) {
  assert(!CurrentModuleReaders[Public] &&
         "already loaded API notes for current module?");

  FileManager &FM = SM.getFileManager();
  auto Name = M->getTopLevelModuleName();

  if (ModuleRelative) {
    bool Found = false;
    unsigned Readers = 0;

    auto LoadFoundAPINotes = [this, &Readers, Name, M](const DirectoryEntry *DE,
                                                       bool Private) -> bool {
      auto *APINotes = findAPINotesFile(DE, Name, Public);
      if (!APINotes)
        return false;

      if (Private)
        CheckPrivateAPINotesName(SM.getDiagnostics(), APINotes, M);

      if (auto *Reader = loadAPINotes(APINotes).release()) {
        CurrentModuleReaders[Readers++] = Reader;
        M->APINotesFile = APINotes->getName().str();
      }

      return true;
    };

    if (M->IsFramework) {
      // For frameworks, search in "Headers" or "PrivteHeaders" subdirectories.
      //
      // Public Modules:
      //  - Headers/Module.apinotes
      //  - PrivateHeaders/Module_private.apinotes
      //      (if there are private submodules)
      // Private Modules:
      //  - PrivateHeaders/Module.apinotes
      //      (except that 'Module' probably already has "Private" in practice)
      llvm::SmallString<128> Path{M->Directory->getName()};

      if (!M->ModuleMapIsPrivate) {
        unsigned Length = Path.size();
        llvm::sys::path::append(Path, "Headers");
        if (auto DE = FM.getDirectory(Path))
          Found |= LoadFoundAPINotes(*DE, /*Private=*/false);
        Path.resize(Length);
      }

      if (M->ModuleMapIsPrivate || HasPrivateSubmodules(M)) {
        unsigned Length = Path.size();
        llvm::sys::path::append(Path, "PrivateHeaders");
        if (auto DE = FM.getDirectory(Path))
          Found |= LoadFoundAPINotes(*DE, /*Private=*/true);
        Path.resize(Length);
      }
    } else {
      // Public Modules:
      //  - Module.apinotes
      //  - Module_private.apinotes
      //      (if there are private submodules)
      // Private Modules:
      //  - Module.apinotes
      //      (except that 'Module' probably already has "Private" in practice)
      Found |= LoadFoundAPINotes(M->Directory, /*Private=*/false);
      if (!M->ModuleMapIsPrivate && HasPrivateSubmodules(M))
        Found |= LoadFoundAPINotes(M->Directory, /*Private=*/true);
    }

    if (Found)
      return Readers > 0;
  }

  for (const auto &Path : Paths) {
    auto Dir = FM.getDirectory(Path);
    if (!Dir)
      continue;

    auto *APINotes = findAPINotesFile(*Dir, Name);
    if (!APINotes)
      continue;

    if ((CurrentModuleReaders[Public] = loadAPINotes(APINotes).release())) {
      M->APINotesFile = APINotes->getName().str();
      return true;
    }
    return false;
  }

  return false;
}

const FileEntry *
APINotesManager::findAPINotesFile(const DirectoryEntry *Directory,
                                  StringRef FileName, bool PrivateNotes) {
  FileManager &FM = SM.getFileManager();

  llvm::SmallString<128> Path{Directory->getName()};
  llvm::sys::path::append(Path, llvm::Twine(FileName) +
                                    (PrivateNotes ? "_private" : "") + "." +
                                    SOURCE_APINOTES_EXTENSION);

  auto File = FM.getFile(Path, /*open=*/true);
  return File ? *File : nullptr;
}
} // namespace api_notes
} // namespace clang
