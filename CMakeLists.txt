# CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称
project(SentinelCore)

# 关键一步：告诉CMake在生成构建系统时，也生成compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 定义可执行文件的名称
add_executable(sentinel_app "") # 源文件在下面添加

# 关键一步：添加全局的头文件包含路径
# 这等价于 gcc 的 -Isentinel/include
target_include_directories(sentinel_app PUBLIC sentinel/include/)

# 使用 GLOB_RECURSE 自动查找所有 .c 源文件并添加到可执行文件
# 注意：更大型的项目建议手动列出源文件，但对于当前规模，自动查找很方便
file(GLOB_RECURSE SOURCES "sentinel/src/*.c")
target_sources(sentinel_app PRIVATE ${SOURCES})

# 如果有链接库的需求，可以在这里添加
# target_link_libraries(sentinel_app PRIVATE your_lib)

