int add2(int a, int b);
int sub2(int a, int b);
int add6(int a, int b, int c, int d, int e, int f);
int add8(int a, int b, int c, int d, int e, int f, int g, int h);
int fib(int a);

int vv(void);
int foo(int a, int b);
int foo();

int da(double a, int b);
int dla(long double a, int b);
int dlb(long double a, double b, double c, double d, double e, float g, double h, double i, double j, double k, int l);

struct A {
  int a, b;
  long c;
};

int sa(struct A tmp);
int sb(struct A tmp);

struct B {
  int a[6];
};

int sc(struct B tmp);

struct C {
  long double a;
};

long double sd(struct C tmp);

int arr1(int a[5]);
int arr2(int a[]);

struct D {
  int a;
  float b;
};

float se(struct D tmp);
