add_library(downloader STATIC
    downloader.cpp
    downloader.h
    torrent_downloader.cpp
    torrent_downloader.h
    common.cpp
    common.h
)

target_link_libraries(downloader PUBLIC cpr::cpr indicators::indicators libtransmission)

add_executable(eru main.cpp)
target_link_libraries(eru PRIVATE downloader CLI11::CLI11 indicators::indicators)
