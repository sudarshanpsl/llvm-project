//===-- HexagonDisassembler.cpp - Disassembler for Hexagon ISA ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hexagon-disassembler"

#include "Hexagon.h"
#include "MCTargetDesc/HexagonBaseInfo.h"
#include "MCTargetDesc/HexagonMCChecker.h"
#include "MCTargetDesc/HexagonMCTargetDesc.h"
#include "MCTargetDesc/HexagonMCInstrInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

using namespace llvm;
using namespace Hexagon;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// \brief Hexagon disassembler for all Hexagon platforms.
class HexagonDisassembler : public MCDisassembler {
public:
  std::unique_ptr<MCInstrInfo const> const MCII;
  std::unique_ptr<MCInst *> CurrentBundle;

  HexagonDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                      MCInstrInfo const *MCII)
      : MCDisassembler(STI, Ctx), MCII(MCII), CurrentBundle(new MCInst *) {}

  DecodeStatus getSingleInstruction(MCInst &Instr, MCInst &MCB,
                                    ArrayRef<uint8_t> Bytes, uint64_t Address,
                                    raw_ostream &VStream, raw_ostream &CStream,
                                    bool &Complete) const;
  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &VStream,
                              raw_ostream &CStream) const override;
  void addSubinstOperands(MCInst *MI, unsigned opcode, unsigned inst) const;
};

namespace {
  uint32_t fullValue(MCInstrInfo const &MCII, MCInst &MCB, MCInst &MI,
                     int64_t Value) {
    MCInst const *Extender = HexagonMCInstrInfo::extenderForIndex(
      MCB, HexagonMCInstrInfo::bundleSize(MCB));
    if (!Extender || MI.size() != HexagonMCInstrInfo::getExtendableOp(MCII, MI))
      return Value;
    unsigned Alignment = HexagonMCInstrInfo::getExtentAlignment(MCII, MI);
    uint32_t Lower6 = static_cast<uint32_t>(Value >> Alignment) & 0x3f;
    int64_t Bits;
    bool Success = Extender->getOperand(0).getExpr()->evaluateAsAbsolute(Bits);
    assert(Success); (void)Success;
    uint32_t Upper26 = static_cast<uint32_t>(Bits);
    uint32_t Operand = Upper26 | Lower6;
    return Operand;
  }
  HexagonDisassembler const &disassembler(void const *Decoder) {
    return *static_cast<HexagonDisassembler const *>(Decoder);
  }
  template <size_t T>
  void signedDecoder(MCInst &MI, unsigned tmp, const void *Decoder) {
    HexagonDisassembler const &Disassembler = disassembler(Decoder);
    int64_t FullValue =
        fullValue(*Disassembler.MCII, **Disassembler.CurrentBundle, MI,
                  SignExtend64<T>(tmp));
    int64_t Extended = SignExtend64<32>(FullValue);
    HexagonMCInstrInfo::addConstant(MI, Extended, Disassembler.getContext());
  }
}
} // end anonymous namespace

// Forward declare these because the auto-generated code will reference them.
// Definitions are further down.

static DecodeStatus DecodeIntRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeGeneralSubRegsRegisterClass(MCInst &Inst,
                                                      unsigned RegNo,
                                                      uint64_t Address,
                                                      const void *Decoder);
static DecodeStatus DecodeIntRegsLow8RegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder);
static DecodeStatus DecodeVectorRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus DecodeDoubleRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus
DecodeGeneralDoubleLow8RegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                         uint64_t Address, const void *Decoder);
static DecodeStatus DecodeVecDblRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus DecodePredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder);
static DecodeStatus DecodeVecPredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder);
static DecodeStatus DecodeCtrRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeModRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeCtrRegs64RegisterClass(MCInst &Inst, unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder);

static DecodeStatus unsignedImmDecoder(MCInst &MI, unsigned tmp,
                                       uint64_t Address, const void *Decoder);
static DecodeStatus s32_0ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder);
static DecodeStatus s8_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                 const void *Decoder);
static DecodeStatus s6_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_1ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_2ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_3ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_6ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s3_6ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus brtargetDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);

#include "HexagonDepDecoders.h"
#include "HexagonGenDisassemblerTables.inc"

static MCDisassembler *createHexagonDisassembler(const Target &T,
                                                 const MCSubtargetInfo &STI,
                                                 MCContext &Ctx) {
  return new HexagonDisassembler(STI, Ctx, T.createMCInstrInfo());
}

extern "C" void LLVMInitializeHexagonDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheHexagonTarget(),
                                         createHexagonDisassembler);
}

DecodeStatus HexagonDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                                                 ArrayRef<uint8_t> Bytes,
                                                 uint64_t Address,
                                                 raw_ostream &os,
                                                 raw_ostream &cs) const {
  DecodeStatus Result = DecodeStatus::Success;
  bool Complete = false;
  Size = 0;

  *CurrentBundle = &MI;
  MI = HexagonMCInstrInfo::createBundle();
  while (Result == Success && !Complete) {
    if (Bytes.size() < HEXAGON_INSTR_SIZE)
      return MCDisassembler::Fail;
    MCInst *Inst = new (getContext()) MCInst;
    Result = getSingleInstruction(*Inst, MI, Bytes, Address, os, cs, Complete);
    MI.addOperand(MCOperand::createInst(Inst));
    Size += HEXAGON_INSTR_SIZE;
    Bytes = Bytes.slice(HEXAGON_INSTR_SIZE);
  }
  if (Result == MCDisassembler::Fail)
    return Result;
  if (Size > HEXAGON_MAX_PACKET_SIZE)
    return MCDisassembler::Fail;
  HexagonMCChecker Checker(*MCII, STI, MI, MI, *getContext().getRegisterInfo());
  if (!Checker.check())
    return MCDisassembler::Fail;
  return MCDisassembler::Success;
}

namespace {
void adjustDuplex(MCInst &MI, MCContext &Context) {
  switch (MI.getOpcode()) {
  case Hexagon::SA1_setin1:
    MI.insert(MI.begin() + 1,
              MCOperand::createExpr(MCConstantExpr::create(-1, Context)));
    break;
  case Hexagon::SA1_dec:
    MI.insert(MI.begin() + 2,
              MCOperand::createExpr(MCConstantExpr::create(-1, Context)));
    break;
  default:
    break;
  }
}
}

DecodeStatus HexagonDisassembler::getSingleInstruction(
    MCInst &MI, MCInst &MCB, ArrayRef<uint8_t> Bytes, uint64_t Address,
    raw_ostream &os, raw_ostream &cs, bool &Complete) const {
  assert(Bytes.size() >= HEXAGON_INSTR_SIZE);

  uint32_t Instruction = support::endian::read32le(Bytes.data());

  auto BundleSize = HexagonMCInstrInfo::bundleSize(MCB);
  if ((Instruction & HexagonII::INST_PARSE_MASK) ==
      HexagonII::INST_PARSE_LOOP_END) {
    if (BundleSize == 0)
      HexagonMCInstrInfo::setInnerLoop(MCB);
    else if (BundleSize == 1)
      HexagonMCInstrInfo::setOuterLoop(MCB);
    else
      return DecodeStatus::Fail;
  }

  MCInst const *Extender = HexagonMCInstrInfo::extenderForIndex(
      MCB, HexagonMCInstrInfo::bundleSize(MCB));

  DecodeStatus Result = DecodeStatus::Fail;
  if ((Instruction & HexagonII::INST_PARSE_MASK) ==
      HexagonII::INST_PARSE_DUPLEX) {
    unsigned duplexIClass;
    uint8_t const *DecodeLow, *DecodeHigh;
    duplexIClass = ((Instruction >> 28) & 0xe) | ((Instruction >> 13) & 0x1);
    switch (duplexIClass) {
    default:
      return MCDisassembler::Fail;
    case 0:
      DecodeLow = DecoderTableSUBINSN_L132;
      DecodeHigh = DecoderTableSUBINSN_L132;
      break;
    case 1:
      DecodeLow = DecoderTableSUBINSN_L232;
      DecodeHigh = DecoderTableSUBINSN_L132;
      break;
    case 2:
      DecodeLow = DecoderTableSUBINSN_L232;
      DecodeHigh = DecoderTableSUBINSN_L232;
      break;
    case 3:
      DecodeLow = DecoderTableSUBINSN_A32;
      DecodeHigh = DecoderTableSUBINSN_A32;
      break;
    case 4:
      DecodeLow = DecoderTableSUBINSN_L132;
      DecodeHigh = DecoderTableSUBINSN_A32;
      break;
    case 5:
      DecodeLow = DecoderTableSUBINSN_L232;
      DecodeHigh = DecoderTableSUBINSN_A32;
      break;
    case 6:
      DecodeLow = DecoderTableSUBINSN_S132;
      DecodeHigh = DecoderTableSUBINSN_A32;
      break;
    case 7:
      DecodeLow = DecoderTableSUBINSN_S232;
      DecodeHigh = DecoderTableSUBINSN_A32;
      break;
    case 8:
      DecodeLow = DecoderTableSUBINSN_S132;
      DecodeHigh = DecoderTableSUBINSN_L132;
      break;
    case 9:
      DecodeLow = DecoderTableSUBINSN_S132;
      DecodeHigh = DecoderTableSUBINSN_L232;
      break;
    case 10:
      DecodeLow = DecoderTableSUBINSN_S132;
      DecodeHigh = DecoderTableSUBINSN_S132;
      break;
    case 11:
      DecodeLow = DecoderTableSUBINSN_S232;
      DecodeHigh = DecoderTableSUBINSN_S132;
      break;
    case 12:
      DecodeLow = DecoderTableSUBINSN_S232;
      DecodeHigh = DecoderTableSUBINSN_L132;
      break;
    case 13:
      DecodeLow = DecoderTableSUBINSN_S232;
      DecodeHigh = DecoderTableSUBINSN_L232;
      break;
    case 14:
      DecodeLow = DecoderTableSUBINSN_S232;
      DecodeHigh = DecoderTableSUBINSN_S232;
      break;
    }
    MI.setOpcode(Hexagon::DuplexIClass0 + duplexIClass);
    MCInst *MILow = new (getContext()) MCInst;
    MCInst *MIHigh = new (getContext()) MCInst;
    Result = decodeInstruction(DecodeLow, *MILow, Instruction & 0x1fff, Address,
                               this, STI);
    if (Result != DecodeStatus::Success)
      return DecodeStatus::Fail;
    adjustDuplex(*MILow, getContext());
    Result = decodeInstruction(
        DecodeHigh, *MIHigh, (Instruction >> 16) & 0x1fff, Address, this, STI);
    if (Result != DecodeStatus::Success)
      return DecodeStatus::Fail;
    adjustDuplex(*MIHigh, getContext());
    MCOperand OPLow = MCOperand::createInst(MILow);
    MCOperand OPHigh = MCOperand::createInst(MIHigh);
    MI.addOperand(OPLow);
    MI.addOperand(OPHigh);
    Complete = true;
  } else {
    if ((Instruction & HexagonII::INST_PARSE_MASK) ==
        HexagonII::INST_PARSE_PACKET_END)
      Complete = true;

    if (Extender != nullptr)
      Result = decodeInstruction(DecoderTableMustExtend32, MI, Instruction,
                                 Address, this, STI);

    if (Result != MCDisassembler::Success)
      Result = decodeInstruction(DecoderTable32, MI, Instruction, Address, this,
                                 STI);

    if (Result != MCDisassembler::Success &&
        STI.getFeatureBits()[Hexagon::ExtensionHVX])
      Result = decodeInstruction(DecoderTableEXT_mmvec32, MI, Instruction,
                                 Address, this, STI);

  }

  switch (MI.getOpcode()) {
  case Hexagon::J4_cmpeqn1_f_jumpnv_nt:
  case Hexagon::J4_cmpeqn1_f_jumpnv_t:
  case Hexagon::J4_cmpeqn1_fp0_jump_nt:
  case Hexagon::J4_cmpeqn1_fp0_jump_t:
  case Hexagon::J4_cmpeqn1_fp1_jump_nt:
  case Hexagon::J4_cmpeqn1_fp1_jump_t:
  case Hexagon::J4_cmpeqn1_t_jumpnv_nt:
  case Hexagon::J4_cmpeqn1_t_jumpnv_t:
  case Hexagon::J4_cmpeqn1_tp0_jump_nt:
  case Hexagon::J4_cmpeqn1_tp0_jump_t:
  case Hexagon::J4_cmpeqn1_tp1_jump_nt:
  case Hexagon::J4_cmpeqn1_tp1_jump_t:
  case Hexagon::J4_cmpgtn1_f_jumpnv_nt:
  case Hexagon::J4_cmpgtn1_f_jumpnv_t:
  case Hexagon::J4_cmpgtn1_fp0_jump_nt:
  case Hexagon::J4_cmpgtn1_fp0_jump_t:
  case Hexagon::J4_cmpgtn1_fp1_jump_nt:
  case Hexagon::J4_cmpgtn1_fp1_jump_t:
  case Hexagon::J4_cmpgtn1_t_jumpnv_nt:
  case Hexagon::J4_cmpgtn1_t_jumpnv_t:
  case Hexagon::J4_cmpgtn1_tp0_jump_nt:
  case Hexagon::J4_cmpgtn1_tp0_jump_t:
  case Hexagon::J4_cmpgtn1_tp1_jump_nt:
  case Hexagon::J4_cmpgtn1_tp1_jump_t:
    MI.insert(MI.begin() + 1,
              MCOperand::createExpr(MCConstantExpr::create(-1, getContext())));
    break;
  default:
    break;
  }

  if (HexagonMCInstrInfo::isNewValue(*MCII, MI)) {
    unsigned OpIndex = HexagonMCInstrInfo::getNewValueOp(*MCII, MI);
    MCOperand &MCO = MI.getOperand(OpIndex);
    assert(MCO.isReg() && "New value consumers must be registers");
    unsigned Register =
        getContext().getRegisterInfo()->getEncodingValue(MCO.getReg());
    if ((Register & 0x6) == 0)
      // HexagonPRM 10.11 Bit 1-2 == 0 is reserved
      return MCDisassembler::Fail;
    unsigned Lookback = (Register & 0x6) >> 1;
    unsigned Offset = 1;
    bool Vector = HexagonMCInstrInfo::isVector(*MCII, MI);
    auto Instructions = HexagonMCInstrInfo::bundleInstructions(**CurrentBundle);
    auto i = Instructions.end() - 1;
    for (auto n = Instructions.begin() - 1;; --i, ++Offset) {
      if (i == n)
        // Couldn't find producer
        return MCDisassembler::Fail;
      if (Vector && !HexagonMCInstrInfo::isVector(*MCII, *i->getInst()))
        // Skip scalars when calculating distances for vectors
        ++Lookback;
      if (HexagonMCInstrInfo::isImmext(*i->getInst()))
        ++Lookback;
      if (Offset == Lookback)
        break;
    }
    auto const &Inst = *i->getInst();
    bool SubregBit = (Register & 0x1) != 0;
    if (SubregBit && HexagonMCInstrInfo::hasNewValue2(*MCII, Inst)) {
      // If subreg bit is set we're selecting the second produced newvalue
      unsigned Producer =
          HexagonMCInstrInfo::getNewValueOperand2(*MCII, Inst).getReg();
      assert(Producer != Hexagon::NoRegister);
      MCO.setReg(Producer);
    } else if (HexagonMCInstrInfo::hasNewValue(*MCII, Inst)) {
      unsigned Producer =
          HexagonMCInstrInfo::getNewValueOperand(*MCII, Inst).getReg();
      if (Producer >= Hexagon::W0 && Producer <= Hexagon::W15)
        Producer = ((Producer - Hexagon::W0) << 1) + SubregBit + Hexagon::V0;
      else if (SubregBit)
        // Hexagon PRM 10.11 New-value operands
        // Nt[0] is reserved and should always be encoded as zero.
        return MCDisassembler::Fail;
      assert(Producer != Hexagon::NoRegister);
      MCO.setReg(Producer);
    } else
      return MCDisassembler::Fail;
  }

  if (Extender != nullptr) {
    MCInst const &Inst = HexagonMCInstrInfo::isDuplex(*MCII, MI)
                             ? *MI.getOperand(1).getInst()
                             : MI;
    if (!HexagonMCInstrInfo::isExtendable(*MCII, Inst) &&
        !HexagonMCInstrInfo::isExtended(*MCII, Inst))
      return MCDisassembler::Fail;
  }
  return Result;
}

static DecodeStatus DecodeRegisterClass(MCInst &Inst, unsigned RegNo,
                                        ArrayRef<MCPhysReg> Table) {
  if (RegNo < Table.size()) {
    Inst.addOperand(MCOperand::createReg(Table[RegNo]));
    return MCDisassembler::Success;
  }

  return MCDisassembler::Fail;
}

static DecodeStatus DecodeIntRegsLow8RegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder) {
  return DecodeIntRegsRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeIntRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  static const MCPhysReg IntRegDecoderTable[] = {
      Hexagon::R0,  Hexagon::R1,  Hexagon::R2,  Hexagon::R3,  Hexagon::R4,
      Hexagon::R5,  Hexagon::R6,  Hexagon::R7,  Hexagon::R8,  Hexagon::R9,
      Hexagon::R10, Hexagon::R11, Hexagon::R12, Hexagon::R13, Hexagon::R14,
      Hexagon::R15, Hexagon::R16, Hexagon::R17, Hexagon::R18, Hexagon::R19,
      Hexagon::R20, Hexagon::R21, Hexagon::R22, Hexagon::R23, Hexagon::R24,
      Hexagon::R25, Hexagon::R26, Hexagon::R27, Hexagon::R28, Hexagon::R29,
      Hexagon::R30, Hexagon::R31};

  return DecodeRegisterClass(Inst, RegNo, IntRegDecoderTable);
}

static DecodeStatus DecodeGeneralSubRegsRegisterClass(MCInst &Inst,
                                                      unsigned RegNo,
                                                      uint64_t Address,
                                                      const void *Decoder) {
  static const MCPhysReg GeneralSubRegDecoderTable[] = {
      Hexagon::R0,  Hexagon::R1,  Hexagon::R2,  Hexagon::R3,
      Hexagon::R4,  Hexagon::R5,  Hexagon::R6,  Hexagon::R7,
      Hexagon::R16, Hexagon::R17, Hexagon::R18, Hexagon::R19,
      Hexagon::R20, Hexagon::R21, Hexagon::R22, Hexagon::R23,
  };

  return DecodeRegisterClass(Inst, RegNo, GeneralSubRegDecoderTable);
}

static DecodeStatus DecodeVectorRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg VecRegDecoderTable[] = {
      Hexagon::V0,  Hexagon::V1,  Hexagon::V2,  Hexagon::V3,  Hexagon::V4,
      Hexagon::V5,  Hexagon::V6,  Hexagon::V7,  Hexagon::V8,  Hexagon::V9,
      Hexagon::V10, Hexagon::V11, Hexagon::V12, Hexagon::V13, Hexagon::V14,
      Hexagon::V15, Hexagon::V16, Hexagon::V17, Hexagon::V18, Hexagon::V19,
      Hexagon::V20, Hexagon::V21, Hexagon::V22, Hexagon::V23, Hexagon::V24,
      Hexagon::V25, Hexagon::V26, Hexagon::V27, Hexagon::V28, Hexagon::V29,
      Hexagon::V30, Hexagon::V31};

  return DecodeRegisterClass(Inst, RegNo, VecRegDecoderTable);
}

static DecodeStatus DecodeDoubleRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg DoubleRegDecoderTable[] = {
      Hexagon::D0,  Hexagon::D1,  Hexagon::D2,  Hexagon::D3,
      Hexagon::D4,  Hexagon::D5,  Hexagon::D6,  Hexagon::D7,
      Hexagon::D8,  Hexagon::D9,  Hexagon::D10, Hexagon::D11,
      Hexagon::D12, Hexagon::D13, Hexagon::D14, Hexagon::D15};

  return DecodeRegisterClass(Inst, RegNo >> 1, DoubleRegDecoderTable);
}

static DecodeStatus DecodeGeneralDoubleLow8RegsRegisterClass(
    MCInst &Inst, unsigned RegNo, uint64_t /*Address*/, const void *Decoder) {
  static const MCPhysReg GeneralDoubleLow8RegDecoderTable[] = {
      Hexagon::D0, Hexagon::D1, Hexagon::D2,  Hexagon::D3,
      Hexagon::D8, Hexagon::D9, Hexagon::D10, Hexagon::D11};

  return DecodeRegisterClass(Inst, RegNo, GeneralDoubleLow8RegDecoderTable);
}

static DecodeStatus DecodeVecDblRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg VecDblRegDecoderTable[] = {
      Hexagon::W0,  Hexagon::W1,  Hexagon::W2,  Hexagon::W3,
      Hexagon::W4,  Hexagon::W5,  Hexagon::W6,  Hexagon::W7,
      Hexagon::W8,  Hexagon::W9,  Hexagon::W10, Hexagon::W11,
      Hexagon::W12, Hexagon::W13, Hexagon::W14, Hexagon::W15};

  return (DecodeRegisterClass(Inst, RegNo >> 1, VecDblRegDecoderTable));
}

static DecodeStatus DecodePredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                uint64_t /*Address*/,
                                                const void *Decoder) {
  static const MCPhysReg PredRegDecoderTable[] = {Hexagon::P0, Hexagon::P1,
                                                  Hexagon::P2, Hexagon::P3};

  return DecodeRegisterClass(Inst, RegNo, PredRegDecoderTable);
}

static DecodeStatus DecodeVecPredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t /*Address*/,
                                                   const void *Decoder) {
  static const MCPhysReg VecPredRegDecoderTable[] = {Hexagon::Q0, Hexagon::Q1,
                                                     Hexagon::Q2, Hexagon::Q3};

  return DecodeRegisterClass(Inst, RegNo, VecPredRegDecoderTable);
}

static DecodeStatus DecodeCtrRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t /*Address*/,
                                               const void *Decoder) {
  static const MCPhysReg CtrlRegDecoderTable[] = {
    Hexagon::SA0,        Hexagon::LC0,        Hexagon::SA1,
    Hexagon::LC1,        Hexagon::P3_0,       Hexagon::C5,
    Hexagon::C6,         Hexagon::C7,         Hexagon::USR,
    Hexagon::PC,         Hexagon::UGP,        Hexagon::GP,
    Hexagon::CS0,        Hexagon::CS1,        Hexagon::UPCL,
    Hexagon::UPC
  };

  if (RegNo >= array_lengthof(CtrlRegDecoderTable))
    return MCDisassembler::Fail;

  if (CtrlRegDecoderTable[RegNo] == Hexagon::NoRegister)
    return MCDisassembler::Fail;

  unsigned Register = CtrlRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCtrRegs64RegisterClass(MCInst &Inst, unsigned RegNo,
                                                 uint64_t /*Address*/,
                                                 const void *Decoder) {
  static const MCPhysReg CtrlReg64DecoderTable[] = {
    Hexagon::C1_0,       Hexagon::NoRegister, Hexagon::C3_2,
    Hexagon::NoRegister,
    Hexagon::C7_6,       Hexagon::NoRegister, Hexagon::C9_8,
    Hexagon::NoRegister, Hexagon::C11_10,     Hexagon::NoRegister,
    Hexagon::CS,         Hexagon::NoRegister, Hexagon::UPC,
    Hexagon::NoRegister
  };

  if (RegNo >= array_lengthof(CtrlReg64DecoderTable))
    return MCDisassembler::Fail;

  if (CtrlReg64DecoderTable[RegNo] == Hexagon::NoRegister)
    return MCDisassembler::Fail;

  unsigned Register = CtrlReg64DecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeModRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t /*Address*/,
                                               const void *Decoder) {
  unsigned Register = 0;
  switch (RegNo) {
  case 0:
    Register = Hexagon::M0;
    break;
  case 1:
    Register = Hexagon::M1;
    break;
  default:
    return MCDisassembler::Fail;
  }
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static DecodeStatus unsignedImmDecoder(MCInst &MI, unsigned tmp,
                                       uint64_t /*Address*/,
                                       const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  int64_t FullValue =
      fullValue(*Disassembler.MCII, **Disassembler.CurrentBundle, MI, tmp);
  assert(FullValue >= 0 && "Negative in unsigned decoder");
  HexagonMCInstrInfo::addConstant(MI, FullValue, Disassembler.getContext());
  return MCDisassembler::Success;
}

static DecodeStatus s4_6ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<10>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s3_6ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<19>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s32_0ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  unsigned Bits = HexagonMCInstrInfo::getExtentBits(*Disassembler.MCII, MI);
  tmp = SignExtend64(tmp, Bits);
  signedDecoder<32>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

// custom decoder for various jump/call immediates
static DecodeStatus brtargetDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  unsigned Bits = HexagonMCInstrInfo::getExtentBits(*Disassembler.MCII, MI);
  // r13_2 is not extendable, so if there are no extent bits, it's r13_2
  if (Bits == 0)
    Bits = 15;
  uint32_t FullValue =
      fullValue(*Disassembler.MCII, **Disassembler.CurrentBundle, MI,
                SignExtend64(tmp, Bits));
  int64_t Extended = SignExtend64<32>(FullValue) + Address;
  if (!Disassembler.tryAddingSymbolicOperand(MI, Extended, Address, true, 0, 4))
    HexagonMCInstrInfo::addConstant(MI, Extended, Disassembler.getContext());
  return MCDisassembler::Success;
}


