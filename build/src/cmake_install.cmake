# Install script for directory: /home/sandesh/work/project-managers/Arches/Arches-HAL/src

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/sandesh/work/project-managers/Arches/Arches-HAL/build/lib/liblib_base.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/acadia_hal" TYPE FILE FILES "/home/sandesh/work/project-managers/Arches/Arches-HAL/src/lib_base.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/sandesh/work/project-managers/Arches/Arches-HAL/build/lib/libarches_platform.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/acadia_hal" TYPE FILE FILES
    "/home/sandesh/work/project-managers/Arches/Arches-HAL/src/platform_arches.h"
    "/home/sandesh/work/project-managers/Arches/Arches-HAL/src/stm32_transport.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/gpu_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/cpu_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/deep_learning_accelerator_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/vision_accelerator_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/system_memory_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/storage_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/video_encode_decode_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/pcie_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/ethernet_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/usb_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/camera_interfaces_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/display_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/communication_interface_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/gpio_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/audio_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/power_management_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/thermal_management_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/clock_and_timing_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/security_features_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/watchdog_and_reset_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/external_connector_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/mcu_interface_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/ethernet_switch_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/gnss_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/imu_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/serializer_deserializer_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/environmental_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/reliability_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/software_interface_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/functional_safety_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/system_integration_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/debug_and_development_interfaces/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/sense_hat_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/camera_module3_lib/cmake_install.cmake")
  include("/home/sandesh/work/project-managers/Arches/Arches-HAL/build/src/poe_hat_lib/cmake_install.cmake")

endif()

