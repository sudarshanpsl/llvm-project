//=====---- X86Subtarget.h - Define Subtarget for the X86 -----*- C++ -*--====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the X86 specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#ifndef X86SUBTARGET_H
#define X86SUBTARGET_H

#include "llvm/Target/TargetSubtarget.h"

#include <string>

namespace llvm {
class Module;
class GlobalValue;
class TargetMachine;
  
namespace PICStyle {
enum Style {
  Stub, GOT, RIPRel, WinPIC, None
};
}

class X86Subtarget : public TargetSubtarget {
public:
  enum AsmWriterFlavorTy {
    // Note: This numbering has to match the GCC assembler dialects for inline
    // asm alternatives to work right.
    ATT = 0, Intel = 1, Unset
  };
protected:
  enum X86SSEEnum {
    NoMMXSSE, MMX, SSE1, SSE2, SSE3, SSSE3, SSE41, SSE42
  };

  enum X863DNowEnum {
    NoThreeDNow, ThreeDNow, ThreeDNowA
  };

  /// AsmFlavor - Which x86 asm dialect to use.
  ///
  AsmWriterFlavorTy AsmFlavor;

  /// PICStyle - Which PIC style to use
  ///
  PICStyle::Style PICStyle;
  
  /// X86SSELevel - MMX, SSE1, SSE2, SSE3, SSSE3, SSE41, SSE42, or
  /// none supported.
  X86SSEEnum X86SSELevel;

  /// X863DNowLevel - 3DNow or 3DNow Athlon, or none supported.
  ///
  X863DNowEnum X863DNowLevel;

  /// HasX86_64 - True if the processor supports X86-64 instructions.
  ///
  bool HasX86_64;
  
  /// DarwinVers - Nonzero if this is a darwin platform: the numeric
  /// version of the platform, e.g. 8 = 10.4 (Tiger), 9 = 10.5 (Leopard), etc.
  unsigned char DarwinVers; // Is any darwin-x86 platform.

  /// isLinux - true if this is a "linux" platform.
  bool IsLinux;

  /// stackAlignment - The minimum alignment known to hold of the stack frame on
  /// entry to the function and which must be maintained by every function.
  unsigned stackAlignment;

  /// Max. memset / memcpy size that is turned into rep/movs, rep/stos ops.
  ///
  unsigned MaxInlineSizeThreshold;

private:
  /// Is64Bit - True if the processor supports 64-bit instructions and module
  /// pointer size is 64 bit.
  bool Is64Bit;

public:
  enum {
    isELF, isCygwin, isDarwin, isWindows, isMingw
  } TargetType;

  /// This constructor initializes the data members to match that
  /// of the specified module.
  ///
  X86Subtarget(const Module &M, const std::string &FS, bool is64Bit);

  /// getStackAlignment - Returns the minimum alignment known to hold of the
  /// stack frame on entry to the function and which must be maintained by every
  /// function for this subtarget.
  unsigned getStackAlignment() const { return stackAlignment; }

  /// getMaxInlineSizeThreshold - Returns the maximum memset / memcpy size
  /// that still makes it profitable to inline the call.
  unsigned getMaxInlineSizeThreshold() const { return MaxInlineSizeThreshold; }

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(const std::string &FS, const std::string &CPU);

  /// AutoDetectSubtargetFeatures - Auto-detect CPU features using CPUID
  /// instruction.
  void AutoDetectSubtargetFeatures();

  bool is64Bit() const { return Is64Bit; }

  PICStyle::Style getPICStyle() const { return PICStyle; }
  void setPICStyle(PICStyle::Style Style)  { PICStyle = Style; }

  bool hasMMX() const { return X86SSELevel >= MMX; }
  bool hasSSE1() const { return X86SSELevel >= SSE1; }
  bool hasSSE2() const { return X86SSELevel >= SSE2; }
  bool hasSSE3() const { return X86SSELevel >= SSE3; }
  bool hasSSSE3() const { return X86SSELevel >= SSSE3; }
  bool hasSSE41() const { return X86SSELevel >= SSE41; }
  bool hasSSE42() const { return X86SSELevel >= SSE42; }
  bool has3DNow() const { return X863DNowLevel >= ThreeDNow; }
  bool has3DNowA() const { return X863DNowLevel >= ThreeDNowA; }

  unsigned getAsmFlavor() const {
    return AsmFlavor != Unset ? unsigned(AsmFlavor) : 0;
  }

  bool isFlavorAtt() const { return AsmFlavor == ATT; }
  bool isFlavorIntel() const { return AsmFlavor == Intel; }

  bool isTargetDarwin() const { return TargetType == isDarwin; }
  bool isTargetELF() const {
    return TargetType == isELF;
  }
  bool isTargetWindows() const { return TargetType == isWindows; }
  bool isTargetMingw() const { return TargetType == isMingw; }
  bool isTargetCygMing() const { return (TargetType == isMingw ||
                                         TargetType == isCygwin); }
  bool isTargetCygwin() const { return TargetType == isCygwin; }
  bool isTargetWin64() const {
    return (Is64Bit && (TargetType == isMingw || TargetType == isWindows));
  }

  std::string getDataLayout() const {
    const char *p;
    if (is64Bit())
      p = "e-p:64:64-s:64-f64:64:64-i64:64:64-f80:128:128";
    else {
      if (isTargetDarwin())
        p = "e-p:32:32-f64:32:64-i64:32:64-f80:128:128";
      else
        p = "e-p:32:32-f64:32:64-i64:32:64-f80:32:32";
    }
    return std::string(p);
  }

  bool isPICStyleSet() const { return PICStyle != PICStyle::None; }
  bool isPICStyleGOT() const { return PICStyle == PICStyle::GOT; }
  bool isPICStyleStub() const { return PICStyle == PICStyle::Stub; }
  bool isPICStyleRIPRel() const { return PICStyle == PICStyle::RIPRel; }
  bool isPICStyleWinPIC() const { return PICStyle == PICStyle:: WinPIC; }
  
  /// getDarwinVers - Return the darwin version number, 8 = tiger, 9 = leopard.
  unsigned getDarwinVers() const { return DarwinVers; }
  
  /// isLinux - Return true if the target is "Linux".
  bool isLinux() const { return IsLinux; }

  /// True if accessing the GV requires an extra load. For Windows, dllimported
  /// symbols are indirect, loading the value at address GV rather then the
  /// value of GV itself. This means that the GlobalAddress must be in the base
  /// or index register of the address, not the GV offset field.
  bool GVRequiresExtraLoad(const GlobalValue* GV, const TargetMachine& TM,
                           bool isDirectCall) const;

  /// This function returns the name of a function which has an interface
  /// like the non-standard bzero function, if such a function exists on
  /// the current subtarget and it is considered prefereable over
  /// memset with zero passed as the second argument. Otherwise it
  /// returns null.
  const char *getBZeroEntry() const;
};

namespace X86 {
  /// GetCpuIDAndInfo - Execute the specified cpuid and return the 4 values in
  /// the specified arguments.  If we can't run cpuid on the host, return true.
  bool GetCpuIDAndInfo(unsigned value, unsigned *rEAX, unsigned *rEBX,
                       unsigned *rECX, unsigned *rEDX);
}

} // End llvm namespace

#endif
