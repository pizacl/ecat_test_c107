cmake_minimum_required(VERSION 3.5)
project(soem_loopback C CXX)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

# SOEMのインストール先（環境に応じて変更）
include_directories(soem/include)
link_directories(soem/lib)

add_executable(loopback main.cpp)

target_link_libraries(loopback
    soem
    Threads::Threads
)