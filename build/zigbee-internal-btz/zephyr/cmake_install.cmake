# Install script for directory: /home/goodbyte/projects/workspace_zigbee/zephyr

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
    set(CMAKE_INSTALL_CONFIG_NAME "MinSizeRel")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/goodbyte/ncs/toolchains/b77d8c1312/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/arch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/soc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/boards/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/subsys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/drivers/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/ncs-zigbee/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/nrf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/mcuboot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/trusted-firmware-m/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/cjson/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/azure-sdk-for-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/cirrus-logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/suit-processor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/memfault-firmware-sdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/canopennode/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/chre/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/lz4/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/nanopb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/zscilib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/cmsis/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/cmsis-dsp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/cmsis-nn/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/hal_nordic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/hal_st/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/hal_wurthelektronik/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/hostap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/libmetal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/liblc3/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/littlefs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/loramac-node/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/lvgl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/mipi-sys-t/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/nrf_wifi/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/open-amp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/picolibc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/segger/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/tinycrypt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/uoscore-uedhoc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/zcbor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/nrfxlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/nrf_hw_models/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/modules/connectedhomeip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/kernel/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/cmake/flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/cmake/usage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/goodbyte/projects/workspace_zigbee/zigbee-internal-btz/build/zigbee-internal-btz/zephyr/cmake/reports/cmake_install.cmake")
endif()

