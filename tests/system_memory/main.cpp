#include "system_memory_lib/system_memory_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SystemMemoryTesting>("SystemMemoryTesting", argc, argv);
}
