#include "i2c_lib/i2c_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<I2cTesting>("I2cTesting", argc, argv);
}
