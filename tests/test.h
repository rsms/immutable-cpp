#pragma once
#include <assert.h>
#include <stdio.h>
#include <functional>

#if defined(NDEBUG)
#error "NDEBUG defined for test"
#endif

//using TestFunc = std::function<void()>;
using TestFunc = void(*)();
void TestAdd(const char* name, TestFunc, unsigned int priority);

#define TEST(name) \
  static void name##_test(); \
  static void __attribute__((constructor)) name##_init() { \
    TestAdd(#name, name##_test, (unsigned int)__LINE__); \
  } \
  static void name##_test()
