//===- BlackfinISelDAGToDAG.cpp - A dag to dag inst selector for Blackfin -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an instruction selector for the Blackfin target.
//
//===----------------------------------------------------------------------===//

#include "Blackfin.h"
#include "BlackfinTargetMachine.h"
#include "BlackfinRegisterInfo.h"
#include "llvm/Intrinsics.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Instruction Selector Implementation
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
/// BlackfinDAGToDAGISel - Blackfin specific code to select blackfin machine
/// instructions for SelectionDAG operations.
namespace {
  class BlackfinDAGToDAGISel : public SelectionDAGISel {
    /// Subtarget - Keep a pointer to the Blackfin Subtarget around so that we
    /// can make the right decision when generating code for different targets.
    //const BlackfinSubtarget &Subtarget;
  public:
    BlackfinDAGToDAGISel(BlackfinTargetMachine &TM, CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

    virtual void PostprocessISelDAG();

    virtual const char *getPassName() const {
      return "Blackfin DAG->DAG Pattern Instruction Selection";
    }

    // Include the pieces autogenerated from the target description.
#include "BlackfinGenDAGISel.inc"

  private:
    SDNode *Select(SDNode *N);
    bool SelectADDRspii(SDValue Addr, SDValue &Base, SDValue &Offset);

    // Walk the DAG after instruction selection, fixing register class issues.
    void FixRegisterClasses(SelectionDAG &DAG);

    const BlackfinInstrInfo &getInstrInfo() {
      return *static_cast<const BlackfinTargetMachine&>(TM).getInstrInfo();
    }
    const BlackfinRegisterInfo *getRegisterInfo() {
      return static_cast<const BlackfinTargetMachine&>(TM).getRegisterInfo();
    }
  };
}  // end anonymous namespace

FunctionPass *llvm::createBlackfinISelDag(BlackfinTargetMachine &TM,
                                          CodeGenOpt::Level OptLevel) {
  return new BlackfinDAGToDAGISel(TM, OptLevel);
}

void BlackfinDAGToDAGISel::PostprocessISelDAG() {
  FixRegisterClasses(*CurDAG);
}

SDNode *BlackfinDAGToDAGISel::Select(SDNode *N) {
  if (N->isMachineOpcode())
    return NULL;   // Already selected.

  switch (N->getOpcode()) {
  default: break;
  case ISD::FrameIndex: {
    // Selects to ADDpp FI, 0 which in turn will become ADDimm7 SP, imm or ADDpp
    // SP, Px
    int FI = cast<FrameIndexSDNode>(N)->getIndex();
    SDValue TFI = CurDAG->getTargetFrameIndex(FI, MVT::i32);
    return CurDAG->SelectNodeTo(N, BF::ADDpp, MVT::i32, TFI,
                                CurDAG->getTargetConstant(0, MVT::i32));
  }
  }

  return SelectCode(N);
}

bool BlackfinDAGToDAGISel::SelectADDRspii(SDValue Addr,
                                          SDValue &Base,
                                          SDValue &Offset) {
  FrameIndexSDNode *FIN = 0;
  if ((FIN = dyn_cast<FrameIndexSDNode>(Addr))) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    Offset = CurDAG->getTargetConstant(0, MVT::i32);
    return true;
  }
  if (Addr.getOpcode() == ISD::ADD) {
    ConstantSDNode *CN = 0;
    if ((FIN = dyn_cast<FrameIndexSDNode>(Addr.getOperand(0))) &&
        (CN = dyn_cast<ConstantSDNode>(Addr.getOperand(1))) &&
        (CN->getSExtValue() % 4 == 0 && CN->getSExtValue() >= 0)) {
      // Constant positive word offset from frame index
      Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
      Offset = CurDAG->getTargetConstant(CN->getSExtValue(), MVT::i32);
      return true;
    }
  }
  return false;
}

static inline bool isCC(const TargetRegisterClass *RC) {
  return BF::AnyCCRegClass.hasSubClassEq(RC);
}

static inline bool isDCC(const TargetRegisterClass *RC) {
  return BF::DRegClass.hasSubClassEq(RC) || isCC(RC);
}

static void UpdateNodeOperand(SelectionDAG &DAG,
                              SDNode *N,
                              unsigned Num,
                              SDValue Val) {
  SmallVector<SDValue, 8> ops(N->op_begin(), N->op_end());
  ops[Num] = Val;
  SDNode *New = DAG.UpdateNodeOperands(N, ops.data(), ops.size());
  DAG.ReplaceAllUsesWith(N, New);
}

// After instruction selection, insert COPY_TO_REGCLASS nodes to help in
// choosing the proper register classes.
void BlackfinDAGToDAGISel::FixRegisterClasses(SelectionDAG &DAG) {
  const BlackfinInstrInfo &TII = getInstrInfo();
  const BlackfinRegisterInfo *TRI = getRegisterInfo();
  DAG.AssignTopologicalOrder();
  HandleSDNode Dummy(DAG.getRoot());

  for (SelectionDAG::allnodes_iterator NI = DAG.allnodes_begin();
       NI != DAG.allnodes_end(); ++NI) {
    if (NI->use_empty() || !NI->isMachineOpcode())
      continue;
    const TargetInstrDesc &DefTID = TII.get(NI->getMachineOpcode());
    for (SDNode::use_iterator UI = NI->use_begin(); !UI.atEnd(); ++UI) {
      if (!UI->isMachineOpcode())
        continue;

      if (UI.getUse().getResNo() >= DefTID.getNumDefs())
        continue;
      const TargetRegisterClass *DefRC =
        TII.getRegClass(DefTID, UI.getUse().getResNo(), TRI);

      const TargetInstrDesc &UseTID = TII.get(UI->getMachineOpcode());
      if (UseTID.getNumDefs()+UI.getOperandNo() >= UseTID.getNumOperands())
        continue;
      const TargetRegisterClass *UseRC =
        TII.getRegClass(UseTID, UseTID.getNumDefs()+UI.getOperandNo(), TRI);
      if (!DefRC || !UseRC)
        continue;
      // We cannot copy CC <-> !(CC/D)
      if ((isCC(DefRC) && !isDCC(UseRC)) || (isCC(UseRC) && !isDCC(DefRC))) {
        SDNode *Copy =
          DAG.getMachineNode(TargetOpcode::COPY_TO_REGCLASS,
                             NI->getDebugLoc(),
                             MVT::i32,
                             UI.getUse().get(),
                             DAG.getTargetConstant(BF::DRegClassID, MVT::i32));
        UpdateNodeOperand(DAG, *UI, UI.getOperandNo(), SDValue(Copy, 0));
      }
    }
  }
  DAG.setRoot(Dummy.getValue());
}

