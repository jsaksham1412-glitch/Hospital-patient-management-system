cmake_minimum_required(VERSION 3.16)
project(HospitalPatientManagement LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(httplib nlohmann_json)

add_executable(hospital main.cpp)
target_compile_options(hospital PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(hospital PRIVATE HOSPITAL_WEB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/web")
target_link_libraries(hospital PRIVATE httplib::httplib nlohmann_json::nlohmann_json)