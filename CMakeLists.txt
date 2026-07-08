cmake_minimum_required(VERSION 3.16)
project(file_downloader VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.x
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.3.2
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
  indicators
  GIT_REPOSITORY https://github.com/p-ranav/indicators.git
  GIT_TAG v2.3
)
FetchContent_MakeAvailable(indicators)

set(ENABLE_DAEMON OFF CACHE BOOL "" FORCE)
set(ENABLE_GTK    OFF CACHE BOOL "" FORCE)
set(ENABLE_QT     OFF CACHE BOOL "" FORCE)
set(ENABLE_MAC    OFF CACHE BOOL "" FORCE)
set(ENABLE_CLI    OFF CACHE BOOL "" FORCE)
set(ENABLE_UTILS  OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTS  OFF CACHE BOOL "" FORCE)
set(ENABLE_NLS    OFF CACHE BOOL "" FORCE)
set(INSTALL_DOC   OFF CACHE BOOL "" FORCE)
set(INSTALL_WEB   OFF CACHE BOOL "" FORCE)
set(REBUILD_WEB   OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  transmission
  GIT_REPOSITORY https://github.com/transmission/transmission.git
  GIT_TAG 4.0.6
  GIT_SHALLOW TRUE
  GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(transmission)

add_subdirectory(src)
