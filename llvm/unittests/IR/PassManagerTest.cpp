//===- llvm/unittest/IR/PassManager.cpp - PassManager tests ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/SourceMgr.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class TestFunctionAnalysis {
public:
  struct Result {
    Result(int Count) : InstructionCount(Count) {}
    int InstructionCount;
  };

  /// \brief Returns an opaque, unique ID for this pass type.
  static void *ID() { return (void *)&PassID; }

  TestFunctionAnalysis(int &Runs) : Runs(Runs) {}

  /// \brief Run the analysis pass over the function and return a result.
  Result run(Function *F, FunctionAnalysisManager *AM) {
    ++Runs;
    int Count = 0;
    for (Function::iterator BBI = F->begin(), BBE = F->end(); BBI != BBE; ++BBI)
      for (BasicBlock::iterator II = BBI->begin(), IE = BBI->end(); II != IE;
           ++II)
        ++Count;
    return Result(Count);
  }

private:
  /// \brief Private static data to provide unique ID.
  static char PassID;

  int &Runs;
};

char TestFunctionAnalysis::PassID;

class TestModuleAnalysis {
public:
  struct Result {
    Result(int Count) : FunctionCount(Count) {}
    int FunctionCount;
  };

  static void *ID() { return (void * )&PassID; }

  TestModuleAnalysis(int &Runs) : Runs(Runs) {}

  Result run(Module *M, ModuleAnalysisManager *AM) {
    ++Runs;
    int Count = 0;
    for (Module::iterator I = M->begin(), E = M->end(); I != E; ++I)
      ++Count;
    return Result(Count);
  }

private:
  static char PassID;

  int &Runs;
};

char TestModuleAnalysis::PassID;

struct TestModulePass {
  TestModulePass(int &RunCount) : RunCount(RunCount) {}

  PreservedAnalyses run(Module *M) {
    ++RunCount;
    return PreservedAnalyses::none();
  }

  int &RunCount;
};

struct TestPreservingModulePass {
  PreservedAnalyses run(Module *M) {
    return PreservedAnalyses::all();
  }
};

struct TestMinPreservingModulePass {
  PreservedAnalyses run(Module *M, ModuleAnalysisManager *AM) {
    PreservedAnalyses PA;

    // Force running an analysis.
    (void)AM->getResult<TestModuleAnalysis>(M);

    PA.preserve<FunctionAnalysisManagerModuleProxy>();
    return PA;
  }
};

struct TestFunctionPass {
  TestFunctionPass(int &RunCount, int &AnalyzedInstrCount,
                   int &AnalyzedFunctionCount,
                   bool OnlyUseCachedResults = false)
      : RunCount(RunCount), AnalyzedInstrCount(AnalyzedInstrCount),
        AnalyzedFunctionCount(AnalyzedFunctionCount),
        OnlyUseCachedResults(OnlyUseCachedResults) {}

  PreservedAnalyses run(Function *F, FunctionAnalysisManager *AM) {
    ++RunCount;

    const ModuleAnalysisManager &MAM =
        AM->getResult<ModuleAnalysisManagerFunctionProxy>(F).getManager();
    if (const TestModuleAnalysis::Result *TMA =
            MAM.getCachedResult<TestModuleAnalysis>(F->getParent()))
      AnalyzedFunctionCount += TMA->FunctionCount;

    if (OnlyUseCachedResults) {
      // Hack to force the use of the cached interface.
      if (const TestFunctionAnalysis::Result *AR =
              AM->getCachedResult<TestFunctionAnalysis>(F))
        AnalyzedInstrCount += AR->InstructionCount;
    } else {
      // Typical path just runs the analysis as needed.
      const TestFunctionAnalysis::Result &AR = AM->getResult<TestFunctionAnalysis>(F);
      AnalyzedInstrCount += AR.InstructionCount;
    }

    return PreservedAnalyses::all();
  }

  int &RunCount;
  int &AnalyzedInstrCount;
  int &AnalyzedFunctionCount;
  bool OnlyUseCachedResults;
};

// A test function pass that invalidates all function analyses for a function
// with a specific name.
struct TestInvalidationFunctionPass {
  TestInvalidationFunctionPass(StringRef FunctionName) : Name(FunctionName) {}

  PreservedAnalyses run(Function *F) {
    return F->getName() == Name ? PreservedAnalyses::none()
                                : PreservedAnalyses::all();
  }

  StringRef Name;
};

Module *parseIR(const char *IR) {
  LLVMContext &C = getGlobalContext();
  SMDiagnostic Err;
  return ParseAssemblyString(IR, 0, Err, C);
}

class PassManagerTest : public ::testing::Test {
protected:
  OwningPtr<Module> M;

public:
  PassManagerTest()
      : M(parseIR("define void @f() {\n"
                  "entry:\n"
                  "  call void @g()\n"
                  "  call void @h()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @g() {\n"
                  "  ret void\n"
                  "}\n"
                  "define void @h() {\n"
                  "  ret void\n"
                  "}\n")) {}
};

TEST_F(PassManagerTest, Basic) {
  FunctionAnalysisManager FAM;
  int FunctionAnalysisRuns = 0;
  FAM.registerPass(TestFunctionAnalysis(FunctionAnalysisRuns));

  ModuleAnalysisManager MAM;
  int ModuleAnalysisRuns = 0;
  MAM.registerPass(TestModuleAnalysis(ModuleAnalysisRuns));
  MAM.registerPass(FunctionAnalysisManagerModuleProxy(FAM));
  FAM.registerPass(ModuleAnalysisManagerFunctionProxy(MAM));

  ModulePassManager MPM;

  // Count the runs over a Function.
  FunctionPassManager FPM1;
  int FunctionPassRunCount1 = 0;
  int AnalyzedInstrCount1 = 0;
  int AnalyzedFunctionCount1 = 0;
  FPM1.addPass(TestFunctionPass(FunctionPassRunCount1, AnalyzedInstrCount1,
                                AnalyzedFunctionCount1));
  MPM.addPass(createModuleToFunctionPassAdaptor(FPM1));

  // Count the runs over a module.
  int ModulePassRunCount = 0;
  MPM.addPass(TestModulePass(ModulePassRunCount));

  // Count the runs over a Function in a separate manager.
  FunctionPassManager FPM2;
  int FunctionPassRunCount2 = 0;
  int AnalyzedInstrCount2 = 0;
  int AnalyzedFunctionCount2 = 0;
  FPM2.addPass(TestFunctionPass(FunctionPassRunCount2, AnalyzedInstrCount2,
                                AnalyzedFunctionCount2));
  MPM.addPass(createModuleToFunctionPassAdaptor(FPM2));

  // A third function pass manager but with only preserving intervening passes
  // and with a function pass that invalidates exactly one analysis.
  MPM.addPass(TestPreservingModulePass());
  FunctionPassManager FPM3;
  int FunctionPassRunCount3 = 0;
  int AnalyzedInstrCount3 = 0;
  int AnalyzedFunctionCount3 = 0;
  FPM3.addPass(TestFunctionPass(FunctionPassRunCount3, AnalyzedInstrCount3,
                                AnalyzedFunctionCount3));
  FPM3.addPass(TestInvalidationFunctionPass("f"));
  MPM.addPass(createModuleToFunctionPassAdaptor(FPM3));

  // A fourth function pass manager but with a minimal intervening passes.
  MPM.addPass(TestMinPreservingModulePass());
  FunctionPassManager FPM4;
  int FunctionPassRunCount4 = 0;
  int AnalyzedInstrCount4 = 0;
  int AnalyzedFunctionCount4 = 0;
  FPM4.addPass(TestFunctionPass(FunctionPassRunCount4, AnalyzedInstrCount4,
                                AnalyzedFunctionCount4));
  MPM.addPass(createModuleToFunctionPassAdaptor(FPM4));

  // A fifth function pass manager but which uses only cached results.
  FunctionPassManager FPM5;
  int FunctionPassRunCount5 = 0;
  int AnalyzedInstrCount5 = 0;
  int AnalyzedFunctionCount5 = 0;
  FPM5.addPass(TestInvalidationFunctionPass("f"));
  FPM5.addPass(TestFunctionPass(FunctionPassRunCount5, AnalyzedInstrCount5,
                                AnalyzedFunctionCount5,
                                /*OnlyUseCachedResults=*/true));
  MPM.addPass(createModuleToFunctionPassAdaptor(FPM5));

  MPM.run(M.get(), &MAM);

  // Validate module pass counters.
  EXPECT_EQ(1, ModulePassRunCount);

  // Validate all function pass counter sets are the same.
  EXPECT_EQ(3, FunctionPassRunCount1);
  EXPECT_EQ(5, AnalyzedInstrCount1);
  EXPECT_EQ(0, AnalyzedFunctionCount1);
  EXPECT_EQ(3, FunctionPassRunCount2);
  EXPECT_EQ(5, AnalyzedInstrCount2);
  EXPECT_EQ(0, AnalyzedFunctionCount2);
  EXPECT_EQ(3, FunctionPassRunCount3);
  EXPECT_EQ(5, AnalyzedInstrCount3);
  EXPECT_EQ(0, AnalyzedFunctionCount3);
  EXPECT_EQ(3, FunctionPassRunCount4);
  EXPECT_EQ(5, AnalyzedInstrCount4);
  EXPECT_EQ(0, AnalyzedFunctionCount4);
  EXPECT_EQ(3, FunctionPassRunCount5);
  EXPECT_EQ(2, AnalyzedInstrCount5); // Only 'g' and 'h' were cached.
  EXPECT_EQ(0, AnalyzedFunctionCount5);

  // Validate the analysis counters:
  //   first run over 3 functions, then module pass invalidates
  //   second run over 3 functions, nothing invalidates
  //   third run over 0 functions, but 1 function invalidated
  //   fourth run over 1 function
  EXPECT_EQ(7, FunctionAnalysisRuns);

  EXPECT_EQ(1, ModuleAnalysisRuns);
}
}
