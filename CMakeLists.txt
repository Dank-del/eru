cmake_minimum_required(VERSION 3.16)
project(file_downloader VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
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
  GIT_TAG v2.4.2
)
FetchContent_MakeAvailable(cli11)

FetchContent_Declare(
  indicators
  GIT_REPOSITORY https://github.com/p-ranav/indicators.git
  GIT_TAG v2.3
)
FetchContent_MakeAvailable(indicators)

FetchContent_Declare(
  libtorrent
  GIT_REPOSITORY https://github.com/arvidn/libtorrent.git
  GIT_TAG v2.0.11
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libtorrent)

add_subdirectory(src)
