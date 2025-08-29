cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

project(coinapi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(OpenSSL REQUIRED)

add_executable(coinapi
    src/main.cpp
    src/websocket.cpp
)

include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_SHALLOW TRUE
  )

  set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
  set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")
   
  FetchContent_MakeAvailable(simdjson)

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
    simdjson
)

