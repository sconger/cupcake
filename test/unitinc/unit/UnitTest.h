
#define FAILF(fmt, ...) do { \
    testf(fmt, __VA_ARGS__); \
    return false; \
} while (0)

int testf(const char* fmt, ...);
