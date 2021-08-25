void check(int expected, int actual, char *str);
#define CHECK(expected, actual) check(expected, actual, #actual)
