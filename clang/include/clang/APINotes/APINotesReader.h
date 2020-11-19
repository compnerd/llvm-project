//===-- APINotesReader.h - API Notes Reader ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APINOTES_READER_H
#define LLVM_CLANG_APINOTES_READER_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/VersionTuple.h"
#include <memory>

namespace clang {
namespace api_notes {
class ContextID;
class ObjCContextInfo;
class ObjCPropertyInfo;
class ObjCMethodInfo;
struct ObjCSelectorRef;
class GlobalVariableInfo;
class GlobalFunctionInfo;
class EnumConstantInfo;
class TagInfo;
class TypedefInfo;

class APINotesReader {
  class Implementation;
  std::unique_ptr<Implementation> Implementation;

  APINotesReader(llvm::MemoryBuffer *MB, bool OwnsMB,
                 llvm::VersionTuple SwiftVersion, bool &Result);

public:
  /// Captures the completed versioned information for a particular part of
  /// API notes, including both unversioned API notes and each versioned API
  /// note for that particular entity.
  template <typename T> class VersionedInfo {
    /// The complete set of information.
    llvm::SmallVector<std::pair<llvm::VersionTuple, T>, 1> Info;

    /// The index of the information that is "selected" based on the desired
    /// version or \c Info.size() if nothing is selected.
    unsigned Selected;

  public:
    /// Form an empty set of versioned information.
    VersionedInfo(llvm::NoneType) : Selected(0) {}

    /// Form a versioned info set given the desired version and a set of info.
    VersionedInfo(llvm::VersionTuple Version,
                  llvm::SmallVector<std::pair<llvm::VersionTuple, T>, 1> Info);

    /// Determine whether there is applicable information selected.
    explicit operator bool() const { return Selected != Info.size(); }

    /// Retrieve the information to apply directly to the AST.
    const T &operator*() const {
      assert(static_cast<bool>(*this) && "no information to apply directly");
      return (*this)[Selected].second;
    }

    /// Retrieve the selected index for the information set.
    llvm::Optional<unsigned> getSelected() const {
      return Selected == Info.size() ? llvm::None : Selected;
    }

    /// Return the number of versioned entries we have.
    unsigned size() const { return Info.size(); }

    /// Access all versioned results.
    const std::pair<llvm::VersionTuple, T> *begin() const {
      return Info.begin();
    }
    const std::pair<llvm::VersionTuple, T> *end() const { return Info.end(); }

    /// Access a specific versioned result.
    const std::pair<llvm::VersionTuple, T> &operator[](unsigned Index) const {
      return Info[Index];
    }
  };

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

  /// Retrieve the name of the module for which this reader is providing API
  /// notes.
  llvm::StringRef getModuleName() const;

  /// Retrieve the size and modification time of the source file from
  /// which this API notes file was created, if known.
  llvm::Optional<std::pair<off_t, time_t>>
  getSourceFileSizeAndModificationTime() const;

#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
  /// Retrieve the module options.
  ModuleOptions getModuleOptions() const;
#endif

  /// Look for the context ID of the given Objective-C class.
  ///
  /// \param name The name of the class we're looking for.
  ///
  /// \returns The ID, if known.
  llvm::Optional<ContextID> lookupObjCClassID(llvm::StringRef Name);

  /// Look for information regarding the given Objective-C class.
  ///
  /// \param name The name of the class we're looking for.
  ///
  /// \returns The information about the class, if known.
  VersionedInfo<ObjCContextInfo> lookupObjCClassInfo(llvm::StringRef Name);

  /// Look for the context ID of the given Objective-C protocol.
  ///
  /// \param name The name of the protocol we're looking for.
  ///
  /// \returns The ID of the protocol, if known.
  llvm::Optional<ContextID> lookupObjCProtocolID(llvm::StringRef Name);

  /// Look for information regarding the given Objective-C protocol.
  ///
  /// \param name The name of the protocol we're looking for.
  ///
  /// \returns The information about the protocol, if known.
  VersionedInfo<ObjCContextInfo> lookupObjCProtocolInfo(llvm::StringRef Name);

  /// Look for information regarding the given Objective-C property in the given
  /// context.
  ///
  /// \param Context The ID that references the context we are looking for.
  /// \param Name The name of the property we're looking for.
  /// \param Instance Whether we are looking for an instance property.
  ///
  /// \returns Information about the property, if known.
  VersionedInfo<ObjCPropertyInfo> lookupObjCPropertyInfo(ContextID Context,
                                                         llvm::StringRef Name,
                                                         bool Instance);

  /// Look for information regarding the given Objective-C method in the given
  /// context.
  ///
  /// \param Context The ID that references the context we are looking for.
  /// \param Selector The selector naming the method we're looking for.
  /// \param Instance Whether we are looking for an instance method.
  ///
  /// \returns Information about the method, if known.
  VersionedInfo<ObjCMethodInfo> lookupObjCMethodInfo(ContextID Context,
                                                     ObjCSelectorRef Selector,
                                                     bool Instance);

  /// Look for information regarding the given global variable.
  ///
  /// \param Name The name of the global variable.
  ///
  /// \returns information about the global variable, if known.
  VersionedInfo<GlobalVariableInfo>
  lookupGlobalVariableInfo(llvm::StringRef Name);

  /// Look for information regarding the given global function.
  ///
  /// \param Name The name of the global function.
  ///
  /// \returns information about the global function, if known.
  VersionedInfo<GlobalFunctionInfo>
  lookupGlobalFunctionInfo(llvm::StringRef Name);

  /// Look for information regarding the given enumerator.
  ///
  /// \param Name The name of the enumerator.
  ///
  /// \returns information about the enumerator, if known.
  VersionedInfo<EnumConstantInfo> lookupEnumConstantInfo(llvm::StringRef Name);

  /// Look for information regarding the given tag (struct/union/enum/class).
  ///
  /// \param Name The name of the tag.
  ///
  /// \returns information about the tag, if known.
  VersionedInfo<TagInfo> lookupTagInfo(llvm::StringRef Name);

  /// Look for information regarding the given typedef.
  ///
  /// \param name The name of the typedef.
  ///
  /// \returns information about the typedef, if known.
  VersionedInfo<TypedefInfo> lookupTypedefInfo(llvm::StringRef Name);
};
} // namespace api_notes
} // namespace clang

#endif
