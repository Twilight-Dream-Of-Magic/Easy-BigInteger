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

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef FINITE_FIELD_HPP
#define FINITE_FIELD_HPP

#include "BigInteger.hpp"
#include "PrimeNumberTester.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace TwilightDream::Math
{
	// 基类，包含所有共享的逻辑
	class FiniteFieldBase
	{
	public:
		using Integer = TwilightDream::BigInteger::BigSignedInteger;
		using UnsignedInteger = TwilightDream::BigInteger::BigInteger;
		using Montgomery = TwilightDream::BigInteger::Montgomery;

	protected:
		UnsignedInteger prime;

		// 构造函数
		FiniteFieldBase( const UnsignedInteger& modulus ) : prime( modulus )
		{}

	public:
		// 虚析构函数
		virtual ~FiniteFieldBase() = default;

		// 计算逆元，公共逻辑
		virtual Integer compute_inverse_signed() const
		{
			std::cerr << "错误: 调用基类的 compute_inverse_signed 方法。" << std::endl;
			throw std::runtime_error( "Called base class compute_inverse_signed method." );
		}

		virtual UnsignedInteger compute_inverse_unsigned() const
		{
			std::cerr << "错误: 调用基类的 compute_inverse_unsigned 方法。" << std::endl;
			throw std::runtime_error( "Called base class compute_inverse_unsigned method." );
		}

		// 获取模数
		virtual const UnsignedInteger& GetPrime() const
		{
			return prime;
		}
	};

	class UnsignedFiniteField;
	class SignedFiniteField;

	class UnsignedFiniteField : public FiniteFieldBase
	{
	protected:
		UnsignedInteger unsigned_value;
		bool			is_tested_prime = false;

		UnsignedInteger compute_inverse_unsigned() const
		{
			return this->power( prime - UnsignedInteger( "2" ) ).unsigned_value;
		}
	public:
		// 构造函数
		UnsignedFiniteField( const UnsignedInteger& number, const UnsignedInteger& modulus )
			:
			unsigned_value( number ),
			FiniteFieldBase( modulus )
		{
			if(!is_tested_prime)
			{
				this->SetPrime(modulus);
			}

			while(unsigned_value >= prime)
			{
				unsigned_value = unsigned_value - prime;
			}
		}

		UnsignedFiniteField( const std::string& number, const std::string& modulus )
			:
			UnsignedFiniteField( UnsignedInteger(number, 10), UnsignedInteger( modulus, 10 ) )
		{
			while(unsigned_value >= prime)
			{
				unsigned_value = unsigned_value - prime;
			}
		}

		UnsignedFiniteField( const std::string& number, uint32_t base_number, const std::string& modulus, uint32_t base_modulus = 10 )
			:
			UnsignedFiniteField( UnsignedInteger(number, base_number) , UnsignedInteger( modulus, base_modulus ) )
		{
			while(unsigned_value >= prime)
			{
				unsigned_value = unsigned_value - prime;
			}
		}

		// 复制构造函数
		UnsignedFiniteField( const UnsignedFiniteField& other )
			:
			unsigned_value( other.unsigned_value ),
			is_tested_prime( other.is_tested_prime ),
			FiniteFieldBase( other.prime )
		{}

		// 移动构造函数
		UnsignedFiniteField( UnsignedFiniteField&& other ) noexcept
			:
			unsigned_value( std::move(other.unsigned_value) ),
			is_tested_prime( std::move( other.is_tested_prime ) ),
			FiniteFieldBase( std::move( other.prime ) )
		{}

		// 赋值运算符
		UnsignedFiniteField& operator=( const UnsignedFiniteField& other )
		{
			if ( this != &other )
			{
				if ( prime != other.prime )
				{
					throw std::invalid_argument( "Cannot assign elements from different fields." );
				}
				this->unsigned_value = other.unsigned_value;
				this->is_tested_prime = other.is_tested_prime;
			}
			return *this;
		}

		// 赋值运算符（移动）
		UnsignedFiniteField& operator=( UnsignedFiniteField&& other )
		{
			if ( this != &other )
			{
				this->unsigned_value = std::move( other.unsigned_value );
				this->prime = std::move( other.prime );
				this->is_tested_prime = other.is_tested_prime;
			}
			return *this;
		}

		// 获取无符号值
		const UnsignedInteger& GetValue() const
		{
			return unsigned_value;
		}

		void SetValue( const UnsignedFiniteField& value ) 
		{
			unsigned_value = value.unsigned_value;
		}

		// 加法
		UnsignedFiniteField operator+( const UnsignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot add elements from different fields." );
			}
			return UnsignedFiniteField( unsigned_value + other.unsigned_value, prime );
		}

		// 减法
		UnsignedFiniteField operator-( const UnsignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot subtract elements from different fields." );
			}
			
			if(this->unsigned_value > other.unsigned_value)
			{
				return UnsignedFiniteField( ( unsigned_value - other.unsigned_value ) % prime, prime );
			}
			else
			{
				UnsignedInteger difference = other.unsigned_value - unsigned_value;
				UnsignedInteger diveded_difference = difference / prime;
				UnsignedInteger result = diveded_difference * prime - difference;
				return UnsignedFiniteField( result, prime );
			}
		}

		// 乘法
		UnsignedFiniteField operator*( const UnsignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot multiply elements from different fields." );
			}
			UnsignedInteger result;
			if ( unsigned_value.BitLength() > 128 || other.unsigned_value.BitLength() > 128 )
			{
				//蒙哥马利算法
				Montgomery montgomery( this->prime );
				result = montgomery.Multiplication(unsigned_value, other.unsigned_value);
				return UnsignedFiniteField( result, prime );
			}
			return UnsignedFiniteField( unsigned_value * other.unsigned_value, prime );
		}

		// 除法
		UnsignedFiniteField operator/( const UnsignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot divide elements from different fields." );
			}
			UnsignedInteger inverse = other.compute_inverse_unsigned();
			return UnsignedFiniteField( unsigned_value * inverse, prime );
		}

		// 左移赋值
		UnsignedFiniteField& operator<<=( size_t shift )
		{
			if ( shift == 0 )
				return *this;

			// 按位左移
			unsigned_value <<= shift;

			if(unsigned_value >= prime)
				unsigned_value %= prime;

			return *this;
		}

		// 右移赋值
		UnsignedFiniteField& operator>>=( size_t shift )
		{
			if ( shift == 0 )
				return *this;

			// 按位右移
			unsigned_value >>= shift;

			if(unsigned_value >= prime)
				unsigned_value %= prime;

			return *this;
		}

		// 左移
		UnsignedFiniteField operator<<( const size_t& shift ) const
		{
			UnsignedFiniteField copy = *this;
			copy <<= shift;
			return copy;
		}

		// 右移
		UnsignedFiniteField operator>>( const size_t& shift ) const
		{
			UnsignedFiniteField copy = *this;
			copy >>= shift;
			return copy;
		}

		// 幂运算
		UnsignedFiniteField power( const UnsignedInteger& exponent ) const
		{
			UnsignedInteger result = UnsignedInteger( "1" );
			UnsignedInteger base = unsigned_value;
			UnsignedInteger copy_exponent = exponent;

			if ( copy_exponent.IsZero() )
			{
				return UnsignedFiniteField( result, prime );
			}

			if ( copy_exponent.IsNegative() )
			{
				throw std::invalid_argument( "Negative exponent not supported for PositiveFiniteField." );
			}

			if ( base.BitLength() > 128 )
			{
				Montgomery montgomery( this->prime );
				result = montgomery.Power(base, copy_exponent);
				return UnsignedFiniteField( result, prime );
			}
			else
			{
				while ( !copy_exponent.IsZero() )
				{
					if (copy_exponent & 1)
					{
						result = ( result * base );
					}
					base = ( base * base );
					copy_exponent >>= 1;
				}
			}

			return UnsignedFiniteField( result, prime );
		}

		// 求逆元
		UnsignedFiniteField inverse() const
		{
			if(unsigned_value == 1)
			{
				return UnsignedFiniteField( unsigned_value, prime );
			}

			UnsignedInteger result = compute_inverse_unsigned();
			return UnsignedFiniteField( result, prime );
		}

		// 取负 并转换到 有符号质数域
		SignedFiniteField Convert() const;

		// 相等性检查
		bool operator==( const UnsignedFiniteField& other ) const
		{
			return ( unsigned_value == other.unsigned_value ) && ( prime == other.prime );
		}

		bool operator!=( const UnsignedFiniteField& other ) const
		{
			return !( *this == other );
		}

		// 转换为字符串
		std::string ToString() const
		{
			return "PositiveFiniteField_" + prime.ToString() + "(" + unsigned_value.ToString() + ")";
		}

		// 设置模数（如果需要）
		void SetPrime( const UnsignedInteger& modulus )
		{
			using PrimeNumberTester = TwilightDream::PrimeNumberTester;

			if ( !is_tested_prime )
			{
				PrimeNumberTester PrimeTester;
				if ( !PrimeTester.IsPrime( modulus ) )
				{
					is_tested_prime = false;
					throw std::invalid_argument( "The modulus must be a prime number." );
				}
				is_tested_prime = true;
			}

			this->prime = modulus;
			this->unsigned_value = this->unsigned_value % prime;
		}
	};

	class SignedFiniteField : public FiniteFieldBase
	{
	protected:
		Integer			signed_value;
		bool			is_tested_prime = false;

		Integer compute_inverse_signed() const
		{
			// 扩展欧几里得算法
			Integer t = 0;
			Integer new_t = 1;
			Integer r = prime;
			Integer new_r = signed_value;

			Integer q;
			Integer temp_t;
			Integer temp_r;
			while ( !new_r.IsZero() )
			{
				q = r / new_r;
				temp_t = t - q * new_t;
				t = new_t;
				new_t = temp_t;

				temp_r = r - q * new_r;
				r = new_r;
				new_r = temp_r;
			}

			if ( r > Integer( "1" ) )
			{ 
				throw std::invalid_argument( "Element has no inverse in the field." );
			}

			return t;
		}
	public:
		// 构造函数
		SignedFiniteField( const Integer& number, const UnsignedInteger& modulus )
			:
			signed_value( number ),
			FiniteFieldBase( modulus )
		{
			if(!is_tested_prime)
			{
				this->SetPrime(modulus);
			}

			while(signed_value >= prime)
			{
				signed_value = signed_value - prime;
			}

			if(signed_value < 0)
			{
				signed_value = signed_value % prime;
			}
		}

		SignedFiniteField( const std::string& number, const std::string& modulus )
			:
			SignedFiniteField( Integer(number, 10) , UnsignedInteger( modulus, 10 ) )
		{
			while(signed_value >= prime)
			{
				signed_value = signed_value - prime;
			}

			if(signed_value < 0)
			{
				signed_value = signed_value % prime;
			}
		}

		SignedFiniteField( const std::string& number, uint32_t base_number, const std::string& modulus, uint32_t base_modulus = 10 )
			:
			SignedFiniteField( Integer(number, base_number) , UnsignedInteger( modulus, base_modulus ) )
		{
			while(signed_value >= prime)
			{
				signed_value = signed_value - prime;
			}

			if(signed_value < 0)
			{
				signed_value = signed_value % prime;
			}
		}

		// 复制构造函数
		SignedFiniteField( const SignedFiniteField& other )
			:
			signed_value( other.signed_value ),
			is_tested_prime( other.is_tested_prime ),
			FiniteFieldBase( other.prime )
		{}

		// 移动构造函数
		SignedFiniteField( SignedFiniteField&& other ) noexcept
			:
			signed_value( std::move(other.signed_value) ),
			is_tested_prime( std::move( other.is_tested_prime ) ),
			FiniteFieldBase( std::move( other.prime ) )
		{}

		// 赋值运算符
		SignedFiniteField& operator=( const SignedFiniteField& other )
		{
			if ( this != &other )
			{
				if ( prime != other.prime )
				{
					throw std::invalid_argument( "Cannot assign elements from different fields." );
				}
				this->signed_value = other.signed_value;
				this->is_tested_prime = other.is_tested_prime;
			}
			return *this;
		}

		// 赋值运算符（移动）
		SignedFiniteField& operator=( SignedFiniteField&& other )
		{
			if ( this != &other )
			{
				this->signed_value = std::move( other.signed_value );
				this->prime = std::move( other.prime );
				this->is_tested_prime = other.is_tested_prime;
			}
			return *this;
		}

		// 获取有符号值
		const Integer& GetValue() const
		{
			return signed_value;
		}

		void SetValue( const SignedFiniteField& value ) 
		{
			signed_value = value.signed_value;
		}

		// 运算符重载

		// 加法
		SignedFiniteField operator+( const SignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot add elements from different fields." );
			}
			return SignedFiniteField( signed_value + other.signed_value, prime );
		}

		// 减法
		SignedFiniteField operator-( const SignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot subtract elements from different fields." );
			}
			return SignedFiniteField( signed_value - other.signed_value, prime );
		}

		// 乘法
		SignedFiniteField operator*( const SignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot multiply elements from different fields." );
			}
			return SignedFiniteField( signed_value * other.signed_value, prime );
		}

		// 除法
		SignedFiniteField operator/( const SignedFiniteField& other ) const
		{
			if ( prime != other.prime )
			{
				throw std::invalid_argument( "Cannot divide elements from different fields." );
			}
			Integer inverse = other.compute_inverse_signed();
			return SignedFiniteField( signed_value * inverse, prime );
		}

		// 左移赋值
		SignedFiniteField& operator<<=( size_t shift )
		{
			if ( shift == 0 )
				return *this;

			// 按位左移
			signed_value <<= shift;

			if(signed_value >= prime)
				signed_value %= prime;

			return *this;
		}

		// 右移赋值
		SignedFiniteField& operator>>=( size_t shift )
		{
			if ( shift == 0 )
				return *this;

			// 按位右移
			signed_value >>= shift;

			if(signed_value >= prime)
				signed_value %= prime;

			return *this;
		}

		// 左移
		SignedFiniteField operator<<( const size_t& shift ) const
		{
			SignedFiniteField copy = *this;
			copy <<= shift;
			return copy;
		}

		// 右移
		SignedFiniteField operator>>( const size_t& shift ) const
		{
			SignedFiniteField copy = *this;
			copy >>= shift;
			return copy;
		}

		// 幂运算
		SignedFiniteField power( const Integer& exponent ) const
		{
			Integer result = Integer( "1" );
			Integer base = signed_value;
			Integer copy_exponent = exponent;

			// 处理负指数
			if ( exponent.IsNegative() )
			{
				copy_exponent = -exponent;
				base = compute_inverse_signed();
			}

			while ( !copy_exponent.IsZero() )
			{
				if ( ( copy_exponent % Integer( "2" ) ) == Integer( "1" ) )
				{
					result = ( result * base );
				}
				base = ( base * base );
				copy_exponent >>= 1;
			}

			return SignedFiniteField( result, prime );
		}

		// 求逆元
		SignedFiniteField inverse() const
		{
			if(signed_value == 1)
			{
				return SignedFiniteField( signed_value, prime );
			}

			Integer result = compute_inverse_signed();
			return SignedFiniteField( result, prime );
		}

		// 取负 并转换到 无符号质数域
		UnsignedFiniteField Convert() const;

		// 相等性检查
		bool operator==( const SignedFiniteField& other ) const
		{
			return ( signed_value == other.signed_value ) && ( prime == other.prime );
		}

		bool operator!=( const SignedFiniteField& other ) const
		{
			return !( *this == other );
		}

		// 转换为字符串
		std::string ToString() const
		{
			return "NegativeFiniteField_" + prime.ToString() + "(" + signed_value.ToString() + ")";
		}

		// 设置模数（如果需要）
		void SetPrime( const Integer& modulus )
		{
			using PrimeNumberTester = TwilightDream::PrimeNumberTester;

			if ( modulus.IsNegative() )
			{
				throw std::invalid_argument( "The modulus must be a positive integer." );
			}

			if ( !is_tested_prime )
			{
				PrimeNumberTester PrimeTester;
				if ( !PrimeTester.IsPrime( modulus.Abs() ) )
				{
					is_tested_prime = false;
					throw std::invalid_argument( "The modulus must be a prime number." );
				}
				is_tested_prime = true;
			}

			this->prime = modulus;
			this->signed_value = this->signed_value % prime;
		}
	};

	inline SignedFiniteField UnsignedFiniteField::Convert() const
	{
		if ( unsigned_value.IsZero() )
		{
			return SignedFiniteField( unsigned_value, prime );
		}
		
		FiniteFieldBase::UnsignedInteger prime = this->GetPrime();
		FiniteFieldBase::Integer result = (-FiniteFieldBase::Integer(unsigned_value)) % prime;
		return SignedFiniteField( result, prime );
	}

	inline UnsignedFiniteField SignedFiniteField::Convert() const
	{
		if ( signed_value.IsZero() )
		{
			return UnsignedFiniteField( signed_value, prime );
		}
			
		FiniteFieldBase::UnsignedInteger prime = this->GetPrime();
		FiniteFieldBase::UnsignedInteger result = FiniteFieldBase::UnsignedInteger(-signed_value) % prime;
		return UnsignedFiniteField( result, prime );
	}

}  // namespace TwilightDream::Math

#endif	// FINITE_FIELD_HPP
