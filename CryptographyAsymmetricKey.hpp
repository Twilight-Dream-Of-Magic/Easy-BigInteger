/*
MIT License

Copyright (c) 2024-2050 Twilight-Dream & With-Sky

https://github.com/Twilight-Dream-Of-Magic/
https://github.com/With-Sky

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TWILIGHT_DREAM_CRYPTOGRAPHY_ASYMMETRIC_HPP
#define TWILIGHT_DREAM_CRYPTOGRAPHY_ASYMMETRIC_HPP


#include "PrimeNumberTester.hpp"
#include "FiniteField.hpp"
#include <cstddef>
#include <future>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

/*
	Twilight-Dream
	Cryptography Asymmetric Key Algorithms
*/


namespace TwilightDream::CryptographyAsymmetric
{
	// Optimal asymmetric encryption padding
	struct OAEP
	{
	};

	// Rivest, Adi Shamir, and Leonard Adleman
	class RSA
	{
	private:
		using BigInteger = TwilightDream::BigInteger::BigInteger;
		using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;
		using PrimeNumberTester = TwilightDream::PrimeNumberTester;
		const BigInteger ONE = BigInteger( 1 );
		const BigInteger TWO = BigInteger( 2 );
		const BigInteger THREE = BigInteger( 3 );

		struct RSA_AlgorithmNumbers
		{
			using BigInteger = TwilightDream::BigInteger::BigInteger;

			BigInteger EncryptExponent = 0;
			BigInteger DecryptExponent = 0;
			BigInteger AlgorithmModulus = 0;
		};

		size_t bit_count = 0;
		BigInteger MIN = 0;
		BigInteger MAX = 0;
		BigInteger RANGE_INTEGER = 0;
		PrimeNumberTester Tester;

		struct FindPrimeState
		{
			bool IsPrime = false;
			BigInteger Number = BigInteger( 0 );
		};

		void GeneratePrimeNumbers(std::vector<FindPrimeState>& primes, size_t primes_index);
		std::optional<FindPrimeState> GeneratePrimesInParallelFunctions();
		BigInteger GeneratePrimeNumber( size_t bit_count );

	public:
		/**
		* @brief Generates RSA key pair (public and private keys).
		*
		* @param bit_count The bit length of the RSA modulus.
		* @param is_pkcs A flag indicating whether to use PKCS#1 standard for key generation.
		* @return RSA_AlgorithmNumbers structure containing the generated key pair.
		*/
		RSA_AlgorithmNumbers GenerateKeys( size_t bit_count, bool is_pkcs );

		/**
		* @brief Encrypts a message using RSA.
		*
		* @param PlainMessage The message to be encrypted.
		* @param EncryptExponent The RSA public key exponent for encryption.
		* @param AlgorithmModulus The RSA modulus used for encryption.
		*/
		void Encryption( BigInteger& PlainMessage, const BigInteger& EncryptExponent, const BigInteger& AlgorithmModulus );

		/**
		* @brief Decrypts a message using RSA.
		*
		* @param CipherMessage The encrypted message to be decrypted.
		* @param DecryptExponent The RSA private key exponent for decryption.
		* @param AlgorithmModulus The RSA modulus used for decryption.
		*/
		void Decryption( BigInteger& CipherMessage, const BigInteger& DecryptExponent, const BigInteger& AlgorithmModulus );

		/**
		* @brief Performs self-sanity check for RSA algorithm.
		*
		* Generates multiple key pairs and tests encryption/decryption consistency.
		*
		* @param bit_count The bit length of the RSA modulus.
		* @param test_count The number of iterations for primality testing during key generation.
		* @param rounds The number of iterations for the self-sanity check.
		* @return True if the self-sanity check all executed, false otherwise.
		*/
		static bool SelfSanityCheck( size_t bit_count, size_t rounds )
		{
			if(rounds == 0)
			{
				return false;
			}

			RSA		   rsa;
			BigInteger OriginalMessage = 0;
			BigInteger EncryptedMessage = 0, DecryptedMessage = 0;
			
			size_t FailureCounter = 0;
			size_t SuccessCounter = 0;

			for ( size_t current_round = 0; current_round < rounds; current_round++ )
			{
				// 生成密钥对
				auto keys = rsa.GenerateKeys( bit_count, false );

				// 加解密测试
				OriginalMessage = BigInteger::RandomGenerateNBit( bit_count / 2 );
				
				EncryptedMessage = OriginalMessage;
				rsa.Encryption( EncryptedMessage, keys.EncryptExponent, keys.AlgorithmModulus );
				
				DecryptedMessage = EncryptedMessage;
				rsa.Decryption( DecryptedMessage, keys.DecryptExponent, keys.AlgorithmModulus );

				// 验证结果
				if ( OriginalMessage != DecryptedMessage )
				{
					std::cerr << "Failure: RSA encryption and decryption are not a pair of mutually inverse functions; it may be that one of the two large numbers is not prime." << "\n";
					
					if(rounds < 2)
					{
						return false;
					}
					else
					{
						++FailureCounter;
					}
				}
				else
				{
					std::cout << "Success: the RSA encryption and decryption are a pair of mutually inverse functions that pass this round of testing." << "\n";
					++SuccessCounter;
				}
			}

			std::cout << "Self sanity check all executed." << std::endl;
			
			//Print the probability of success or failure for this round of testing (%)
			std::cout << "The probability of success is " << SuccessCounter * 100.0 / rounds << "%." << std::endl;
			std::cout << "The probability of failure is " << FailureCounter * 100.0 / rounds << "%." << std::endl;

			return true;
		}
	};
	
	class FastRSA
	{
	private:
		using BigInteger = TwilightDream::BigInteger::BigInteger;
		using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;
		using PrimeNumberTester = TwilightDream::PrimeNumberTester;

		const BigInteger ONE = BigInteger(1);
		const BigInteger TWO = BigInteger(2);
		const BigInteger THREE = BigInteger(3);

	public:
		struct KeyPair
		{
			BigInteger PublicExponent = 0;   // e
			BigInteger PrivateExponent = 0;  // d
			BigInteger Modulus = 0;          // n = p * q

			// CRT acceleration fields
			BigInteger PrimeP = 0;           // p
			BigInteger PrimeQ = 0;           // q
			BigInteger DP = 0;               // d mod (p - 1)
			BigInteger DQ = 0;               // d mod (q - 1)
			BigInteger QInverseModP = 0;     // q^{-1} mod p
		};

	private:
		size_t bit_count = 0;
		BigInteger MIN = 0;
		BigInteger MAX = 0;
		BigInteger RANGE_INTEGER = 0;
		PrimeNumberTester Tester;

		struct FindPrimeState
		{
			bool IsPrime = false;
			BigInteger Number = BigInteger(0);
		};

	private:
		void GeneratePrimeNumbers(std::vector<FindPrimeState>& primes, size_t primes_index);
		std::optional<FindPrimeState> GeneratePrimesInParallelFunctions();
		BigInteger GeneratePrimeNumber(size_t bit_count);

		static BigInteger NormalizeModulo(const BigSignedInteger& value, const BigInteger& modulus)
		{
			BigSignedInteger result = value % BigSignedInteger(modulus);
			if (result.IsNegative())
			{
				result += BigSignedInteger(modulus);
			}
			return static_cast<BigInteger>(result);
		}

		static BigInteger NonNegativeModuloDifference(
			const BigInteger& left,
			const BigInteger& right,
			const BigInteger& modulus)
		{
			// return (left - right) mod modulus, but always in [0, modulus-1]
			if (left >= right)
			{
				return left - right;
			}
			return modulus - (right - left);
		}

		static BigInteger ComputeQInverseModP(const BigInteger& q, const BigInteger& p)
		{
			BigSignedInteger inverse = BigSignedInteger::ModuloInverse(
				BigSignedInteger(q),
				BigSignedInteger(p)
			);
			return NormalizeModulo(inverse, p);
		}

	public:
		KeyPair GenerateKeys(size_t bit_count, bool use_pkcs_public_exponent = true);

		void Encrypt(BigInteger& message, const BigInteger& publicExponent, const BigInteger& modulus) const;

		void DecryptSlow(BigInteger& cipher, const BigInteger& privateExponent, const BigInteger& modulus) const;

		void DecryptCRT(BigInteger& cipher, const KeyPair& keyPair) const;

		bool ValidateCRTKeyPair(const KeyPair& keyPair) const;

		static bool SelfSanityCheck(size_t bit_count, size_t rounds)
		{
			if (rounds == 0)
			{
				return false;
			}

			FastRSA rsa;
			size_t successCounter = 0;
			size_t failureCounter = 0;

			for (size_t current_round = 0; current_round < rounds; ++current_round)
			{
				KeyPair keys = rsa.GenerateKeys(bit_count, true);

				BigInteger originalMessage = BigInteger::RandomGenerateNBit(bit_count / 2);
				if (originalMessage >= keys.Modulus)
				{
					originalMessage %= keys.Modulus;
				}

				BigInteger encryptedMessage = originalMessage;
				rsa.Encrypt(encryptedMessage, keys.PublicExponent, keys.Modulus);

				BigInteger decryptedSlow = encryptedMessage;
				rsa.DecryptSlow(decryptedSlow, keys.PrivateExponent, keys.Modulus);

				BigInteger decryptedFast = encryptedMessage;
				rsa.DecryptCRT(decryptedFast, keys);

				if (decryptedSlow != originalMessage || decryptedFast != originalMessage || decryptedSlow != decryptedFast)
				{
					std::cerr << "Failure: FastRSA self-test failed.\n";
					++failureCounter;
				}
				else
				{
					std::cout << "Success: FastRSA self-test passed.\n";
					++successCounter;
				}
			}

			std::cout << "FastRSA self sanity check all executed.\n";
			std::cout << "The probability of success is " << successCounter * 100.0 / rounds << "%.\n";
			std::cout << "The probability of failure is " << failureCounter * 100.0 / rounds << "%.\n";

			return failureCounter == 0;
		}
	};

	//China Shang Yong Mi Ma 2 (SM2), Elliptic Curve Cryptography (ECC)
	namespace SM2
	{
		using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;
		using SignedFiniteField = TwilightDream::Math::SignedFiniteField;
		using FiniteField = TwilightDream::Math::UnsignedFiniteField;
		
		// 继承自 FiniteField 的 EllipticCurvePrimeField 类
		class EllipticCurvePrimeField : public SignedFiniteField
		{
		public:

			// 定义 P256V1Field 的素数模数
			inline static const BigSignedInteger Prime = BigSignedInteger("FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF", 16);
			inline static const std::string PrimeString = "115792089210356248756420345214020892766250353991924191454421193933289684991999";

			// 默认构造函数
			EllipticCurvePrimeField()
				: SignedFiniteField(0, Prime)
			{
			}

			// 从 BigSignedInteger 构造
			EllipticCurvePrimeField(const BigSignedInteger& number)
				: SignedFiniteField(number, Prime)
			{
			}

			// 从字符串和进制构造
			EllipticCurvePrimeField(const std::string& number_string)
				: SignedFiniteField(number_string, 10, PrimeString, 10)
			{
			}

			// 从字符串和进制构造
			EllipticCurvePrimeField(const std::string& number_string, uint32_t base_number)
				: SignedFiniteField(number_string, base_number, PrimeString, 10)
			{
			}

			EllipticCurvePrimeField(const SignedFiniteField& number) 
				: SignedFiniteField(number)
			{
				
			}

			// 复制构造
			EllipticCurvePrimeField(const EllipticCurvePrimeField& other)
				: SignedFiniteField(other)
			{
			}

			// 移动构造
			EllipticCurvePrimeField(EllipticCurvePrimeField&& other) noexcept
				: SignedFiniteField(std::move(other))
			{
			}

			// 赋值运算符
			EllipticCurvePrimeField& operator=(const EllipticCurvePrimeField& other)
			{
				SignedFiniteField::operator=(other);
				return *this;
			}

			EllipticCurvePrimeField& operator=(EllipticCurvePrimeField&& other) noexcept
			{
				SignedFiniteField::operator=(std::move(other));
				return *this;
			}

			EllipticCurvePrimeField operator-() const
			{
				return (-signed_value) % prime;
			}

			operator FiniteField() const
			{
				return this->Convert();
			}

			EllipticCurvePrimeField operator+(const EllipticCurvePrimeField& other) const
			{
				return SignedFiniteField::operator+(other).GetValue();
			}

			EllipticCurvePrimeField operator-(const EllipticCurvePrimeField& other) const
			{
				return SignedFiniteField::operator-(other).GetValue();
			}

			EllipticCurvePrimeField operator*(const EllipticCurvePrimeField& other) const
			{
				return SignedFiniteField::operator*(other).GetValue();
			}

			EllipticCurvePrimeField operator/(const EllipticCurvePrimeField& other) const
			{
				return SignedFiniteField::operator/(other).GetValue();
			}

			// 运算符重载以返回子类类型
			EllipticCurvePrimeField& operator+=(const EllipticCurvePrimeField& other)
			{
				*this = *this + other;
				return *this;
			}

			EllipticCurvePrimeField& operator-=(const EllipticCurvePrimeField& other)
			{
				*this = *this - other;
				return *this;
			}

			EllipticCurvePrimeField& operator*=(const EllipticCurvePrimeField& other)
			{
				*this = *this * other;
				return *this;
			}

			EllipticCurvePrimeField& operator/=(const EllipticCurvePrimeField& other)
			{
				*this = *this / other;
				return *this;
			}
		};

		// P256V1曲线参数类（根据中国商用密码2 - SM2(ShangYongMiMa 2)标准定义）
		class EllipticCurve
		{
		public:
			using BigInteger = TwilightDream::BigInteger::BigInteger;

			// SM2P256V1 曲线参数
			inline static const BigSignedInteger a = BigSignedInteger("FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC", 16);
			inline static const BigSignedInteger b = BigSignedInteger("28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93", 16);

			// 曲线阶 n
			inline static const BigInteger n = BigInteger("FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123", 16);
			// 标量常数 h
			inline static const BigInteger h = BigInteger(1);
		};

		class EllipticCurvePoint
		{
			using BigInteger = TwilightDream::BigInteger::BigInteger;
			using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;
		private:
			EllipticCurvePrimeField x;
			EllipticCurvePrimeField y;
			EllipticCurvePrimeField z; // Jaccobian坐标 z==0 时是无穷远点
			mutable EllipticCurvePrimeField z_inverse; // z的逆元

			EllipticCurvePoint(const EllipticCurvePrimeField& x_value, const EllipticCurvePrimeField& y_value, const EllipticCurvePrimeField& z_value, bool is_affine_infinity)
				: x(x_value), y(y_value), z(z_value) // 构造Jaccobian坐标默认点 (0,0,h = 1)
			{
				
			}

			bool CheckCurveFormula() const;

		public:

			// 构造函数
			EllipticCurvePoint()
				: x("0", 10), y("0", 10), z("0", 10) // 默认构造为无效的无穷远点
			{}

			EllipticCurvePoint(const EllipticCurvePrimeField& x_value, const EllipticCurvePrimeField& y_value)
				: x(x_value), y(y_value), z(EllipticCurve::h) // 构造Affine坐标
			{

			}

			EllipticCurvePoint(const EllipticCurvePrimeField& x_value, const EllipticCurvePrimeField& y_value, const EllipticCurvePrimeField& z_value)
				: x(x_value), y(y_value), z(z_value) // 构造Jaccobian坐标 (0,0,h = 1)
			{
				if(z.GetValue().IsZero())
				{
					z = EllipticCurvePrimeField(EllipticCurve::h);
				}
			}

			// 复制构造
			EllipticCurvePoint(const EllipticCurvePoint& other)
				: x(other.x), y(other.y), z(other.z), z_inverse(other.z_inverse) {}

			// 移动构造
			EllipticCurvePoint(EllipticCurvePoint&& other) noexcept
				: x(std::move(other.x)), y(std::move(other.y)), z(std::move(other.z)), z_inverse(std::move(other.z_inverse)) {}

			// 赋值运算符
			EllipticCurvePoint& operator=(const EllipticCurvePoint& other)
			{
				if (this != &other)
				{
					if(!IsOnCurve(other))
					{
						this->x = EllipticCurvePrimeField("0", 10);
						this->y = EllipticCurvePrimeField("0", 10);
						this->z = EllipticCurvePrimeField("0", 10);
					}

					x = other.x;
					y = other.y;
					z = other.z;
					z_inverse = other.z_inverse;
				}
				return *this;
			}

			EllipticCurvePoint& operator=(EllipticCurvePoint&& other) noexcept
			{
				if (this != &other)
				{
					if(!IsOnCurve(other))
					{
						this->x = EllipticCurvePrimeField("0", 10);
						this->y = EllipticCurvePrimeField("0", 10);
						this->z = EllipticCurvePrimeField("0", 10);
					}

					x = std::move(other.x);
					y = std::move(other.y);
					z = std::move(other.z);
					z_inverse = std::move(other.z_inverse);
				}
				return *this;
			}

			// Affine Transform to Jaccobian
			EllipticCurvePoint ConvertAffineToJaccobian() const;

			/*
			* Jaccobian Transform to Affine
			* 
			* Note:
			* - This function may have potential issues, especially when handling edge cases like invalid or degenerate points.
			* - To ensure correctness and avoid unnecessary computations, it is highly recommended to use the `X()` and `Y()` 
			*   functions to retrieve the affine coordinates of a point instead of manually converting from Jacobian to affine form.
			* - Double-check the modular inverse calculation for numerical stability and correctness under all possible inputs.
			*/
			EllipticCurvePoint ConvertJaccobianToAffine() const;

			/**
			 * @brief Determines if the point is the point at infinity in Jacobian coordinates.
			 * 
			 * In elliptic curve cryptography, the point at infinity is a special point that serves as the 
			 * identity element for the group operation. In Jacobian (projective) coordinates, this point 
			 * is represented using specific conventions that allow it to be efficiently identified:
			 * 
			 * Algorithm:
			 * 1. Check if both the X and Y coordinates are zero:
			 *    - `(x == 0 && y == 0)`:
			 *      - This representation is commonly used for the point at infinity in some elliptic curve 
			 *        implementations.
			 * 2. Check if the Z-coordinate is zero while the Y-coordinate is non-zero:
			 *    - `(z == 0 && y != 0)`:
			 *      - In Jacobian coordinates, the point at infinity can also be represented by a zero Z-coordinate.
			 *      - The additional check for a non-zero Y-coordinate ensures the representation is consistent 
			 *        with elliptic curve properties and avoids ambiguity with other invalid points.
			 * 
			 * Explanation:
			 * - In projective (Jacobian) coordinates, a point (X, Y, Z) maps to affine coordinates (x, y) as:
			 *     - `x_affine = X / Z^2`
			 *     - `y_affine = Y / Z^3`
			 * - The point at infinity does not correspond to a finite affine coordinate and is represented in Jacobian 
			 *   coordinates with Z = 0 or with X = Y = 0.
			 * - By checking the conditions `(x == 0 && y == 0)` or `(z == 0 && y != 0)`, the function determines whether 
			 *   the point is at infinity based on these conventions.
			 * 
			 * Special Cases:
			 * - If both X and Y are zero, the point is explicitly treated as the point at infinity.
			 * - If Z is zero, the point is at infinity unless Y is also zero, in which case the point may be invalid.
			 * 
			 * Returns:
			 * - `true` if the point is the point at infinity in Jacobian coordinates.
			 * - `false` otherwise.
			 */
			bool IsInfinityPoint() const
			{
				// Check if X and Y are both zero
				if (x.GetValue().IsZero() && y.GetValue().IsZero())
				{
					return true;
				}

				// Check if Z is zero and Y is non-zero
				return z.GetValue().IsZero() && !y.GetValue().IsZero();
			}

			//The point obtained by reflecting the x-coordinate of the current point across the x-axis.
			EllipticCurvePoint operator-() const
			{
				return EllipticCurvePoint(x, -y, z);
			}

			/**
			* @briefPoint addition function.
			* 
			* This function implements point addition in elliptic curve cryptography. which calculates P + Q
			* where P and Q is a Difference between Two points on the elliptic curve.
			* 
			* @details
			* - `*this` represents the point in Jacobian coordinates with weighted projective coordinates.
			* - `other` is represented in affine coordinates (z = 1) or Jacobian coordinates.
			* 
			* The algorithm is based on the formulas for point addition in the standard projective coordinate system.
			* 
			* @reference
			* Reference Code: https://github.com/lcawen/SM23Crypto/blob/main/Utils.cs
			* 
			* @algorithm
			* Given points P(x1, y1, z1) in Jacobian coordinates and Q(x2, y2, z2) in affine coordinates,
			* the addition P + Q is calculated as follows:
			*
			* λ1 = x1 * z2  
			* λ2 = x2 * z1  
			* λ3 = λ1 − λ2  
			* λ4 = y1 * z2  
			* λ5 = y2 * z1  
			* λ6 = λ4 − λ5  
			* λ7 = λ1 + λ2  
			* λ8 = z1 * z2  
			* λ9 = λ3^2  
			* λ10 = λ3 * λ9  
			* λ11 = λ8 * λ6^2 − λ7 * λ9  
			* x3 = λ3 * λ11  
			* y3 = λ6 * (λ9 * λ1 − λ11) − λ4 * λ10  
			* z3 = λ10 * λ8  
			* 
			* @note
			* Special cases:
			* - If either point is the point at infinity, return the other point.
			* - If the points are equal (`this == other`), return the result of doubling the point.
			* - If the points are negatives of each other (`this == -other`), return the point at infinity.
			* 
			* @return 
			* A new `EllipticCurvePoint` representing the result of the addition in Jacobian coordinates.
			*/
			EllipticCurvePoint operator+(const EllipticCurvePoint& other) const;

			/**
			* @brief Point doubling function.
			* 
			* This function implements point doubling in elliptic curve cryptography, which calculates P + P
			* where P is a point on the elliptic curve.
			* 
			* @details
			* - The point `*this` is represented in Jacobian coordinates.
			* - The function handles edge cases such as the point at infinity or a vertical tangent (y = 0).
			* 
			* @algorithm
			* Given a point P(x1, y1, z1) in Jacobian coordinates, the doubling operation 2P is computed as follows:
			*
			* λ1 = 3 * x1^2 + a * z1^2  
			* λ2 = 2 * y1 * z1  
			* λ3 = y1^2  
			* λ4 = λ3 * x1 * z1  
			* λ5 = λ2^2  
			* λ6 = λ1^2 − 8 * λ4  
			* x3 = λ2 * λ6  
			* y3 = λ1 * (4 * λ4 − λ6) − 2 * λ5 * λ3  
			* z3 = λ2 * λ5  
			* 
			* @note
			* Special cases:
			* - If the point is at infinity, return the point itself.
			* - If y1 = 0, the tangent at the point is vertical, and the result is the point at infinity.
			* 
			* @return 
			* A new `EllipticCurvePoint` representing the result of doubling the point in Jacobian coordinates.
			*/
			EllipticCurvePoint Twice() const;

			/**
			* @brief Retrieve the affine X-coordinate of the elliptic curve point.
			* 
			* This function computes the X-coordinate in affine coordinates from the point represented 
			* in Jacobian coordinates. The computation ensures the result is valid and lies on the curve.
			* 
			* @details
			* Algorithm:
			* Given a point P(x, y, z) in Jacobian coordinates, the affine X-coordinate is calculated as:
			* 
			* X_affine = X / Z^2
			* 
			* The function computes this using the modular inverse of `z` (denoted as `z_inverse`), ensuring 
			* efficient computation without directly dividing by `z^2` (as modular division is non-trivial):
			* 1. If `z_inverse` is zero (uncomputed), calculate it using `z.inverse()`.
			* 2. Multiply the X-coordinate by `z_inverse` to derive the affine X-coordinate.
			* 
			* @explanation
			* - Points in Jacobian coordinates are represented in a projective form:
			*     - (X, Y, Z) represents the same point as (X/Z^2, Y/Z^3, 1) in affine coordinates.
			* - To retrieve the affine X-coordinate, the formula X_affine = X / Z^2 is used.
			* - Modular arithmetic does not allow division directly, so the modular inverse of `z` 
			*   is computed and cached for subsequent calls.
			* 
			* @why
			* - The modular inverse ensures that the calculation is performed in the same finite field
			*   as the elliptic curve's definition, preserving the point's validity on the curve.
			* - By working in the field defined by the curve's prime, the result remains consistent with
			*   the elliptic curve's equation.
			* 
			* @return 
			* The affine X-coordinate of the elliptic curve point as an `EllipticCurvePrimeField`.
			*/
			EllipticCurvePrimeField X() const
			{
				if (this->z_inverse.GetValue().IsZero())
				{
					auto inverse = this->z.inverse();
					z_inverse = inverse;
				}
				return this->x * z_inverse;
			}

			/**
			* @brief Retrieve the affine Y-coordinate of the elliptic curve point.
			* 
			* This function computes the Y-coordinate in affine coordinates from the point represented 
			* in Jacobian coordinates. The computation ensures the result is valid and lies on the curve.
			* 
			* @details
			* Algorithm:
			* Given a point P(x, y, z) in Jacobian coordinates, the affine Y-coordinate is calculated as:
			* 
			* Y_affine = Y / Z^3
			* 
			* The function uses the modular inverse of `z` (denoted as `z_inverse`), ensuring efficient 
			* computation without directly dividing by `z^3`:
			* 1. If `z_inverse` is zero (uncomputed), calculate it using `z.inverse()`.
			* 2. Multiply the Y-coordinate by `z_inverse` to derive the affine Y-coordinate.
			* 
			* @explanation
			* - Points in Jacobian coordinates are represented in a projective form:
			*     - (X, Y, Z) represents the same point as (X/Z^2, Y/Z^3, 1) in affine coordinates.
			* - To retrieve the affine Y-coordinate, the formula Y_affine = Y / Z^3 is used.
			* - Modular arithmetic does not allow direct division, so the modular inverse of `z` is 
			*   computed and cached for efficiency.
			* 
			* @why
			* - The modular inverse ensures that the calculation adheres to the finite field operations
			*   defined by the elliptic curve's prime modulus.
			* - By performing the computation within the field, the result remains consistent with the 
			*   elliptic curve's equation.
			* 
			* @return 
			* The affine Y-coordinate of the elliptic curve point as an `EllipticCurvePrimeField`.
			*/
			EllipticCurvePrimeField Y() const
			{
				if (this->z_inverse.GetValue().IsZero())
				{
					auto inverse = this->z.inverse();
					z_inverse = inverse;
				}
				return this->y * z_inverse;
			}

			/**
			* @brief Scalar multiplication function.
			* 
			* This function performs scalar multiplication in elliptic curve cryptography, which calculates
			* `k * P`, where `P` is a point on the elliptic curve and `k` is an integer scalar.
			* 
			* @details
			* - The point `*this` is represented in Jacobian coordinates.
			* - The scalar `k` is a `BigInteger`.
			* 
			* Algorithm:
			* This implementation uses an efficient algorithm called the "Non-Adjacent Form (NAF)" method
			* for scalar multiplication. The algorithm can be described as follows:
			*
			* 1. Compute k3 = 3 * scalar.
			* 2. Define `negate_point` as the negation of the current point `*this`.
			* 3. Initialize Q = P (`*this`).
			* 4. Iterate from the second most significant bit to the least significant bit of `k3`:
			*    - Double the current point Q: `Q = Q.Twice()`.
			*    - Check the bit values of `k3` and `scalar` at the current position:
			*        - If the bits differ (`k3_bit != k_bit`), add `P` or `-P` to Q:
			*            - If `k3_bit == 1`, add `P` to Q.
			*            - If `k3_bit == 0`, add `-P` to Q.
			* 5. Return Q as the result of the scalar multiplication.
			*
			* @note
			* Special cases:
			* - If the point is at infinity, return the point itself.
			* - If the scalar is zero, return the point at infinity.
			* 
			* @reference
			* The algorithm is inspired by techniques from scalar multiplication optimizations commonly
			* used in elliptic curve cryptography.
			* 
			* @return 
			* A new `EllipticCurvePoint` representing the result of the scalar multiplication in Jacobian coordinates.
			*/
			EllipticCurvePoint operator*(const BigInteger& scalar) const;

			friend EllipticCurvePoint operator*(const BigInteger& scalar, const EllipticCurvePoint& point) 
			{
				return point * scalar;
			}

			/**
			* @brief Equality operator for elliptic curve points.
			* 
			* This function checks whether two elliptic curve points are equal. 
			* It accounts for the representation of points in Jacobian coordinates, where the Z-coordinate 
			* can affect comparisons.
			* 
			* @details
			* Algorithm:
			* Given two points P1(x1, y1, z1) and P2(x2, y2, z2) in Jacobian coordinates:
			* 
			* 1. If the two points are the same object in memory (`this == &other`), return `true`.
			* 2. Check if either point is the point at infinity:
			*    - If both are at infinity, they are equal.
			*    - If only one is at infinity, they are not equal.
			* 3. To compare points in Jacobian coordinates, normalize the coordinates:
			*    - Compare the Y-coordinates: `u = y2 * z1 - y1 * z2`. 
			*      If `u != 0`, the points are not equal.
			*    - Compare the X-coordinates: `v = x2 * z1 - x1 * z2`.
			*      If `v == 0`, the points are equal; otherwise, they are not equal.
			* 
			* Explanation:
			* - Points in Jacobian coordinates are projective, meaning that the actual point on the curve 
			*   is represented as a ratio of coordinates. Specifically:
			*     - The affine coordinates (x, y) are derived from Jacobian coordinates as:
			*       - x_affine = x / z^2
			*       - y_affine = y / z^3
			* - To determine equality without converting to affine coordinates, we use cross-multiplication 
			*   to eliminate the denominators:
			*     - y1 / z1^3 == y2 / z2^3 → y2 * z1 == y1 * z2 (represented as `u` in the algorithm).
			*     - x1 / z1^2 == x2 / z2^2 → x2 * z1 == x1 * z2 (represented as `v` in the algorithm).
			* - If both `u` and `v` are satisfied, the points are equal in their projective representation.
			* 
			* @note
			* - Special cases:
			*   - If the points are the same in memory (`this == &other`), they are trivially equal.
			*   - If one or both points are at infinity, handle these as special cases.
			* 
			* @return 
			* - `true` if the two points are equal.
			* - `false` otherwise.
			*/
			bool operator==(const EllipticCurvePoint& other) const;

			bool operator!=(const EllipticCurvePoint& other) const;

			// 输出运算符重载
			friend std::ostream& operator<<(std::ostream& os, const EllipticCurvePoint& point)
			{
				if (point.IsInfinityPoint())
				{
					os << "Point is infinity!";
				}
				else
				{
					BigSignedInteger x_value = point.X().GetValue();
					BigSignedInteger y_value = point.Y().GetValue();
					BigSignedInteger z_value = point.z.GetValue();
					os << "(" << x_value.ToString(16) << ", " << y_value.ToString(16) << ", " << z_value.ToString(16) << ")";
				}
				return os;
			}

			/**
			* @brief Determines if a point lies on the elliptic curve.
			* 
			* This function checks whether a given point is valid and satisfies the elliptic curve equation. 
			* It uses the point's affine coordinates, derived from the projective representation, to perform 
			* the check.
			* 
			* @details
			* Algorithm:
			* 1. Check if the point is invalid using `IsInvalidPoint(point)`.
			*    - If the point is invalid, it cannot lie on the curve; return `false`.
			* 2. Convert the point's projective coordinates to affine coordinates using the `X()` and `Y()` functions.
			*    - These functions ensure that the coordinates are normalized and suitable for validation.
			* 3. Verify the point against the elliptic curve's equation in the finite field:
			*    - Check if the point satisfies the congruence relation defined by the elliptic curve.
			* 
			* @why
			* - The elliptic curve equation in affine coordinates (mod prime) is typically expressed as:
			*   y^2 ≡ x^3 + ax + b (mod prime),
			*   where `a` and `b` are curve parameters.
			* - By converting projective coordinates to affine form, we ensure the comparison adheres to 
			*   the finite field arithmetic and properly validates the point's membership on the curve.
			* - The function `CheckCurveFormula()` encapsulates this logic, making the code modular and efficient.
			* 
			* @note
			* Special Cases:
			* - If the point is invalid (e.g., (0,0,0), undefined coordinates), it cannot be on the curve.
			* - The function relies on modular arithmetic to ensure the correctness of the validation process.
			* 
			* @param point The elliptic curve point to be checked.
			* 
			* @return 
			* - `true` if the point lies on the curve.
			* - `false` otherwise.
			*/
			static bool IsOnCurve(const EllipticCurvePoint& point);

			static bool IsInvalidPoint(const EllipticCurvePoint& point)
			{
				//(x, y, z), true = (0, 0, 0), true
				return point.x.GetValue().IsZero() && point.y.GetValue().IsZero() && point.z.GetValue().IsZero();
			}

			static bool IsAffineInfinity(const EllipticCurvePoint& point)
			{
				if(IsInvalidPoint(point))
				{
					return false;
				}
				return point.x.GetValue().IsZero() && point.y.GetValue() == 1 && point.z.GetValue().IsZero();
			}

			static bool IsJacobianInfinity(const EllipticCurvePoint& point)
			{
				if(IsInvalidPoint(point))
				{
					return false;
				}
				return point.x.GetValue().IsZero() && point.y.GetValue().IsZero() && point.z.GetValue() == 1;
			}

			/**
			* @brief Constructs the point at infinity in affine coordinates.
			* 
			* The point at infinity is a special point on an elliptic curve that acts as the identity 
			* element for the group operation. In affine coordinates, this is typically represented 
			* with a unique convention depending on the implementation.
			* 
			* @return The point at infinity in affine coordinates.
			*/
			static const EllipticCurvePoint MakeAffineInfinity()
			{
				return EllipticCurvePoint(
					EllipticCurvePrimeField("0", 10), // X-coordinate
					EllipticCurvePrimeField(1),      // Y-coordinate
					EllipticCurvePrimeField("0", 10),// Z-coordinate (projective representation)
					true                             // Flag indicating this is the point at infinity
				);
			}

			/**
			* @brief Constructs the point at infinity in Jacobian coordinates.
			* 
			* The point at infinity is a special point on an elliptic curve that acts as the identity 
			* element for the group operation. In Jacobian coordinates, the point at infinity is 
			* represented as (0, 0, 1), which ensures that the Z-coordinate (used for projective 
			* representation) properly indicates infinity.
			* 
			* @return The point at infinity in Jacobian coordinates.
			*/
			static const EllipticCurvePoint MakeJaccobianInfinity()
			{
				return EllipticCurvePoint(
					EllipticCurvePrimeField("0", 10), // X-coordinate
					EllipticCurvePrimeField("0", 10), // Y-coordinate
					EllipticCurvePrimeField(1),       // Z-coordinate (indicates infinity in Jacobian coordinates)
					false                             // Flag indicating this is not a standard affine point
				);
			}
		};

		// SM2P256V1 generates the coordinates of the base point G
		inline const EllipticCurvePoint G = EllipticCurvePoint(
			EllipticCurvePrimeField(
				"32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7",
				16
			),
			EllipticCurvePrimeField(
				"BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0",
				16
			)
		);

		/**
		* @class SM2KeyPair
		* @brief Represents an SM2 key pair, including a private key and a public key.
		* 
		* The `SM2KeyPair` class is used to generate and manage a cryptographic key pair for the SM2 
		* elliptic curve cryptography algorithm. The SM2 algorithm is widely used in Chinese cryptographic 
		* standards and is based on elliptic curve cryptography over the `SM2P256V1` curve.
		*/
		class SM2KeyPair
		{
			using BigInteger = TwilightDream::BigInteger::BigInteger;
			using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

		private:
			BigInteger privateKey;                // The private key, stored as a `BigInteger`.
			EllipticCurvePoint publicKey;         // The public key, represented as an elliptic curve point.

		public:
			/**
			 * @brief Constructor that generates a new key pair.
			 * 
			 * Upon instantiation, this constructor automatically generates a private and public key pair 
			 * using the SM2 algorithm. The keys are securely stored within the object.
			 */
			SM2KeyPair()
			{
				GenerateKeyPair();
			}

			/**
			 * @brief Retrieves the private key.
			 * 
			 * The private key is a large integer used as the secret component of the key pair. It should 
			 * be kept secure and not shared.
			 * 
			 * @return The private key as a `BigInteger`.
			 */
			const BigInteger& GetPrivateKey() const
			{
				return privateKey;
			}

			/**
			 * @brief Retrieves the public key.
			 * 
			 * The public key is derived from the private key and is represented as a point on the 
			 * `SM2P256V1` elliptic curve. It is used for encryption, signature verification, or other 
			 * public operations.
			 * 
			 * @return The public key as an `EllipticCurvePoint`.
			 */
			const EllipticCurvePoint& GetPublicKey() const
			{
				return publicKey;
			}

			/**
			* @brief Generates an SM2P256V1 key pair (private and public keys).
			* 
			* This function generates a cryptographically secure key pair for the SM2 elliptic curve 
			* cryptography algorithm. The private key is randomly selected within a defined range, and the 
			* corresponding public key is calculated as `P = d * G`, where:
			* - `d` is the private key.
			* - `G` is the generator point of the `SM2P256V1` elliptic curve.
			* 
			* Algorithm:
			* 1. Define the range for the private key:
			*    - The private key `d` must be in the range [1, n-2], where:
			*      - `n` is the order of the elliptic curve.
			* 2. Generate a random private key within the range:
			*    - Compute the range `max - min + 1`.
			*    - Generate a random number of appropriate bit length and ensure it falls within the range.
			*    - Add the minimum value (`min = 1`) to map the random number to the valid range.
			* 3. Compute the public key:
			*    - Calculate `P = d * G`, where `G` is the generator point of the elliptic curve.
			* 4. Validate the public key:
			*    - Verify that the generated public key lies on the elliptic curve using `IsOnCurve()`.
			*    - If the public key is invalid, restart the key generation process.
			* 
			* Why This is Secure:
			* - The private key is randomly chosen in a secure range, ensuring unpredictability.
			* - The public key is derived mathematically, ensuring it corresponds to the private key.
			* - The validation step ensures the public key is a valid point on the elliptic curve, mitigating 
			*   errors from invalid key generation.
			* 
			* Debugging and Output:
			* - If the public key fails the curve validation, the function restarts key generation.
			* - A successful key pair generation prints a confirmation message.
			* 
			* Note:
			* - This function uses a `goto` statement to retry the key generation process if validation fails.
			*   While this is functional, consider replacing it with a loop for improved readability and structure.
			* 
			* @throws May retry indefinitely if random generation repeatedly produces invalid keys (unlikely in practice).
			*/
			void GenerateKeyPair();
		};


	}  // namespace SM2

}  // namespace TwilightDream::CryptographyAsymmetric

#endif
