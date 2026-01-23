# CMakeLists.txt: LargeDynamicBitSetAndIntegerNumber 的 CMake 项目，
# 在此处包括源代码并定义项目特定的逻辑。

cmake_minimum_required(VERSION 3.8)

project("LargeDynamicBitSetAndIntegerNumber" LANGUAGES CXX)

# ============================================================
# 可配置的 C++ 标准
#
# 默认使用 C++17。
# 可在 CMake GUI、Visual Studio 的 CMake 缓存变量或命令行中覆盖：
#   -DCMAKE_CXX_STANDARD=20
# ============================================================
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard (17, 20 or 23)")
set_property(CACHE CMAKE_CXX_STANDARD PROPERTY STRINGS 17 20 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# 默认构建类型
#
# Ninja / Unix Makefiles 等单配置生成器使用 CMAKE_BUILD_TYPE。
# Visual Studio 是多配置生成器，会在 VS 顶部选择 Debug / Release，
# 因此不能在 CMakeLists.txt 中用 CMAKE_BUILD_TYPE 强行替它选择。
# ============================================================
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build (Debug, Release, RelWithDebInfo or MinSizeRel)"
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# ============================================================
# 编译选项
# CMake / Visual Studio 会自动给 Debug 和 Release 添加各自的
# /Od、/O2、调试信息和 NDEBUG 等标准配置，不需要手动重复设置。
# ============================================================
if(MSVC)
    add_compile_options(
        /W4
        /Zc:__cplusplus
        /utf-8
    )
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11")
            message(FATAL_ERROR "GNU CXX compiler version is too small!")
        endif()

        add_compile_options(
            -Wall
            -Wextra
            -fsigned-char
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
            $<$<CONFIG:Debug>:-O0>
            $<$<CONFIG:Debug>:-g>
            $<$<CONFIG:Release>:-O2>
        )
    endif()
endif()

add_library(BigNumberSystem
    BigInteger.hpp
    BigInteger.cpp
    BigFraction.hpp
    BigFraction.cpp
    PrimeNumberTester.hpp
    PrimeNumberTester.cpp
    Lenstra-Pomerance_AKS.hpp
    Lenstra-Pomerance_AKS.cpp
    SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.hpp
    SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.cpp
)

add_library(CryptographySymmetricKey
    BinaryCipherTest.hpp
    BinaryCipherTest.cpp
    HardPoly1305.hpp
    HardPoly1305.cpp
)
target_link_libraries(CryptographySymmetricKey PRIVATE BigNumberSystem)

add_library(CryptographyAsymmetricKey
    CryptographyAsymmetricKey.hpp
    CryptographyAsymmetricKey.cpp
)
target_link_libraries(CryptographyAsymmetricKey PRIVATE BigNumberSystem)

add_executable(TestCryptography
    BigIntegerTest.hpp
    BigIntegerTest.cpp
    main.cpp
)
target_link_libraries(TestCryptography
    PRIVATE
        CryptographySymmetricKey
        CryptographyAsymmetricKey
)

# 在配置阶段打印当前设置，方便确认。
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Generator configurations: ${CMAKE_CONFIGURATION_TYPES}")
    message(STATUS "Select Debug or Release from Visual Studio's configuration dropdown.")
else()
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

# TODO: 如有需要，请添加测试并安装目标。
