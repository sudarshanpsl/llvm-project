// RUN: clang-cc -fnext-runtime -fobjc-gc -fobjc-newgc-api -emit-llvm -o %t %s &&
// RUN: grep -F '@objc_assign_ivar' %t  | count 11 &&
// RUN: true

typedef struct {
  id  element;
  id elementArray[10];
  __strong id cfElement;
  __strong id cfElementArray[10];
} struct_with_ids_t;


@interface NSString @end

@interface Foo  {
@public
// assignments to any/all of these fields should generate objc_assign_ivar
  __strong id dict;
  __strong id dictArray[3];
  id ivar;
  id array[10];
  id nsobject;
  NSString *stringArray[10];
  struct_with_ids_t inner;
}
@end

// The test cases
int IvarAssigns;
void *rhs = 0;
#define ASSIGNTEST(expr, global) expr = rhs

void testIvars() {
  Foo *foo;
  ASSIGNTEST(foo->ivar, IvarAssigns);                                   // objc_assign_ivar
  ASSIGNTEST(foo->dict, IvarAssigns);                                   // objc_assign_ivar
  ASSIGNTEST(foo->dictArray[0], IvarAssigns);                           // objc_assign_ivar
  ASSIGNTEST(foo->array[0], IvarAssigns);                               // objc_assign_ivar
  ASSIGNTEST(foo->nsobject, IvarAssigns);                               // objc_assign_ivar
  ASSIGNTEST(foo->stringArray[0], IvarAssigns);                         // objc_assign_ivar
  ASSIGNTEST(foo->inner.element, IvarAssigns);                          // objc_assign_ivar
  ASSIGNTEST(foo->inner.elementArray[0], IvarAssigns);                  // objc_assign_ivar
  ASSIGNTEST(foo->inner.cfElement, IvarAssigns);                        // objc_assign_ivar
  ASSIGNTEST(foo->inner.cfElementArray[0], IvarAssigns);                // objc_assign_ivar

}

