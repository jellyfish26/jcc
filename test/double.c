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

  return 0;
}
