#include "debug_and_development_interfaces/debug_and_development_interfaces.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<DebugAndDevelopmentInterfaces>(
      "DebugAndDevelopmentInterfaces", argc, argv);
}
