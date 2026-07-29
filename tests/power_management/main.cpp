#include "power_management_lib/power_management_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<PowerManagementTesting>("PowerManagementTesting", argc,
                                            argv);
}
