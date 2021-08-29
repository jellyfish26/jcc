void check(int expected, int actual, char *str);
void checkl(long expected, long actual, char *str);
void checkul(unsigned long expected, unsigned long actual, char *str);
void checkf(float expected, float actual, char *str);
void checkd(double expected, double actual, char *str);
#define CHECK(expected, actual) check(expected, actual, #actual)
#define CHECKL(expected, actual) checkl(expected, actual, #actual)
#define CHECKUL(expected, actual) checkul(expected, actual, #actual)
#define CHECKF(expected, actual) checkf(expected, actual, #actual)
#define CHECKD(expected, actual) checkd(expected, actual, #actual)
