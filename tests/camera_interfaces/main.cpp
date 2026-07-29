#include "camera_interfaces_lib/camera_interfaces_lib.h"
#include "test_runner.h"

int main(int argc, char *argv[]) {
  return runTestApp<CameraInterfacesTesting>("CameraInterfacesTesting", argc,
                                             argv);
}
