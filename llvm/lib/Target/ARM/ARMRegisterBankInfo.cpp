//===- ARMRegisterBankInfo.cpp -----------------------------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the targeting of the RegisterBankInfo class for ARM.
/// \todo This should be generated by TableGen.
//===----------------------------------------------------------------------===//

#include "ARMRegisterBankInfo.h"
#include "ARMInstrInfo.h" // For the register classes
#include "ARMSubtarget.h"
#include "llvm/CodeGen/GlobalISel/RegisterBank.h"
#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Target/TargetRegisterInfo.h"

#define GET_TARGET_REGBANK_IMPL
#include "ARMGenRegisterBank.inc"

using namespace llvm;

#ifndef LLVM_BUILD_GLOBAL_ISEL
#error "You shouldn't build this"
#endif

// FIXME: TableGen this.
// If it grows too much and TableGen still isn't ready to do the job, extract it
// into an ARMGenRegisterBankInfo.def (similar to AArch64).
namespace llvm {
namespace ARM {
RegisterBankInfo::PartialMapping GPRPartialMapping{0, 32, GPRRegBank};
RegisterBankInfo::PartialMapping SPRPartialMapping{0, 32, FPRRegBank};
RegisterBankInfo::PartialMapping DPRPartialMapping{0, 64, FPRRegBank};

// FIXME: Add the mapping for S(2n+1) as {32, 64, FPRRegBank}
RegisterBankInfo::ValueMapping ValueMappings[] = {
    {&GPRPartialMapping, 1}, {&GPRPartialMapping, 1}, {&GPRPartialMapping, 1},
    {&SPRPartialMapping, 1}, {&SPRPartialMapping, 1}, {&SPRPartialMapping, 1},
    {&DPRPartialMapping, 1}, {&DPRPartialMapping, 1}, {&DPRPartialMapping, 1}};
} // end namespace arm
} // end namespace llvm

ARMRegisterBankInfo::ARMRegisterBankInfo(const TargetRegisterInfo &TRI)
    : ARMGenRegisterBankInfo() {
  static bool AlreadyInit = false;
  // We have only one set of register banks, whatever the subtarget
  // is. Therefore, the initialization of the RegBanks table should be
  // done only once. Indeed the table of all register banks
  // (ARM::RegBanks) is unique in the compiler. At some point, it
  // will get tablegen'ed and the whole constructor becomes empty.
  if (AlreadyInit)
    return;
  AlreadyInit = true;

  const RegisterBank &RBGPR = getRegBank(ARM::GPRRegBankID);
  (void)RBGPR;
  assert(&ARM::GPRRegBank == &RBGPR && "The order in RegBanks is messed up");

  // Initialize the GPR bank.
  assert(RBGPR.covers(*TRI.getRegClass(ARM::GPRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::GPRwithAPSRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::GPRnopcRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::rGPRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::tGPRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::tcGPRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.covers(*TRI.getRegClass(ARM::tGPR_and_tcGPRRegClassID)) &&
         "Subclass not added?");
  assert(RBGPR.getSize() == 32 && "GPRs should hold up to 32-bit");
}

const RegisterBank &ARMRegisterBankInfo::getRegBankFromRegClass(
    const TargetRegisterClass &RC) const {
  using namespace ARM;

  switch (RC.getID()) {
  case GPRRegClassID:
  case GPRnopcRegClassID:
  case tGPR_and_tcGPRRegClassID:
    return getRegBank(ARM::GPRRegBankID);
  case SPR_8RegClassID:
  case SPRRegClassID:
  case DPR_8RegClassID:
  case DPRRegClassID:
    return getRegBank(ARM::FPRRegBankID);
  default:
    llvm_unreachable("Unsupported register kind");
  }

  llvm_unreachable("Switch should handle all register classes");
}

RegisterBankInfo::InstructionMapping
ARMRegisterBankInfo::getInstrMapping(const MachineInstr &MI) const {
  auto Opc = MI.getOpcode();

  // Try the default logic for non-generic instructions that are either copies
  // or already have some operands assigned to banks.
  if (!isPreISelGenericOpcode(Opc)) {
    InstructionMapping Mapping = getInstrMappingImpl(MI);
    if (Mapping.isValid())
      return Mapping;
  }

  using namespace TargetOpcode;

  const MachineFunction &MF = *MI.getParent()->getParent();
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  LLT Ty = MRI.getType(MI.getOperand(0).getReg());

  unsigned NumOperands = MI.getNumOperands();
  const ValueMapping *OperandsMapping = &ARM::ValueMappings[0];

  switch (Opc) {
  case G_ADD:
  case G_SEXT:
  case G_ZEXT:
    // FIXME: We're abusing the fact that everything lives in a GPR for now; in
    // the real world we would use different mappings.
    OperandsMapping = &ARM::ValueMappings[0];
    break;
  case G_LOAD:
    OperandsMapping = Ty.getSizeInBits() == 64
                          ? getOperandsMapping({&ARM::ValueMappings[6],
                                                &ARM::ValueMappings[0]})
                          : &ARM::ValueMappings[0];
    break;
  case G_FADD:
    assert((Ty.getSizeInBits() == 32 || Ty.getSizeInBits() == 64) &&
           "Unsupported size for G_FADD");
    OperandsMapping = Ty.getSizeInBits() == 64 ? &ARM::ValueMappings[6]
                                               : &ARM::ValueMappings[3];
    break;
  case G_FRAME_INDEX:
    OperandsMapping = getOperandsMapping({&ARM::ValueMappings[0], nullptr});
    break;
  case G_SEQUENCE: {
    // We only support G_SEQUENCE for creating a double precision floating point
    // value out of two GPRs.
    LLT Ty1 = MRI.getType(MI.getOperand(1).getReg());
    LLT Ty2 = MRI.getType(MI.getOperand(3).getReg());
    if (Ty.getSizeInBits() != 64 || Ty1.getSizeInBits() != 32 ||
        Ty2.getSizeInBits() != 32)
      return InstructionMapping{};
    OperandsMapping =
        getOperandsMapping({&ARM::ValueMappings[6], &ARM::ValueMappings[0],
                            nullptr, &ARM::ValueMappings[0], nullptr});
    break;
  }
  case G_EXTRACT: {
    // We only support G_EXTRACT for splitting a double precision floating point
    // value into two GPRs.
    LLT Ty1 = MRI.getType(MI.getOperand(1).getReg());
    LLT Ty2 = MRI.getType(MI.getOperand(2).getReg());
    if (Ty.getSizeInBits() != 32 || Ty1.getSizeInBits() != 32 ||
        Ty2.getSizeInBits() != 64)
      return InstructionMapping{};
    OperandsMapping =
        getOperandsMapping({&ARM::ValueMappings[0], &ARM::ValueMappings[0],
                            &ARM::ValueMappings[6], nullptr, nullptr});
    break;
  }
  default:
    return InstructionMapping{};
  }

#ifndef NDEBUG
  for (unsigned i = 0; i < NumOperands; i++) {
    for (const auto &Mapping : OperandsMapping[i]) {
      assert(
          (Mapping.RegBank->getID() != ARM::FPRRegBankID ||
           MF.getSubtarget<ARMSubtarget>().hasVFP2()) &&
          "Trying to use floating point register bank on target without vfp");
    }
  }
#endif

  return InstructionMapping{DefaultMappingID, /*Cost=*/1, OperandsMapping,
                            NumOperands};
}
