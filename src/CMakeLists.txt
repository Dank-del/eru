add_library(downloader STATIC
    downloader.cpp
    downloader.h
)

target_link_libraries(downloader PUBLIC cpr::cpr indicators::indicators)

add_executable(eru main.cpp)
target_link_libraries(eru PRIVATE downloader CLI11::CLI11 indicators::indicators)
