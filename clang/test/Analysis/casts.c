// RUN: clang-cc -analyze -checker-cfref -analyzer-store=region --verify %s

// Test if the 'storage' region gets properly initialized after it is cast to
// 'struct sockaddr *'. 

#include <sys/socket.h>
void f(int sock) {
  struct sockaddr_storage storage;
  struct sockaddr* sockaddr = (struct sockaddr*)&storage;
  socklen_t addrlen = sizeof(storage);
  getsockname(sock, sockaddr, &addrlen);
  switch (sockaddr->sa_family) { // no-warning
  default:
    ;
  }
}

struct s {
  struct s *value;
};

int f1(struct s **pval) {
  int *tbool = ((void*)0);
  struct s *t = *pval;
  pval = &(t->value);
  tbool = (int *)pval; // Should record the cast-to type here.
  char c = (unsigned char) *tbool; // Should use cast-to type to create symbol.
}

void f2(const char *str) {
 unsigned char ch, cl, *p;

 p = (unsigned char *)str;
 ch = *p++; // use cast-to type 'unsigned char' to create element region.
 cl = *p++;
 if(!cl)
    cl = 'a';
}
