# Easy-BigInteger Library

Easy-BigInteger is a C++ library designed for handling large integer arithmetic, optimized for cryptography and mathematical applications.
It is a CMake project compatible with both MSVC (Microsoft Visual C++) and GCC/G++ compilers.

## Overview

Easy-BigInteger provides an extensive set of operations for big integers, including but not limited to:

- Basic arithmetic: addition, subtraction, multiplication, division.
- Bitwise operations: AND, OR, NOT, XOR, shifts.
- Modular arithmetic.
- Efficient exponentiation and square root calculations.
- Data import/export functionalities.

## Getting Started

To integrate Easy-BigInteger into your project, include the `BigInteger.hpp` file in your C++ source code.

### Building with CMake

Easy-BigInteger uses CMake to manage the build process. To build the library and run tests, follow these steps:

1. Clone the repository:
   ```
   git clone https://github.com/Twilight-Dream-Of-Magic/Easy-BigInteger.git
   cd Easy-BigInteger
   ```
2. Create a build directory and run CMake:
   ```
   mkdir build
   cd build
   cmake ..
   ```
3. Compile the project:
   ```
   cmake --build .
   ```

### Example Usage

Here's a quick example to demonstrate the initialization and addition operation:

```cpp
#include "BigInteger.hpp"

int main()
{
	using PrimeNumberTester = TwilightDream::PrimeNumberTester;
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	// Initialize large integers with strings of a particular binary
	// 使用特定进制的字符串初始化大整数
	BigInteger bigInt1("1234567890123456789012345678901234567890", 10);
	BigInteger bigInt2("9876543210987654321098765432109876543210", 10);

	// Undefined Behavior: String data does not match the binary parameter to which it belongs.
	// 未定义行为: 字符串数据和所属进制参数不匹配。
	// BigInteger bigInt3("1234567890ABCDEF1234567890ABCDEF", 10);

	// Prints a binary representation of the original large integer, with each block of bits separated by a space
	// 打印原始大整数的二进制表示，每个比特块之间用空格分隔
	std::cout << "Binary representation of bigInt1 (spaced):\n";
	bigInt1.PrintBinary(true);

	// Prints a binary representation of the original large integer, with no spaces separating each block of bits.
	// 打印原始大整数的二进制表示，每个比特块之间不用空格分隔
	std::cout << "Binary representation of bigInt2 (unspaced):\n";
	bigInt2.PrintBinary(false);

	// Bit shift operations: left and right shifts
	// 比特位移操作：左移和右移
	BigInteger shiftedLeft = bigInt1 << 10; // 左移10位
	BigInteger shiftedRight = bigInt2 >> 10; // 右移10位

	// Bitwise operations: AND, OR, NOT, XOR
	// 比特位运算：AND, OR, NOT, XOR
	BigInteger andResult = bigInt1 & bigInt2;
	BigInteger orResult = bigInt1 | bigInt2;
	BigInteger notResult = ~bigInt1;
	BigInteger xorResult = bigInt1 ^ bigInt2;

	// Arithmetic operations: addition
	// 算术操作：加法
	BigInteger sum = bigInt1 + bigInt2;

	// Arithmetic operations: subtraction
	// 算术操作：减法
	BigInteger difference = bigInt1 - bigInt2;

	// Arithmetic operations: multiplication
	// 算术操作：乘法
	BigInteger product = bigInt1 * bigInt2;

	// Arithmetic operations: division and balance
	// 算术操作：除法和取余
	BigInteger quotient = bigInt1 / bigInt2;
	BigInteger remainder = bigInt1 % bigInt2;

	// Converts large integers to a string representation in a different hexadecimal system.
	// 将大整数转换为不同进制的字符串表示
	std::cout << "bigInt1 in base 2: " << bigInt1.ToString(2) << std::endl;
	std::cout << "bigInt2 in base 16: " << bigInt2.ToString(16) << std::endl;

	// Print results
	// 打印结果
	std::cout << "Sum: " << sum.ToString() << std::endl;
	std::cout << "Difference: " << difference.ToString() << std::endl;
	std::cout << "Product: " << product.ToString() << std::endl;
	std::cout << "Quotient: " << quotient.ToString() << std::endl;
	std::cout << "Remainder: " << remainder.ToString() << std::endl;

	return 0;
}
```

If you need to rotate or what is called circularly shift bits correctly, use our API.
Don't try this bits rotation algorithm on your own, as we consider special cases.

```cpp
/**
	* @brief Rotates the bits of a BigInteger to the left by a specified number of positions.
	*
	* @param bits The BigInteger to be rotated.
	* @param shift The number of positions to rotate the bits to the left.
	* @param reference_bit_capacity The bit capacity to reference for the rotation (default is 0).
	* @return A new BigInteger that represents the result of the left bit rotation.
	*
	* This method performs a left bitwise rotation on the given BigInteger. The rotation
	* effectively shifts the bits to the left by the specified number of positions, with the bits
	* that overflow on the left being placed back on the right.
	* 
	* The rotation is performed with respect to a reference bit capacity:
	* - If the reference bit capacity is greater than or equal to the actual bit length of the input,
	*   the rotation is straightforward, and the leading bits are squeezed to fit the reference capacity.
	* - If the reference bit capacity is less than the actual bit length, the rotation is split into two
	*   parts: the part to be rotated and the steady part that remains unchanged. The rotated part is
	*   processed and combined with the steady part to form the final result.
	*
	* @example
	* BigInteger num("1001", 2); // Binary representation: 1001
	* BigInteger result = BigInteger::BitRotateLeft(num, 2, 4); // Result: 0110 (binary)
	*/
	static BigInteger BitRotateLeft( const BigInteger& bits, size_t shift, size_t reference_bit_capacity );

/**
	* @brief Rotates the bits of a BigInteger to the right by a specified number of positions.
	*
	* @param bits The BigInteger to be rotated.
	* @param shift The number of positions to rotate the bits to the right.
	* @param reference_bit_capacity The bit capacity to reference for the rotation (default is 0).
	* @return A new BigInteger that represents the result of the right bit rotation.
	*
	* This method performs a right bitwise rotation on the given BigInteger. The rotation
	* effectively shifts the bits to the right by the specified number of positions, with the bits
	* that overflow on the right being placed back on the left.
	* 
	* The rotation is performed with respect to a reference bit capacity:
	* - If the reference bit capacity is greater than or equal to the actual bit length of the input,
	*   the rotation is straightforward, and the leading bits are squeezed to fit the reference capacity.
	* - If the reference bit capacity is less than the actual bit length, the rotation is split into two
	*   parts: the part to be rotated and the steady part that remains unchanged. The rotated part is
	*   processed and combined with the steady part to form the final result.
	*
	* @example
	* BigInteger num("1001", 2); // Binary representation: 1001
	* BigInteger result = BigInteger::BitRotateRight(num, 2, 4); // Result: 0110 (binary)
	*/
	static BigInteger BitRotateRight( const BigInteger& bits, size_t shift, size_t reference_bit_capacity );
```

## Test Cases

Test cases for the Easy-BigInteger library are located in the root directory of the repository in the files `BigIntegerTest.hpp` and `BigIntegerTest.cpp`. 
These test cases provide a comprehensive suite to verify the functionality and performance of the library.

1. **Binary Input/Output**: Tests initialization from and conversion to binary strings.
2. **Shift Operations**: Validates left and right bit shifts.
3. **Squeeze Leading Bits**: Checks the removal of leading zeros.
4. **Bitwise Operations**: Assesses AND, OR, NOT, and XOR operations on big integers.
5. **Rotate Shift**: Examines rotate left and right operations on bit levels.
6. **String Input/Output**: Evaluates initialization from and conversion to strings of various numerical bases.
7. **Addition and Subtraction**: Tests basic arithmetic operations.
8. **Multiplication**: Measures performance of multiplication and verifies results.
9. **Reciprocal Newton**: Tests Newton's method for reciprocal calculation.
10. **Division**: Checks division and modulo operations, ensuring accuracy.
11. **Extended GCD**: Applies the extended Euclidean algorithm for GCD and coefficient calculation.
12. **Modular Inverse**: Determines the modular inverse of a number.
13. **Montgomery Reduction**: Tests the Montgomery algorithm for efficient modular exponentiation.
14. **Power with Modulo**: Calculates powers under a modulo.
15. **Byte Export/Import**: Tests byte data handling with import and export functions.
16. **Prime Number Testing**: Utilizes the Miller-Rabin test for prime number generation.

## Contributing

We welcome contributions to the Easy-BigInteger library. 
Please feel free to submit Pull Requests or create Issues to help us improve.

## License

Easy-BigInteger is released under the [MIT License](LICENSE).

## Acknowledgements

While the Easy-BigInteger library has been significantly restructured and can be considered a new project, it retains a repository connection to its predecessor. 
We respect and acknowledge the original author's contributions and labor in the development of this project. 
The current project stands as a testament to the evolution and continued development inspired by the initial work.

The original project can be found at [NoahBz/Easy-BigInteger](https://github.com/NoahBz/Easy-BigInt).
