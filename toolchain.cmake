# toolchain.cmake for Linaro 4.9.4 arm-linux-gnueabihf

# 1. 设置目标系统信息
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 2. 指定交叉编译器
#    根据您的GCC输出，工具链的根目录是:
set(TOOLCHAIN_ROOT "/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf")
set(CMAKE_C_COMPILER   "${TOOLCHAIN_ROOT}/bin/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_ROOT}/bin/arm-linux-gnueabihf-g++")

# 3. (关键!) 设置交叉编译环境的根路径 (sysroot)
#    这个路径告诉CMake在哪里寻找目标系统的头文件和库文件。
set(CMAKE_SYSROOT "${TOOLCHAIN_ROOT}/arm-linux-gnueabihf/libc")
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

# 4. 设置CMake的查找模式
#    确保CMake只在交叉编译环境的根路径(sysroot)中查找。
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

