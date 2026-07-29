#include "gpu_lib/gpu_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<GpuTesting>("GpuTesting", argc, argv);
}
