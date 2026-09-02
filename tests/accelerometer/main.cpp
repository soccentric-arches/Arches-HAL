#include "accelerometer_lib/accelerometer_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<AccelerometerTesting>("AccelerometerTesting", argc, argv);
}
