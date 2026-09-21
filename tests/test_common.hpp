#pragma once
#include <cstdlib>
#include <iostream>

#define CHECK(cond) \
  do { \
    if (!(cond)) { \
      std::cerr << "[TEST FAILED] " << __FILE__ << ":" << __LINE__ << " in " << __func__ \
                << "(): assertion failed: (" #cond ")\n"; \
      std::exit(1); \
    } \
  } while (0)
