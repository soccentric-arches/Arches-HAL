#include "gpio_lib/gpio_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<GpioTesting>("GpioTesting", argc, argv);
}
