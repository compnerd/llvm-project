//===-- llvm/Support/Compression.h ---Compression----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains basic functions for compression/uncompression.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_COMPRESSION_H
#define LLVM_SUPPORT_COMPRESSION_H

#include "llvm/Support/DataTypes.h"
#include "llvm/Support/LLVMSupportExports.h"

namespace llvm {
template <typename T> class SmallVectorImpl;
class Error;
class StringRef;

namespace zlib {

static constexpr int NoCompression = 0;
static constexpr int BestSpeedCompression = 1;
static constexpr int DefaultCompression = 6;
static constexpr int BestSizeCompression = 9;

LLVM_SUPPORT_ABI bool isAvailable();

LLVM_SUPPORT_ABI Error compress(StringRef InputBuffer, SmallVectorImpl<char> &CompressedBuffer,
               int Level = DefaultCompression);

LLVM_SUPPORT_ABI Error uncompress(StringRef InputBuffer, char *UncompressedBuffer,
                 size_t &UncompressedSize);

LLVM_SUPPORT_ABI Error uncompress(StringRef InputBuffer,
                 SmallVectorImpl<char> &UncompressedBuffer,
                 size_t UncompressedSize);

uint32_t crc32(StringRef Buffer);

}  // End of namespace zlib

} // End of namespace llvm

#endif
