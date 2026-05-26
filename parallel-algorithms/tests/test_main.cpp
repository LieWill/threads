#include <iostream>

#include "test_assert.h"

int failed = 0;
int passed = 0;

int main() {
    return TestRunner::instance().run_all();
}