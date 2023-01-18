cmake_minimum_required(VERSION 3.24)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)



include(FetchContent)
FetchContent_Declare(_project_options URL https://github.com/aminya/project_options/archive/refs/tags/v0.24.0.zip)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)

project(zip_split LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
project_options(
        ENABLE_CACHE
        #ENABLE_CPPCHECK
        #ENABLE_CLANG_TIDY
        # ENABLE_CONAN
        ENABLE_INTERPROCEDURAL_OPTIMIZATION
        ENABLE_NATIVE_OPTIMIZATION
        # ENABLE_DOXYGEN
        # ENABLE_COVERAGE
        #ENABLE_SANITIZER_ADDRESS
        #ENABLE_SANITIZER_LEAK
        #ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
        #ENABLE_SANITIZER_THREAD
        #ENABLE_SANITIZER_MEMORY
        #ENABLE_PCH
        #PCH_HEADERS
        # WARNINGS_AS_ERRORS
        #ENABLE_INCLUDE_WHAT_YOU_USE
        # ENABLE_USER_LINKER
        #ENABLE_BUILD_WITH_TIME_TRACE
        # ENABLE_UNITY
        # CONAN_OPTIONS
)
find_package(Poco CONFIG REQUIRED COMPONENTS Zip)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(indicators CONFIG REQUIRED)
find_package(fmt REQUIRED)
find_package(LibArchive REQUIRED)
#add_subdirectory(bit7z)
#add_subdirectory(7zip-cpp)
add_subdirectory(moor)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${LibArchive_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE moor/moor)

target_link_libraries(${PROJECT_NAME} PRIVATE project_options project_warnings
        PUBLIC Poco::Zip
        ${LibArchive_LIBRARIES}
        moor
        fmt::fmt
        Boost::program_options
        TBB::tbb
        indicators::indicators)

#target_include_directories(${PROJECT_NAME} PUBLIC "7zip-cpp/Include" )
#target_include_directories(${PROJECT_NAME} PUBLIC "bit7z/include" )