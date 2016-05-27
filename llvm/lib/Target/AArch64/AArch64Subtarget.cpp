//===-- AArch64Subtarget.cpp - AArch64 Subtarget Information ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AArch64 specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "AArch64Subtarget.h"
#include "AArch64InstrInfo.h"
#include "AArch64PBQPRegAlloc.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-subtarget"

#define GET_SUBTARGETINFO_CTOR
#define GET_SUBTARGETINFO_TARGET_DESC
#include "AArch64GenSubtargetInfo.inc"

static cl::opt<bool>
EnableEarlyIfConvert("aarch64-early-ifcvt", cl::desc("Enable the early if "
                     "converter pass"), cl::init(true), cl::Hidden);

// If OS supports TBI, use this flag to enable it.
static cl::opt<bool>
UseAddressTopByteIgnored("aarch64-use-tbi", cl::desc("Assume that top byte of "
                         "an address is ignored"), cl::init(false), cl::Hidden);

AArch64Subtarget &
AArch64Subtarget::initializeSubtargetDependencies(StringRef FS) {
  // Determine default and user-specified characteristics

  if (CPUString.empty())
    CPUString = "generic";

  ParseSubtargetFeatures(CPUString, FS);
  return *this;
}

AArch64Subtarget::AArch64Subtarget(const Triple &TT, const std::string &CPU,
                                   const std::string &FS,
                                   const TargetMachine &TM, bool LittleEndian)
    : AArch64GenSubtargetInfo(TT, CPU, FS), ReserveX18(TT.isOSDarwin()),
      IsLittle(LittleEndian), CPUString(CPU), TargetTriple(TT), FrameLowering(),
      InstrInfo(initializeSubtargetDependencies(FS)), TSInfo(),
      TLInfo(TM, *this), GISel() {}

const CallLowering *AArch64Subtarget::getCallLowering() const {
  assert(GISel && "Access to GlobalISel APIs not set");
  return GISel->getCallLowering();
}

const RegisterBankInfo *AArch64Subtarget::getRegBankInfo() const {
  assert(GISel && "Access to GlobalISel APIs not set");
  return GISel->getRegBankInfo();
}

/// Find the target operand flags that describe how a global value should be
/// referenced for the current subtarget.
unsigned char
AArch64Subtarget::ClassifyGlobalReference(const GlobalValue *GV,
                                          const TargetMachine &TM) const {
  // MachO large model always goes via a GOT, simply to get a single 8-byte
  // absolute relocation on all global addresses.
  if (TM.getCodeModel() == CodeModel::Large && isTargetMachO())
    return AArch64II::MO_GOT;

  Reloc::Model RM = TM.getRelocationModel();
  if (!shouldAssumeDSOLocal(RM, TargetTriple, *GV->getParent(), GV))
    return AArch64II::MO_GOT;

  // The small code mode's direct accesses use ADRP, which cannot necessarily
  // produce the value 0 (if the code is above 4GB).
  if (TM.getCodeModel() == CodeModel::Small && GV->hasExternalWeakLinkage()) {
    // In PIC mode use the GOT, but in absolute mode use a constant pool load.
    if (RM == Reloc::Static)
      return AArch64II::MO_CONSTPOOL;
    else
      return AArch64II::MO_GOT;
  }

  return AArch64II::MO_NO_FLAG;
}

/// This function returns the name of a function which has an interface
/// like the non-standard bzero function, if such a function exists on
/// the current subtarget and it is considered prefereable over
/// memset with zero passed as the second argument. Otherwise it
/// returns null.
const char *AArch64Subtarget::getBZeroEntry() const {
  // Prefer bzero on Darwin only.
  if(isTargetDarwin())
    return "bzero";

  return nullptr;
}

void AArch64Subtarget::overrideSchedPolicy(MachineSchedPolicy &Policy,
                                         MachineInstr *begin, MachineInstr *end,
                                         unsigned NumRegionInstrs) const {
  // LNT run (at least on Cyclone) showed reasonably significant gains for
  // bi-directional scheduling. 253.perlbmk.
  Policy.OnlyTopDown = false;
  Policy.OnlyBottomUp = false;
  // Enabling or Disabling the latency heuristic is a close call: It seems to
  // help nearly no benchmark on out-of-order architectures, on the other hand
  // it regresses register pressure on a few benchmarking.
  if (isCyclone())
    Policy.DisableLatencyHeuristic = true;
}

bool AArch64Subtarget::enableEarlyIfConversion() const {
  return EnableEarlyIfConvert;
}

bool AArch64Subtarget::supportsAddressTopByteIgnored() const {
  if (!UseAddressTopByteIgnored)
    return false;

  if (TargetTriple.isiOS()) {
    unsigned Major, Minor, Micro;
    TargetTriple.getiOSVersion(Major, Minor, Micro);
    return Major >= 8;
  }

  return false;
}

std::unique_ptr<PBQPRAConstraint>
AArch64Subtarget::getCustomPBQPConstraints() const {
  if (!isCortexA57())
    return nullptr;

  return llvm::make_unique<A57ChainingConstraint>();
}
