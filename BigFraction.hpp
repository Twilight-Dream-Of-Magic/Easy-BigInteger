/*
MIT License

Copyright (c) 2024 Twilight-Dream & With-Sky

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

#ifndef TWILIGHT_DREAM_BIG_FRACTION_HPP
#define TWILIGHT_DREAM_BIG_FRACTION_HPP

#include "BigInteger.hpp"

namespace TwilightDream::BigFraction
{
	enum class DecimalPrecisionMode : uint32_t
	{
		Fixed = 0,	 // Specify the precision mode
		Full = 1	 // Full the precision mode
	};

	class BigFraction
	{
	public:
		DecimalPrecisionMode PrecisionMode = DecimalPrecisionMode::Full;
		uint64_t FixedPrecisionCount = 2;

		using BigInteger = TwilightDream::BigInteger::BigInteger;
		using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

		static const BigInteger ONE;
		static const BigInteger TWO;
		static const BigInteger THREE;
		static const BigInteger FIVE;
		static const BigInteger TEN;
		static const BigInteger RADIX;
		
		BigFraction();
		BigFraction( const BigFraction& other ) noexcept;
		BigFraction( BigFraction&& other ) noexcept;
		explicit BigFraction( const BigInteger& numerator );
		BigFraction( const BigInteger& numerator, const BigInteger& denominator );
		BigFraction( const BigInteger& numerator, const BigInteger& denominator, int sign );
		explicit BigFraction( const std::string& complexString );

		void	   SetSimplifyReduced( bool value );
		bool	   IsNaN() const;
		bool	   IsZero() const;
		bool	   IsNegative() const;
		BigInteger GetNumerator() const;
		BigInteger GetDenominator() const;
		void	   SetNumerator( const BigInteger& number );
		void	   SetDenominator( const BigInteger& number );
		BigFraction GetFullPrecision() const;
		void SetFullPrecision(BigInteger number);

		void		ComputeAndFromDecimalString( const std::string& complexString );
		std::string ComputeAndToDecimalString() const;

		BigFraction& operator=( const BigFraction& other );
		BigFraction& operator=( BigFraction&& other );
		BigFraction& operator+=( const BigFraction& other );
		BigFraction& operator+=( const BigInteger& other );
		BigFraction& operator-=( const BigFraction& other );
		BigFraction& operator-=( const BigInteger& other );
		BigFraction& operator*=( const BigFraction& other );
		BigFraction& operator*=( const BigInteger& other );
		BigFraction& operator/=( const BigFraction& other );
		BigFraction& operator/=( const BigInteger& other );

		BigFraction operator+( const BigFraction& other ) const;
		BigFraction operator+( const BigInteger& other ) const;
		BigFraction operator-( const BigFraction& other ) const;
		BigFraction operator-( const BigInteger& other ) const;
		BigFraction operator*( const BigFraction& other ) const;
		BigFraction operator*( const BigInteger& other ) const;
		BigFraction operator/( const BigFraction& other ) const;
		BigFraction operator/( const BigInteger& other ) const;

		bool operator<( const BigFraction& other ) const;
		bool operator<=( const BigFraction& other ) const;
		bool operator>( const BigFraction& other ) const;
		bool operator>=( const BigFraction& other ) const;
		bool operator==( const BigFraction& other ) const;
		bool operator!=( const BigFraction& other ) const;

		BigFraction Abs() const;
		BigFraction Reciprocal() const;
		BigFraction Sqrt();
		BigFraction Cbrt();
		BigFraction Log( const BigInteger& value ) const;
		BigFraction Log() const;
		BigFraction Log10( const BigInteger& fraction ) const;
		BigFraction Log10() const;
		BigFraction Exp( const BigInteger& value ) const;
		BigFraction Exp( const BigFraction& value ) const;
		// nth Power of a BigFraction
		BigFraction Power( const BigInteger& exponent ) const;
		BigFraction Power( const BigFraction& exponent ) const;
		// nth Root of a BigFraction
		BigFraction NthRoot(const BigInteger& n) const;
		BigFraction NthRoot(const BigFraction& fraction, const BigInteger& n) const;

		BigInteger	Floor() const;
		BigInteger	Ceil() const;
		BigInteger	Round() const;
		void		FromDouble( double value, uint64_t maxIterations = 128 );
		void		FromFloat( float value, uint64_t maxIterations = 128 );

		operator BigInteger() const;
		operator double() const;
		operator float() const;

		static bool IsPerfectPower( const BigInteger& N );

		friend std::istream& operator>>( std::istream& is, BigFraction& fraction );
		friend std::ostream& operator<<( std::ostream& os, const BigFraction& fraction );

	private:
		BigInteger numerator;
		BigInteger denominator;
		int32_t	   sign = 1;
		bool	   simplify_reduced = true;

		void ReduceSimplify();
		BigFraction LogCF(const BigInteger& value) const;
		BigInteger NthRoot( const BigInteger& value, const BigInteger& n ) const;
	};

	inline BigFraction BigFractionFullPrecision(1, 0);

}  // namespace TwilightDream::BigFraction

#endif