cmake_minimum_required(VERSION 3.12)

project(MyNetLib VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall
    -Wextra
    -Werror=return-type
    -g
)

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(spdlog REQUIRED)

set(SOURCE_FILES
    src/Acceptor.cpp
    src/Buffer.cpp
    src/Channel.cpp        
    src/EpollPoller.cpp
    src/EventLoop.cpp
    src/EventLoopThreadPool.cpp
    src/InetAddr.cpp
    src/Socket.cpp
    src/TcpConnection.cpp
    src/TcpServer.cpp
)

# add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

add_executable(net_test main.cpp ${SOURCE_FILES})

target_link_libraries(net_test PRIVATE spdlog::spdlog)