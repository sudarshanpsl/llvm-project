; RUN: not llvm-as < %s -disable-output 2>&1 | FileCheck %s

; CHECK: [[@LINE+1]]:36: error: missing required field 'scope'
!0 = !MDNamespace(name: "Namespace")
