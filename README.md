# weebcentral-download

A simple C++ application for downloading manga content from [Weeb Central](https://weebcentral.com/).

Given the URI to the series page you are interested in, it will create a sub-directory under the working directory with the name of the series, then create sub-directories under that for each chapter, where the images for each chapter will be downloaded into each chapter's directory.

The application simply downloads the images in serial (no parallelization/multi-threading) and sleeps for 4 seconds between chapters to reduce load on weebcentral.com servers.

If a directory with the chapter name already exists, it will be skipped. This means you can run the tool again to download newly released chapters. If a chapter was not fully downloaded, for example, because you exited the application while it was running, then you should delete or rename the incomplete chapter's directory so that it can be downloaded by the application again.

## Usage:

```bash
./weebcentral-download <manga_uri>
```

Example:

```bash
# with series title in URI
./weebcentral-download https://weebcentral.com/series/01J76XYFCDK6Y8GY447DTTTZ2F/The-Girl-in-the-Arcade

# or without
./weebcentral-download https://weebcentral.com/series/01J76XYFCDK6Y8GY447DTTTZ2F
```

# Similar projects

- [weebcentral-dl](https://github.com/axsddlr/weebcentral-dl)

The above project contains a lot more features and configuration. I created this project for my own use since I wanted something that just works on its own, as I can't really be bothered with Python or Docker. I'm also trying to get back into C++ after having not used it in 13 or so years, and thought this would be a good little exercise.

# Building

I'm personally using JetBrains CLion 2025.2.4 on Arch Linux, which just requires opening the project folder and clicking the build icon in the top right.

Below instructions are AI generated using Claude 4.5 Sonnet for anyone else.

## Prerequisites

Before building this project, you need to install the following tools and dependencies:

### All Platforms

- **CMake** (version 4.0 or higher)
- **C++ Compiler** with C++20 support
- **Git** (for cloning the repository and fetching dependencies)
- **OpenSSL** (required by curl for HTTPS support)

### Linux

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install cmake g++ git libssl-dev build-essential
```

#### Fedora/RHEL
```bash
sudo dnf install cmake gcc-c++ git openssl-devel
```

#### Arch Linux
```bash
sudo pacman -S cmake gcc git openssl
```

### macOS

Install Xcode Command Line Tools and Homebrew, then:
```bash
xcode-select --install
brew install cmake openssl git
```

### Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community Edition is free)
   - During installation, select "Desktop development with C++"
2. Install [CMake](https://cmake.org/download/) (3.20 or higher recommended)
3. Install [Git](https://git-scm.com/download/win)
4. **OpenSSL** will be automatically handled by curl's build system on Windows

## Building from Source

### Step 1: Clone the Repository

```bash
git clone https://github.com/Reikooters/weebcentral-download.git
cd weebcentral-download
```

### Step 2: Configure and Build

The project uses CMake's FetchContent feature to automatically download and build dependencies (lexbor and curl), so you don't need to manually install them.

#### Linux & macOS

```bash
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project (use -j to speed up compilation with multiple cores)
cmake --build . -j$(nproc)

# The executable will be in the build directory
./weebcentral-download
```

#### Windows (Command Prompt)

```cmd
# Create a build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
cmake --build . --config Release

# The executable will be in build\Release\
Release\weebcentral-download.exe
```

#### Windows (Using Visual Studio)

Alternatively, you can open the project directly in Visual Studio 2022:
1. Open Visual Studio
2. Select "Open a local folder"
3. Navigate to the `weebcentral-download` folder
4. Visual Studio will automatically configure CMake
5. Click the build button or press F7

### Step 3: Run the Application

```bash
# Linux/macOS
./weebcentral-download

# Windows
weebcentral-download.exe
```

## Build Options

### Release Build (Optimized)

For better performance, build in Release mode:

```bash
# Linux/macOS
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Windows
cmake --build . --config Release
```

### Debug Build

For development with debugging symbols:

```bash
# Linux/macOS
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Windows
cmake --build . --config Debug
```

## Troubleshooting

### Build fails with "C++20 not supported"

Make sure you have a recent compiler:
- GCC 12+ 
- Clang 15+
- MSVC 2022 (Latest version of Visual Studio 17)

### OpenSSL not found (Linux/macOS)

If curl fails to build due to missing OpenSSL:

```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
# Then configure with:
cmake -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl ..
```

### Internet connection required during first build

The first time you build, CMake will download lexbor and curl from GitHub. This requires an active internet connection. After the first successful build, these dependencies are cached.

### Build takes a long time

The first build compiles both lexbor and curl from source, which can take several minutes. Subsequent builds will be much faster as dependencies are already built.

## Dependencies

This project automatically fetches and builds the following dependencies:

- **[lexbor](https://github.com/lexbor/lexbor)** (v2.5.0) - HTML parser
- **[curl](https://github.com/curl/curl)** (8.16.0) - HTTP client library

On Linux and macOS, the build system will use the system-installed libcurl if available. On Windows, curl is always built from source.

## License

[MIT License](LICENSE)
