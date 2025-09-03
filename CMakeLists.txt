cmake_minimum_required(VERSION 3.16)

project(coinapi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VCPKG_INCLUDE_DIR "/opt/homebrew/vcpkg/installed/x64-osx/include")

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(coinapi
    ${SRC_FILES}
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
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(coinapi PRIVATE
    Boost::filesystem
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    Threads::Threads
    simdjson
    CURL::libcurl
    jwt-cpp::jwt-cpp
)

