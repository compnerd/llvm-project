//===-- APINotesYAMLCompiler.h - API Notes YAML Format Reader ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APINOTES_APINOTESYAMLCOMPILER_H
#define LLVM_CLANG_APINOTES_APINOTESYAMLCOMPILER_H

#include "llvm/ADT/StringRef.h"

namespace clang {
namespace api_notes {
bool parseAndDumpAPINotes(llvm::StringRef YI);
}
}

#endif
