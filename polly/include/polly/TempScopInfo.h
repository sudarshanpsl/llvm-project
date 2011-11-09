//===-------- polly/TempScopInfo.h - Extract TempScops ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Collect information about the control flow regions detected by the Scop
// detection, such that this information can be translated info its polyhedral
// representation.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_TEMP_SCOP_EXTRACTION_H
#define POLLY_TEMP_SCOP_EXTRACTION_H

#include "polly/MayAliasSet.h"
#include "polly/ScopDetection.h"

#include "llvm/Analysis/RegionPass.h"
#include "llvm/Instructions.h"

namespace llvm {
  class TargetData;
}

using namespace llvm;

namespace polly {
class MayAliasSetInfo;

//===---------------------------------------------------------------------===//
/// @brief Affine function represent in llvm SCEV expressions.
///
/// A helper class for collect affine function information
class SCEVAffFunc {
  // Temporary hack
  friend class TempScopInfo;

public:
  // The scalar evolution expression from which we derived this affine
  // expression.
  //
  // We will use it to directly translation from scalar expressions to the
  // corresponding isl objects. As soon as this finished, most of SCEVAffFunc
  // can be removed.
  const SCEV *OriginalSCEV;

  // The type of the scev affine function
  enum SCEVAffFuncType {
    ReadMem,
    WriteMem
  };

private:
  // The base address of the address SCEV, if the Value is a pointer, this is
  // an array access, otherwise, this is a value access.
  // And the Write/Read modifier
  Value *BaseAddr;
  unsigned ElemBytes        : 28;
  SCEVAffFuncType FuncType  : 3;

public:
  /// @brief Create a new SCEV affine function with memory access type or
  ///        condition type
  explicit SCEVAffFunc(SCEVAffFuncType Type, const SCEV *OriginalSCEV,
                       unsigned elemBytes = 0)
    : OriginalSCEV(OriginalSCEV), BaseAddr(0),
      ElemBytes(elemBytes), FuncType(Type) {}

  enum SCEVAffFuncType getType() const { return FuncType; }

  unsigned getElemSizeInBytes() const {
    return ElemBytes;
  }

  bool isRead() const { return FuncType == ReadMem; }

  const Value *getBaseAddr() const { return BaseAddr; }
};

class Comparison {

  const SCEV *LHS;
  const SCEV *RHS;

  ICmpInst::Predicate Pred;

public:
  Comparison(const SCEV *LHS, const SCEV *RHS, ICmpInst::Predicate Pred)
    : LHS(LHS), RHS(RHS), Pred(Pred) {}

  const SCEV *getLHS() const { return LHS; }
  const SCEV *getRHS() const { return RHS; }

  ICmpInst::Predicate getPred() const { return Pred; }
  void print(raw_ostream &OS) const;
};

//===---------------------------------------------------------------------===//
/// Types
// The condition of a Basicblock, combine brcond with "And" operator.
typedef SmallVector<Comparison, 4> BBCond;

/// Maps from a loop to the affine function expressing its backedge taken count.
/// The backedge taken count already enough to express iteration domain as we
/// only allow loops with canonical induction variable.
/// A canonical induction variable is:
/// an integer recurrence that starts at 0 and increments by one each time
/// through the loop.
typedef std::map<const Loop*, const SCEV*> LoopBoundMapType;

/// Mapping BBs to its condition constrains
typedef std::map<const BasicBlock*, BBCond> BBCondMapType;

typedef std::vector<std::pair<SCEVAffFunc, Instruction*> > AccFuncSetType;
typedef std::map<const BasicBlock*, AccFuncSetType> AccFuncMapType;

//===---------------------------------------------------------------------===//
/// @brief Scop represent with llvm objects.
///
/// A helper class for remembering the parameter number and the max depth in
/// this Scop, and others context.
class TempScop {
  // The Region.
  Region &R;

  // Parameters used in this Scop.
  ParamSetType Params;

  // The max loop depth of this Scop
  unsigned MaxLoopDepth;

  // Remember the bounds of loops, to help us build iteration domain of BBs.
  const LoopBoundMapType &LoopBounds;
  const BBCondMapType &BBConds;

  // Access function of bbs.
  const AccFuncMapType &AccFuncMap;
  
  // The alias information about this SCoP.
  MayAliasSetInfo *MayASInfo;

  friend class TempScopInfo;

  explicit TempScop(Region &r, LoopBoundMapType &loopBounds,
                    BBCondMapType &BBCmps, AccFuncMapType &accFuncMap)
    : R(r), MaxLoopDepth(0), LoopBounds(loopBounds), BBConds(BBCmps),
    AccFuncMap(accFuncMap), MayASInfo(new MayAliasSetInfo()) {}

public:
  ~TempScop();

  /// @name Information about this Temporary Scop.
  ///
  //@{
  /// @brief Get the parameters used in this Scop.
  ///
  /// @return The parameters use in region.
  ParamSetType &getParamSet() { return Params; }

  /// @brief Get the maximum Region contained by this Scop.
  ///
  /// @return The maximum Region contained by this Scop.
  Region &getMaxRegion() const { return R; }

  /// @brief Get the maximum loop depth of Region R.
  ///
  /// @return The maximum loop depth of Region R.
  unsigned getMaxLoopDepth() const { return MaxLoopDepth; }

  /// @brief Get the loop bounds of the given loop.
  ///
  /// @param L The loop to get the bounds.
  ///
  /// @return The bounds of the loop L in { Lower bound, Upper bound } form.
  ///
  const SCEV *getLoopBound(const Loop *L) const {
    LoopBoundMapType::const_iterator at = LoopBounds.find(L);
    assert(at != LoopBounds.end() && "Only valid loop is allow!");
    return at->second;
  }

  /// @brief Get the condition from entry block of the Scop to a BasicBlock
  ///
  /// @param BB The BasicBlock
  ///
  /// @return The condition from entry block of the Scop to a BB
  ///
  const BBCond *getBBCond(const BasicBlock *BB) const {
    BBCondMapType::const_iterator at = BBConds.find(BB);
    return at != BBConds.end() ? &(at->second) : 0;
  }

  /// @brief Get all access functions in a BasicBlock
  ///
  /// @param  BB The BasicBlock that containing the access functions.
  ///
  /// @return All access functions in BB
  ///
  const AccFuncSetType *getAccessFunctions(const BasicBlock* BB) const {
    AccFuncMapType::const_iterator at = AccFuncMap.find(BB);
    return at != AccFuncMap.end()? &(at->second) : 0;
  }
  //@}

  /// @brief Print the Temporary Scop information.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing Temporary Scop
  ///           information.
  /// @param LI The LoopInfo that help printing the access functions.
  void print(raw_ostream &OS, ScalarEvolution *SE, LoopInfo *LI) const;

  /// @brief Print the access functions and loop bounds in this Scop.
  ///
  /// @param OS The output stream the access functions is printed to.
  /// @param SE The ScalarEvolution that help printing the access functions.
  /// @param LI The LoopInfo that help printing the access functions.
  void printDetail(raw_ostream &OS, ScalarEvolution *SE,
                   LoopInfo *LI, const Region *Reg, unsigned ind) const;
};

typedef std::map<const Region*, TempScop*> TempScopMapType;
//===----------------------------------------------------------------------===//
/// @brief The Function Pass to extract temporary information for Static control
///        part in llvm function.
///
class TempScopInfo : public FunctionPass {
  //===-------------------------------------------------------------------===//
  // DO NOT IMPLEMENT
  TempScopInfo(const TempScopInfo &);
  // DO NOT IMPLEMENT
  const TempScopInfo &operator=(const TempScopInfo &);

  // The ScalarEvolution to help building Scop.
  ScalarEvolution* SE;

  // LoopInfo for information about loops
  LoopInfo *LI;

  // The AliasAnalysis to build AliasSetTracker.
  AliasAnalysis *AA;

  // Valid Regions for Scop
  ScopDetection *SD;

  // For condition extraction support.
  DominatorTree *DT;
  PostDominatorTree *PDT;

  // Target data for element size computing.
  TargetData *TD;

  // Remember the bounds of loops, to help us build iteration domain of BBs.
  LoopBoundMapType LoopBounds;

  // And also Remember the constrains for BBs
  BBCondMapType BBConds;

  // Access function of bbs.
  AccFuncMapType AccFuncMap;

  // Mapping regions to the corresponding Scop in current function.
  TempScopMapType TempScops;

  // Clear the context.
  void clear();

  /// @brief Build condition constrains to BBs in a valid Scop.
  ///
  /// @param BB           The BasicBlock to build condition constrains
  /// @param RegionEntry  The entry block of the Smallest Region that containing
  ///                     BB
  /// @param Cond         The built condition
  void buildCondition(BasicBlock *BB, BasicBlock *Region, TempScop &Scop);

  // Build the affine function of the given condition
  void buildAffineCondition(Value &V, bool inverted, Comparison **Comp,
                            TempScop &Scop) const;

  // Return the temporary Scop information of Region R, where R must be a valid
  // part of Scop
  TempScop *getTempScop(Region &R);

  // Build the temprory information of Region R, where R must be a valid part
  // of Scop.
  TempScop *buildTempScop(Region &R);

  bool isReduction(BasicBlock &BB);

  void buildAccessFunctions(Region &RefRegion, ParamSetType &Params,
                            BasicBlock &BB);

  void buildLoopBounds(TempScop &Scop);

public:
  static char ID;
  explicit TempScopInfo() : FunctionPass(ID) {}
  ~TempScopInfo();

  /// @brief Get the temporay Scop information in LLVM IR represent
  ///        for Region R.
  ///
  /// @return The Scop information in LLVM IR represent.
  TempScop *getTempScop(const Region *R) const;

  /// @name FunctionPass interface
  //@{
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void releaseMemory() { clear(); }
  virtual bool runOnFunction(Function &F);
  virtual void print(raw_ostream &OS, const Module *) const;
  //@}
};

} // end namespace polly

namespace llvm {
  class PassRegistry;
  void initializeTempScopInfoPass(llvm::PassRegistry&);
}

#endif
