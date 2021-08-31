#include "test.h"

int a;
int b;
int c, d;
int ar[2][2];
short s;

int init_a = 2;
short init_b = 8;
char init_c = 'a';
long init_d = 348214;

int init_arr1[5] = {1, 2, 3, 4, 5};
int init_arr2[2][3] = {{1, 2, 3}, {4, 5, 6}};
int init_arr3[7] = {1, 2, 3, 4};
int init_arr4[4][4] = {{1, 2, 3, 4}, {5}, {6, 7}, {}};

int init_arr5[] = {1, 2, 3};
int init_arr6[][2] = {{1, 2}, {3, 4}};

const int ca = 7;
int const_expr1 = 2 + 3;
int const_expr2 = (2 + 1) * (4 + 2) / 2; 
int const_expr3 = 'a' + 2;
int const_expr4 = 1<<5;
int const_expr5 = 256>>2;
int const_expr6 = 423145 | 41321;
const int const_expr7 = 1 == 1 ? 4 : 5;
int const_expr8 = const_expr7 * 3;

int const_expr9 = 3;
int *const_expr10 = &const_expr9;

int const_expr11[2][3] = {{1, 3, 2}, {6, 9, 2}};
int *const_expr12 = *(const_expr11 + 1) + 1;
int const_expr13 = (2 - 4) * (2) * -1;


double da = 2.3;
float fa = 2.3f;
float fb = 2.3;

double db = 2.0 + 4.5 - 3.0;
_Bool dc = 0.0 && 2.0;
_Bool dd = 0.0 || 2.0;
int de = (0.01 || 2.0) + 10;
const double df = 2.0 + 3;
double dg;
double dhh = df + 2.0;

int const_expr14 = 2.0 + 3;
double init_arr7[4][4] = {{1.2, 2.3f, 3.2, 4.3f}, {5.2}, {6.2, 7.5}, {}};
float init_arr8[4][4] = {{1.5, 2.3f, 0.0, 4.3f}, {5.2}, {6.2f, 7.5f}, {}};

int main() {
  CHECK(2, ({a = 2; a;}));
  CHECK(5, ({b = 1; int *c = &a; *c = 4; a + b;}));
  CHECK(4, ({
    for (int i = 0; i < 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        ar[i][j] = i + j;
      }
    }
    ar[0][0] + ar[1][1] + ar[0][1] + ar[1][0];
  }));
  CHECK(5, ({short a = 2; s = a + 3; s;}));
  CHECK(10, ({
    short a = 2;
    s = a + 3;
    // a = 5;
    /* c = 8; */
    s * a;
  }));

  CHECK(2, ({
    init_a;
  }));

  CHECK(8, ({
    init_b;
  }));

  CHECK(97, ({
    init_c;
  }));

  CHECK(348214, ({
    init_d;
  }));

  CHECK(15, ({
    int ans = 0;
    for (int i = 0; i < 5; i++) ans += init_arr1[i]; 
    ans;
  }));

  CHECK(21, ({
    int ans = 0;
    for (int i = 0; i < 2; i++) for (int j = 0; j < 3; j++) ans += init_arr2[i][j];
    ans;
  }));

  CHECK(10, ({
    int ans = 0;
    for (int i = 0; i < 7; i++) ans += init_arr3[i]; 
    ans;
  }));

  CHECK(28, ({
    int ans = 0;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) ans += init_arr4[i][j];
    ans;
  }));

  CHECK(6, ({
    int ans = 0;
    for (int i = 0; i < 3; i++) ans += init_arr5[i]; 
    ans;
  }));

  CHECK(10, ({
    int ans = 0;
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) ans += init_arr6[i][j];
    ans;
  }));

  CHECK(7, ca);
  CHECK(5, const_expr1);
  CHECK(9, const_expr2);
  CHECK(99, const_expr3);
  CHECK(32, const_expr4);
  CHECK(64, const_expr5);
  CHECK(456169, const_expr6);
  CHECK(4, const_expr7);
  CHECK(12, const_expr8);
  CHECK(3, *const_expr10);
  CHECK(9, *const_expr12);
  CHECK(4, const_expr13);

  CHECKD(2.3, da);
  CHECKF(2.3f, fa);
  CHECKF(2.3f, fb);
  CHECKD(3.5, db);
  CHECK(0, dc);
  CHECK(1, dd);
  CHECK(11, de);
  CHECKD(5.0, df);
  CHECKD(0.0, dg);
  CHECKD(7.0, dhh);

  CHECK(5, const_expr14);

  CHECKD(5.2, init_arr7[1][0]);
  CHECKD(29.9, ({
    double ans = 0;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        ans += init_arr7[i][j];
      }
    }
    ans;
  }));

  CHECKF(27.0f, ({
    float ans = 0;
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        ans += init_arr8[i][j];
      }
    }
    ans;
  }));

  return 0;
}
