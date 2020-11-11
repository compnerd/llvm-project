//===-- APINotesReader.h - API Notes Reader ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APINOTES_READER_H
#define LLVM_CLANG_APINOTES_READER_H

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VersionTuple.h"
#include <memory>

namespace clang {
namespace api_notes {
class APINotesReader {
  class Implementation;
  std::unique_ptr<Implementation> Implementation;

  APINotesReader(llvm::MemoryBuffer *MB, bool OwnsMB,
                 llvm::VersionTuple SwiftVersion, bool &Result);

public:
  ~APINotesReader();

  APINotesReader(const APINotesReader &) = delete;
  APINotesReader &operator=(const APINotesReader &) = delete;

  /// Create a new API notes reader from the given member buffer, which contains
  /// the contents of a binary API notes file.
  ///
  /// \returns the new API notes reader, or null if an error occurred.
  static std::unique_ptr<APINotesReader>
  create(std::unique_ptr<llvm::MemoryBuffer> MB,
         llvm::VersionTuple SwiftVersion);
};
} // namespace api_notes
} // namespace clang

#endif
