// RUN: %clang_cc1 -std=c++1z %s -emit-llvm -o - -triple %itanium_abi_triple | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-ITANIUM
// RUN: %clang_cc1 -std=c++1z %s -emit-llvm -o - -triple i686-windows | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-WINDOWS
// RUN: %clang_cc1 -std=c++1z %s -emit-llvm -o - -triple x86_64-windows | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-WINDOWS

struct B;
struct A {
  A();
  A(const A&);

  void operator[](B b);

  int a_member_f(B);
};
struct B {
  B();
  ~B();
};

struct C {
  operator int *();
  A *operator->();
  void operator->*(A);
  friend void operator->*(C, B);

  friend void operator<<(C, B);
  friend void operator>>(C, B);
  void operator<<(A);
  void operator>>(A);

  void operator=(A);
  void operator+=(A);
  friend void operator+=(C, B);

  void operator,(A);
  friend void operator,(C, B);

  void operator&&(A);
  void operator||(A);
  friend void operator&&(C, B);
  friend void operator||(C, B);
};

A make_a();
A *make_a_ptr();
int A::*make_mem_ptr_a();
void (A::*make_mem_fn_ptr_a())();
B make_b();
C make_c();
void side_effect();

void callee(A);
void (*get_f())(A);


// CHECK-LABEL: define {{.*}}@{{.*}}postfix_before_args{{.*}}(
void postfix_before_args() {
  // CHECK: call {{.*}}@{{.*}}get_f{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@_ZN1AC1Ev(
  // CHECK-WINDOWS: call {{.*}}@"\01??0A@@Q{{AE|EAA}}@XZ"(
  // CHECK: call {{.*}}%{{.*}}(
  get_f()(A{});

  // CHECK: call {{.*}}@{{.*}}side_effect{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@_ZN1AC1Ev(
  // CHECK-WINDOWS: call {{.*}}@"\01??0A@@Q{{AE|EAA}}@XZ"(
  // CHECK: call {{.*}}@{{.*}}callee{{.*}}(
  (side_effect(), callee)(A{});
// CHECK: }
}


// CHECK-LABEL: define {{.*}}@{{.*}}dot_lhs_before_rhs{{.*}}(
void dot_lhs_before_rhs() {
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK: call {{.*}}@{{.*}}a_member_f{{.*}}(
  make_a().a_member_f(make_b());

  // CHECK: call {{.*}}@{{.*}}make_a_ptr{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK: call {{.*}}@{{.*}}a_member_f{{.*}}(
  make_a_ptr()->a_member_f(make_b());

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK: call {{.*}}@{{.*}}a_member_f{{.*}}(
  make_c()->a_member_f(make_b());
// CHECK: }
}


// CHECK-LABEL: define {{.*}}@{{.*}}array_lhs_before_rhs{{.*}}(
void array_lhs_before_rhs() {
  int (&get_arr())[10];
  extern int get_index();

  // CHECK: call {{.*}}@{{.*}}get_arr{{.*}}(
  // CHECK: call {{.*}}@{{.*}}get_index{{.*}}(
  get_arr()[get_index()] = 0;

  // CHECK: call {{.*}}@{{.*}}get_index{{.*}}(
  // CHECK: call {{.*}}@{{.*}}get_arr{{.*}}(
  get_index()[get_arr()] = 0;

  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK: call
  make_a()[make_b()];

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}get_index{{.*}}(
  // CHECK: call
  make_c()[get_index()] = 0;

  // CHECK: call {{.*}}@{{.*}}get_index{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call
  get_index()[make_c()] = 0;
// CHECK: }
}


void *operator new(decltype(sizeof(0)), C);

// CHECK-LABEL: define {{.*}}@{{.*}}alloc_before_init{{.*}}(
void alloc_before_init() {
  struct Q { Q(A) {} };
  // CHECK-ITANIUM: call {{.*}}@_Znw{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@"\01??2@YAP{{EAX_K|AXI}}@Z"(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  delete new Q(make_a());

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  new (make_c()) Q(make_a());
// CHECK: }
}


// CHECK-LABEL: define {{.*}}@{{.*}}dotstar_lhs_before_rhs{{.*}}(
int dotstar_lhs_before_rhs() {
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_mem_ptr_a{{.*}}(
  int a = make_a().*make_mem_ptr_a();

  // CHECK: call {{.*}}@{{.*}}make_a_ptr{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_mem_ptr_a{{.*}}(
  int b = make_a_ptr()->*make_mem_ptr_a();

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c()->*make_a();

  // FIXME: The corresponding case for Windows ABIs is unimplementable if the
  // operand has a non-trivially-destructible type, because the order of
  // construction of function arguments is defined by the ABI there (it's the
  // reverse of the order in which the parameters are destroyed in the callee).
  // But we could follow the C++17 rule in the case where the operand type is
  // trivially-destructible.
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c()->*make_b();

  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_mem_fn_ptr_a{{.*}}(
  // CHECK: call
  (make_a().*make_mem_fn_ptr_a())();

  // CHECK: call {{.*}}@{{.*}}make_a_ptr{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_mem_fn_ptr_a{{.*}}(
  // CHECK: call
  (make_a_ptr()->*make_mem_fn_ptr_a())();

  return a + b;
// CHECK: }
}


// CHECK-LABEL: define {{.*}}@{{.*}}assign_rhs_before_lhs{{.*}}(
void assign_rhs_before_lhs() {
  extern int &lhs_ref(), rhs();

  // CHECK: call {{.*}}@{{.*}}rhs{{.*}}(
  // CHECK: call {{.*}}@{{.*}}lhs_ref{{.*}}(
  lhs_ref() = rhs();

  // CHECK: call {{.*}}@{{.*}}rhs{{.*}}(
  // CHECK: call {{.*}}@{{.*}}lhs_ref{{.*}}(
  lhs_ref() += rhs();

  // CHECK: call {{.*}}@{{.*}}rhs{{.*}}(
  // CHECK: call {{.*}}@{{.*}}lhs_ref{{.*}}(
  lhs_ref() %= rhs();

  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() = make_a();

  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() += make_a();

  // CHECK: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() += make_b();
// CHECK: }
}

// CHECK-LABEL: define {{.*}}@{{.*}}shift_lhs_before_rhs{{.*}}(
void shift_lhs_before_rhs() {
  extern int lhs(), rhs();

  // CHECK: call {{.*}}@{{.*}}lhs{{.*}}(
  // CHECK: call {{.*}}@{{.*}}rhs{{.*}}(
  (void)(lhs() << rhs());

  // CHECK: call {{.*}}@{{.*}}lhs{{.*}}(
  // CHECK: call {{.*}}@{{.*}}rhs{{.*}}(
  (void)(lhs() >> rhs());

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c() << make_a();

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c() >> make_a();

  // FIXME: This is unimplementable for Windows ABIs, see above.
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() << make_b();

  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() >> make_b();
// CHECK: }
}

// CHECK-LABEL: define {{.*}}@{{.*}}comma_lhs_before_rhs{{.*}}(
void comma_lhs_before_rhs() {
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c() , make_a();

  // FIXME: This is unimplementable for Windows ABIs, see above.
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() , make_b();
}

// CHECK-LABEL: define {{.*}}@{{.*}}andor_lhs_before_rhs{{.*}}(
void andor_lhs_before_rhs() {
  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c() && make_a();

  // CHECK: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK: call {{.*}}@{{.*}}make_a{{.*}}(
  make_c() || make_a();

  // FIXME: This is unimplementable for Windows ABIs, see above.
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() && make_b();

  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_c{{.*}}(
  // CHECK-ITANIUM: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_b{{.*}}(
  // CHECK-WINDOWS: call {{.*}}@{{.*}}make_c{{.*}}(
  make_c() || make_b();
}
