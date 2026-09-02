#include "mipi_dsi_lib/mipi_dsi_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<MipiDsiTesting>("MipiDsiTesting", argc, argv);
}
