cmake_minimum_required(VERSION 3.15)
project(merkelrex VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SQLite3 REQUIRED)
message(STATUS "SQLite3_FOUND: ${SQLite3_FOUND}")
message(STATUS "SQLite3_INCLUDE_DIRS: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3_LIBRARIES: ${SQLite3_LIBRARIES}")

include(FetchContent)

FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.0
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Crow)

find_package(Threads REQUIRED)

file(GLOB SOURCES "src/*.cpp")
add_executable(merkelrex ${SOURCES})

target_include_directories(merkelrex PRIVATE include)

if(TARGET SQLite::SQLite3)
    target_link_libraries(merkelrex PRIVATE SQLite::SQLite3)
else()
    target_include_directories(merkelrex PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(merkelrex PRIVATE ${SQLite3_LIBRARIES})
endif()

target_link_libraries(merkelrex PRIVATE Crow::Crow Threads::Threads)

target_compile_options(merkelrex PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -O2>
)