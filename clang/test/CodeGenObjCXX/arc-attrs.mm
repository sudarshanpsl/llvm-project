// RUN: %clang_cc1 -triple x86_64-apple-darwin11 -emit-llvm -fobjc-arc -o - %s | FileCheck %s

id makeObject1() __attribute__((ns_returns_retained));
id makeObject2() __attribute__((ns_returns_retained));
void releaseObject(__attribute__((ns_consumed)) id);

// CHECK-LABEL: define void @_Z10sanityTestv
void sanityTest() {
  // CHECK: [[X:%.*]] = alloca i8*, align 8
  // CHECK-NEXT: [[OBJ1:%.*]] = call i8* @_Z11makeObject1v()
  // CHECK-NEXT: store i8* [[OBJ1]], i8** [[X]], align 8
  id x = makeObject1();

  // CHECK-NEXT: [[OBJ2:%.*]] = call i8* @_Z11makeObject2v()
  // CHECK-NEXT: call void @_Z13releaseObjectU11ns_consumedP11objc_object(i8* [[OBJ2]])
  releaseObject(makeObject2());

  // CHECK-NEXT: call void @objc_storeStrong(i8** [[X]], i8* null)
  // CHECK-NEXT: ret void
}


template <typename T>
T makeObjectT1() __attribute__((ns_returns_retained));
template <typename T>
T makeObjectT2() __attribute__((ns_returns_retained));

template <typename T>
void releaseObjectT(__attribute__((ns_consumed)) T);  

// CHECK-LABEL: define void @_Z12templateTestv
void templateTest() {
  // CHECK: [[X:%.*]] = alloca i8*, align 8
  // CHECK-NEXT: [[OBJ1:%.*]] = call i8* @_Z12makeObjectT1IU8__strongP11objc_objectEU19ns_returns_retainedT_v()
  // CHECK-NEXT: store i8* [[OBJ1]], i8** [[X]], align 8
  id x = makeObjectT1<id>();

  // CHECK-NEXT: [[OBJ2:%.*]] = call i8* @_Z12makeObjectT2IU8__strongP11objc_objectEU19ns_returns_retainedT_v()
  // CHECK-NEXT: call void @_Z13releaseObjectU11ns_consumedP11objc_object(i8* [[OBJ2]])
  releaseObject(makeObjectT2<id>());

  // CHECK-NEXT: [[OBJ3:%.*]] = call i8* @_Z11makeObject1v()
  // CHECK-NEXT: call void @_Z14releaseObjectTIU8__strongP11objc_objectEvU11ns_consumedT_(i8* [[OBJ3]])
  releaseObjectT(makeObject1());

  // CHECK-NEXT: call void @objc_storeStrong(i8** [[X]], i8* null)
  // CHECK-NEXT: ret void
}
