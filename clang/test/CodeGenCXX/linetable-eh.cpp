// RUN: %clang_cc1 -emit-llvm -g -triple x86_64-apple-macosx10.9.0 -munwind-tables -std=c++11 -fcxx-exceptions -fexceptions %s -o - | FileCheck %s

// Test that emitting a landing pad does not affect the line table
// entries for the code that triggered it.

// CHECK: call void @llvm.dbg.declare
// CHECK: call void @llvm.dbg.declare(metadata !{{{.*}}}, metadata ![[CURRENT_ADDR:.*]]), !dbg ![[DBG1:.*]]
// CHECK: unwind label %{{.*}}, !dbg ![[DBG1]]
// CHECK: store i64 %{{.*}}, i64* %current_address, align 8, !dbg ![[DBG1]]
// CHECK-NEXT: call void @llvm.dbg.declare(metadata !{{{.*}}}, metadata ![[FOUND_IT:.*]]), !dbg ![[DBG2:.*]]
// CHECK: = landingpad
// CHECK-NEXT: cleanup, !dbg ![[DBG3:.*]]
// CHECK-DAG: ![[CURRENT_ADDR]] = {{.*}} [current_address]
// CHECK-DAG: ![[FOUND_IT]] = {{.*}} [found_it]
// CHECK-DAG: ![[DBG1]] = metadata !{i32 256,
// CHECK-DAG: ![[DBG2]] = metadata !{i32 257,
// CHECK-DAG: ![[DBG3]] = metadata !{i32 268,
typedef unsigned long long uint64_t;
template<class _Tp> class shared_ptr {
public:
  typedef _Tp element_type;
  element_type* __ptr_;
  ~shared_ptr();
  element_type* operator->() const noexcept {return __ptr_;}
};
class Context {
public:
    uint64_t GetIt();
};
class Foo
{
    bool bar();
    virtual shared_ptr<Context> GetContext () = 0;
};
# 253 "Foo.cpp" 3
bool
Foo::bar ()
{
  uint64_t current_address = GetContext()->GetIt();
  bool found_it = false;
# 267 "Foo.cpp" 3
  return found_it;
}
