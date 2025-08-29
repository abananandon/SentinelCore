cmake_minimum_required(VERSION 3.10)

project(SentinelCore C) # 明确指定项目语言为 C

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 定义可执行文件的名称
add_executable(sentinel_app) 

# 获取工具链的sysroot路径
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-sysroot
    OUTPUT_VARIABLE TOOLCHAIN_SYSROOT_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 确保TOOLCHAIN_SYSROOT指向的是sysroot的根目录
# 例如，如果 -print-sysroot 返回 /path/to/libc/usr，我们只想要 /path/to/libc
if(TOOLCHAIN_SYSROOT_RAW MATCHES "/usr$")
    string(REPLACE "/usr" "" TOOLCHAIN_SYSROOT "${TOOLCHAIN_SYSROOT_RAW}")
else()
    set(TOOLCHAIN_SYSROOT "${TOOLCHAIN_SYSROOT_RAW}")
endif()

# 获取GCC内部的include路径
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-file-name=include
    OUTPUT_VARIABLE GCC_C_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 添加所有头文件搜索路径
# 对于可执行文件，通常使用 PRIVATE 即可
target_include_directories(sentinel_app PRIVATE
    ${PROJECT_SOURCE_DIR}/sentinel/include/        # 您项目自己的头文件
    ${PROJECT_SOURCE_DIR}/sentinel/src/third_party           # 第三方库头文件，检查 cJSON.h 是否在此路径下一级
    SYSTEM ${TOOLCHAIN_SYSROOT}/usr/include         # 明确添加目标系统的标准头文件路径为系统路径
    SYSTEM ${GCC_C_INCLUDE_DIR}                     # 明确添加GCC内部头文件路径为系统路径
)

# 查找 Paho MQTT C 库
find_library(PAHO_MQTT_C_LIBRARY NAMES paho-mqtt3c paho-mqtt3cs
             HINTS "${TOOLCHAIN_SYSROOT}/usr/lib" "${TOOLCHAIN_SYSROOT}/lib"
             )

# 查找 Paho MQTT C 头文件
find_path(PAHO_MQTT_C_INCLUDE_DIR NAMES MQTTClient.h
          HINTS "${TOOLCHAIN_SYSROOT}/usr/include" "${TOOLCHAIN_SYSROOT}/usr/local/include"
          )

# 检查是否找到库
if (NOT PAHO_MQTT_C_LIBRARY)
  message(FATAL_ERROR "Paho MQTT C library not found. Please check its installation in your sysroot: ${TOOLCHAIN_SYSROOT}")
endif()

# 如果找到了 Paho 头文件路径，添加到包含路径
if (PAHO_MQTT_C_INCLUDE_DIR) # 移除避免重复添加的复杂逻辑，CMake会处理
  target_include_directories(sentinel_app PRIVATE ${PAHO_MQTT_C_INCLUDE_DIR})
endif()

# 使用 GLOB_RECURSE 自动查找所有 .c 源文件并添加到可执行文件
file(GLOB_RECURSE SOURCES "sentinel/src/*.c")
target_sources(sentinel_app PRIVATE ${SOURCES})

# 添加宏定义以确保 pthread 相关功能可用
target_compile_definitions(sentinel_app PRIVATE -D_POSIX_SOURCE -D_GNU_SOURCE)

# 链接 Paho MQTT 库和 pthread 库
target_link_libraries(sentinel_app PRIVATE ${PAHO_MQTT_C_LIBRARY} pthread)

