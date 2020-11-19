//===-- APINotesReader.cpp - API Notes Reader -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/APINotes/APINotesReader.h"
#include "APINotesFormat.h"
#include "clang/APINotes/Types.h"
#include "llvm/Bitstream/BitstreamReader.h"
#include "llvm/Support/DJB.h"
#include "llvm/Support/OnDiskHashTable.h"

namespace clang {
namespace api_notes {
namespace {
/// Used to deserialize the on-disk identifier table.
class IdentifierTableInfo {
public:
  using internal_key_type = llvm::StringRef;
  using external_key_type = llvm::StringRef;
  using data_type = clang::api_notes::IdentifierID;
  using hash_value_type = uint32_t;
  using offset_type = unsigned;

  internal_key_type GetInternalKey(external_key_type Key) { return Key; }

  external_key_type GetExternalKey(internal_key_type Key) { return Key; }

  hash_value_type ComputeHash(internal_key_type Key) {
    return llvm::djbHash(Key);
  }

  static bool EqualKey(internal_key_type LHS, internal_key_type RHS) {
    return LHS == RHS;
  }

  static std::pair<unsigned, unsigned> ReadKeyDataLength(const uint8_t *&Data) {
    unsigned KeyLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    unsigned DataLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return {KeyLength, DataLength};
  }

  static internal_key_type ReadKey(const uint8_t *Data, unsigned Length) {
    return llvm::StringRef{reinterpret_cast<const char *>(Data), Length};
  }

  static data_type ReadData(internal_key_type Key, const uint8_t *Data,
                            unsigned Length) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }
};

/// Used to deserialize the on-disk Objective-C class table.
class ObjCContextIDTableInfo {
public:
  // identifier ID, is-protocol
  using internal_key_type = std::pair<unsigned, char>;
  using external_key_type = internal_key_type;
  using data_type = unsigned;
  using hash_value_type = size_t;
  using offset_type = unsigned;

  internal_key_type GetInternalKey(external_key_type Key) { return Key; }

  external_key_type GetExternalKey(internal_key_type Key) { return Key; }

  hash_value_type ComputeHash(internal_key_type Key) {
    return static_cast<size_t>(llvm::hash_value(Key));
  }

  static bool EqualKey(internal_key_type LHS, internal_key_type RHS) {
    return LHS == RHS;
  }

  static std::pair<unsigned, unsigned> ReadKeyDataLength(const uint8_t *&Data) {
    unsigned KeyLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    unsigned DataLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return {KeyLength, DataLength};
  }

  static internal_key_type ReadKey(const uint8_t *Data, unsigned Length) {
    auto Name = llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                                llvm::support::unaligned>(Data);
    auto IsProtocol =
        llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return {Name, IsProtocol};
  }

  static data_type ReadData(internal_key_type, const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }
};

/// Deserialize a version tuple.
llvm::VersionTuple readVersionTuple(const uint8_t *&Data) {
  uint8_t Components = (*Data++) & 0x3;

  unsigned Major =
      llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  if (Components == 1)
    return llvm::VersionTuple{Major};

  unsigned Minor =
      llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  if (Components == 2)
    return llvm::VersionTuple{Major, Minor};

  unsigned Patch =
      llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  if (Components == 3)
    return llvm::VersionTuple{Major, Minor, Patch};

  unsigned Build =
      llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);

  return llvm::VersionTuple{Major, Minor, Patch, Build};
}

/// An on-disk hash table whose data is versioned based on the Swift version.
template <typename Derived, typename KeyType, typename UnversionedDataType>
class VersionedTableInfo {
public:
  using internal_key_type = KeyType;
  using external_key_type = KeyType;
  using data_type =
      llvm::SmallVector<std::pair<llvm::VersionTuple, UnversionedDataType>, 1>;
  using hash_value_type = size_t;
  using offset_type = unsigned;
  using unversioned_type = UnversionedDataType;

  internal_key_type GetInternalKey(external_key_type Key) { return Key; }

  external_key_type GetExternalKey(internal_key_type Key) { return Key; }

  hash_value_type ComputeHash(internal_key_type Key) {
    return static_cast<size_t>(llvm::hash_value(Key));
  }

  static bool EqualKey(internal_key_type LHS, internal_key_type RHS) {
    return LHS == RHS;
  }

  static std::pair<unsigned, unsigned> ReadKeyDataLength(const uint8_t *&Data) {
    unsigned KeyLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    unsigned DataLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return {KeyLength, DataLength};
  }

  static data_type ReadData(internal_key_type Key, const uint8_t *Data,
                            unsigned) {
    unsigned EE =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    data_type Value;
    Value.reserve(EE);
    for (unsigned EI = 0; EI != EE; ++EI) {
      llvm::VersionTuple Version = readVersionTuple(Data);
      auto Ptr = Data;
      (void)Ptr;
      auto UnversionedData = Derived::readUnversioned(Key, Data);
      assert(Data != Ptr && "Unversioned data reader didn't move pointer");
      Value.emplace_back(Version, UnversionedData);
    }
    return Value;
  }
};

/// Read serialized CommonEntityInfo.
void readCommonEntityInfo(const uint8_t *&Data, CommonEntityInfo &CEI) {
  uint8_t Unavailable = *Data++;
  unsigned Length;

  CEI.UnavailableInSwift = Unavailable & 0x1;
  CEI.Unavailable = (Unavailable >> 1) & 0x1;

  if (Unavailable >> 2 & 0x1)
    CEI.setSwiftPrivate(static_cast<bool>(Unavailable >> 3 & 0x1));

  Length = llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  CEI.UnavailableMsg =
      std::string{reinterpret_cast<const char *>(Data), Length};
  Data += Length;

  Length = llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  CEI.SwiftName = std::string{reinterpret_cast<const char *>(Data), Length};
  Data += Length;
}

/// Read serialized CommonTypeInfo.
void readCommonTypeInfo(const uint8_t *&Data, CommonTypeInfo &CTI) {
  readCommonEntityInfo(Data, CTI);

  unsigned Length;

  Length = llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  if (Length > 0) {
    CTI.setSwiftBridge(
        std::string{reinterpret_cast<const char *>(Data), Length - 1});
    Data += Length - 1;
  }

  Length = llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  if (Length > 0) {
    CTI.setNSErrorDomain(
        std::string{reinterpret_cast<const char *>(Data), Length - 1});
    Data += Length - 1;
  }
}

/// Used to deserialize the on-disk Objective-C property table.
class ObjCContextInfoTableInfo
    : public VersionedTableInfo<ObjCContextInfoTableInfo, unsigned,
                                ObjCContextInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static ObjCContextInfo readUnversioned(internal_key_type Key,
                                         const uint8_t *&Data) {
    ObjCContextInfo OCI;

    readCommonTypeInfo(Data, OCI);

    uint8_t Flags = *Data++;

    if (Flags & 0x1)
      OCI.setHasDesignatedInits(true);
    Flags >>= 1;

    if (Flags & 0x4)
      OCI.setDefaultNullability(static_cast<NullabilityKind>(Flags & 0x3));
    Flags >>= 3;

    if (Flags & 0x2)
      OCI.setSwiftObjCMembers(Flags & 0x1);
    Flags >>= 2;

    if (Flags & 0x2)
      OCI.setSwiftImportAsNonGeneric(Flags & 0x1);

    return OCI;
  }
};

/// Read serialized VariableInfo.
void readVariableInfo(const uint8_t *&Data, VariableInfo &VI) {
  readCommonEntityInfo(Data, VI);

  if (*Data++)
    VI.setNullabilityAudited(static_cast<NullabilityKind>(*Data));
  ++Data;

  unsigned Length =
      llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  VI.setType(std::string{Data, Data + Length});
  Data += Length;
}

/// Used to deserialize the on-disk Objective-C property table.
class ObjCPropertyTableInfo
    : public VersionedTableInfo<ObjCPropertyTableInfo,
                                std::tuple<unsigned, unsigned, char>,
                                ObjCPropertyInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    auto ClassID =
        llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    auto NameID =
        llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    auto IsInstance =
        llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return std::make_tuple(ClassID, NameID, IsInstance);
  }

  static ObjCPropertyInfo readUnversioned(internal_key_type,
                                          const uint8_t *&Data) {
    ObjCPropertyInfo OPI;

    readVariableInfo(Data, OPI);

    uint8_t Flags = *Data++;
    if (Flags & 0x1)
      OPI.setSwiftImportAsAccessors(Flags & (1 << 1));

    return OPI;
  }
};

/// Read serialized ParamInfo.
void readParamInfo(const uint8_t *&Data, ParamInfo &PI) {
  readVariableInfo(Data, PI);

  uint8_t Flags =
      llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  if (auto Convention = Flags & 0x7)
    PI.setRetainCountConvention(
        static_cast<RetainCountConventionKind>(Convention - 1));
  Flags >>= 3;

  if (Flags & 0x1)
    PI.setNoEscape(Flags & 0x2);
  Flags >>= 2;
}

/// Read serialized FunctionInfo.
void readFunctionInfo(const uint8_t *&Data, FunctionInfo &FI) {
  readCommonEntityInfo(Data, FI);

  uint8_t Flags =
      llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);

  if (auto Convention = Flags & 0x7)
    FI.setRetainCountConvention(
        static_cast<RetainCountConventionKind>(Convention - 1));
  Flags >>= 3;

  FI.NullabilityAudited = Flags & 0x1;
  Flags >>= 1;

  FI.NumAdjustedNullable =
      llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  FI.NullabilityPayload =
      llvm::support::endian::readNext<uint64_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);

  unsigned Params =
      llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  for (; Params; --Params) {
    ParamInfo PI;
    readParamInfo(Data, PI);
    FI.Params.push_back(PI);
  }

  unsigned Length =
      llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                      llvm::support::unaligned>(Data);
  FI.ResultType = std::string(Data, Data + Length);
  Data += Length;
}

/// Used to deserialize the on-disk Objective-C method table.
class ObjCMethodTableInfo
    : public VersionedTableInfo<ObjCMethodTableInfo,
                                std::tuple<unsigned, unsigned, char>,
                                ObjCMethodInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    auto ClassID =
        llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    auto SelectorID =
        llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    auto IsInstance =
        llvm::support::endian::readNext<uint8_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return internal_key_type{ClassID, SelectorID, IsInstance};
  }

  static ObjCMethodInfo readUnversioned(internal_key_type,
                                        const uint8_t *&Data) {
    ObjCMethodInfo OMI;

    uint8_t Flags = *Data++;

    OMI.RequiredInit = Flags & 0x1;
    Flags >>= 1;

    OMI.DesignatedInit = Flags & 0x1;
    Flags >>= 1;

    readFunctionInfo(Data, OMI);

    return OMI;
  }
};

/// Used to deserialize the on-disk Objective-C selector table.
class ObjCSelectorTableInfo {
public:
  using internal_key_type = StoredObjCSelector;
  using external_key_type = internal_key_type;
  using data_type = SelectorID;
  using hash_value_type = unsigned;
  using offset_type = unsigned;

  internal_key_type GetInternalKey(external_key_type Key) { return Key; }

  external_key_type GetExternalKey(internal_key_type Key) { return Key; }

  hash_value_type ComputeHash(internal_key_type key) {
    return llvm::DenseMapInfo<StoredObjCSelector>::getHashValue(key);
  }

  static bool EqualKey(internal_key_type LHS, internal_key_type RHS) {
    return llvm::DenseMapInfo<StoredObjCSelector>::isEqual(LHS, RHS);
  }

  static std::pair<unsigned, unsigned> ReadKeyDataLength(const uint8_t *&Data) {
    unsigned KeyLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);
    unsigned DataLength =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    return {KeyLength, DataLength};
  }

  static internal_key_type ReadKey(const uint8_t *Data, unsigned Length) {
    internal_key_type Key;
    Key.NumPieces =
        llvm::support::endian::readNext<uint16_t, llvm::support::little,
                                        llvm::support::unaligned>(Data);

    unsigned Idents = (Length - sizeof(uint16_t)) / sizeof(uint32_t);
    ;
    for (unsigned Ident = 0; Ident != Idents; ++Ident)
      Key.Identifiers.push_back(
          llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                          llvm::support::unaligned>(Data));
    return Key;
  }

  static data_type ReadData(internal_key_type, const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }
};

/// Used to deserialize the on-disk global variable table.
class GlobalVariableTableInfo
    : public VersionedTableInfo<GlobalVariableTableInfo, unsigned,
                                GlobalVariableInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static GlobalVariableInfo readUnversioned(internal_key_type,
                                            const uint8_t *&Data) {
    GlobalVariableInfo GVI;
    readVariableInfo(Data, GVI);
    return GVI;
  }
};

/// Used to deserialize the on-disk global function table.
class GlobalFunctionTableInfo
    : public VersionedTableInfo<GlobalFunctionTableInfo, unsigned,
                                GlobalFunctionInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static GlobalFunctionInfo readUnversioned(internal_key_type,
                                            const uint8_t *&Data) {
    GlobalFunctionInfo GFI;
    readFunctionInfo(Data, GFI);
    return GFI;
  }
};

/// Used to deserialize the on-disk enumerator table.
class EnumConstantTableInfo
    : public VersionedTableInfo<EnumConstantTableInfo, unsigned,
                                EnumConstantInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static EnumConstantInfo readUnversioned(internal_key_type,
                                          const uint8_t *&Data) {
    EnumConstantInfo ECI;
    readCommonEntityInfo(Data, ECI);
    return ECI;
  }
};

/// Used to deserialize the on-disk tag table.
class TagTableInfo
    : public VersionedTableInfo<TagTableInfo, unsigned, TagInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static TagInfo readUnversioned(internal_key_type, const uint8_t *&Data) {
    TagInfo TI;

    uint8_t Flags = *Data++;

    if (Flags & 0x1)
      TI.setFlagEnum(Flags & 0x2);
    Flags >>= 2;

    if (Flags)
      TI.EnumExtensibility =
          static_cast<EnumExtensibilityKind>((Flags & 0x3) - 1);

    readCommonTypeInfo(Data, TI);
    return TI;
  }
};

/// Used to deserialize the on-disk typedef table.
class TypedefTableInfo
    : public VersionedTableInfo<TypedefTableInfo, unsigned, TypedefInfo> {
public:
  static internal_key_type ReadKey(const uint8_t *Data, unsigned) {
    return llvm::support::endian::readNext<uint32_t, llvm::support::little,
                                           llvm::support::unaligned>(Data);
  }

  static TypedefInfo readUnversioned(internal_key_type, const uint8_t *&Data) {
    TypedefInfo TI;

    uint8_t Flags = *Data++;

    if (Flags)
      TI.SwiftWrapper = static_cast<SwiftNewTypeKind>((Flags & 0x3) - 1);

    readCommonTypeInfo(Data, TI);
    return TI;
  }
};
} // namespace

class APINotesReader::Implementation {
  /// The input buffer for the API notes data.
  llvm::MemoryBuffer *MB;

  /// Whether we own the input buffer.
  bool OwnsMB;

public:
  /// The Swift version to use for filtering.
  llvm::VersionTuple SwiftVersion;

  /// The name of the module that we read from the control block.
  std::string ModuleName;

#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
  ModuleOptions ModuleOpts;
#endif

  // The size and modification time of the source file from which this API notes
  // file was created, if known.
  llvm::Optional<std::pair<off_t, time_t>> SourceFileSizeAndModificationTime;

  using SerializedIdentifierTable =
      llvm::OnDiskIterableChainedHashTable<IdentifierTableInfo>;

  /// The identifier table.
  std::unique_ptr<SerializedIdentifierTable> IdentifierTable;

  using SerializedObjCContextIDTable =
      llvm::OnDiskIterableChainedHashTable<ObjCContextIDTableInfo>;

  /// The Objective-C context ID table.
  std::unique_ptr<SerializedObjCContextIDTable> ObjCContextIDTable;

  using SerializedObjCContextInfoTable =
      llvm::OnDiskIterableChainedHashTable<ObjCContextInfoTableInfo>;

  /// The Objective-C context info table.
  std::unique_ptr<SerializedObjCContextInfoTable> ObjCContextInfoTable;

  using SerializedObjCPropertyTable =
      llvm::OnDiskIterableChainedHashTable<ObjCPropertyTableInfo>;

  /// The Objective-C property table.
  std::unique_ptr<SerializedObjCPropertyTable> ObjCPropertyTable;

  using SerializedObjCMethodTable =
      llvm::OnDiskIterableChainedHashTable<ObjCMethodTableInfo>;

  /// The Objective-C method table.
  std::unique_ptr<SerializedObjCMethodTable> ObjCMethodTable;

  using SerializedObjCSelectorTable =
      llvm::OnDiskIterableChainedHashTable<ObjCSelectorTableInfo>;

  /// The Objective-C selector table.
  std::unique_ptr<SerializedObjCSelectorTable> ObjCSelectorTable;

  using SerializedGlobalVariableTable =
      llvm::OnDiskIterableChainedHashTable<GlobalVariableTableInfo>;

  /// The global variable table.
  std::unique_ptr<SerializedGlobalVariableTable> GlobalVariableTable;

  using SerializedGlobalFunctionTable =
      llvm::OnDiskIterableChainedHashTable<GlobalFunctionTableInfo>;

  /// The global function table.
  std::unique_ptr<SerializedGlobalFunctionTable> GlobalFunctionTable;

  using SerializedEnumConstantTable =
      llvm::OnDiskIterableChainedHashTable<EnumConstantTableInfo>;

  /// The enumerator table.
  std::unique_ptr<SerializedEnumConstantTable> EnumConstantTable;

  using SerializedTagTable = llvm::OnDiskIterableChainedHashTable<TagTableInfo>;

  /// The tag table.
  std::unique_ptr<SerializedTagTable> TagTable;

  using SerializedTypedefTable =
      llvm::OnDiskIterableChainedHashTable<TypedefTableInfo>;

  /// The typedef table.
  std::unique_ptr<SerializedTypedefTable> TypedefTable;

public:
  Implementation(llvm::MemoryBuffer *MB, bool OwnsMB,
                 llvm::VersionTuple SwiftVersion)
      : MB(MB), OwnsMB(OwnsMB), SwiftVersion(SwiftVersion) {}

  ~Implementation() {
    if (OwnsMB)
      delete MB;
  }

  bool readControlBlock(llvm::BitstreamCursor &Cursor,
                        llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readIdentifierBlock(llvm::BitstreamCursor &Cursor,
                           llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readObjCContextBlock(llvm::BitstreamCursor &Cursor,
                            llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readObjCPropertyBlock(llvm::BitstreamCursor &Cursor,
                             llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readObjCMethodBlock(llvm::BitstreamCursor &Cursor,
                           llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readObjCSelectorBlock(llvm::BitstreamCursor &Cursor,
                             llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readGlobalVariableBlock(llvm::BitstreamCursor &Cursor,
                               llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readGlobalFunctionBlock(llvm::BitstreamCursor &Cursor,
                               llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readEnumConstantBlock(llvm::BitstreamCursor &Cursor,
                             llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readTagBlock(llvm::BitstreamCursor &Cursor,
                    llvm::SmallVectorImpl<uint64_t> &Scratch);
  bool readTypedefBlock(llvm::BitstreamCursor &Cursor,
                        llvm::SmallVectorImpl<uint64_t> &Scratch);

  bool Load();

  /// Retrieve the identifier ID for the given string.
  llvm::Optional<IdentifierID> getIdentifier(llvm::StringRef Identifier);

  /// Retrieve the selector ID for the given selector.
  llvm::Optional<SelectorID> getSelector(ObjCSelectorRef Selector);

  template <typename TableType>
  APINotesReader::VersionedInfo<
      typename TableType::base_type::InfoType::unversioned_type>
  lookup(std::unique_ptr<TableType> &Table, llvm::StringRef Name) {
    if (!Table)
      return llvm::None;

    llvm::Optional<IdentifierID> IID = getIdentifier(Name);
    if (!IID)
      return llvm::None;

    auto KnownInfo = Table->find(*IID);
    if (KnownInfo == Table->end())
      return llvm::None;

    return {SwiftVersion, *KnownInfo};
  }
};

namespace {
template <unsigned BlockID>
bool DeserializeBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch,
    llvm::function_ref<bool(unsigned, llvm::StringRef)> Handler) {
  if (Cursor.EnterSubBlock(BlockID))
    return false;

  llvm::Expected<llvm::BitstreamEntry> Entry = Cursor.advance();
  if (!Entry) {
    // FIXME this drops the error on the floor.
    consumeError(Entry.takeError());
    return false;
  }

  while (Entry->Kind != llvm::BitstreamEntry::EndBlock) {
    if (Entry->Kind == llvm::BitstreamEntry::Error)
      return false;

    if (Entry->Kind == llvm::BitstreamEntry::SubBlock) {
      // Unknown metadata sub-block, possibly for use by a future version of
      // the API notes format.
      if (Cursor.SkipBlock())
        return false;

      Entry = Cursor.advance();
      if (!Entry) {
        // FIXME this drops the error on the floor.
        consumeError(Entry.takeError());
        return false;
      }
      continue;
    }

    Scratch.clear();
    llvm::StringRef BlobData;
    llvm::Expected<unsigned> Kind =
        Cursor.readRecord(Entry->ID, Scratch, &BlobData);
    if (!Kind) {
      // FIXME this drops the error on the floor.
      consumeError(Kind.takeError());
      return false;
    }

    if (!Handler(*Kind, BlobData.data()))
      return false;

    Entry = Cursor.advance();
    if (!Entry) {
      // FIXME this drops the error on the floor.
      consumeError(Entry.takeError());
      return false;
    }
  }

  return true;
}
} // namespace

bool APINotesReader::Implementation::readControlBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  bool SawMetadata = false;
  return DeserializeBlock<CONTROL_BLOCK_ID>(
             Cursor, Scratch,
             [this, &Scratch, &SawMetadata](unsigned Kind,
                                            llvm::StringRef BlobData) -> bool {
               switch (Kind) {
               case control_block::METADATA:
                 if (SawMetadata)
                   return false;

                 if (Scratch[0] == VERSION_MAJOR &&
                     Scratch[1] == VERSION_MINOR) {
                   SawMetadata = true;
                   break;
                 }
                 return false;

               case control_block::MODULE_NAME:
                 ModuleName = BlobData.str();
                 break;

#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
               case control_block::MODULE_OPTIONS:
                 ModuleOpts.SwiftInferImportAsMember =
                     (Scratch.front() & 1) != 0;
                 break;
#endif

               case control_block::SOURCE_FILE:
                 SourceFileSizeAndModificationTime = {Scratch[0], Scratch[1]};
                 break;

               default:
                 // Unknown metadata record, possibly for use by a future
                 // version of the module format.
                 break;
               }
               return true;
             }) &&
         SawMetadata;
}

bool APINotesReader::Implementation::readIdentifierBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<IDENTIFIER_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case identifier_block::IDENTIFIER_DATA: {
          // Already saw identifier table.
          if (IdentifierTable)
            return false;

          uint32_t Offset;
          identifier_block::IdentifierDataLayout::readRecord(Scratch, Offset);
          auto *Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          IdentifierTable.reset(SerializedIdentifierTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));
          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readObjCContextBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<OBJC_CONTEXT_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case objc_context_block::OBJC_CONTEXT_ID_DATA: {
          // Already saw Objective-C context ID table.
          if (ObjCContextIDTable)
            return false;

          uint32_t Offset;
          objc_context_block::ObjCContextIDLayout::readRecord(Scratch, Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          ObjCContextIDTable.reset(SerializedObjCContextIDTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));
          break;
        }

        case objc_context_block::OBJC_CONTEXT_INFO_DATA: {
          // Already saw Objective-C context info table.
          if (ObjCContextInfoTable)
            return false;

          uint32_t Offset;
          objc_context_block::ObjCContextInfoLayout::readRecord(Scratch,
                                                                Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          ObjCContextInfoTable.reset(SerializedObjCContextInfoTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));
          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readObjCPropertyBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<OBJC_PROPERTY_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case objc_property_block::OBJC_PROPERTY_DATA: {
          // Already saw Objective-C property table.
          if (ObjCPropertyTable)
            return false;

          uint32_t Offset;
          objc_property_block::ObjCPropertyDataLayout::readRecord(Scratch,
                                                                  Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          ObjCPropertyTable.reset(SerializedObjCPropertyTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readObjCMethodBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<OBJC_METHOD_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case objc_method_block::OBJC_METHOD_DATA: {
          // Already saw Objective-C method table.
          if (ObjCMethodTable)
            return false;

          uint32_t Offset;
          objc_method_block::ObjCMethodDataLayout::readRecord(Scratch, Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          ObjCMethodTable.reset(SerializedObjCMethodTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readObjCSelectorBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<OBJC_SELECTOR_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case objc_selector_block::OBJC_SELECTOR_DATA: {
          // Already saw Objective-C selector table.
          if (ObjCSelectorTable)
            return false;

          uint32_t Offset;
          objc_selector_block::ObjCSelectorDataLayout::readRecord(Scratch,
                                                                  Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          ObjCSelectorTable.reset(SerializedObjCSelectorTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));
          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readGlobalVariableBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<GLOBAL_VARIABLE_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case global_variable_block::GLOBAL_VARIABLE_DATA: {
          // Already saw global variable table.
          if (GlobalVariableTable)
            return false;

          uint32_t Offset;
          global_variable_block::GlobalVariableDataLayout::readRecord(Scratch,
                                                                      Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          GlobalVariableTable.reset(SerializedGlobalVariableTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));
          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readGlobalFunctionBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<GLOBAL_FUNCTION_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case global_function_block::GLOBAL_FUNCTION_DATA: {
          // Already saw global function table.
          if (GlobalFunctionTable)
            return false;

          uint32_t Offset;
          global_function_block::GlobalFunctionDataLayout::readRecord(Scratch,
                                                                      Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          GlobalFunctionTable.reset(SerializedGlobalFunctionTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readEnumConstantBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<ENUM_CONSTANT_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case enum_constant_block::ENUM_CONSTANT_DATA: {
          // Already saw enumerator table.
          if (EnumConstantTable)
            return false;

          uint32_t Offset;
          enum_constant_block::EnumConstantDataLayout::readRecord(Scratch,
                                                                  Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          EnumConstantTable.reset(SerializedEnumConstantTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readTagBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<TAG_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case tag_block::TAG_DATA: {
          // Already saw tag table.
          if (TagTable)
            return false;

          uint32_t Offset;
          tag_block::TagDataLayout::readRecord(Scratch, Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          TagTable.reset(SerializedTagTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::readTypedefBlock(
    llvm::BitstreamCursor &Cursor, llvm::SmallVectorImpl<uint64_t> &Scratch) {
  return DeserializeBlock<TYPEDEF_BLOCK_ID>(
      Cursor, Scratch,
      [this, &Scratch](unsigned Kind, llvm::StringRef BlobData) -> bool {
        switch (Kind) {
        case typedef_block::TYPEDEF_DATA: {
          // Already saw typedef table.
          if (TypedefTable)
            return false;

          uint32_t Offset;
          typedef_block::TypedefDataLayout::readRecord(Scratch, Offset);
          auto Data = reinterpret_cast<const uint8_t *>(BlobData.data());

          TypedefTable.reset(SerializedTypedefTable::Create(
              Data + Offset, Data + sizeof(uint32_t), Data));

          break;
        }

        default:
          // Unknown metadata record, possibly for use by a future version of
          // the module format.
          break;
        }

        return true;
      });
}

bool APINotesReader::Implementation::Load() {
  llvm::BitstreamCursor Cursor(*MB);

  // Validate signature.
  for (auto SigByte : API_NOTES_SIGNATURE) {
    if (Cursor.AtEndOfStream())
      return false;

    llvm::Expected<llvm::SimpleBitstreamCursor::word_t> CB = Cursor.Read(8);
    if (!CB) {
      // FIXME: this drops the error on the floor.
      consumeError(CB.takeError());
      return false;
    }

    if (CB.get() != SigByte)
      return false;
  }

  // Look at all of the blocks.
  bool HasValidControlBlock = false;
  llvm::SmallVector<uint64_t, 64> Scratch;
  while (!Cursor.AtEndOfStream()) {
    llvm::Expected<llvm::BitstreamEntry> Entry = Cursor.advance();
    if (!Entry) {
      // FIXME: this drops the error on the floor.
      consumeError(Entry.takeError());
      return false;
    }

    if (Entry->Kind != llvm::BitstreamEntry::SubBlock)
      break;

    switch (Entry->ID) {
    case llvm::bitc::BLOCKINFO_BLOCK_ID:
      if (!Cursor.ReadBlockInfoBlock())
        return false;
      break;

    case CONTROL_BLOCK_ID:
      // Only allow a single control block.
      if (HasValidControlBlock)
        return false;

      if (!readControlBlock(Cursor, Scratch))
        return false;
      HasValidControlBlock = true;
      break;

    case IDENTIFIER_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readIdentifierBlock(Cursor, Scratch))
        return false;
      break;

    case OBJC_CONTEXT_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readObjCContextBlock(Cursor, Scratch))
        return false;
      break;

    case OBJC_PROPERTY_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readObjCPropertyBlock(Cursor, Scratch))
        return false;
      break;

    case OBJC_METHOD_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readObjCMethodBlock(Cursor, Scratch))
        return false;
      break;

    case OBJC_SELECTOR_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readObjCSelectorBlock(Cursor, Scratch))
        return false;
      break;

    case GLOBAL_VARIABLE_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readGlobalVariableBlock(Cursor, Scratch))
        return false;
      break;

    case GLOBAL_FUNCTION_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readGlobalFunctionBlock(Cursor, Scratch))
        return false;
      break;

    case ENUM_CONSTANT_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readEnumConstantBlock(Cursor, Scratch))
        return false;
      break;

    case TAG_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readTagBlock(Cursor, Scratch))
        return false;
      break;

    case TYPEDEF_BLOCK_ID:
      if (!HasValidControlBlock)
        return false;

      if (!readTypedefBlock(Cursor, Scratch))
        return false;
      break;

    default:
      // Unknown top-level block, possibly for use by a future version of the
      // module format.
      if (Cursor.SkipBlock())
        return false;
      break;
    }
  }

  assert(Cursor.AtEndOfStream() && "expected to be at end of stream");
  return true;
}

llvm::Optional<IdentifierID>
APINotesReader::Implementation::getIdentifier(llvm::StringRef Name) {
  if (!IdentifierTable)
    return llvm::None;

  if (Name.empty())
    return IdentifierID(0);

  auto IID = IdentifierTable->find(Name);
  if (IID == IdentifierTable->end())
    return llvm::None;
  return *IID;
}

template <typename T>
APINotesReader::VersionedInfo<T>::VersionedInfo(llvm::VersionTuple Version,
                                                llvm::SmallVector<std::pair<llvm::VersionTuple, T>, 1> Input)
    : Info(std::move(Input)) {
  assert(!Info.empty());
  assert(std::is_sorted(std::begin(Info), std::end(Info),
                        [](const std::pair<llvm::VersionTuple, T> &LHS,
                           const std::pair<llvm::VersionTuple, T> &RHS) -> bool {
                        assert(LHS.first != RHS.first && "duplicate entries for the same version");
                        return LHS.first < RHS.first;
                        }));

  Selected = Info.size();
  for (auto &VI : Info) {
    if (!Version.empty() && VI.first >= Version) {
      // If the current version is "4", then entries for 4 are better than
      // entries for 5, but both are valid. Because entries are sorted, we get
      // that behavior by picking the first match.
      Selected = std::distance(&VI, std::begin(Info));
      break;
    }
  }

  // If we didn't find a match but we have an unversioned result, use the
  // unversioned result. This will always be the first entry because we encode
  // it as version 0.
  if (Selected == Info.size() && Info[0].first.empty())
    Selected = 0;
}

// APINotesReader

APINotesReader::APINotesReader(llvm::MemoryBuffer *MB, bool OwnsMB,
                               llvm::VersionTuple SwiftVersion, bool &Result)
    : Implementation(new class Implementation(MB, OwnsMB, SwiftVersion)) {
  Result = Implementation->Load();
}

APINotesReader::~APINotesReader() = default;

std::unique_ptr<APINotesReader>
APINotesReader::create(std::unique_ptr<llvm::MemoryBuffer> MB,
                       llvm::VersionTuple SwiftVersion) {
  bool Result;
  std::unique_ptr<APINotesReader> reader{
      new APINotesReader(MB.release(), /*OwnsMB=*/true, SwiftVersion, Result)};
  if (Result)
    return reader;
  return nullptr;
}

llvm::StringRef APINotesReader::getModuleName() const {
  return Implementation->ModuleName;
}

llvm::Optional<std::pair<off_t, time_t>>
APINotesReader::getSourceFileSizeAndModificationTime() const {
  return Implementation->SourceFileSizeAndModificationTime;
}

#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
ModuleOptions APINotesReader::getModuleOptions() const {
  return Implementation->ModuleOpts;
}
#endif

llvm::Optional<ContextID>
APINotesReader::lookupObjCClassID(llvm::StringRef Name) {
  if (!Implementation->ObjCContextIDTable)
    return llvm::None;

  llvm::Optional<IdentifierID> IID = Implementation->getIdentifier(Name);
  if (!IID)
    return llvm::None;

  auto KnownID = Implementation->ObjCContextIDTable->find({*IID, '\0'});
  if (KnownID == Implementation->ObjCContextIDTable->end())
    return llvm::None;

  return ContextID(*KnownID);
}

APINotesReader::VersionedInfo<ObjCContextInfo>
APINotesReader::lookupObjCClassInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->ObjCContextInfoTable, Name);
}

llvm::Optional<ContextID>
APINotesReader::lookupObjCProtocolID(llvm::StringRef Name) {
  if (!Implementation->ObjCContextIDTable)
    return llvm::None;

  llvm::Optional<IdentifierID> IID = Implementation->getIdentifier(Name);
  if (!IID)
    return llvm::None;

  auto KnownID = Implementation->ObjCContextIDTable->find({*IID, '\1'});
  if (KnownID == Implementation->ObjCContextIDTable->end())
    return llvm::None;

  return ContextID(*KnownID);
}

APINotesReader::VersionedInfo<ObjCContextInfo>
APINotesReader::lookupObjCProtocolInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->ObjCContextInfoTable, Name);
}

APINotesReader::VersionedInfo<ObjCPropertyInfo>
APINotesReader::lookupObjCPropertyInfo(ContextID Context, llvm::StringRef Name,
                                       bool Instance) {
  if (!Implementation->ObjCPropertyTable)
    return llvm::None;

  llvm::Optional<IdentifierID> IID = Implementation->getIdentifier(Name);
  if (!IID)
    return llvm::None;

  auto KnownInfo = Implementation->ObjCPropertyTable->find(
      std::make_tuple(Context.Value, *IID, static_cast<char>(Instance)));
  if (KnownInfo == Implementation->ObjCPropertyTable->end())
    return llvm::None;

  return {Implementation->SwiftVersion, *KnownInfo};
}

APINotesReader::VersionedInfo<ObjCMethodInfo>
APINotesReader::lookupObjCMethodInfo(ContextID Context,
                                     ObjCSelectorRef Selector, bool Instance) {
  if (!Implementation->ObjCMethodTable)
    return llvm::None;

  llvm::Optional<SelectorID> SID = Implementation->getSelector(Selector);
  if (!SID)
    return llvm::None;

  auto KnownInfo = Implementation->ObjCMethodTable->find(
      ObjCMethodTableInfo::internal_key_type{Context.Value, *SID, Instance});
  if (KnownInfo == Implementation->ObjCMethodTable->end())
    return llvm::None;

  return {Implementation->SwiftVersion, *KnownInfo};
}

APINotesReader::VersionedInfo<GlobalVariableInfo>
APINotesReader::lookupGlobalVariableInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->GlobalVariableTable, Name);
}

APINotesReader::VersionedInfo<GlobalFunctionInfo>
APINotesReader::lookupGlobalFunctionInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->GlobalFunctionTable, Name);
}

APINotesReader::VersionedInfo<EnumConstantInfo>
APINotesReader::lookupEnumConstantInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->EnumConstantTable, Name);
}

APINotesReader::VersionedInfo<TagInfo>
APINotesReader::lookupTagInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->TagTable, Name);
}

APINotesReader::VersionedInfo<TypedefInfo>
APINotesReader::lookupTypedefInfo(llvm::StringRef Name) {
  return Implementation->lookup(Implementation->TypedefTable, Name);
}
} // namespace api_notes
} // namespace clang
