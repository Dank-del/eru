add_library(downloader STATIC
    downloader.cpp
    downloader.h
    torrent_downloader.cpp
    torrent_downloader.h
    common.cpp
    common.h
)

target_link_libraries(downloader PUBLIC cpr::cpr indicators::indicators)

if(TARGET libaria2)
    add_dependencies(downloader aria2_external)
    target_link_libraries(downloader PUBLIC libaria2)
    target_compile_definitions(downloader PRIVATE ERU_HAVE_LIBARIA2)
endif()

add_executable(eru main.cpp)
target_link_libraries(eru PRIVATE downloader CLI11::CLI11 indicators::indicators)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ERU_STATIC_RUNTIME)
    target_link_options(eru PRIVATE -static-libgcc -static-libstdc++)
endif()
