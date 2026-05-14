cmake_minimum_required(VERSION 3.12)
project(vpi VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(VPI_STATIC "Build with static linking" OFF)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
set(VPI_SOURCES
    src/main.c
    src/vpi.c
    src/utils.c
    src/http.c
    src/zip.c
    src/registry.c
    src/commands.c
    src/cmd_init.c
    src/miniz.c
)
add_executable(vpi ${VPI_SOURCES})
target_include_directories(vpi PRIVATE src ${CURL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(vpi PRIVATE ${CURL_LIBRARIES} ${ZLIB_LIBRARIES})
if(WIN32)
    target_link_libraries(vpi PRIVATE shell32 ole32)
    if(VPI_STATIC)
        target_link_options(vpi PRIVATE -static)
    endif()
elseif(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    if(COREFOUNDATION_LIBRARY)
        target_link_libraries(vpi PRIVATE ${COREFOUNDATION_LIBRARY})
    endif()
    if(VPI_STATIC)
        target_link_options(vpi PRIVATE -static-libgcc)
    endif()
else()
    target_link_libraries(vpi PRIVATE m)
    if(VPI_STATIC)
        target_link_options(vpi PRIVATE -static)
    endif()
endif()
if(MSVC)
    target_compile_options(vpi PRIVATE /W3 /WX- /wd4996)
    target_compile_definitions(vpi PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(vpi PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
target_compile_definitions(vpi PRIVATE
    VPI_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    VPI_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    VPI_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
install(TARGETS vpi
    RUNTIME DESTINATION bin
)
if(UNIX)
    configure_file(${CMAKE_SOURCE_DIR}/scripts/vpi.1.in ${CMAKE_BINARY_DIR}/vpi.1 @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/vpi.1 DESTINATION share/man/man1)
endif()
