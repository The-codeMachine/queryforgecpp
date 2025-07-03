cmake_minimum_required(VERSION 3.15)
project(QueryForge++ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build shared or static library
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Define source files
set(SOURCES
    src/QueryPool.cpp
    src/MySQLConnection.cpp
    # add other source files here
)

set(HEADERS
    include/QueryPool.h
    include/IDatabaseConnection.h
    include/MySQLConnection.h
    include/ILogger.h
    # add other headers here
)

# Add library target
add_library(QueryForge ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(QueryForge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Find MySQL Connector/C++ (adjust as needed for your system)
find_path(MYSQL_INCLUDE_DIR NAMES mysql_driver.h PATHS /usr/include /usr/local/include /opt/homebrew/include)
find_library(MYSQL_LIB NAMES mysqlcppconn PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)

if(NOT MYSQL_INCLUDE_DIR OR NOT MYSQL_LIB)
    message(FATAL_ERROR "MySQL Connector/C++ not found. Please install it.")
endif()

target_include_directories(QueryForge PRIVATE ${MYSQL_INCLUDE_DIR})
target_link_libraries(QueryForge PRIVATE ${MYSQL_LIB})

# Optional: Enable warnings
target_compile_options(QueryForge PRIVATE -Wall -Wextra -Wpedantic)

# Example executable
add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE QueryForge)

# Install rules
include(GNUInstallDirs)
install(TARGETS QueryForge
    EXPORT QueryForgeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # for Windows DLLs
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT QueryForgeTargets
    FILE QueryForgeTargets.cmake
    NAMESPACE QueryForge::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QueryForge
)

# Package config (optional)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QueryForgeConfigVersion.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QueryForgeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QueryForgeConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QueryForge
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QueryForgeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QueryForgeConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QueryForge
)