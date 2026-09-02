#include "rtc_lib/rtc_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<RtcTesting>("RtcTesting", argc, argv);
}
