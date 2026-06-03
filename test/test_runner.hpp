#pragma once
#include <cstdio>
#include <cstdlib>

inline int s_passed = 0;
inline int s_failed = 0;

#define TEST(name) void name()
#define RUN(name)  do { printf("  %-40s", #name); name(); printf("PASS\n"); s_passed++; } while(0)

#define ASSERT_TRUE(expr) \
    do { if (!(expr)) { printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, #expr); s_failed++; return; } } while(0)

#define ASSERT_EQ(a, b) \
    do { if ((a) != (b)) { printf("FAIL\n    %s:%d: expected %d got %d\n", __FILE__, __LINE__, (int)(b), (int)(a)); s_failed++; return; } } while(0)

inline void print_summary()
{
    printf("\n%d passed, %d failed\n", s_passed, s_failed);
    if (s_failed > 0) exit(1);
}
