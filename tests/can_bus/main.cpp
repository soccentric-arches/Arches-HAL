#include "can_bus_lib/can_bus_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<CanBusTesting>("CanBusTesting", argc, argv);
}
