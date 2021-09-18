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
void arr3(int a[2][5]);
int arr3_cnt(int a[2][5]);

struct D {
  int a;
  float b;
};

float se(struct D tmp);

struct E {
  char a;
  long double b;
};

long double sf(struct E tmp);
long double sg(struct E tmp, long double a, int b);
float si(struct D tmp, int a, int b, float c);

struct F {
  char a: 2;
  int b: 30;
  char c: 2, d: 2;
  short e: 12;
};

int sj(struct F tmp);
int sk(struct F tmp);

union G {
  int a: 3;
  int b: 8;
};

int ua(union G tmp);

int instatic();
int inauto();

struct A retsta();
struct B retstb();
struct C retstc();
struct D retstd();
struct E retste();
struct F retstf();

void ivla_1(int a, int b[a]);
int ivla_1cnt(int a, int b[a]);
void ivla_2(int a, int b, int c[b][a]);
int ivla_2cnt(int a, int b, int c[b][a]);
void dvla_1(int a, int b, double c[b][2][a]);
double dvla_1cnt(int a, int b, double c[b][2][a]);
