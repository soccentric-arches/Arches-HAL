#include "emmc_lib/emmc_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<EmmcTesting>("EmmcTesting", argc, argv);
}
