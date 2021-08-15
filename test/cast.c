#include "test.h"

int main() {
  CHECK(736033791, (int)82340412415);
  CHECK(-1025, (short)82340412415);
  CHECK(-1, (char)82340412415);
  CHECK(-1, (char)255);
  CHECK(-1, (signed char)255);
  CHECK(255, (unsigned char)255);
  CHECK(-1, (short)65535);
  CHECK(65535, (unsigned short)65535);

  CHECK(1, -1 < 1);
  CHECK(254, (char)127 + (char)127);
  CHECK(65534, (short)32767 + (short)32767);
  CHECK(-1, -1>>1);
  CHECK(2147483647, ((unsigned int)-1)>>1);
  CHECK(-64, (-128) / 2);
  CHECK(-1, (-100) % 3);
  CHECK(0, ((long)-1) / (unsigned int)100);
  CHECK(-1, (unsigned long)-1);

  CHECK(65535, (int)(unsigned short)65535);
  CHECK(-1, (int)(short)65535);
  CHECK(65535, ({ unsigned short x = 65535; x; }));
  CHECK(65535, ({ unsigned short x = 65535; (int)x; }));

  CHECK(0, -1<(unsigned)1);
  // CHECK(65537, ({int a = 65536; *(short *)&a=1; a;}));

  return 0;
}
