#include <stdarg.h>
#include "test.h"

int va_list_parameter_size(va_list ap) {
  return sizeof(ap);
}

int main() {
  va_list ap;

  CHECK(24, sizeof(__builtin_va_list));
  CHECK(24, sizeof(ap));
  CHECK(8, _Alignof(va_list));
  CHECK(8, va_list_parameter_size(ap));
  CHECK(4, sizeof(ap[0].gp_offset));
  CHECK(4, sizeof(ap[0].fp_offset));
  CHECK(8, sizeof(ap[0].overflow_arg_area));
  CHECK(8, sizeof(ap[0].reg_save_area));

  ap[0].gp_offset = 16;
  ap[0].fp_offset = 48;
  CHECK(16, ap[0].gp_offset);
  CHECK(48, ap[0].fp_offset);

  return 0;
}
