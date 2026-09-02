#include "battery_lib/battery_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<BatteryTesting>("BatteryTesting", argc, argv);
}
