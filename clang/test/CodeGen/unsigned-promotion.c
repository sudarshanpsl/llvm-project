// Check -fsanitize=signed-integer-overflow and
// -fsanitize=unsigned-integer-overflow with promoted unsigned types
//
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-llvm -o - %s \
// RUN:   -fsanitize=signed-integer-overflow | FileCheck %s --check-prefix=CHECKS
// RUN: %clang_cc1 -triple x86_64-apple-darwin10 -emit-llvm -o - %s \
// RUN:   -fsanitize=unsigned-integer-overflow | FileCheck %s --check-prefix=CHECKU

unsigned short si, sj, sk;
unsigned char ci, cj, ck;

extern void opaqueshort(unsigned short);
extern void opaquechar(unsigned char);

// CHECKS-LABEL:   define void @testshortadd()
// CHECKU-LABEL: define void @testshortadd()
void testshortadd() {
  // CHECKS:        load i16, i16* @sj
  // CHECKS:        load i16, i16* @sk
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_add_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i16, i16* @sj
  // CHECKU:      [[T2:%.*]] = zext i16 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i16, i16* @sk
  // CHECKU:      [[T4:%.*]] = zext i16 [[T3]]
  // CHECKU-NOT:  llvm.sadd
  // CHECKU-NOT:  llvm.uadd
  // CHECKU:      [[T5:%.*]] = add nsw i32 [[T2]], [[T4]]

  si = sj + sk;
}

// CHECKS-LABEL:   define void @testshortsub()
// CHECKU-LABEL: define void @testshortsub()
void testshortsub() {

  // CHECKS:        load i16, i16* @sj
  // CHECKS:        load i16, i16* @sk
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_sub_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i16, i16* @sj
  // CHECKU:      [[T2:%.*]] = zext i16 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i16, i16* @sk
  // CHECKU:      [[T4:%.*]] = zext i16 [[T3]]
  // CHECKU-NOT:  llvm.ssub
  // CHECKU-NOT:  llvm.usub
  // CHECKU:      [[T5:%.*]] = sub nsw i32 [[T2]], [[T4]]

  si = sj - sk;
}

// CHECKS-LABEL:   define void @testshortmul()
// CHECKU-LABEL: define void @testshortmul()
void testshortmul() {

  // CHECKS:        load i16, i16* @sj
  // CHECKS:        load i16, i16* @sk
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_mul_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i16, i16* @sj
  // CHECKU:      [[T2:%.*]] = zext i16 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i16, i16* @sk
  // CHECKU:      [[T4:%.*]] = zext i16 [[T3]]
  // CHECKU-NOT:  llvm.smul
  // CHECKU-NOT:  llvm.umul
  // CHECKU:      [[T5:%.*]] = mul nsw i32 [[T2]], [[T4]]
  si = sj * sk;
}

// CHECKS-LABEL:   define void @testcharadd()
// CHECKU-LABEL: define void @testcharadd()
void testcharadd() {

  // CHECKS:        load i8, i8* @cj
  // CHECKS:        load i8, i8* @ck
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_add_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i8, i8* @cj
  // CHECKU:      [[T2:%.*]] = zext i8 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i8, i8* @ck
  // CHECKU:      [[T4:%.*]] = zext i8 [[T3]]
  // CHECKU-NOT:  llvm.sadd
  // CHECKU-NOT:  llvm.uadd
  // CHECKU:      [[T5:%.*]] = add nsw i32 [[T2]], [[T4]]

  ci = cj + ck;
}

// CHECKS-LABEL:   define void @testcharsub()
// CHECKU-LABEL: define void @testcharsub()
void testcharsub() {

  // CHECKS:        load i8, i8* @cj
  // CHECKS:        load i8, i8* @ck
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_sub_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i8, i8* @cj
  // CHECKU:      [[T2:%.*]] = zext i8 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i8, i8* @ck
  // CHECKU:      [[T4:%.*]] = zext i8 [[T3]]
  // CHECKU-NOT:  llvm.ssub
  // CHECKU-NOT:  llvm.usub
  // CHECKU:      [[T5:%.*]] = sub nsw i32 [[T2]], [[T4]]

  ci = cj - ck;
}

// CHECKS-LABEL:   define void @testcharmul()
// CHECKU-LABEL: define void @testcharmul()
void testcharmul() {

  // CHECKS:        load i8, i8* @cj
  // CHECKS:        load i8, i8* @ck
  // CHECKS:        [[T1:%.*]] = call { i32, i1 } @llvm.smul.with.overflow.i32(i32 [[T2:%.*]], i32 [[T3:%.*]])
  // CHECKS-NEXT:   [[T4:%.*]] = extractvalue { i32, i1 } [[T1]], 0
  // CHECKS-NEXT:   [[T5:%.*]] = extractvalue { i32, i1 } [[T1]], 1
  // CHECKS:        call void @__ubsan_handle_mul_overflow
  //
  // CHECKU:      [[T1:%.*]] = load i8, i8* @cj
  // CHECKU:      [[T2:%.*]] = zext i8 [[T1]]
  // CHECKU:      [[T3:%.*]] = load i8, i8* @ck
  // CHECKU:      [[T4:%.*]] = zext i8 [[T3]]
  // CHECKU-NOT:  llvm.smul
  // CHECKU-NOT:  llvm.umul
  // CHECKU:      [[T5:%.*]] = mul nsw i32 [[T2]], [[T4]]

  ci = cj * ck;
}
