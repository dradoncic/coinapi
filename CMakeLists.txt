cmake_minimum_required(VERSION 3.16)
project(coinapi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(OpenSSL REQUIRED)

add_executable(coinapi
    src/main.cpp
    src/websocket.cpp
    src/spscqueue.cpp
)

target_include_directories(coinapi PRIVATE
    include
    ${Boost_INCLUDE_DIRS}   
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(coinapi PRIVATE
    Boost::filesystem
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

