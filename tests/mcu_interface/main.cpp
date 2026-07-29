#include "mcu_interface_lib/mcu_interface_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<McuInterfaceTesting>("McuInterfaceTesting", argc, argv);
}
