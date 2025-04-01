# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/rishi/esp/esp-idf/components/bootloader/subproject"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/tmp"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/src/bootloader-stamp"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/src"
  "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/rishi/esp/esp-idf/examples/airowl/m5core-s3-aqi-sensor/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
