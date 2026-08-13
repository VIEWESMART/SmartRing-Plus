# SPDX-FileCopyrightText: 2024-2026 Ayang / SHENZHEN VIEWE TECHNOLOGY CO.,LTD
# SPDX-License-Identifier: Apache-2.0
#
# SmartRing-Plus 整机演示工程（smartring_plus_full_device）
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smartring_plus_full_device)
if(NOT IDF_TARGET STREQUAL "esp32s3")
    message(FATAL_ERROR "本工程仅支持 ESP32-S3（SmartRing-Plus / N16R8）")
endif()
idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_LV_DRAW_SW_SUPPORT_RGB565_SWAPPED=1" APPEND)
