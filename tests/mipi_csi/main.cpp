#include "mipi_csi_lib/mipi_csi_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<MipiCsiTesting>("MipiCsiTesting", argc, argv);
}
