#ifndef EP_TEST_H
#define EP_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond)                                                   \
    do {                                                                    \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            return 1;                                                       \
        }                                                                   \
    } while (0)

#define TEST_ASSERT_EQ(a, b)                                           \
    do {                                                               \
        if ((a) != (b)) {                                              \
            fprintf(stderr, "FAIL %s:%d: %s == %s (%ld != %ld)\n",     \
                    __FILE__, __LINE__, #a, #b, (long)(a), (long)(b)); \
            return 1;                                                  \
        }                                                              \
    } while (0)

#define TEST_RUN(fn)                                    \
    do {                                                \
        int _r = (fn)();                                \
        if (_r != 0) {                                  \
            fprintf(stderr, "suite failed: %s\n", #fn); \
            return _r;                                  \
        }                                               \
        printf("  ok %s\n", #fn);                       \
    } while (0)

#endif
