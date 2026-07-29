#include "functional_safety_lib/functional_safety_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<FunctionalSafetyTesting>("FunctionalSafetyTesting", argc,
                                             argv);
}
