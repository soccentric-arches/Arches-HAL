#include "imu_lib/imu_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<ImuTesting>("ImuTesting", argc, argv);
}
