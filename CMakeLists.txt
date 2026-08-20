cmake_minimum_required(VERSION 3.16)
project(file_downloader VERSION 1.0)

# >>> nodus:begin
# Managed by Nodus. Edit dependencies with `nodus add` and `nodus remove`.
set(EXTRACTED_CPM_VERSION "0.43.1")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/nodus/CPM.cmake")
CPMUsePackageLock("${CMAKE_CURRENT_LIST_DIR}/cmake/nodus/package-lock.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/nodus/dependencies.cmake")
# <<< nodus:end

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

if(WIN32)
  set(ERU_EMBED_ARIA2_DEFAULT OFF)
else()
  set(ERU_EMBED_ARIA2_DEFAULT ON)
endif()
option(ERU_EMBED_ARIA2 "Embed libaria2 for in-process torrent downloads" ${ERU_EMBED_ARIA2_DEFAULT})
option(ERU_STATIC_RUNTIME "Statically link the GNU C/C++ runtime" ON)

if(ERU_EMBED_ARIA2)
  include(ExternalProject)
  find_package(OpenSSL REQUIRED)
  find_package(ZLIB REQUIRED)
  find_package(Threads REQUIRED)

  set(ARIA2_INSTALL_DIR ${CMAKE_BINARY_DIR}/aria2-install)
  file(MAKE_DIRECTORY ${ARIA2_INSTALL_DIR}/include)

  ExternalProject_Add(aria2_external
    GIT_REPOSITORY https://github.com/aria2/aria2.git
    GIT_TAG release-1.37.0
    GIT_SHALLOW TRUE
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND autoreconf -i
      COMMAND <SOURCE_DIR>/configure --prefix=${ARIA2_INSTALL_DIR}
        --enable-libaria2 --enable-static --disable-shared
        --without-gnutls --without-libnettle --without-libgmp --without-libgcrypt
        --without-libxml2 --without-libexpat
        --without-libcares --without-sqlite3 --without-libssh2
        --disable-nls
    BUILD_COMMAND make -j2
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS ${ARIA2_INSTALL_DIR}/lib/libaria2.a
  )

  add_library(libaria2 STATIC IMPORTED GLOBAL)
  set_target_properties(libaria2 PROPERTIES
    IMPORTED_LOCATION ${ARIA2_INSTALL_DIR}/lib/libaria2.a
    INTERFACE_INCLUDE_DIRECTORIES ${ARIA2_INSTALL_DIR}/include)
  set_property(TARGET libaria2 PROPERTY INTERFACE_LINK_LIBRARIES
    OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB Threads::Threads ${CMAKE_DL_LIBS})
endif()

add_subdirectory(src)
