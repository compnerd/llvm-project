; RUN: llc -mtriple riscv64-unknown-linux-gnu -filetype obj %s -o - | llvm-dwarfdump - | FileCheck %s -check-prefix CHECK-DWARF
; RUN: llc -mtriple riscv64-unknown-linux-gnu -filetype obj %s -o - | llvm-readobj -r - | FileCheck %s -check-prefix CHECK-OBJ

source_filename = "reduced.c"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-linux-gnu"

define dso_local signext i32 @f() #0 !dbg !7 {
entry:
  ret i32 32, !dbg !13
}

attributes #0 = { noinline nounwind optnone "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-features"="+64bit" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 15.0.0", isOptimized: false, runtimeVersion: 0, splitDebugFilename: "reduced.dwo", emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "reduced.c", directory: "/SourceCache")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 1, !"target-abi", !"lp64"}
!5 = !{i32 1, !"SmallDataLimit", i32 0}
!6 = !{!"clang version 15.0.0"}
!7 = distinct !DISubprogram(name: "f", scope: !8, file: !8, line: 2, type: !9, scopeLine: 2, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !12)
!8 = !DIFile(filename: "/SourceCache/reduced.c", directory: "")
!9 = !DISubroutineType(types: !10)
!10 = !{!11}
!11 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!12 = !{}
!13 = !DILocation(line: 2, column: 15, scope: !7)

; CHECK-DWARF:      0x0000002a:   DW_TAG_subprogram
; CHECK-DWARF-NEXT:                 DW_AT_low_pc	(0x0000000000000000)
; CHECK-DWARF-NEXT:                 DW_AT_high_pc	(0x0000000000000008)

; CHECK-OBJ: Section (5) .rela.debug_info {
; CHECK-OBJ:       0x1A R_RISCV_32 - 0x0
; CHECK-OBJ:       0x1E R_RISCV_64 - 0x0
; CHECK-OBJ: }
