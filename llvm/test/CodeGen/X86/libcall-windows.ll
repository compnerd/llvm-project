; RUN: llc -mtriple i686-windows-msvc -filetype asm -o - %s | FileCheck %s

define i64 @ftol3(half %h) {
entry:
  %conv = fptosi half %h to i64
  ret i64 %conv
}

; CHECK-LABEL: _ftol3
; CHECK: movss   (%esp), %xmm0
; CHECK: calll   __ftol3

define i64 @ftoul3(half %h) {
entry:
  %conv = fptoui half %h to i64
  ret i64 %conv
}

; CHECK-LABEL: _ftoul3
; CHECK: movss   (%esp), %xmm0
; CHECK: calll   __ftoul3
