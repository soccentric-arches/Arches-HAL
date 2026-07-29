#include "pcie_lib/pcie_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<PcieTesting>("PcieTesting", argc, argv);
}
