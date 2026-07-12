// The single translation unit that provides doctest's entry point. Every other
// test_*.cpp only includes the doctest header and registers TEST_CASEs.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"
