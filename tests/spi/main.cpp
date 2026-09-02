#include "spi_lib/spi_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SpiTesting>("SpiTesting", argc, argv);
}
