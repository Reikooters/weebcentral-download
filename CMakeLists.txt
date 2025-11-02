cmake_minimum_required(VERSION 4.0)
project(weebcentral-download)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include FetchContent module
include(FetchContent)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch lexbor from GitHub
FetchContent_Declare(
        lexbor
        GIT_REPOSITORY https://github.com/lexbor/lexbor.git
        GIT_TAG v2.5.0
)

# Make available with error checking
FetchContent_MakeAvailable(lexbor)

# Check if lexbor was successfully fetched
if(NOT lexbor_POPULATED)
    message(FATAL_ERROR "Failed to fetch lexbor from GitHub. Please check your internet connection and try again.")
endif()

# Verify lexbor_static target exists
if(NOT TARGET lexbor_static)
    message(FATAL_ERROR "lexbor_static target not found. The lexbor library may not have been built correctly.")
endif()

# Handle CURL: Use FetchContent on Windows or if system CURL not found
if(WIN32)
    message(STATUS "Windows detected, using FetchContent for libcurl...")
    set(USE_FETCHCONTENT_CURL TRUE)
else()
    # Try to find system curl on non-Windows platforms
    find_package(CURL QUIET)

    if(CURL_FOUND)
        message(STATUS "Using system libcurl")
        set(USE_FETCHCONTENT_CURL FALSE)
    else()
        message(STATUS "System libcurl not found, fetching from source...")
        set(USE_FETCHCONTENT_CURL TRUE)
    endif()
endif()

# Fetch CURL if needed
if(USE_FETCHCONTENT_CURL)
    FetchContent_Declare(
            curl
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG curl-8_16_0
    )

    # Configure curl build options
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_TESTS ON CACHE BOOL "" FORCE)
    set(HTTP_ONLY ON CACHE BOOL "" FORCE)  # Only HTTP/HTTPS support, reduces dependencies
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)  # Disable libpsl dependency

    # Use Schannel on Windows, OpenSSL on other platforms
    if(WIN32)
        set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
        set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    else()
        set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    endif()

    # Make available with error checking
    FetchContent_MakeAvailable(curl)

    # Check if curl was successfully fetched
    if(NOT curl_POPULATED)
        message(FATAL_ERROR "Failed to fetch libcurl from GitHub. Please check your internet connection and try again.")
    endif()

    # Verify libcurl target exists
    if(NOT TARGET libcurl)
        message(FATAL_ERROR "libcurl target not found. The curl library may not have been built correctly.")
    endif()
endif()

# Define the executable
add_executable(weebcentral-download
        Utils.cpp
        Utils.h
        main.cpp
        HttpClient.cpp
        HttpClient.h
        models/Chapter.h
)

# Add static linking flags for Windows + MinGW
if(WIN32 AND MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# Link lexbor to executable
target_link_libraries(weebcentral-download PRIVATE lexbor_static)

# Include lexbor headers
target_include_directories(weebcentral-download PRIVATE
        ${lexbor_SOURCE_DIR}/source
)

# Link CURL (system or fetched)
if(USE_FETCHCONTENT_CURL)
    target_link_libraries(weebcentral-download PRIVATE libcurl)
else()
    target_link_libraries(weebcentral-download PRIVATE CURL::libcurl)
    target_include_directories(weebcentral-download PRIVATE ${CURL_INCLUDE_DIRS})
endif()
