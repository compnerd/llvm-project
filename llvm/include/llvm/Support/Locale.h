//===--- Locale.h - Locale helpers -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_LOCALE_H
#define LLVM_SUPPORT_LOCALE_H

#include "llvm/Support/LLVMSupportExports.h"

namespace llvm {
class StringRef;

namespace sys {
namespace locale {

LLVM_SUPPORT_ABI int columnWidth(StringRef s);
LLVM_SUPPORT_ABI bool isPrint(int c);

}
}
}

#endif // LLVM_SUPPORT_LOCALE_H
