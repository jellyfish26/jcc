void check(int expected, int actual, char *str);
void checkd(double expected, double actual, char *str);
#define CHECK(expected, actual) check(expected, actual, #actual)
#define CHECKD(expected, actual) checkd(expected, actual, #actual)
