//===-- APINotesTest.cpp - API Notes Testing Tool ------------------ C++ --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/APINotes/APINotesYAMLCompiler.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Signals.h"

static llvm::cl::list<std::string>
APINotes(llvm::cl::Positional, llvm::cl::desc("[<apinotes> ...]"),
         llvm::cl::Required);

int main(int argc, const char **argv) {
  const bool DisableCrashReporting = true;
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0], DisableCrashReporting);
  llvm::cl::ParseCommandLineOptions(argc, argv);

  for (const auto &Notes : APINotes) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> NotesOrError =
        llvm::MemoryBuffer::getFileOrSTDIN(Notes);
    if (std::error_code EC = NotesOrError.getError()) {
      llvm::errs() << EC.message() << '\n';
      return 1;
    }

    clang::api_notes::parseAndDumpAPINotes((*NotesOrError)->getBuffer());
  }

  return 0;
}
