# Create lib_base library
add_library(lib_base lib_base.cpp lib_base.h)
target_include_directories(lib_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib_base Threads::Threads)
install(
  TARGETS lib_base
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES lib_base.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/arches_hal)

add_library(arches_platform stm32_transport.cpp stm32_transport.h
                            platform_arches.h)
target_include_directories(arches_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(arches_platform PUBLIC Threads::Threads lib_base)
install(
  TARGETS arches_platform
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES platform_arches.h stm32_transport.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/arches_hal)

# Add all feature libraries
add_subdirectory(gpu_lib)
add_subdirectory(cpu_lib)
add_subdirectory(deep_learning_accelerator_lib)
add_subdirectory(vision_accelerator_lib)
add_subdirectory(system_memory_lib)
add_subdirectory(storage_lib)
add_subdirectory(video_encode_decode_lib)
add_subdirectory(pcie_lib)
add_subdirectory(ethernet_lib)
add_subdirectory(usb_lib)
add_subdirectory(camera_interfaces_lib)
add_subdirectory(display_interfaces)
add_subdirectory(communication_interface_lib)
add_subdirectory(gpio_lib)
add_subdirectory(audio_interfaces)
add_subdirectory(power_management_lib)
add_subdirectory(thermal_management_lib)
add_subdirectory(clock_and_timing_lib)
add_subdirectory(security_features_lib)
add_subdirectory(watchdog_and_reset_lib)
add_subdirectory(external_connector_lib)
add_subdirectory(mcu_interface_lib)
add_subdirectory(ethernet_switch_lib)
add_subdirectory(gnss_lib)
add_subdirectory(imu_lib)
add_subdirectory(serializer_deserializer_lib)
add_subdirectory(environmental_lib)
add_subdirectory(reliability_lib)
add_subdirectory(software_interface_lib)
add_subdirectory(functional_safety_lib)
add_subdirectory(system_integration_lib)
add_subdirectory(debug_and_development_interfaces)
add_subdirectory(sense_hat_lib)
add_subdirectory(camera_module3_lib)
add_subdirectory(poe_hat_lib)

# Every native module exposes both a static and shared library variant.
set(HAL_MODULE_TARGETS
    audio_interfaces
    camera_interfaces_lib
    camera_module3_lib
    clock_and_timing_lib
    communication_interface_lib
    cpu_lib
    debug_and_development_interfaces
    deep_learning_accelerator_lib
    display_interfaces
    environmental_lib
    ethernet_lib
    ethernet_switch_lib
    external_connector_lib
    functional_safety_lib
    gnss_lib
    gpio_lib
    gpu_lib
    imu_lib
    mcu_interface_lib
    pcie_lib
    poe_hat_lib
    power_management_lib
    reliability_lib
    security_features_lib
    sense_hat_lib
    serializer_deserializer_lib
    software_interface_lib
    storage_lib
    system_integration_lib
    system_memory_lib
    thermal_management_lib
    usb_lib
    video_encode_decode_lib
    vision_accelerator_lib
    watchdog_and_reset_lib)

foreach(HAL_MODULE_TARGET IN LISTS HAL_MODULE_TARGETS)
  get_target_property(HAL_MODULE_SOURCES "${HAL_MODULE_TARGET}" SOURCES)
  get_target_property(HAL_MODULE_SOURCE_DIR "${HAL_MODULE_TARGET}" SOURCE_DIR)
  set(HAL_MODULE_ABSOLUTE_SOURCES)
  foreach(HAL_MODULE_SOURCE IN LISTS HAL_MODULE_SOURCES)
    if(IS_ABSOLUTE "${HAL_MODULE_SOURCE}")
      list(APPEND HAL_MODULE_ABSOLUTE_SOURCES "${HAL_MODULE_SOURCE}")
    else()
      list(APPEND HAL_MODULE_ABSOLUTE_SOURCES
           "${HAL_MODULE_SOURCE_DIR}/${HAL_MODULE_SOURCE}")
    endif()
  endforeach()

  add_library("${HAL_MODULE_TARGET}_shared" SHARED
              ${HAL_MODULE_ABSOLUTE_SOURCES})

  get_target_property(HAL_MODULE_INCLUDES "${HAL_MODULE_TARGET}"
                      INCLUDE_DIRECTORIES)
  if(HAL_MODULE_INCLUDES AND NOT HAL_MODULE_INCLUDES MATCHES "-NOTFOUND$")
    target_include_directories("${HAL_MODULE_TARGET}_shared"
                               PUBLIC ${HAL_MODULE_INCLUDES})
  endif()

  get_target_property(HAL_MODULE_LINKS "${HAL_MODULE_TARGET}" LINK_LIBRARIES)
  if(HAL_MODULE_LINKS AND NOT HAL_MODULE_LINKS MATCHES "-NOTFOUND$")
    target_link_libraries("${HAL_MODULE_TARGET}_shared"
                          PUBLIC ${HAL_MODULE_LINKS})
  endif()

  install(TARGETS "${HAL_MODULE_TARGET}_shared"
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach()
