#include "nvme_lib/nvme_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<NvmeTesting>("NvmeTesting", argc, argv);
}
