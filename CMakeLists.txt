# CMakeList.txt: LargeDynamicBitSetAndIntegerNumber 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

#set(CMAKE_BUILD_TYPE release)

project ("LargeDynamicBitSetAndIntegerNumber")

# 设置默认的构建类型为 Debug，如果没有指定构建类型
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
	set(CMAKE_BUILD_TYPE debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# 设置 Debug 和 Release 的编译选项

if(MSVC)
	add_compile_options(/W4 /Zc:__cplusplus /utf-8)
else()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11")
			message(FATAL_ERROR "GNU CXX compiler version is too small !")
		endif()
		if(CMAKE_BUILD_TYPE EQUAL release)
			add_compile_options(-Wall -Wextra -fsigned-char -finput-charset=UTF-8 -fexec-charset=UTF-8)
		else()
			add_compile_options(-g -Wall -Wextra -fsigned-char -finput-charset=UTF-8 -fexec-charset=UTF-8)
		endif()
	endif()
endif()

add_library(BigNumberSystem
	BigInteger.hpp
	BigInteger.cpp
	BigFraction.hpp
	BigFraction.cpp
	PrimeNumberTester.hpp
	PrimeNumberTester.cpp
)

add_library(CryptographySymmetricKey
	BinaryCipherTest.hpp
	BinaryCipherTest.cpp
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
target_link_libraries(TestCryptography PRIVATE CryptographySymmetricKey CryptographyAsymmetricKey)

if (CMAKE_VERSION VERSION_GREATER 3.12)
	set_property(TARGET BigNumberSystem PROPERTY CXX_STANDARD 17)
    set_property(TARGET CryptographySymmetricKey PROPERTY CXX_STANDARD 17)
    set_property(TARGET CryptographyAsymmetricKey PROPERTY CXX_STANDARD 17)
    set_property(TARGET TestCryptography PROPERTY CXX_STANDARD 17)
    set_property(TARGET BigNumberSystem PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET CryptographySymmetricKey PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET CryptographyAsymmetricKey PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET TestCryptography PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

# TODO: 如有需要，请添加测试并安装目标。
