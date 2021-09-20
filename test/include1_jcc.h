void check(int expected, int actual, char *str);
#define CHECK(expected, actual) check(expected, actual, #actual)

int a = 2;
int b = 3;
