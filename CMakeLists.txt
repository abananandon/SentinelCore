cmake_minimum_required(VERSION 3.10)

project(SentinelCore C) # 明确指定项目语言为 C

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 定义可执行文件的名称
add_executable(sentinel_app "") # 源文件在下面添加

# 关键一步：添加全局的头文件包含路径 - 适合你项目自身的头文件
target_include_directories(sentinel_app PUBLIC "${PROJECT_SOURCE_DIR}/sentinel/include/"
  PUBLIC "${PROJECT_SOURCE_DIR}/sentinel/src/third_party")

# 查找 Paho MQTT C 库
# Paho 库通常命名为 paho-mqtt3c (标准) 或 paho-mqtt3cs (带 SSL)
# 由于你找到了 libpaho-mqtt3c.so 和 libpaho-mqtt3cs.so，这个 NAMES 是匹配的
find_library(PAHO_MQTT_C_LIBRARY NAMES paho-mqtt3c paho-mqtt3cs
             HINTS "${CMAKE_SYSROOT}/usr/lib" "${CMAKE_SYSROOT}/lib" # 在 sysroot 的标准库路径下查找
             )

# 查找 Paho MQTT C 头文件 (你之前是在 /usr/local/arm/.../libc/usr/include 下发现头文件的)
find_path(PAHO_MQTT_C_INCLUDE_DIR NAMES MQTTClient.h
          HINTS "${CMAKE_SYSROOT}/usr/include" "${CMAKE_SYSROOT}/usr/local/include" # 在 sysroot 的标准头文件路径下查找
          )

# 检查是否找到库
if (NOT PAHO_MQTT_C_LIBRARY)
  message(FATAL_ERROR "Paho MQTT C library not found. Please check its installation in your sysroot: ${CMAKE_SYSROOT}")
endif()

# 如果找到了 Paho 头文件路径，添加到包含路径 (可选，但推荐为了避免万一查找不到)
if (PAHO_MQTT_C_INCLUDE_DIR)
  target_include_directories(sentinel_app PRIVATE ${PAHO_MQTT_C_INCLUDE_DIR})
endif()

# 使用 GLOB_RECURSE 自动查找所有 .c 源文件并添加到可执行文件
file(GLOB_RECURSE SOURCES "sentinel/src/*.c")
# 添加宏定义以确保 pthread 相关功能可用，解决可能的警告或编译问题
target_compile_definitions(sentinel_app PRIVATE -D_POSIX_SOURCE -D_GNU_SOURCE)

target_sources(sentinel_app PRIVATE ${SOURCES})

# 关键一步：链接 Paho MQTT 库和 pthread 库
# CMake 会优先链接动态库 (.so) 如果找到的话。
# paho-mqtt3c/cs 是你找到的共享库名，pthread 是多线程库
target_link_libraries(sentinel_app PRIVATE ${PAHO_MQTT_C_LIBRARY} pthread)


