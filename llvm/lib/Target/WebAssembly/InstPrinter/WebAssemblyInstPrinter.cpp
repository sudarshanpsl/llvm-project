//=- WebAssemblyInstPrinter.cpp - WebAssembly assembly instruction printing -=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Print MCInst instructions to wasm format.
///
//===----------------------------------------------------------------------===//

#include "InstPrinter/WebAssemblyInstPrinter.h"
#include "MCTargetDesc/WebAssemblyMCTargetDesc.h"
#include "WebAssembly.h"
#include "WebAssemblyMachineFunctionInfo.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Target/TargetRegisterInfo.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "WebAssemblyGenAsmWriter.inc"

WebAssemblyInstPrinter::WebAssemblyInstPrinter(const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI)
    : MCInstPrinter(MAI, MII, MRI), ControlFlowCounter(0) {}

void WebAssemblyInstPrinter::printRegName(raw_ostream &OS,
                                          unsigned RegNo) const {
  assert(RegNo != WebAssemblyFunctionInfo::UnusedReg);
  // Note that there's an implicit get_local/set_local here!
  OS << "$" << RegNo;
}

void WebAssemblyInstPrinter::printInst(const MCInst *MI, raw_ostream &OS,
                                       StringRef Annot,
                                       const MCSubtargetInfo & /*STI*/) {
  // Print the instruction (this uses the AsmStrings from the .td files).
  printInstruction(MI, OS);

  // Print any additional variadic operands.
  const MCInstrDesc &Desc = MII.get(MI->getOpcode());
  if (Desc.isVariadic())
    for (auto i = Desc.getNumOperands(), e = MI->getNumOperands(); i < e; ++i) {
      if (i != 0)
        OS << ", ";
      printOperand(MI, i, OS);
    }

  // Print any added annotation.
  printAnnotation(OS, Annot);

  if (CommentStream) {
    // Observe any effects on the control flow stack, for use in annotating
    // control flow label references.
    switch (MI->getOpcode()) {
    default:
      break;
    case WebAssembly::LOOP: {
      // Grab the TopLabel value first so that labels print in numeric order.
      uint64_t TopLabel = ControlFlowCounter++;
      ControlFlowStack.push_back(std::make_pair(ControlFlowCounter++, false));
      printAnnotation(OS, "label" + utostr(TopLabel) + ':');
      ControlFlowStack.push_back(std::make_pair(TopLabel, true));
      break;
    }
    case WebAssembly::BLOCK:
      ControlFlowStack.push_back(std::make_pair(ControlFlowCounter++, false));
      break;
    case WebAssembly::END_LOOP:
      ControlFlowStack.pop_back();
      printAnnotation(
          OS, "label" + utostr(ControlFlowStack.pop_back_val().first) + ':');
      break;
    case WebAssembly::END_BLOCK:
      printAnnotation(
          OS, "label" + utostr(ControlFlowStack.pop_back_val().first) + ':');
      break;
    }

    // Annotate any control flow label references.
    unsigned NumFixedOperands = Desc.NumOperands;
    SmallSet<uint64_t, 8> Printed;
    for (unsigned i = 0, e = MI->getNumOperands(); i < e; ++i) {
      const MCOperandInfo &Info = Desc.OpInfo[i];
      if (!(i < NumFixedOperands
                ? (Info.OperandType == WebAssembly::OPERAND_BASIC_BLOCK)
                : (Desc.TSFlags & WebAssemblyII::VariableOpImmediateIsLabel)))
        continue;
      uint64_t Depth = MI->getOperand(i).getImm();
      if (!Printed.insert(Depth).second)
        continue;
      const auto &Pair = ControlFlowStack.rbegin()[Depth];
      printAnnotation(OS, utostr(Depth) + ": " + (Pair.second ? "up" : "down") +
                              " to label" + utostr(Pair.first));
    }
  }
}

static std::string toString(const APFloat &FP) {
  // Print NaNs with custom payloads specially.
  if (FP.isNaN() &&
      !FP.bitwiseIsEqual(APFloat::getQNaN(FP.getSemantics())) &&
      !FP.bitwiseIsEqual(APFloat::getQNaN(FP.getSemantics(), /*Negative=*/true))) {
    APInt AI = FP.bitcastToAPInt();
    return
        std::string(AI.isNegative() ? "-" : "") + "nan:0x" +
        utohexstr(AI.getZExtValue() &
                  (AI.getBitWidth() == 32 ? INT64_C(0x007fffff) :
                                            INT64_C(0x000fffffffffffff)),
                  /*LowerCase=*/true);
  }

  // Use C99's hexadecimal floating-point representation.
  static const size_t BufBytes = 128;
  char buf[BufBytes];
  auto Written = FP.convertToHexString(
      buf, /*hexDigits=*/0, /*upperCase=*/false, APFloat::rmNearestTiesToEven);
  (void)Written;
  assert(Written != 0);
  assert(Written < BufBytes);
  return buf;
}

void WebAssemblyInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                          raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) {
    assert((OpNo < MII.get(MI->getOpcode()).getNumOperands() ||
            MII.get(MI->getOpcode()).TSFlags == 0) &&
           "WebAssembly variable_ops register ops don't use TSFlags");
    unsigned WAReg = Op.getReg();
    if (int(WAReg) >= 0)
      printRegName(O, WAReg);
    else if (OpNo >= MII.get(MI->getOpcode()).getNumDefs())
      O << "$pop" << WebAssemblyFunctionInfo::getWARegStackId(WAReg);
    else if (WAReg != WebAssemblyFunctionInfo::UnusedReg)
      O << "$push" << WebAssemblyFunctionInfo::getWARegStackId(WAReg);
    else
      O << "$drop";
    // Add a '=' suffix if this is a def.
    if (OpNo < MII.get(MI->getOpcode()).getNumDefs())
      O << '=';
  } else if (Op.isImm()) {
    assert((OpNo < MII.get(MI->getOpcode()).getNumOperands() ||
            (MII.get(MI->getOpcode()).TSFlags &
             WebAssemblyII::VariableOpIsImmediate)) &&
           "WebAssemblyII::VariableOpIsImmediate should be set for "
           "variable_ops immediate ops");
    // TODO: (MII.get(MI->getOpcode()).TSFlags &
    //        WebAssemblyII::VariableOpImmediateIsLabel)
    // can tell us whether this is an immediate referencing a label in the
    // control flow stack, and it may be nice to pretty-print.
    O << Op.getImm();
  } else if (Op.isFPImm()) {
    const MCInstrDesc &Desc = MII.get(MI->getOpcode());
    assert(OpNo < Desc.getNumOperands() &&
           "Unexpected floating-point immediate as a non-fixed operand");
    assert(Desc.TSFlags == 0 &&
           "WebAssembly variable_ops floating point ops don't use TSFlags");
    const MCOperandInfo &Info = Desc.OpInfo[OpNo];
    if (Info.OperandType == WebAssembly::OPERAND_F32IMM) {
      // TODO: MC converts all floating point immediate operands to double.
      // This is fine for numeric values, but may cause NaNs to change bits.
      O << toString(APFloat(float(Op.getFPImm())));
    } else {
      assert(Info.OperandType == WebAssembly::OPERAND_F64IMM);
      O << toString(APFloat(Op.getFPImm()));
    }
  } else {
    assert((OpNo < MII.get(MI->getOpcode()).getNumOperands() ||
            (MII.get(MI->getOpcode()).TSFlags &
             WebAssemblyII::VariableOpIsImmediate)) &&
           "WebAssemblyII::VariableOpIsImmediate should be set for "
           "variable_ops expr ops");
    assert(Op.isExpr() && "unknown operand kind in printOperand");
    Op.getExpr()->print(O, &MAI);
  }
}

void
WebAssemblyInstPrinter::printWebAssemblyP2AlignOperand(const MCInst *MI,
                                                       unsigned OpNo,
                                                       raw_ostream &O) {
  int64_t Imm = MI->getOperand(OpNo).getImm();
  if (Imm == WebAssembly::GetDefaultP2Align(MI->getOpcode()))
    return;
  O << ":p2align=" << Imm;
}

const char *llvm::WebAssembly::TypeToString(MVT Ty) {
  switch (Ty.SimpleTy) {
  case MVT::i32:
    return "i32";
  case MVT::i64:
    return "i64";
  case MVT::f32:
    return "f32";
  case MVT::f64:
    return "f64";
  case MVT::v16i8:
  case MVT::v8i16:
  case MVT::v4i32:
  case MVT::v4f32:
    return "v128";
  default:
    llvm_unreachable("unsupported type");
  }
}
