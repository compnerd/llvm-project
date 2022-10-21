//===-- RISCVInstrInfoXsfvcp.td - RISC-V VCIX Instructions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0, HasVLOp = 1, HasSEWOp = 1 in {

class VCIXInst<dag outs, dag ins, string suffix, string args>
    : RVInst<outs, ins, "sf.vc." # suffix, args, [], InstFormatOther>,
      Sched<[]> {
  let Opcode = 0b1011011;
}

class _1<dag outs, dag ins, string suffix, string args>
    : VCIXInst<outs, ins, suffix, args> {
  bits<2> op;
  bits<5> rs2;
  bits<5> vd;

  let Inst{31-28} = 0b0000;
  let Inst{27-26} = op;
  let Inst{24-20} = rs2;
  let Inst{11-7} = vd;
}

class X<dag outs, dag ins, string suffix>
    : _1<outs, ins, suffix, "$op, $vd, $rs1, $rs2"> {
  bits<5> rs1;

  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b100;
}

class I<dag outs, dag ins, string suffix>
    : _1<outs, ins, suffix, "$op, $vd, $imm, $rs2"> {
  bits<5> imm;

  let Inst{19-15} = imm;
  let Inst{14-12} = 0b011;
}

class _2<dag outs, dag ins, string suffix, string args>
    : VCIXInst<outs, ins, suffix, args> {
  bits<5> vs2;
  bits<5> vd;

  let Inst{31-28} = 0b0010;
  let Inst{24-20} = vs2;
  let Inst{11-7} = vd;
}

class VV<dag outs, dag ins, string suffix>
    : _2<outs, ins, suffix, "$op, $vd, $vs1, $vs2"> {
  bits<2> op;
  bits<5> vs1;

  let Inst{27-26} = op;
  let Inst{19-15} = vs1;
  let Inst{14-12} = 0b000;
}

class XV<dag outs, dag ins, string suffix>
    : _2<outs, ins, suffix, "$op, $vd, $rs1, $vs2"> {
  bits<2> op;
  bits<5> rs1;

  let Inst{27-26} = op;
  let Inst{19-15} = rs1;
  let Inst{14-12} = 0b100;
}

class IV<dag outs, dag ins, string suffix>
    : _2<outs, ins, suffix, "$op, $vd, $imm, $vs2"> {
  bits<2> op;
  bits<5> imm;

  let Inst{27-26} = op;
  let Inst{19-15} = imm;
  let Inst{14-12} = 0b011;
}

class FV<dag outs, dag ins, string suffix>
    : _2<outs, ins, suffix, "$op, $vd, $fs1, $vs2"> {
  bits<1> op;
  bits<5> fs1;

  let Inst{27} = 1;
  let Inst{26} = op;
  let Inst{19-15} = fs1;
  let Inst{14-12} = 0b101;
}

class _3<dag outs, dag ins, string suffix, string args>
    : VCIXInst<outs, ins, suffix, args> {
  bits<5> vs2;
  bits<5> vd;

  let Inst{31-28} = 0b1010;
  let Inst{24-20} = vs2;
  let Inst{11-7} = vd;
}

class VVV<dag outs, dag ins, string suffix>
    : _3<outs, ins, suffix, "$op, $vd, $vs1, $vs2"> {
  bits<2> op;
  bits<5> vs1;

  let Inst{27-26} = op;
  let Inst{19-15} = vs1;
  let Inst{14-12} = 0b000;
}

class XVV<dag outs, dag ins, string suffix>
    : _3<outs, ins, suffix, "$op, $vd, $xs1, $vs2"> {
  bits<2> op;
  bits<5> xs1;

  let Inst{27-26} = op;
  let Inst{19-15} = xs1;
  let Inst{14-12} = 0b100;
}

class IVV<dag outs, dag ins, string suffix>
    : _3<outs, ins, suffix, "$op, $vd, $imm, $vs2"> {
  bits<2> op;
  bits<5> imm;

  let Inst{27-26} = op;
  let Inst{19-15} = imm;
  let Inst{14-12} = 0b011;
}

class FVV<dag outs, dag ins, string suffix>
    : _3<outs, ins, suffix, "$op, $vd, $fs1, $vs2"> {
  bits<1> op;
  bits<5> fs1;

  let Inst{27} = 1;
  let Inst{26} = op;
  let Inst{19-15} = fs1;
  let Inst{14-12} = 0b101;
}

class _4<dag outs, dag ins, string suffix, string args>
    : VCIXInst<outs, ins, suffix, args> {
  bits<5> vs2;
  bits<5> vd;

  let Inst{31-28} = 0b1111;
  let Inst{24-20} = vs2;
  let Inst{11-7} = vd;
}

class VVW<dag outs, dag ins, string suffix>
    : _4<outs, ins, suffix, "$op, $vd, $vs1, $vs2"> {
  bits<2> op;
  bits<5> vs1;

  let Inst{27-26} = op;
  let Inst{19-15} = vs1;
  let Inst{14-12} = 0b000;
}

class XVW<dag outs, dag ins, string suffix>
    : _4<outs, ins, suffix, "$op, $vd, $xs1, $vs2"> {
  bits<2> op;
  bits<5> xs1;

  let Inst{27-26} = op;
  let Inst{19-15} = xs1;
  let Inst{14-12} = 0b100;
}

class IVW<dag outs, dag ins, string suffix>
    : _4<outs, ins, suffix, "$op, $vd, $imm, $vs2"> {
  bits<2> op;
  bits<5> imm;

  let Inst{27-26} = op;
  let Inst{19-15} = imm;
  let Inst{14-12} = 0b011;
}

class FVW<dag outs, dag ins, string suffix>
    : _4<outs, ins, suffix, "$op, $vd, $fs1, $vs2"> {
  bits<1> op;
  bits<5> fs1;

  let Inst{27} = 1;
  let Inst{26} = op;
  let Inst{19-15} = fs1;
  let Inst{14-12} = 0b101;
}

}

let Predicates = [HasVendorExtXsfvcp] in {

def SF_VC_X
    : X<(outs), (ins simm5:$vd, uimm2:$op, GPR:$rs1, simm5:$rs2), "x"> {
  let Inst{25} = 1;
}

def SF_VC_V_X
    : X<(outs VR:$vd), (ins uimm2:$op, GPR:$rs1, simm5:$rs2), "v.x"> {
  let Inst{25} = 0;
}

def SF_VC_I
    : I<(outs), (ins simm5:$vd, uimm2:$op, simm5:$imm, simm5:$rs2), "i"> {
  let Inst{25} = 1;
}

def SF_VC_V_I
    : I<(outs VR:$vd), (ins uimm2:$op, simm5:$imm, simm5:$rs2), "v.i"> {
  let Inst{25} = 0;
}

def SF_VC_VV
    : VV<(outs), (ins simm5:$vd, uimm2:$op, VR:$vs1, VR:$vs2), "vv"> {
  let Inst{25} = 1;
}

def SF_VC_V_VV
    : VV<(outs VR:$vd), (ins uimm2:$op, VR:$vs1, VR:$vs2), "v.vv"> {
  let Inst{25} = 0;
}

def SF_VC_XV
    : XV<(outs), (ins simm5:$vd, uimm2:$op, GPR:$rs1, VR:$vs2), "xv"> {
  let Inst{25} = 1;
}

def SF_VC_V_XV
    : XV<(outs VR:$vd), (ins uimm2:$op, GPR:$rs1, VR:$vs2), "v.xv"> {
  let Inst{25} = 0;
}

def SF_VC_IV
    : IV<(outs), (ins simm5:$vd, uimm2:$op, simm5:$imm, VR:$vs2), "iv"> {
  let Inst{25} = 1;
}

def SF_VC_V_IV
    : IV<(outs VR:$vd), (ins uimm2:$op, simm5:$imm, VR:$vs2), "v.iv"> {
  let Inst{25} = 0;
}

def SF_VC_FV
    : FV<(outs), (ins simm5:$vd, uimm1:$op, FPR32:$fs1, VR:$vs2), "fv"> {
  let Inst{25} = 1;
}

def SF_VC_V_FV
    : FV<(outs VR:$vd), (ins uimm1:$op, FPR32:$fs1, VR:$vs2), "v.fv"> {
  let Inst{25} = 0;
}

def SF_VC_VVV
    : VVV<(outs), (ins VR:$vd, uimm2:$op, VR:$vs1, VR:$vs2), "vvv"> {
  let Inst{25} = 1;
}

def SF_VC_V_VVV
    : VVV<(outs VR:$vd), (ins uimm2:$op, VR:$vs1, VR:$vs2), "v.vvv"> {
  let Inst{25} = 0;
}

def SF_VC_XVV
    : XVV<(outs), (ins VR:$vd, uimm2:$op, GPR:$xs1, VR:$vs2), "xvv"> {
  let Inst{25} = 1;
}

def SF_VC_V_XVV
    : XVV<(outs VR:$vd), (ins uimm2:$op, GPR:$xs1, VR:$vs2), "v.xvv"> {
  let Inst{25} = 0;
}

def SF_VC_IVV
    : IVV<(outs), (ins VR:$vd, uimm2:$op, simm5:$imm, VR:$vs2), "ivv"> {
  let Inst{25} = 1;
}

def SF_VC_V_IVV
    : IVV<(outs VR:$vd), (ins uimm2:$op, simm5:$imm, VR:$vs2), "v.ivv"> {
  let Inst{25} = 0;
}

def SF_VC_FVV
    : FVV<(outs), (ins VR:$vd, uimm1:$op, FPR32:$fs1, VR:$vs2), "fvv"> {
  let Inst{25} = 1;
}

def SF_VC_V_FVV
    : FVV<(outs VR:$vd), (ins uimm1:$op, FPR32:$fs1, VR:$vs2), "v.fvv"> {
  let Inst{25} = 0;
}

def SF_VC_VVW
    : VVW<(outs), (ins VR:$vd, uimm2:$op, VR:$vs1, VR:$vs2), "vvw"> {
  let Inst{25} = 1;
}

def SF_VC_V_VVW
    : VVW<(outs VR:$vd), (ins uimm2:$op, VR:$vs1, VR:$vs2), "v.vvw"> {
  let Inst{25} = 0;
}

def SF_VC_XVW
    : XVW<(outs), (ins VR:$vd, uimm2:$op, GPR:$xs1, VR:$vs2), "xvw"> {
  let Inst{25} = 1;
}

def SF_VC_V_XVW
    : XVW<(outs VR:$vd), (ins uimm2:$op, GPR:$xs1, VR:$vs2), "v.xvw"> {
  let Inst{25} = 0;
}

def SF_VC_IVW
    : IVW<(outs), (ins VR:$vd, uimm2:$op, simm5:$imm, VR:$vs2), "ivw"> {
  let Inst{25} = 1;
}

def SF_VC_V_IVW
    : IVW<(outs VR:$vd), (ins uimm2:$op, simm5:$imm, VR:$vs2), "v.ivw"> {
  let Inst{25} = 0;
}

def SF_VC_FVW
    : FVW<(outs), (ins VR:$vd, uimm1:$op, FPR32:$fs1, VR:$vs2), "fvw"> {
  let Inst{25} = 1;
}

def SF_VC_V_FVW
    : FVW<(outs VR:$vd), (ins uimm1:$op, FPR32:$fs1, VR:$vs2), "v.fvw"> {
  let Inst{25} = 0;
}

}
