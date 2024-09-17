cmake_minimum_required(VERSION 3.10)

project(flyzero VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(test-net)

# 添加子目录
add_subdirectory(external/llhttp)

# 添加可执行文件
add_executable(test-net
    src/event_dispatcher.cpp
    src/server.cpp
    src/main.cpp
    src/http_request.cpp)

# 链接库
target_link_libraries(test-net boost_context llhttp_shared)

# 设置 include 目录
target_include_directories(test-net
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/llhttp/include)