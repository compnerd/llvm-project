//===--- SupportMacros.h - ABI Macro Definitions ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_LLVMSUPPORT_EXPORTS_H
#define LLVM_SUPPORT_LLVMSUPPORT_EXPORTS_H

#if defined(__ELF__)
# if defined(LLVM_SUPPORT_STATIC)
#   define LLVM_SUPPORT_ABI
# else
#   if defined(LLVMSupport_EXPORTS)
#     define LLVM_SUPPORT_ABI __attribute__((__visibility__("protected")))
#   else
#     define LLVM_SUPPORT_ABI __attribute__((__visibility__("default")))
#   endif
# endif
#elif defined(__MACH__) || defined(__WASM__)
# if defined(LLVM_SUPPORT_STATIC)
#   define LLVM_SUPPORT_ABI
# else
#   define LLVM_SUPPORT_ABI __attribute__((__visibility__("default")))
# endif
#else
# if defined(LLVM_SUPPORT_STATIC)
#   define LLVM_SUPPORT_ABI
# else
#   if defined(LLVMSupport_EXPORTS)
#     define LLVM_SUPPORT_ABI __declspec(dllexport)
#   else
#     define LLVM_SUPPORT_ABI __declspec(dllimport)
#   endif
# endif
#endif

#endif
