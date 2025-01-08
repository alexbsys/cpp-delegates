
#ifdef __VLD_LIB
#include <vld.h>
#endif /*__VLD_LIB*/

#include "gtest/gtest.h"

#include "utils/mem_checker.h"
#include "utils/resource_checker.hpp"

int main(int argc, char* argv[]) {
  MemChecker mem_checker;
  int ret;
  {
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(print_time) = true;
    testing::UnitTest::GetInstance()->listeners().Append(new testing::ResourceCheckerShowUsage());
    ret = RUN_ALL_TESTS();
  }

  return ret;
}
