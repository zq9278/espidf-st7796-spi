# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/zq/esp/esp-idf/components/bootloader/subproject"
  "/home/zq/project-spi-lcd-touch/build/bootloader"
  "/home/zq/project-spi-lcd-touch/build/bootloader-prefix"
  "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/tmp"
  "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/src/bootloader-stamp"
  "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/src"
  "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/zq/project-spi-lcd-touch/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
