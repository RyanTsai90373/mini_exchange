cmake_minimum_required(VERSION 3.14)
project(tickengine CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----- Build types ---------------------------------------------------------
# Default to Debug if user did not specify
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Build type: Debug | RelWithDebInfo | Release | ASan | UBSan | TSan" FORCE)
endif()

# Custom sanitizer build types (ASan + TSan are mutually exclusive — separate types)
set(CMAKE_CXX_FLAGS_ASAN
    "-O1 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined"
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_ASAN
    "-fsanitize=address -fsanitize=undefined"
    CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_UBSAN
    "-O1 -g -fno-omit-frame-pointer -fsanitize=undefined"
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_UBSAN
    "-fsanitize=undefined"
    CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_TSAN
    "-O1 -g -fno-omit-frame-pointer -fsanitize=thread"
    CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_TSAN
    "-fsanitize=thread"
    CACHE STRING "" FORCE)

mark_as_advanced(
    CMAKE_CXX_FLAGS_ASAN  CMAKE_EXE_LINKER_FLAGS_ASAN
    CMAKE_CXX_FLAGS_UBSAN CMAKE_EXE_LINKER_FLAGS_UBSAN
    CMAKE_CXX_FLAGS_TSAN  CMAKE_EXE_LINKER_FLAGS_TSAN)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# ----- Dependencies --------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    hdr_histogram
    GIT_REPOSITORY https://github.com/HdrHistogram/HdrHistogram_c.git
    GIT_TAG        0.11.8
)
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(HDR_LOG_REQUIRED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hdr_histogram)

# ----- Targets (to be added as the strategy engine is built) --------------
# src/tools/ is header-only utilities (Timestamp, LatencyRecorder)
# Future targets will be added here as they're implemented.
