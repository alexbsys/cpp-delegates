
#ifdef __VLD_LIB
#include <vld.h>
#endif /*__VLD_LIB*/

#include "gtest/gtest.h"

#include "utils/mem_checker.h"
#include "utils/resource_checker.hpp"

int main(int argc, char* argv[]) {
  MemChecker mem_checker;

 {
/*    LOG_SET_CONFIG_PARAM("logger::LoadPlugins", "builtin win_config_macro ini_config modules_cmd stacktrace_cmd binary_cmd crashhandler_cmd objmon_cmd win_registry_config file_output");

    LOG_SET_CONFIG_PARAM("IniFilePaths", LOG_DEFAULT_INI_PATHS);

    LOG_RELOAD_CONFIG();
    LOG_INFO("tests started");

    LOG_DUMP_STATE(LOGGER_VERBOSE_INFO);

    // enable crash handler
    LOG_CMD(0x100A, 0, NULL, 0);
    */
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(print_time) = true;
    testing::UnitTest::GetInstance()->listeners().Append(new testing::ResourceCheckerShowUsage());

    RUN_ALL_TESTS();

/*    LOG_OBJMON_DUMP_INFO();
    LOG_FLUSH();
    LOG_SHUTDOWN();*/
  }

  return 0;
}
