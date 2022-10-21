// RUN: llvm-mc -triple riscv64 -defsym POSITIVE=1 -mattr +v,+experimental-xsfvcp -show-inst %s | FileCheck %s -check-prefix CHECK-ENCD
// RUN: llvm-mc -triple riscv64 -defsym POSITIVE=1 -mattr +v,+experimental-xsfvcp -show-inst %s | FileCheck %s -check-prefix CHECK-INST
// RUN: not llvm-mc -triple riscv64 -defsym NEGATIVE=1 -mattr +v,+experimental-xsfvcp %s -filetype asm -o /dev/null 2>&1 | FileCheck %s -check-prefix CHECK-ERR

	.text

.ifdef POSITIVE
	sf.vc.x	0, -1, x2, -1

// CHECK-ENCD: # encoding: [0xdb,0x4f,0xf1,0x03]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_X
// CHECK-INST: #   <MCOperand Imm:-1>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Imm:-1>

	sf.vc.v.x 0, v3, x2, 11

// CHECK-ENCD: # encoding: [0xdb,0x41,0xb1,0x00]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_X
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Imm:11>

	sf.vc.i 0, 0, 0, 0

// CHECK-ENCD: # encoding: [0x5b,0x30,0x00,0x02]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_I
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>

	sf.vc.v.i 0, v1, 0, 0

// CHECK-ENCD: # encoding: [0xdb,0x30,0x00,0x00]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_I
// CHECK-INST: #   <MCOperand Reg:9>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>

	sf.vc.vv 0, 1, v1, v0

// CHECK-ENCD: # encoding: [0xdb,0x80,0x00,0x22]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_VV
// CHECK-INST: #   <MCOperand Imm:1>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:9>
// CHECK-INST: #   <MCOperand Reg:8>

	sf.vc.v.vv 0, v3, v1, v2

// CHECK-ENCD: # encoding: [0xdb,0x81,0x20,0x20]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_VV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:9>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.xv 0, -1, x2, v2

// CHECK-ENCD: # encoding: [0xdb,0x4f,0x21,0x22]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_XV
// CHECK-INST: #   <MCOperand Imm:-1>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.v.xv 0, v2, x2, v3

// CHECK-ENCD: # encoding: [0x5b,0x41,0x31,0x20]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_XV
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:11>

	sf.vc.iv 0, 10, 0, v2

// CHECK-ENCD: # encoding: [0x5b,0x35,0x20,0x22]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_IV
// CHECK-INST: #   <MCOperand Imm:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.v.iv 0, v2, 0, v2

// CHECK-ENCD: # encoding: [0x5b,0x31,0x20,0x20]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_IV
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.fv 0, -1, f1, v2

// CHECK-ENCD: # encoding: [0xdb,0xdf,0x20,0x2a]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_FV
// CHECK-INST: #   <MCOperand Imm:-1>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.v.fv 0, v1, f1, v1

// CHECK-ENCD: # encoding: [0xdb,0xd0,0x10,0x28]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_FV
// CHECK-INST: #   <MCOperand Reg:9>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:9>

	sf.vc.vvv 0, v3, v4, v5

// CHECK-ENCD: # encoding: [0xdb,0x01,0x52,0xa2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_VVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:12>
// CHECK-INST: #   <MCOperand Reg:13>

	sf.vc.v.vvv 0, v3, v4, v5

// CHECK-ENCD: # encoding: [0xdb,0x01,0x52,0xa0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_VVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:12>
// CHECK-INST: #   <MCOperand Reg:13>

	sf.vc.xvv 0, v3, x2, v2

// CHECK-ENCD: # encoding: [0xdb,0x41,0x21,0xa2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_XVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.v.xvv 0, v3, x2, v2

// CHECK-ENCD: # encoding: [0xdb,0x41,0x21,0xa0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_XVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:10>

	sf.vc.ivv 0, v3, 0, v3

// CHECK-ENCD: # encoding: [0xdb,0x31,0x30,0xa2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_IVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:11>

	sf.vc.v.ivv 0, v3, 0, v3

// CHECK-ENCD: # encoding: [0xdb,0x31,0x30,0xa0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_IVV
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:11>

	sf.vc.fvv 0, v2, f1, v3

// CHECK-ENCD: # encoding: [0x5b,0xd1,0x30,0xaa]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_FVV
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:11>

	sf.vc.v.fvv 0, v2, f1, v3

// CHECK-ENCD: # encoding: [0x5b,0xd1,0x30,0xa8]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_FVV
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:11>

	sf.vc.vvw 0, v2, v3, v4

// CHECK-ENCD: # encoding: [0x5b,0x81,0x41,0xf2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_VVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.v.vvw 0, v2, v3, v4

// CHECK-ENCD: # encoding: [0x5b,0x81,0x41,0xf0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_VVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:11>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.xvw 0, v2, x2, v4

// CHECK-ENCD: # encoding: [0x5b,0x41,0x41,0xf2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_XVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.v.xvw 0, v2, x2, v4

// CHECK-ENCD: # encoding: [0x5b,0x41,0x41,0xf0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_XVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:42>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.ivw 0, v2, 0, v4

// CHECK-ENCD: # encoding: [0x5b,0x31,0x40,0xf2]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_IVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.v.ivw 0, v2, 0, v4

// CHECK-ENCD: # encoding: [0x5b,0x31,0x40,0xf0]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_IVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.fvw 0, v2, f1, v4

// CHECK-ENCD: # encoding: [0x5b,0xd1,0x40,0xfa]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_FVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:12>

	sf.vc.v.fvw 0, v2, f1, v4

// CHECK-ENCD: # encoding: [0x5b,0xd1,0x40,0xf8]

// CHECK-INST: # <MCInst #{{[0-9]+}} SF_VC_V_FVW
// CHECK-INST: #   <MCOperand Reg:10>
// CHECK-INST: #   <MCOperand Imm:0>
// CHECK-INST: #   <MCOperand Reg:105>
// CHECK-INST: #   <MCOperand Reg:12>

.endif

.ifdef NEGATIVE
	sf.vc.x 4, 0, x1, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.x 4, 0, x1, 0
// CHECK-ERR:	        ^

	sf.vc.x 0, 16, x1, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.x 0, 16, x1, 0
// CHECK-ERR:	           ^
//
	sf.vc.x 0, 0, v1, 0		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.x 0, 0, v1, 0
// CHECK-ERR:	              ^

	sf.vc.x 0, 0, x1, 16		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.x 0, 0, x1, 16
// CHECK-ERR:	                  ^

	sf.vc.i 4, 0, 0, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.i 4, 0, 0, 0
// CHECK-ERR:	        ^

	sf.vc.i 0, 16, 0, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.i 0, 16, 0, 0
// CHECK-ERR:	           ^

	sf.vc.i 0, 0, 16, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.i 0, 0, 16, 0
// CHECK-ERR:	              ^

	sf.vc.i 0, 0, 0, 16		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.i 0, 0, 0, 16
// CHECK-ERR:	                 ^

	sf.vc.v.i 4, v1, 0, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.i 4, v1, 0, 0
// CHECK-ERR:	          ^

	sf.vc.v.i 0, x1, 0, 0		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.i 0, x1, 0, 0
// CHECK-ERR:	             ^

	sf.vc.v.i 0, v1, 16, 0		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.v.i 0, v1, 16, 0
// CHECK-ERR:	                 ^

	sf.vc.v.i 0, v1, 0, 16		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.v.i 0, v1, 0, 16
// CHECK-ERR:	                    ^

	sf.vc.vv 4, 0, v1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.vv 4, 0, v1, v2
// CHECK-ERR:	         ^

	sf.vc.vv 0, 16, v1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.vv 0, 16, v1, v2
// CHECK-ERR:	            ^

	sf.vc.vv 0, 0, x1, v2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vv 0, 0, x1, v2
// CHECK-ERR:	               ^

	sf.vc.vv 0, 0, v1, x2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vv 0, 0, v1, x2
// CHECK-ERR:	                   ^

	sf.vc.v.vv 4, v3, v1, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.vv 4, v3, v1, v2
// CHECK-ERR:	           ^

	sf.vc.v.vv 0, x1, v1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vv 0, x1, v1, v2
// CHECK-ERR:	              ^

	sf.vc.v.vv 0, v3, x1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vv 0, v3, x1, v2
// CHECK-ERR:	                  ^

	sf.vc.v.vv 0, v3, v1, x2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vv 0, v3, v1, x2
// CHECK-ERR:	                      ^

	sf.vc.xv 4, 10, x1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.xv 4, 10, x1, v2
// CHECK-ERR:	         ^

	sf.vc.xv 0, 16, x1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.xv 0, 16, x1, v2
// CHECK-ERR:	            ^

	sf.vc.xv 0, 10, v1, v2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xv 0, 10, v1, v2
// CHECK-ERR:	                ^

	sf.vc.xv 0, 10, x1, x2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xv 0, 10, x1, x2
// CHECK-ERR:	                    ^

	sf.vc.v.xv 4, v3, x1, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.xv 4, v3, x1, v2
// CHECK-ERR:	           ^

	sf.vc.v.xv 0, x3, v1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xv 0, x3, v1, v2
// CHECK-ERR:	              ^

	sf.vc.v.xv 0, v3, v1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xv 0, v3, v1, v2
// CHECK-ERR:	                  ^

	sf.vc.v.xv 0, v3, x1, x2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xv 0, v3, x1, x2
// CHECK-ERR:	                      ^

	sf.vc.iv 4, 10, 10, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.iv 4, 10, 10, v2
// CHECK-ERR:	         ^

	sf.vc.iv 0, 16, 10, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.iv 0, 16, 10, v2
// CHECK-ERR:	            ^

	sf.vc.iv 0, 10, 16, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.iv 0, 10, 16, v2
// CHECK-ERR:	                ^

	sf.vc.iv 0, 10, 10, x2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.iv 0, 10, 10, x2
// CHECK-ERR:	                    ^

	sf.vc.v.iv 4, v3, 10, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.iv 4, v3, 10, v2
// CHECK-ERR:	           ^

	sf.vc.v.iv 0, x3, 10, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.iv 0, x3, 10, v2
// CHECK-ERR:	              ^

	sf.vc.v.iv 0, v3, 16, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.v.iv 0, v3, 16, v2
// CHECK-ERR:	                  ^

	sf.vc.v.iv 0, v3, 10, x2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.iv 0, v3, 10, x2
// CHECK-ERR:	                      ^

	sf.vc.fv 2, 10, f1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.fv 2, 10, f1, v2
// CHECK-ERR:	         ^

	sf.vc.fv 0, 16, f1, v2		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.fv 0, 16, f1, v2
// CHECK-ERR:	            ^

	sf.vc.fv 0, 10, x1, v2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fv 0, 10, x1, v2
// CHECK-ERR:	                ^

	sf.vc.fv 0, 10, f1, x2		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fv 0, 10, f1, x2
// CHECK-ERR:	                    ^

	sf.vc.v.fv 2, v3, f1, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.v.fv 2, v3, f1, v2
// CHECK-ERR:	           ^

	sf.vc.v.fv 0, x3, f1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fv 0, x3, f1, v2
// CHECK-ERR:	              ^

	sf.vc.v.fv 0, v3, x1, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fv 0, v3, x1, v2
// CHECK-ERR:	                  ^

	sf.vc.v.fv 0, v3, f1, x2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fv 0, v3, f1, x2
// CHECK-ERR:	                      ^

	sf.vc.vvv 4, v1, v2, v3		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.vvv 4, v1, v2, v3
// CHECK-ERR:	          ^

	sf.vc.vvv 0, x1, v2, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvv 0, x1, v2, v3
// CHECK-ERR:	             ^

	sf.vc.vvv 0, v1, x2, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvv 0, v1, x2, v3
// CHECK-ERR:	                 ^

	sf.vc.vvv 0, v1, v2, x3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvv 0, v1, v2, x3
// CHECK-ERR:	                     ^

	sf.vc.v.vvv 4, v1, v2, v3	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.vvv 4, v1, v2, v3
// CHECK-ERR:	            ^

	sf.vc.v.vvv 0, x1, v2, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvv 0, x1, v2, v3
// CHECK-ERR:	               ^

	sf.vc.v.vvv 0, v1, x2, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvv 0, v1, x2, v3
// CHECK-ERR:	                   ^

	sf.vc.v.vvv 0, v1, v2, x3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvv 0, v1, v2, x3
// CHECK-ERR:	                       ^

	sf.vc.xvv 4, x1, v2, v3		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.xvv 4, x1, v2, v3
// CHECK-ERR:	          ^

	sf.vc.xvv 0, v1, v2, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvv 0, v1, v2, v3
// CHECK-ERR:	             ^

	sf.vc.xvv 0, x1, x2, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvv 0, x1, x2, v3
// CHECK-ERR:	                 ^

	sf.vc.xvv 0, x1, v2, x3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvv 0, x1, v2, x3
// CHECK-ERR:	                     ^

	sf.vc.v.xvv 4, x1, v2, v3	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.xvv 4, x1, v2, v3
// CHECK-ERR:	            ^

	sf.vc.v.xvv 0, v1, v2, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvv 0, v1, v2, v3
// CHECK-ERR:	               ^

	sf.vc.v.xvv 0, x1, x2, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvv 0, x1, x2, v3
// CHECK-ERR:	                   ^

	sf.vc.v.xvv 0, x1, v2, x3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvv 0, x1, v2, x3
// CHECK-ERR:	                       ^

	sf.vc.ivv 4, v2, 10, v1		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.ivv 4, v2, 10, v1
// CHECK-ERR:	          ^

	sf.vc.ivv 0, x2, 10, v1		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.ivv 0, x2, 10, v1
// CHECK-ERR:	             ^

	sf.vc.ivv 0, v2, 16, v1		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.ivv 0, v2, 16, v1
// CHECK-ERR:	                 ^

	sf.vc.ivv 0, v2, 10, x1		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.ivv 0, v2, 10, x1
// CHECK-ERR:	                     ^

	sf.vc.v.ivv 4, v3, 10, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.ivv 4, v3, 10, v2
// CHECK-ERR:	            ^

	sf.vc.v.ivv 0, x3, 10, v2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.ivv 0, x3, 10, v2
// CHECK-ERR:	               ^

	sf.vc.v.ivv 0, v3, 16, v2	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.v.ivv 0, v3, 16, v2
// CHECK-ERR:	                   ^

	sf.vc.v.ivv 0, v3, 10, x2	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.ivv 0, v3, 10, x2
// CHECK-ERR:	                       ^

	sf.vc.fvv 2, v2, f1, v3		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.fvv 2, v2, f1, v3
// CHECK-ERR:	          ^

	sf.vc.fvv 0, x2, f1, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvv 0, x2, f1, v3
// CHECK-ERR:	             ^

	sf.vc.fvv 0, v2, v1, v3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvv 0, v2, v1, v3
// CHECK-ERR:	                 ^

	sf.vc.fvv 0, v2, f1, x3		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvv 0, v2, f1, x3
// CHECK-ERR:	                     ^

	sf.vc.v.fvv 2, v2, f1, v3	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.v.fvv 2, v2, f1, v3
// CHECK-ERR:	            ^

	sf.vc.v.fvv 0, x2, f1, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvv 0, x2, f1, v3
// CHECK-ERR:	               ^

	sf.vc.v.fvv 0, v2, v1, v3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvv 0, v2, v1, v3
// CHECK-ERR:	                   ^

	sf.vc.v.fvv 0, v2, f1, x3	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvv 0, v2, f1, x3
// CHECK-ERR:	                       ^

	sf.vc.vvw 4, v2, v3, v4		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.vvw 4, v2, v3, v4
// CHECK-ERR:	          ^

	sf.vc.vvw 0, x2, v3, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvw 0, x2, v3, v4
// CHECK-ERR:	             ^

	sf.vc.vvw 0, v2, x3, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvw 0, v2, x3, v4
// CHECK-ERR:	                 ^

	sf.vc.vvw 0, v2, v3, x4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.vvw 0, v2, v3, x4
// CHECK-ERR:	                     ^

	sf.vc.v.vvw 4, v2, v3, v4	// immediate out of range
	sf.vc.v.vvw 0, x2, v3, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvw 0, x2, v3, v4
// CHECK-ERR:	               ^

	sf.vc.v.vvw 0, v2, x3, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvw 0, v2, x3, v4
// CHECK-ERR:	                   ^

	sf.vc.v.vvw 0, v2, v3, x4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.vvw 0, v2, v3, x4
// CHECK-ERR:	                       ^

	sf.vc.xvw 4, v2, x2, v4		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.xvw 4, v2, x2, v4
// CHECK-ERR:	          ^

	sf.vc.xvw 0, x2, x2, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvw 0, x2, x2, v4
// CHECK-ERR:	             ^

	sf.vc.xvw 0, v2, v2, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvw 0, v2, v2, v4
// CHECK-ERR:	                 ^

	sf.vc.xvw 0, v2, x2, x4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.xvw 0, v2, x2, x4
// CHECK-ERR:	                     ^

	sf.vc.v.xvw 4, v2, x2, v4	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.xvw 4, v2, x2, v4
// CHECK-ERR:	            ^

	sf.vc.v.xvw 0, x2, x2, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvw 0, x2, x2, v4
// CHECK-ERR:	               ^

	sf.vc.v.xvw 0, v2, v2, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvw 0, v2, v2, v4
// CHECK-ERR:	                   ^

	sf.vc.v.xvw 0, v2, x2, x4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.xvw 0, v2, x2, x4
// CHECK-ERR:	                       ^

	sf.vc.ivw 4, v2, 10, v4		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.ivw 4, v2, 10, v4
// CHECK-ERR:	          ^

	sf.vc.ivw 0, x2, 10, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.ivw 0, x2, 10, v4
// CHECK-ERR:	             ^

	sf.vc.ivw 0, v2, 16, v4		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.ivw 0, v2, 16, v4
// CHECK-ERR:	                 ^

	sf.vc.ivw 0, v2, 10, x4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.ivw 0, v2, 10, x4
// CHECK-ERR:	                     ^

	sf.vc.v.ivw 4, v2, 10, v4	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 3]
// CHECK-ERR:	sf.vc.v.ivw 4, v2, 10, v4
// CHECK-ERR:	            ^

	sf.vc.v.ivw 0, x2, 10, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.ivw 0, x2, 10, v4
// CHECK-ERR:	               ^

	sf.vc.v.ivw 0, v2, 16, v4	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [-16, 15]
// CHECK-ERR:	sf.vc.v.ivw 0, v2, 16, v4
// CHECK-ERR:	                   ^

	sf.vc.v.ivw 0, v2, 10, x4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.ivw 0, v2, 10, x4
// CHECK-ERR:	                       ^

	sf.vc.fvw 2, v2, f1, v4		// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.fvw 2, v2, f1, v4
// CHECK-ERR:	          ^

	sf.vc.fvw 0, x2, f1, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvw 0, x2, f1, v4
// CHECK-ERR:	             ^

	sf.vc.fvw 0, v2, v1, v4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvw 0, v2, v1, v4
// CHECK-ERR:	                 ^

	sf.vc.fvw 0, v2, f1, x4		// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.fvw 0, v2, f1, x4
// CHECK-ERR:	                     ^

	sf.vc.v.fvw 2, v2, f1, v4	// immediate out of range

// CHECK-ERR:  error: immediate must be an integer in the range [0, 1]
// CHECK-ERR:	sf.vc.v.fvw 2, v2, f1, v4
// CHECK-ERR:	          ^

	sf.vc.v.fvw 0, x2, f1, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvw 0, x2, f1, v4
// CHECK-ERR:	               ^

	sf.vc.v.fvw 0, v2, v1, v4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvw 0, v2, v1, v4
// CHECK-ERR:	                   ^

	sf.vc.v.fvw 0, v2, f1, x4	// invalid register class

// CHECK-ERR:  error: invalid operand for instruction
// CHECK-ERR:	sf.vc.v.fvw 0, v2, f1, x4
// CHECK-ERR:	                       ^

.endif
