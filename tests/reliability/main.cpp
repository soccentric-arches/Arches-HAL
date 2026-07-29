#include "reliability_lib/reliability_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<ReliabilityTesting>("ReliabilityTesting", argc, argv);
}
