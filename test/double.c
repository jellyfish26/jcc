#include "test.h"

int main() {
  CHECKD(2.3, 2.3);
  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(2.3, ({
    double a = 2.3;
    a;
  }));


  CHECKD(8.0, ({
    double a = 3, b = 5;
    a + b;
  }));

  CHECKD(5.5, ({
    double a = 3.5;
    a + 2;
  }));

  CHECK(5, ({
    double a = 3.5;
    int b = a + 2;
    b;
  }));

  CHECKD(7.75, ({
    double a = 3.55, b = 4.2;
    a + b;
  }));

  CHECKD(7.75, ({
    double a = 3.55, b = 4.2;
    a += b;
    a;
  }));

  CHECKD(3.2, ({
    double a = 5.5, b = 2.3;
    a - b;
  }));

  CHECKD(3.2, ({
    double a = 5.5, b = 2.3;
    a -= b;
    a;
  }));

  CHECKD(-2.3, ({
    double a = 5.0, b = 7.3;
    a - b;
  }));

  CHECKD(-2.3, ({
    double a = 5.0, b = 7.3;
    a -= b;
    a;
  }));

  CHECKD(36.5, ({
    double a = 5.0, b = 7.3;
    a * b;
  }));

  CHECKD(36.5, ({
    double a = 5.0, b = 7.3;
    a *= b;
    a;
  }));

  CHECKD(2.5, ({
    double a = 5.0, b = 2.0;
    a / b;
  }));

  CHECKD(2.5, ({
    double a = 5.0, b = 2.0;
    a /= b;
    a;
  }));

  CHECK(0, ({
    double a = 5.0, b = 5.0;
    a < b;
  }));

  CHECK(1, ({
    double a = 5.0, b = 5.0;
    a <= b;
  }));

  CHECK(1, ({
    double a = 5.0, b = 5.1;
    a < b;
  }));

  CHECK(1, ({
    double a = 5.0, b = 5.1;
    a <= b;
  }));

  CHECK(0, ({
    double a = 5.0, b = 5.1;
    b <= a;
  }));

  CHECK(0, ({
    double a = 5.0, b = 5.1;
    b < a;
  }));

  CHECK(1, ({
    double a = 5.0, b = 5.0;
    a == b;
  }));

  CHECK(0, ({
    double a = 5.0, b = 5.1;
    a == b;
  }));

  CHECK(0, ({
    double a = 5.0, b = 5.0;
    a != b;
  }));

  CHECK(1, ({
    double a = 5.0, b = 5.1;
    a != b;
  }));

  CHECK(1, ({
    double a = 0.01, b = 5.1;
    a && b;
  }));

  CHECK(0, ({
    double a = 0.0, b = 5.1;
    a && b;
  }));

  CHECK(0, ({
    double a = 0.0, b = 0.0;
    a && b;
  }));

  CHECK(0, ({
    double a = 5.1, b = 0.0;
    a && b;
  }));

  CHECK(1, ({
    double a = 5.1, b = 0.01;
    a && b;
  }));

  CHECK(1, ({
    double a = 0.01, b = 5.1;
    a || b;
  }));

  CHECK(1, ({
    double a = 0.0, b = 5.1;
    a || b;
  }));

  CHECK(0, ({
    double a = 0.0, b = 0.0;
    a || b;
  }));

  CHECK(1, ({
    double a = 5.1, b = 0.0;
    a || b;
  }));

  CHECK(1, ({
    double a = 5.1, b = 0.01;
    a || b;
  }));

  CHECK(0, ({
    double a = 5.1;
    !a;
  }));

  CHECK(0, ({
    double a = 0.00001;
    !a;
  }));

  CHECK(1, ({
    double a = 0.0;
    !a;
  }));

  CHECKD(5.1, ({
    double a = 5.1;
    a++;
  }));

  CHECKD(6.1, ({
    double a = 5.1;
    a++;
    a;
  }));

  CHECKD(6.1, ({
    double a = 5.1;
    ++a;
  }));

  CHECKD(6.1, ({
    double a = 5.1;
    ++a;
    a;
  }));

  CHECKF(2.3f, 2.3f);
  CHECKF(4.3f, 2.3f + 2.0f);
  CHECKD(4.3, 2.3 + 2.0f);

  CHECKF(2.3f, ({
    float a = 2.3f;
    a;
  }));

  CHECKF(2.3f, ({
    float a = 2.3;
    a;
  }));

  CHECKF(2.0f, ({
    float a = 2;
    a;
  }));

  CHECKF(8.0f, ({
    float a = 3.0f, b = 5.0f;
    a + b;
  }));

  CHECKF(5.5f, ({
    float a = 3.5f;
    a + 2;
  }));

  CHECK(5, ({
    float a = 3.5;
    int b = a + 2;
    b;
  }));

  CHECKF(7.75f, ({
    float a = 3.55f, b = 4.2f;
    a + b;
  }));

  CHECKF(7.75f, ({
    float a = 3.55f, b = 4.2f;
    a += b;
    a;
  }));

  CHECKF(3.2f, ({
    float a = 5.5f, b = 2.3f;
    a - b;
  }));

  CHECKF(3.2f, ({
    float a = 5.5f, b = 2.3f;
    a -= b;
    a;
  }));

  CHECKF(-2.1f, ({
    float a = 5.1f, b = 7.2f;
    a - b;
  }));

  CHECKF(-2.1f, ({
    float a = 5.1f, b = 7.2f;
    a -= b;
    a;
  }));

  CHECKF(36.5f, ({
    float a = 5.0f, b = 7.3f;
    a * b;
  }));

  CHECKF(36.5f, ({
    float a = 5.0f, b = 7.3f;
    a *= b;
    a;
  }));

  CHECKF(2.5f, ({
    float a = 5.0f, b = 2.0f;
    a / b;
  }));

  CHECKF(2.5, ({
    float a = 5.0f, b = 2.0f;
    a /= b;
    a;
  }));

  CHECK(0, ({
    float a = 5.0f, b = 5.0f;
    a < b;
  }));

  CHECK(1, ({
    float a = 5.0f, b = 5.0f;
    a <= b;
  }));

  CHECK(1, ({
    float a = 5.0f, b = 5.1f;
    a < b;
  }));

  CHECK(1, ({
    float a = 5.0f, b = 5.1f;
    a <= b;
  }));

  CHECK(0, ({
    float a = 5.0f, b = 5.1f;
    b <= a;
  }));

  CHECK(0, ({
    float a = 5.0f, b = 5.1f;
    b < a;
  }));

  CHECK(1, ({
    float a = 5.0f, b = 5.0f;
    a == b;
  }));

  CHECK(0, ({
    float a = 5.0f, b = 5.1f;
    a == b;
  }));

  CHECK(0, ({
    float a = 5.0f, b = 5.0f;
    a != b;
  }));

  CHECK(1, ({
    float a = 5.0f, b = 5.1f;
    a != b;
  }));

  CHECK(1, ({
    float a = 0.01f, b = 5.1f;
    a && b;
  }));

  CHECK(0, ({
    float a = 0.0f, b = 5.1f;
    a && b;
  }));

  CHECK(0, ({
    float a = 0.0f, b = 0.0f;
    a && b;
  }));

  CHECK(0, ({
    float a = 5.1f, b = 0.0f;
    a && b;
  }));

  CHECK(1, ({
    float a = 5.1f, b = 0.01f;
    a && b;
  }));

  CHECK(1, ({
    float a = 0.01f, b = 5.1f;
    a || b;
  }));

  CHECK(1, ({
    float a = 0.0f, b = 5.1f;
    a || b;
  }));

  CHECK(0, ({
    float a = 0.0f, b = 0.0f;
    a || b;
  }));

  CHECK(1, ({
    float a = 5.1f, b = 0.0f;
    a || b;
  }));

  CHECK(1, ({
    float a = 5.1f, b = 0.01f;
    a || b;
  }));

  CHECK(0, ({
    float a = 5.1f;
    !a;
  }));

  CHECK(0, ({
    float a = 0.00001f;
    !a;
  }));

  CHECK(1, ({
    float a = 0.0f;
    !a;
  }));

  CHECKF(5.1f, ({
    float a = 5.1f;
    a++;
  }));

  CHECKF(6.1f, ({
    float a = 5.1f;
    a++;
    a;
  }));

  CHECKF(6.1f, ({
    float a = 5.1f;
    ++a;
  }));

  CHECKF(6.1f, ({
    float a = 5.1f;
    ++a;
    a;
  }));

  CHECKD(4.6, ({
    double a[2] = {2.2, 2.4};
    a[0] + a[1];
  }));

  CHECKD(4.8, ({
    double a[2][2] = {{2.2, 2.4}, {0.2}};
    double ans = 0;
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        ans += a[i][j];
      }
    }
    ans;
  }));

  return 0;
}
