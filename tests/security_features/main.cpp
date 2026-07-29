#include "security_features_lib/security_features_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<SecurityFeaturesTesting>("SecurityFeaturesTesting", argc,
                                             argv);
}
