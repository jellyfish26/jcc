#include "test.h"

int main() {
  CHECKD(2.3, 2.3);
  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(4.3, 2.3 + 2.0);

  CHECKD(2.3, ({
    double a = 2.3;
    a;
  }));

  // Bug
  // CHECKD(7.65, ({
  //   double a = 3.55;
  //   a;
  // }));

  return 0;
}
