# Install script for directory: /home/sandesh/work/project-managers/Arches/Arches-HAL/tests

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/gpu/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/cpu/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/deep_learning_accelerator/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/vision_accelerator/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/system_memory/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/storage/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/video_encode_decode/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/pcie/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/ethernet/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/usb/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/camera_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/display_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/communication_interface/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/gpio/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/audio_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/power_management/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/thermal_management/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/clock_and_timing/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/security_features/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/watchdog_and_reset/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/external_connector/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/mcu_interface/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/ethernet_switch/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/gnss/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/imu/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/serializer_deserializer/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/environmental/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/reliability/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/software_interface/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/functional_safety/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/system_integration/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/debug_and_development_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/sense_hat/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/camera_module3/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/poe_hat/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/stm32_transport/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/tests/pinmux_config/cmake_install.cmake")

endif()

