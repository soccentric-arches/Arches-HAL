#include "cpu_lib/cpu_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<CpuTesting>("CpuTesting", argc, argv);
}
