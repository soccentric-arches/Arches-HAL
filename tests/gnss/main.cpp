#include "gnss_lib/gnss_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<GnssTesting>("GnssTesting", argc, argv);
}
