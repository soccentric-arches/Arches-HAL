#include "pwm_output_lib/pwm_output_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<PwmOutputTesting>("PwmOutputTesting", argc, argv);
}
