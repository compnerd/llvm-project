//===-- APINotesWriter.h - API Notes Writer ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APINOTES_WRITER_H
#define LLVM_CLANG_APINOTES_WRITER_H

#include "clang/APINotes/Types.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/VersionTuple.h"

namespace clang {
class FileEntry;

namespace api_notes {
class APINotesWriter {
  class Implementation;
  std::unique_ptr<Implementation> Implementation;

public:
  APINotesWriter(llvm::StringRef ModuleName, const FileEntry *SF);
  ~APINotesWriter();

  APINotesWriter(const APINotesWriter &) = delete;
  APINotesWriter &operator=(const APINotesWriter &) = delete;

  /// Write the API notes data to the given stream.
  void writeToStream(llvm::raw_ostream &OS);

#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
  /// Add module options
  void addModuleOptions(ModuleOptions MO);
#endif

  /// Add information about a specific Objective-C class or protocol.
  ///
  /// \param Name The name of this class/protocol.
  /// \param IsClass Whether this is a class (vs. a protocol).
  /// \param OCI Information about this class/protocol.
  /// \param SwiftVersion
  ///
  /// \returns the ID of the class or protocol, which can be used to add
  /// properties and methods to the class/protocol.
  ContextID addObjCContext(llvm::StringRef Name, bool IsClass,
                           const ObjCContextInfo &OCI,
                           llvm::VersionTuple SwiftVersion);

  /// Add information about a specific Objective-C property.
  ///
  /// \param ContextID The context in which this property resides.
  /// \param Name The name of this property.
  /// \param OPI Information about this property.
  /// \param SwiftVersion
  void addObjCProperty(ContextID ContextID, llvm::StringRef Name,
                       bool IsInstanceProperty, const ObjCPropertyInfo &OPI,
                       llvm::VersionTuple SwiftVersion);

  /// Add information about a specific Objective-C method.
  ///
  /// \param ContextID The context in which this method resides.
  /// \param Selector The selector that names this method.
  /// \param IsInstanceMethod Whether this method is an instance method
  ///        (vs. a class method).
  /// \param OMI Information about this method.
  /// \param SwiftVersion
  void addObjCMethod(ContextID ContextID, ObjCSelectorRef Selector,
                     bool IsInstanceMethod, const ObjCMethodInfo &OMI,
                     llvm::VersionTuple SwiftVersion);

  /// Add information about a global variable.
  ///
  /// \param Name The name of this global variable.
  /// \param GVI Information about this global variable.
  /// \param SwiftVersion
  void addGlobalVariable(llvm::StringRef Name, const GlobalVariableInfo &GVI,
                         llvm::VersionTuple SwiftVersion);

  /// Add information about a global function.
  ///
  /// \param Name The name of this global function.
  /// \param FI Information about this global function.
  /// \param SwiftVersion
  void addGlobalFunction(llvm::StringRef Name, const GlobalFunctionInfo &FI,
                         llvm::VersionTuple SwiftVersion);

  /// Add information about an enumerator.
  ///
  /// \param Name The name of this enumerator.
  /// \param ECI Information about this enumerator.
  /// \param SwiftVersion
  void addEnumConstant(llvm::StringRef Name, const EnumConstantInfo &ECI,
                       llvm::VersionTuple SwiftVersion);

  /// Add information about a tag (struct/union/enum/C++ class).
  ///
  /// \param Name The name of this tag.
  /// \param TI Information about this tag.
  /// \param SwiftVersion
  void addTag(llvm::StringRef Name, const TagInfo &TI,
              llvm::VersionTuple SwiftVersion);

  /// Add information about a typedef.
  ///
  /// \param Name The name of this typedef.
  /// \param TI Information about this typedef.
  /// \param SwiftVersion
  void addTypedef(llvm::StringRef name, const TypedefInfo &TI,
                  llvm::VersionTuple SwiftVersion);
};
} // namespace api_notes
} // namespace clang

#endif
