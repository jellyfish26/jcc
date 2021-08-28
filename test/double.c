#include "test.h"

int main() {
  CHECKD(2.3, 2.3);
  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(2.3, ({
    double a = 2.3;
    a;
  }));

  CHECKD(7.75, ({
    double a = 3.55, b = 4.2;
    a + b;
  }));

  CHECKD(8.0, ({
    double a = 3, b = 5;
    a + b;
  }));

  CHECKD(5.5, ({
    double a = 3.5;
    a + 2;
  }));


  return 0;
}
