//===-- APINotesYAMLCompiler.cpp - API Notes YAML Format Reader -*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The types defined locally are designed to represent the YAML state, which
// adds an additional bit of state: e.g. a tri-state boolean attribute (yes, no,
// not applied) becomes a tri-state boolean + present.  As a result, while these
// enumerations appear to be redefining constants from the attributes table
// data, they are distinct.
//

#include "clang/APINotes/APINotesYAMLCompiler.h"
#include "clang/APINotes/APINotesWriter.h"
#include "clang/APINotes/Types.h"
#include "clang/Basic/FileEntry.h"
#include "clang/Basic/LLVM.h"
#include "clang/Basic/Specifiers.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/VersionTuple.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"
#include <vector>
using namespace clang;
using namespace api_notes;

namespace {
enum class APIAvailability {
  Available = 0,
  OSX,
  IOS,
  None,
  NonSwift,
};
} // namespace

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<APIAvailability> {
  static void enumeration(IO &IO, APIAvailability &AA) {
    IO.enumCase(AA, "OSX", APIAvailability::OSX);
    IO.enumCase(AA, "iOS", APIAvailability::IOS);
    IO.enumCase(AA, "none", APIAvailability::None);
    IO.enumCase(AA, "nonswift", APIAvailability::NonSwift);
    IO.enumCase(AA, "available", APIAvailability::Available);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
enum class MethodKind {
  Class,
  Instance,
};
} // namespace

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<MethodKind> {
  static void enumeration(IO &IO, MethodKind &MK) {
    IO.enumCase(MK, "Class", MethodKind::Class);
    IO.enumCase(MK, "Instance", MethodKind::Instance);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Param {
  unsigned Position;
  Optional<bool> NoEscape = false;
  Optional<NullabilityKind> Nullability;
  Optional<RetainCountConventionKind> RetainCountConvention;
  StringRef Type;
};

typedef std::vector<Param> ParamsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Param)
LLVM_YAML_IS_FLOW_SEQUENCE_VECTOR(NullabilityKind)

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<NullabilityKind> {
  static void enumeration(IO &IO, NullabilityKind &NK) {
    IO.enumCase(NK, "Nonnull", NullabilityKind::NonNull);
    IO.enumCase(NK, "Optional", NullabilityKind::Nullable);
    IO.enumCase(NK, "Unspecified", NullabilityKind::Unspecified);
    IO.enumCase(NK, "NullableResult", NullabilityKind::NullableResult);
    // TODO: Mapping this to it's own value would allow for better cross
    // checking. Also the default should be Unknown.
    IO.enumCase(NK, "Scalar", NullabilityKind::Unspecified);

    // Aliases for compatibility with existing APINotes.
    IO.enumCase(NK, "N", NullabilityKind::NonNull);
    IO.enumCase(NK, "O", NullabilityKind::Nullable);
    IO.enumCase(NK, "U", NullabilityKind::Unspecified);
    IO.enumCase(NK, "S", NullabilityKind::Unspecified);
  }
};

template <> struct ScalarEnumerationTraits<RetainCountConventionKind> {
  static void enumeration(IO &IO, RetainCountConventionKind &RCCK) {
    IO.enumCase(RCCK, "none", RetainCountConventionKind::None);
    IO.enumCase(RCCK, "CFReturnsRetained",
                RetainCountConventionKind::CFReturnsRetained);
    IO.enumCase(RCCK, "CFReturnsNotRetained",
                RetainCountConventionKind::CFReturnsNotRetained);
    IO.enumCase(RCCK, "NSReturnsRetained",
                RetainCountConventionKind::NSReturnsRetained);
    IO.enumCase(RCCK, "NSReturnsNotRetained",
                RetainCountConventionKind::NSReturnsNotRetained);
  }
};

template <> struct MappingTraits<Param> {
  static void mapping(IO &IO, Param &P) {
    IO.mapRequired("Position", P.Position);
    IO.mapOptional("Nullability", P.Nullability, llvm::None);
    IO.mapOptional("RetainCountConvention", P.RetainCountConvention);
    IO.mapOptional("NoEscape", P.NoEscape);
    IO.mapOptional("Type", P.Type, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
typedef std::vector<NullabilityKind> NullabilitySeq;

struct AvailabilityItem {
  APIAvailability Mode = APIAvailability::Available;
  StringRef Msg;
};

/// Old attribute deprecated in favor of SwiftName.
enum class FactoryAsInitKind {
  /// Infer based on name and type (the default).
  Infer,
  /// Treat as a class method.
  AsClassMethod,
  /// Treat as an initializer.
  AsInitializer,
};

struct Method {
  StringRef Selector;
  MethodKind Kind;
  ParamsSeq Params;
  NullabilitySeq Nullability;
  Optional<NullabilityKind> NullabilityOfRet;
  Optional<RetainCountConventionKind> RetainCountConvention;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
  FactoryAsInitKind FactoryAsInit = FactoryAsInitKind::Infer;
  bool DesignatedInit = false;
  bool Required = false;
  StringRef ResultType;
};

typedef std::vector<Method> MethodsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Method)

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<FactoryAsInitKind> {
  static void enumeration(IO &IO, FactoryAsInitKind &FIK) {
    IO.enumCase(FIK, "A", FactoryAsInitKind::Infer);
    IO.enumCase(FIK, "C", FactoryAsInitKind::AsClassMethod);
    IO.enumCase(FIK, "I", FactoryAsInitKind::AsInitializer);
  }
};

template <> struct MappingTraits<Method> {
  static void mapping(IO &IO, Method &M) {
    IO.mapRequired("Selector", M.Selector);
    IO.mapRequired("MethodKind", M.Kind);
    IO.mapOptional("Parameters", M.Params);
    IO.mapOptional("Nullability", M.Nullability);
    IO.mapOptional("NullabilityOfRet", M.NullabilityOfRet, llvm::None);
    IO.mapOptional("RetainCountConvention", M.RetainCountConvention);
    IO.mapOptional("Availability", M.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", M.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", M.SwiftPrivate);
    IO.mapOptional("SwiftName", M.SwiftName, StringRef(""));
    IO.mapOptional("FactoryAsInit", M.FactoryAsInit, FactoryAsInitKind::Infer);
    IO.mapOptional("DesignatedInit", M.DesignatedInit, false);
    IO.mapOptional("Required", M.Required, false);
    IO.mapOptional("ResultType", M.ResultType, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Property {
  StringRef Name;
  llvm::Optional<MethodKind> Kind;
  llvm::Optional<NullabilityKind> Nullability;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
  Optional<bool> SwiftImportAsAccessors;
  StringRef Type;
};

typedef std::vector<Property> PropertiesSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Property)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<Property> {
  static void mapping(IO &IO, Property &P) {
    IO.mapRequired("Name", P.Name);
    IO.mapOptional("PropertyKind", P.Kind);
    IO.mapOptional("Nullability", P.Nullability, llvm::None);
    IO.mapOptional("Availability", P.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", P.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", P.SwiftPrivate);
    IO.mapOptional("SwiftName", P.SwiftName, StringRef(""));
    IO.mapOptional("SwiftImportAsAccessors", P.SwiftImportAsAccessors);
    IO.mapOptional("Type", P.Type, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Class {
  StringRef Name;
  bool AuditedForNullability = false;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
  Optional<StringRef> SwiftBridge;
  Optional<StringRef> NSErrorDomain;
  Optional<bool> SwiftImportAsNonGeneric;
  Optional<bool> SwiftObjCMembers;
  MethodsSeq Methods;
  PropertiesSeq Properties;
};

typedef std::vector<Class> ClassesSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Class)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<Class> {
  static void mapping(IO &IO, Class &C) {
    IO.mapRequired("Name", C.Name);
    IO.mapOptional("AuditedForNullability", C.AuditedForNullability, false);
    IO.mapOptional("Availability", C.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", C.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", C.SwiftPrivate);
    IO.mapOptional("SwiftName", C.SwiftName, StringRef(""));
    IO.mapOptional("SwiftBridge", C.SwiftBridge);
    IO.mapOptional("NSErrorDomain", C.NSErrorDomain);
    IO.mapOptional("SwiftImportAsNonGeneric", C.SwiftImportAsNonGeneric);
    IO.mapOptional("SwiftObjCMembers", C.SwiftObjCMembers);
    IO.mapOptional("Methods", C.Methods);
    IO.mapOptional("Properties", C.Properties);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Function {
  StringRef Name;
  ParamsSeq Params;
  NullabilitySeq Nullability;
  Optional<NullabilityKind> NullabilityOfRet;
  Optional<api_notes::RetainCountConventionKind> RetainCountConvention;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
  StringRef Type;
  StringRef ResultType;
};

typedef std::vector<Function> FunctionsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Function)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<Function> {
  static void mapping(IO &IO, Function &F) {
    IO.mapRequired("Name", F.Name);
    IO.mapOptional("Parameters", F.Params);
    IO.mapOptional("Nullability", F.Nullability);
    IO.mapOptional("NullabilityOfRet", F.NullabilityOfRet, llvm::None);
    IO.mapOptional("RetainCountConvention", F.RetainCountConvention);
    IO.mapOptional("Availability", F.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", F.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", F.SwiftPrivate);
    IO.mapOptional("SwiftName", F.SwiftName, StringRef(""));
    IO.mapOptional("ResultType", F.ResultType, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct GlobalVariable {
  StringRef Name;
  llvm::Optional<NullabilityKind> Nullability;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
  StringRef Type;
};

typedef std::vector<GlobalVariable> GlobalVariablesSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(GlobalVariable)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<GlobalVariable> {
  static void mapping(IO &IO, GlobalVariable &GV) {
    IO.mapRequired("Name", GV.Name);
    IO.mapOptional("Nullability", GV.Nullability, llvm::None);
    IO.mapOptional("Availability", GV.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", GV.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", GV.SwiftPrivate);
    IO.mapOptional("SwiftName", GV.SwiftName, StringRef(""));
    IO.mapOptional("Type", GV.Type, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct EnumConstant {
  StringRef Name;
  AvailabilityItem Availability;
  Optional<bool> SwiftPrivate;
  StringRef SwiftName;
};

typedef std::vector<EnumConstant> EnumConstantsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(EnumConstant)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<EnumConstant> {
  static void mapping(IO &IO, EnumConstant &EC) {
    IO.mapRequired("Name", EC.Name);
    IO.mapOptional("Availability", EC.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", EC.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", EC.SwiftPrivate);
    IO.mapOptional("SwiftName", EC.SwiftName, StringRef(""));
  }
};
} // namespace yaml
} // namespace llvm

namespace {
/// Syntactic sugar for EnumExtensibility and FlagEnum
enum class EnumConvenienceAliasKind {
  /// EnumExtensibility: none, FlagEnum: false
  None,
  /// EnumExtensibility: open, FlagEnum: false
  CFEnum,
  /// EnumExtensibility: open, FlagEnum: true
  CFOptions,
  /// EnumExtensibility: closed, FlagEnum: false
  CFClosedEnum
};
} // namespace

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<EnumConvenienceAliasKind> {
  static void enumeration(IO &IO, EnumConvenienceAliasKind &ECAK) {
    IO.enumCase(ECAK, "none", EnumConvenienceAliasKind::None);
    IO.enumCase(ECAK, "CFEnum", EnumConvenienceAliasKind::CFEnum);
    IO.enumCase(ECAK, "NSEnum", EnumConvenienceAliasKind::CFEnum);
    IO.enumCase(ECAK, "CFOptions", EnumConvenienceAliasKind::CFOptions);
    IO.enumCase(ECAK, "NSOptions", EnumConvenienceAliasKind::CFOptions);
    IO.enumCase(ECAK, "CFClosedEnum", EnumConvenienceAliasKind::CFClosedEnum);
    IO.enumCase(ECAK, "NSClosedEnum", EnumConvenienceAliasKind::CFClosedEnum);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Tag {
  StringRef Name;
  AvailabilityItem Availability;
  StringRef SwiftName;
  Optional<bool> SwiftPrivate;
  Optional<StringRef> SwiftBridge;
  Optional<StringRef> NSErrorDomain;
  Optional<EnumExtensibilityKind> EnumExtensibility;
  Optional<bool> FlagEnum;
  Optional<EnumConvenienceAliasKind> EnumConvenienceKind;
};

typedef std::vector<Tag> TagsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Tag)

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<EnumExtensibilityKind> {
  static void enumeration(IO &IO, EnumExtensibilityKind &EEK) {
    IO.enumCase(EEK, "none", EnumExtensibilityKind::None);
    IO.enumCase(EEK, "open", EnumExtensibilityKind::Open);
    IO.enumCase(EEK, "closed", EnumExtensibilityKind::Closed);
  }
};

template <> struct MappingTraits<Tag> {
  static void mapping(IO &IO, Tag &T) {
    IO.mapRequired("Name", T.Name);
    IO.mapOptional("Availability", T.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", T.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", T.SwiftPrivate);
    IO.mapOptional("SwiftName", T.SwiftName, StringRef(""));
    IO.mapOptional("SwiftBridge", T.SwiftBridge);
    IO.mapOptional("NSErrorDomain", T.NSErrorDomain);
    IO.mapOptional("EnumExtensibility", T.EnumExtensibility);
    IO.mapOptional("FlagEnum", T.FlagEnum);
    IO.mapOptional("EnumKind", T.EnumConvenienceKind);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Typedef {
  StringRef Name;
  AvailabilityItem Availability;
  StringRef SwiftName;
  Optional<bool> SwiftPrivate;
  Optional<StringRef> SwiftBridge;
  Optional<StringRef> NSErrorDomain;
  Optional<SwiftNewTypeKind> SwiftType;
};

typedef std::vector<Typedef> TypedefsSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Typedef)

namespace llvm {
namespace yaml {
template <> struct ScalarEnumerationTraits<SwiftNewTypeKind> {
  static void enumeration(IO &IO, SwiftNewTypeKind &SWK) {
    IO.enumCase(SWK, "none", SwiftNewTypeKind::None);
    IO.enumCase(SWK, "struct", SwiftNewTypeKind::Struct);
    IO.enumCase(SWK, "enum", SwiftNewTypeKind::Enum);
  }
};

template <> struct MappingTraits<Typedef> {
  static void mapping(IO &IO, Typedef &T) {
    IO.mapRequired("Name", T.Name);
    IO.mapOptional("Availability", T.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", T.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftPrivate", T.SwiftPrivate);
    IO.mapOptional("SwiftName", T.SwiftName, StringRef(""));
    IO.mapOptional("SwiftBridge", T.SwiftBridge);
    IO.mapOptional("NSErrorDomain", T.NSErrorDomain);
    IO.mapOptional("SwiftWrapper", T.SwiftType);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct TopLevelItems {
  ClassesSeq Classes;
  ClassesSeq Protocols;
  FunctionsSeq Functions;
  GlobalVariablesSeq Globals;
  EnumConstantsSeq EnumConstants;
  TagsSeq Tags;
  TypedefsSeq Typedefs;
};
} // namespace

namespace llvm {
namespace yaml {
static void mapTopLevelItems(IO &IO, TopLevelItems &TLI) {
  IO.mapOptional("Classes", TLI.Classes);
  IO.mapOptional("Protocols", TLI.Protocols);
  IO.mapOptional("Functions", TLI.Functions);
  IO.mapOptional("Globals", TLI.Globals);
  IO.mapOptional("Enumerators", TLI.EnumConstants);
  IO.mapOptional("Tags", TLI.Tags);
  IO.mapOptional("Typedefs", TLI.Typedefs);
}
} // namespace yaml
} // namespace llvm

namespace {
struct Versioned {
  VersionTuple Version;
  TopLevelItems Items;
};

typedef std::vector<Versioned> VersionedSeq;
} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(Versioned)

namespace llvm {
namespace yaml {
template <> struct MappingTraits<Versioned> {
  static void mapping(IO &IO, Versioned &V) {
    IO.mapRequired("Version", V.Version);
    mapTopLevelItems(IO, V.Items);
  }
};
} // namespace yaml
} // namespace llvm

namespace {
struct Module {
  StringRef Name;
  AvailabilityItem Availability;
  TopLevelItems TopLevel;
  VersionedSeq SwiftVersions;

  llvm::Optional<bool> SwiftInferImportAsMember = {llvm::None};

  LLVM_DUMP_METHOD void dump() /*const*/;
};
} // namespace

namespace llvm {
namespace yaml {
template <> struct MappingTraits<Module> {
  static void mapping(IO &IO, Module &M) {
    IO.mapRequired("Name", M.Name);
    IO.mapOptional("Availability", M.Availability.Mode,
                   APIAvailability::Available);
    IO.mapOptional("AvailabilityMsg", M.Availability.Msg, StringRef(""));
    IO.mapOptional("SwiftInferImportAsMember", M.SwiftInferImportAsMember);
    mapTopLevelItems(IO, M.TopLevel);
    IO.mapOptional("SwiftVersions", M.SwiftVersions);
  }
};
} // namespace yaml
} // namespace llvm

void Module::dump() {
  llvm::yaml::Output OS(llvm::errs());
  OS << *this;
}

namespace {
bool parseAPINotes(StringRef YI, Module &M, llvm::SourceMgr::DiagHandlerTy Diag,
                   void *DiagContext) {
  llvm::yaml::Input IS(YI, nullptr, Diag, DiagContext);
  IS >> M;
  return static_cast<bool>(IS.error());
}
} // namespace

bool clang::api_notes::parseAndDumpAPINotes(StringRef YI,
                                            llvm::raw_ostream &OS) {
  Module M;
  if (parseAPINotes(YI, M, nullptr, nullptr))
    return true;

  llvm::yaml::Output YOS(OS);
  YOS << M;

  return false;
}

namespace {
void printDiagnosticToStderr(const llvm::SMDiagnostic &Diag, void *Context) {
  Diag.print(nullptr, llvm::errs());
}

class YAMLConverter {
  const Module &M;
  const FileEntry *SF;
  APINotesWriter *Writer;
  llvm::raw_ostream &OS;
  llvm::SourceMgr::DiagHandlerTy DiagHandler;
  void *DiagContext;
  bool ErrorOccurred;

  /// Emit a diagnostic.
  bool emitError(llvm::Twine Msg) {
    DiagHandler(llvm::SMDiagnostic("", llvm::SourceMgr::DK_Error, Msg.str()),
                DiagContext);
    ErrorOccurred = true;
    return true;
  }

  void convertParams(const ParamsSeq &Params, FunctionInfo &FI) {
    for (const auto &Param : Params) {
      ParamInfo PI;
      if (Param.Nullability)
        PI.setNullabilityAudited(*Param.Nullability);
      PI.setNoEscape(Param.NoEscape);
      PI.setType(std::string(Param.Type));
      PI.setRetainCountConvention(Param.RetainCountConvention);
      while (FI.Params.size() <= Param.Position)
        FI.Params.emplace_back();
      FI.Params[Param.Position] |= PI;
    }
  }

  void convertNullability(const NullabilitySeq &Nullability,
                          llvm::Optional<NullabilityKind> NullabilityOfRet,
                          FunctionInfo &FI, llvm::StringRef Name) {
    if (Nullability.size() > FunctionInfo::getMaxNullabilityIndex()) {
      emitError("nullability info for " + Name + " exceeds storage");
      return;
    }

    bool Audited = false;
    unsigned int Index = 1;
    for (auto I = Nullability.begin(), E = Nullability.end(); I != E;
         ++I, ++Index) {
      FI.addTypeInfo(Index, *I);
      Audited = true;
    }

    if (NullabilityOfRet) {
      FI.addTypeInfo(FunctionInfo::ReturnInfoIndex, *NullabilityOfRet);
      Audited = true;
    } else if (Audited) {
      FI.addTypeInfo(FunctionInfo::ReturnInfoIndex, NullabilityKind::NonNull);
    }

    if (Audited) {
      FI.NullabilityAudited = true;
      FI.NumAdjustedNullable = Index;
    }
  }

  template <typename Callable>
  void convertSignature(Callable &C, FunctionInfo &FI, llvm::StringRef Name) {
    convertParams(C.Params, FI);
    convertNullability(C.Nullability, C.NullabilityOfRet, FI, Name);
    FI.ResultType = std::string(C.ResultType);
    FI.setRetainCountConvention(C.RetainCountConvention);
  }

  bool convertAvailability(const AvailabilityItem &AI, CommonEntityInfo &CEI,
                           llvm::StringRef Name) {
    // Populate the unavailability information.
    CEI.Unavailable = AI.Mode == APIAvailability::None;
    CEI.UnavailableInSwift = AI.Mode == APIAvailability::NonSwift;
    if (CEI.Unavailable || CEI.UnavailableInSwift)
      CEI.UnavailableMsg = std::string(AI.Msg);
    else if (!AI.Msg.empty())
      emitError("availability message for available API '" + Name +
                "' will not be used");
    return false;
  }

  template <typename CommonEntity>
  bool convertCommonEntity(const CommonEntity &CE, CommonEntityInfo &CEI,
                           llvm::StringRef Name) {
    convertAvailability(CE.Availability, CEI, Name);
    CEI.setSwiftPrivate(CE.SwiftPrivate);
    CEI.SwiftName = std::string(CE.SwiftName);
    return false;
  }

  template <typename CommonType>
  bool convertCommonType(const CommonType &CT, CommonTypeInfo &CTI,
                         llvm::StringRef Name) {
    if (convertCommonEntity(CT, CTI, Name))
      return true;
    CTI.setSwiftBridge(CT.SwiftBridge);
    CTI.setNSErrorDomain(CT.NSErrorDomain);
    return false;
  }

  void convertMethod(const Method &Method, ContextID CID, llvm::StringRef Name,
                     const llvm::VersionTuple &Version) {
    ObjCMethodInfo OMI;

    if (convertCommonEntity(Method, OMI, Method.Selector))
      return;

    // Check if the selector ends with ':' to determine if it takes arguments.
    bool TakesArguments = Method.Selector.endswith(":");

    // Split the selector into pieces.
    llvm::SmallVector<StringRef, 4> Pieces;
    Method.Selector.split(Pieces, ":", /*MaxSplit*/ -1, /*KeepEmpty=*/false);
    if (!TakesArguments && Pieces.size() > 1) {
      emitError("selector '" + Method.Selector +
                "' is missing a ':' at the end");
      return;
    }

    ObjCSelectorRef SelRef{
        static_cast<unsigned int>(TakesArguments ? Pieces.size() : 0), Pieces};

    OMI.DesignatedInit = Method.DesignatedInit;
    OMI.RequiredInit = Method.Required;
    if (Method.FactoryAsInit != FactoryAsInitKind::Infer)
      emitError("'FactoryAsInit' is no longer valid; use 'SwiftName' instead");
    convertSignature(Method, OMI, Method.Selector);

    Writer->addObjCMethod(CID, SelRef, Method.Kind == MethodKind::Instance, OMI,
                          Version);
  }

  void convertContext(const Class &C, bool IsClass,
                      const llvm::VersionTuple &Version) {
    // Write the class.
    ObjCContextInfo OCI;

    if (convertCommonType(C, OCI, C.Name))
      return;

    if (C.AuditedForNullability)
      OCI.setDefaultNullability(NullabilityKind::NonNull);
    if (C.SwiftImportAsNonGeneric)
      OCI.setSwiftImportAsNonGeneric(*C.SwiftImportAsNonGeneric);
    if (C.SwiftObjCMembers)
      OCI.setSwiftObjCMembers(*C.SwiftObjCMembers);

    ContextID CID = Writer->addObjCContext(C.Name, IsClass, OCI, Version);

    // Write all methods.
    llvm::StringMap<std::pair<bool, bool>> KnownMethods;
    for (const auto &Method : C.Methods) {
      // Check for duplicate method definitions.
      bool IsInstance = Method.Kind == MethodKind::Instance;
      bool &IsKnown = IsInstance ? KnownMethods[Method.Selector].first
                                 : KnownMethods[Method.Selector].second;
      if (IsKnown) {
        emitError(llvm::Twine("multiple definitions of method '") +
                  (IsInstance ? "-" : "+") + "[" + C.Name + " " +
                  Method.Selector + "]'");
        continue;
      }

      convertMethod(Method, CID, C.Name, Version);
    }

    // Write all properties.
    llvm::StringSet<> KnownInstanceProperties;
    llvm::StringSet<> KnownClassProperties;
    for (const auto &Property : C.Properties) {
      // Check for duplicate property definitions.
      if (!Property.Kind || *Property.Kind == MethodKind::Instance) {
        if (!KnownInstanceProperties.insert(Property.Name).second) {
          emitError("multiple definitions of instance property '" + C.Name +
                    "." + Property.Name + "'");
          continue;
        }
      }

      if (!Property.Kind || *Property.Kind == MethodKind::Class) {
        if (!KnownClassProperties.insert(Property.Name).second) {
          emitError("multiple definitions of class property '" + C.Name + "." +
                    Property.Name + "'");
          continue;
        }
      }

      ObjCPropertyInfo OPI;
      convertCommonEntity(Property, OPI, Property.Name);
      if (Property.Nullability)
        OPI.setNullabilityAudited(*Property.Nullability);
      if (Property.SwiftImportAsAccessors)
        OPI.setSwiftImportAsAccessors(*Property.SwiftImportAsAccessors);
      OPI.setType(std::string(Property.Type));

      if (Property.Kind) {
        Writer->addObjCProperty(CID, Property.Name,
                                *Property.Kind == MethodKind::Instance, OPI,
                                Version);
      } else {
        // Add as both class and instance.
        Writer->addObjCProperty(CID, Property.Name, /*IsInstance=*/false, OPI,
                                Version);
        Writer->addObjCProperty(CID, Property.Name, /*IsInstance=*/true, OPI,
                                Version);
      }
    }
  }

  void convertTopLevelItems(const TopLevelItems &TLI,
                            const llvm::VersionTuple &Version) {
    // Write all classes.
    llvm::StringSet<> KnownClasses;
    for (const auto &C : TLI.Classes) {
      // Check for duplicate class definitions.
      if (!KnownClasses.insert(C.Name).second) {
        emitError("multiple definitions of class '" + C.Name + "'");
        continue;
      }
      convertContext(C, /*isClass=*/true, Version);
    }

    // Write all protocols.
    llvm::StringSet<> KnownProtocols;
    for (const auto &P : TLI.Protocols) {
      // Check for duplicate protocol definitions.
      if (!KnownProtocols.insert(P.Name).second) {
        emitError("multiple definitions of protocol '" + P.Name + "'");
        continue;
      }
      convertContext(P, /*isClass=*/false, Version);
    }

    // Write all global variables.
    llvm::StringSet<> KnownGlobalVariables;
    for (const auto &GV : TLI.Globals) {
      // Check for duplicate global variables.
      if (!KnownGlobalVariables.insert(GV.Name).second) {
        emitError("multiple definitions of global variable '" + GV.Name + "'");
        continue;
      }

      GlobalVariableInfo GVI;
      convertCommonEntity(GV, GVI, GV.Name);
      if (GV.Nullability)
        GVI.setNullabilityAudited(*GV.Nullability);
      GVI.setType(std::string(GV.Type));
      Writer->addGlobalVariable(GV.Name, GVI, Version);
    }

    // Write all global functions.
    llvm::StringSet<> KnownFunctions;
    for (const auto &F : TLI.Functions) {
      // Check for duplicate global functions.
      if (!KnownFunctions.insert(F.Name).second) {
        emitError("multiple definitions of function '" + F.Name + "'");
        continue;
      }

      GlobalFunctionInfo GFI;
      convertCommonEntity(F, GFI, F.Name);
      convertSignature(F, GFI, F.Name);
      Writer->addGlobalFunction(F.Name, GFI, Version);
    }

    // Write all enumerators.
    llvm::StringSet<> KnownEnumerators;
    for (const auto &EC : TLI.EnumConstants) {
      // Check for duplicate enumerators.
      if (!KnownEnumerators.insert(EC.Name).second) {
        emitError("multiple definitions of enumerator '" + EC.Name + "'");
        continue;
      }

      EnumConstantInfo ECI;
      convertCommonEntity(EC, ECI, EC.Name);
      Writer->addEnumConstant(EC.Name, ECI, Version);
    }

    // Write all tags.
    llvm::StringSet<> KnownTags;
    for (const auto &T : TLI.Tags) {
      // Check for duplicate tag definitions.
      if (!KnownTags.insert(T.Name).second) {
        emitError("multiple definitions of tag '" + T.Name + "'");
        continue;
      }

      TagInfo TI;
      if (convertCommonType(T, TI, T.Name))
        continue;
      if (T.EnumConvenienceKind) {
        if (T.EnumExtensibility) {
          emitError(
              llvm::Twine("cannot mix EnumKind and EnumExtensibility (for ") +
              T.Name + ")");
          continue;
        }

        if (T.FlagEnum) {
          emitError(llvm::Twine("cannot mix EnumKind and FlagEnum (for ") +
                    T.Name + ")");
          continue;
        }

        switch (*T.EnumConvenienceKind) {
        case EnumConvenienceAliasKind::None:
          TI.EnumExtensibility = EnumExtensibilityKind::None;
          TI.setFlagEnum(false);
          break;
        case EnumConvenienceAliasKind::CFEnum:
          TI.EnumExtensibility = EnumExtensibilityKind::Open;
          TI.setFlagEnum(false);
          break;
        case EnumConvenienceAliasKind::CFOptions:
          TI.EnumExtensibility = EnumExtensibilityKind::Open;
          TI.setFlagEnum(true);
          break;
        case EnumConvenienceAliasKind::CFClosedEnum:
          TI.EnumExtensibility = EnumExtensibilityKind::Closed;
          TI.setFlagEnum(false);
          break;
        }
      } else {
        TI.EnumExtensibility = T.EnumExtensibility;
        TI.setFlagEnum(T.FlagEnum);
      }
      Writer->addTag(T.Name, TI, Version);
    }

    // Write all typedefs.
    llvm::StringSet<> KnownTypedefs;
    for (const auto &T : TLI.Typedefs) {
      // Check for duplicate typedef definitions.
      if (!KnownTypedefs.insert(T.Name).second) {
        emitError("multiple definitions of typedef '" + T.Name + "'");
        continue;
      }

      TypedefInfo TI;
      if (convertCommonType(T, TI, T.Name))
        continue;
      TI.SwiftWrapper = T.SwiftType;
      Writer->addTypedef(T.Name, TI, Version);
    }
  }

public:
  YAMLConverter(const Module &M, const FileEntry *SF, llvm::raw_ostream &OS,
                llvm::SourceMgr::DiagHandlerTy DiagHandler, void *DiagContext)
      : M(M), SF(SF), Writer(nullptr), OS(OS), DiagHandler(DiagHandler),
        DiagContext(DiagContext), ErrorOccurred(false) {}

  bool convertModule() {
    // Setup Writer
    APINotesWriter W(M.Name, SF);
    Writer = &W;

    convertTopLevelItems(M.TopLevel, llvm::VersionTuple());
#if defined(__APPLE__) && defined(SWIFT_DOWNSTREAM)
    if (M.SwiftInferImportAsMember) {
      ModuleOptions MO;
      MO.SwiftInferImportAsMember = true;
      Writer->addModuleOptions(MO);
    }
#endif

    // Convert the versioned information.
    for (const auto &Version : M.SwiftVersions)
      convertTopLevelItems(Version.Items, Version.Version);

    if (!ErrorOccurred)
      Writer->writeToStream(OS);

    return ErrorOccurred;
  }
};

bool compile(const Module &M, const FileEntry *SF, llvm::raw_ostream &OS,
             llvm::SourceMgr::DiagHandlerTy DiagHandler, void *DiagContext) {
  YAMLConverter Converter(M, SF, OS, DiagHandler, DiagContext);
  return Converter.convertModule();
}
} // namespace

bool clang::api_notes::compileAPINotes(
    StringRef YI, const FileEntry *SF, llvm::raw_ostream &OS,
    llvm::SourceMgr::DiagHandlerTy DiagHandler, void *DiagContext) {
  Module M;
  if (parseAPINotes(YI, M, DiagHandler ? DiagHandler : &printDiagnosticToStderr,
                    DiagContext))
    return true;

  return compile(M, SF, OS,
                 DiagHandler ? DiagHandler : &printDiagnosticToStderr,
                 DiagContext);
}
