cmake_minimum_required(VERSION 3.16)
project(RatibiApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Controls2)

add_executable(RatibiApp main.cpp)

target_link_libraries(RatibiApp PRIVATE Qt6::Quick Qt6::Controls2)
EOF
