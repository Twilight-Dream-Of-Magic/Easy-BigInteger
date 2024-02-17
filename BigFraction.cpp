#include "BigFraction.hpp"

namespace TwilightDream::BigFraction
{
	const BigFraction::BigInteger BigFraction::ONE = 1;
	const BigFraction::BigInteger BigFraction::TWO = 2;
	const BigFraction::BigInteger BigFraction::THREE = 3;
	const BigFraction::BigInteger BigFraction::FIVE = 5;
	const BigFraction::BigInteger BigFraction::TEN = 10;
	const BigFraction::BigInteger BigFraction::RADIX = TEN;

	BigFraction::BigFraction() : numerator( 0 ), denominator( 1 ), sign( 1 ) {}

	BigFraction::BigFraction( const BigFraction& other ) noexcept
		: numerator( other.numerator ), denominator( other.denominator ), sign( other.sign ),
		simplify_reduced(other.simplify_reduced ),
		PrecisionMode( other.PrecisionMode ), FixedPrecisionCount( other.FixedPrecisionCount )
	{}

	BigFraction::BigFraction( BigFraction&& other ) noexcept
		: numerator( other.numerator ), denominator( other.denominator ), sign( other.sign ),
		simplify_reduced(other.simplify_reduced ),
		PrecisionMode( other.PrecisionMode ), FixedPrecisionCount( other.FixedPrecisionCount )
	{}

	BigFraction::BigFraction( const BigInteger& numerator ) : numerator( numerator ), denominator( 1 ), sign( 1 ) {}

	BigFraction::BigFraction( const BigInteger& numerator, const BigInteger& denominator ) : numerator( numerator ), denominator( denominator ), sign( 1 )
	{
		if ( numerator.IsNegative() )
		{
			this->numerator = numerator.Abs();
			sign *= -1;
		}
		if ( denominator.IsNegative() )
		{
			this->denominator = denominator.Abs();
			sign *= -1;
		}

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}
	}

	BigFraction::BigFraction( const BigInteger& numerator, const BigInteger& denominator, int sign ) : numerator( numerator ), denominator( denominator ), sign( sign >= 0 ? 1 : -1 )
	{
		if ( numerator.IsNegative() )
		{
			this->numerator = numerator.Abs();
		}
		if ( denominator.IsNegative() )
		{
			this->denominator = denominator.Abs();
		}

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}
	}

	BigFraction::BigFraction( const std::string& complexString )
	{
		ComputeAndFromDecimalString( complexString );
	}

	void BigFraction::SetSimplifyReduced( bool value )
	{
		simplify_reduced = value;
	}

	bool BigFraction::IsNaN() const
	{
		if ( numerator.IsZero() && denominator.IsZero() )
		{
			return true;
		}

		if ( !numerator.IsZero() && denominator.IsZero() )
		{
			return true;
		}

		return false;
	}

	bool BigFraction::IsZero() const
	{
		return numerator.IsZero() && !denominator.IsZero();
	}

	bool BigFraction::IsNegative() const
	{
		return sign == -1;
	}

	BigFraction::BigInteger BigFraction::GetNumerator() const
	{
		return numerator;
	}

	BigFraction::BigInteger BigFraction::GetDenominator() const
	{
		return denominator;
	}

	void BigFraction::SetNumerator( const BigInteger& number )
	{
		numerator = number;
	}

	void BigFraction::SetDenominator( const BigInteger& number )
	{
		denominator = number;
	}

	BigFraction BigFraction::GetFullPrecision() const
	{
		if(BigFractionFullPrecision.GetDenominator().IsZero())
		{
			throw std::invalid_argument( "BigFractionFullPrecision Denominator of zero value is undefined." );
		}

		return BigFractionFullPrecision;
	}

	void BigFraction::SetFullPrecision(BigInteger number)
	{
		if(number.IsZero())
		{
			return;
		}

		BigFractionFullPrecision.SetDenominator(number);
	}

	void BigFraction::ComputeAndFromDecimalString( const std::string& complexString )
	{
		// Parsing Complex String
		bool   isNegative = false;
		bool   hasFractionalPart = false;
		size_t integerPartEnd = 0;
		size_t fractionalPartStart = 0;

		// Determine if the number is negative
		if ( !complexString.empty() && complexString[ 0 ] == '-' )
		{
			isNegative = true;
			integerPartEnd = 1;
		}

		// Find the end of integer part
		while ( integerPartEnd < complexString.size() && std::isdigit( complexString[ integerPartEnd ] ) )
		{
			integerPartEnd++;
		}

		// Check if there is a fractional part
		if ( integerPartEnd < complexString.size() && complexString[ integerPartEnd ] == '.' )
		{
			hasFractionalPart = true;
			fractionalPartStart = integerPartEnd + 1;
		}

		// Parse integer part
		BigInteger integerPart( complexString.substr( isNegative, integerPartEnd - isNegative ) );

		// Parse fractional part if exists
		BigInteger fractionalPart = 0;
		BigInteger fractionalMultiplier = 1;
		if ( hasFractionalPart )
		{
			uint64_t LoopCount = 0;

			switch ( this->PrecisionMode )
			{
				case DecimalPrecisionMode::Fixed:
					LoopCount = this->FixedPrecisionCount;
					break;
				case DecimalPrecisionMode::Full:
					LoopCount = complexString.size();
					break;
				default:
					break;
			}

			for ( size_t i = fractionalPartStart; i < LoopCount; ++i )
			{
				if ( !std::isdigit( complexString[ i ] ) )
				{
					// Invalid character found in fractional part
					// You may want to handle this error case appropriately
					return;
				}
				fractionalPart = fractionalPart * RADIX + ( complexString[ i ] - '0' );
				fractionalMultiplier *= RADIX;
			}
		}

		// Combine integer and fractional parts into the numerator
		numerator = integerPart * fractionalMultiplier + fractionalPart;
		denominator = fractionalMultiplier;

		// Apply the sign
		if ( isNegative )
		{
			numerator *= -1;
		}

		// Simplify the fraction to reduce it to its simplest form
		if ( simplify_reduced )
		{
			ReduceSimplify();
		}
	}

	std::string BigFraction::ComputeAndToDecimalString() const
	{
		if ( IsNaN() )
		{
			return "NaN";
		}

		std::string signString = ( sign == -1 ) ? "-" : "";

		BigFraction abs = this->Abs();
		BigInteger absNumerator = abs.GetNumerator();
		BigInteger absDenominator = abs.GetDenominator();

		// Reduce the fraction to its simplest form if necessary
		if ( absNumerator > 1 && absDenominator > 1 )
		{
			BigInteger gcd = BigInteger::GCD( absNumerator, absDenominator );
			absNumerator /= gcd;
			absDenominator /= gcd;
		}

		// Compute the integer part
		BigInteger	integerPart = absNumerator / absDenominator;
		std::string result = signString + integerPart.ToString( 10 ) + ".";

		switch ( this->PrecisionMode )
		{
			case DecimalPrecisionMode::Fixed:
			{
				if ( this->FixedPrecisionCount == 0 )
				{
					// Return only the integer part
					result += "0";
					return result;
				}

				BigInteger fractionalPart = absNumerator % absDenominator;
				for ( size_t i = 0; i < this->FixedPrecisionCount; ++i )
				{
					fractionalPart *= RADIX;
					BigInteger nextDigit = fractionalPart / absDenominator;
					fractionalPart %= absDenominator;
					result += nextDigit.ToString( 10 );
					if ( fractionalPart == 0 )
					{
						// Stop if the fractional part becomes zero before reaching the desired precision
						break;
					}
				}

				break;
			}
			
			case DecimalPrecisionMode::Full:
			{
				BigInteger fractionalPart = absNumerator % absDenominator;
				BigInteger precision = GetFullPrecision();
				while ( fractionalPart > precision )
				{
					fractionalPart *= RADIX;
					BigInteger nextDigit = fractionalPart / absDenominator;
					fractionalPart %= absDenominator;
					result += nextDigit.ToString( 10 );
				}

				break;
			}
			default:
				break;
		}

		return result;
	}

	BigFraction& BigFraction::operator=( const BigFraction& other )
	{
		if ( this != &other )
		{
			this->numerator = other.numerator;
			this->denominator = other.denominator;
			this->sign = other.sign;
			this->simplify_reduced = this->simplify_reduced;
			this->PrecisionMode = other.PrecisionMode;
			this->FixedPrecisionCount = other.FixedPrecisionCount;

		}
		return *this;
	}

	BigFraction& BigFraction::operator=( BigFraction&& other )
	{
		if ( this != &other )
		{
			this->numerator = std::move(other.numerator);
			this->denominator = std::move(other.denominator);
			this->sign = std::move(other.sign);
			this->simplify_reduced = std::move(this->simplify_reduced);
			this->PrecisionMode = std::move(other.PrecisionMode);
			this->FixedPrecisionCount = std::move(other.FixedPrecisionCount);
		}
		return *this;
	}

	BigFraction& BigFraction::operator+=( const BigFraction& other )
	{
		if ( this->IsNaN() || other.IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( IsZero() )
		{
			if ( other.IsZero() )
			{
				sign = 1;
				numerator = 0;
				denominator = 1;
				return *this;
			}
			else if ( other.IsNegative() )
			{
				sign = -1;
				numerator = other.numerator.Abs();
				denominator = other.denominator.Abs();
				return *this;
			}
			else
			{
				sign = 1;
				numerator = other.numerator.Abs();
				denominator = other.denominator.Abs();
				return *this;
			}
		}
		else if ( other.IsZero() )
		{
			return *this;
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger a(this->numerator * other.denominator, this->sign < 0);
		BigSignedInteger b(other.numerator * this->denominator, other.sign < 0);
		BigSignedInteger resultNumeratorSigned = a + b;
		BigInteger resultDenominatorSigned = this->denominator * other.denominator;

		// Determine the sign and absolute values
		sign = resultNumeratorSigned.IsNegative() ? -1 : 1;
		numerator = static_cast<BigInteger>(resultNumeratorSigned.Abs());
		denominator = resultDenominatorSigned;

		if (simplify_reduced)
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator+=( const BigInteger& other )
	{
		if ( this->IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( this->IsZero() )
		{
			if ( other.IsZero() )
			{
				return *this;
			}
			else if ( other.IsNegative() )
			{
				this->sign = -1;
				this->numerator = other.Abs();
				this->denominator = 1;
				return *this;
			}
			else
			{
				this->sign = 1;
				this->numerator = other;
				this->denominator = 1;
				return *this;
			}
		}
		else if ( other == 0 )
		{
			return *this;
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger a(this->numerator, this->sign < 0);
		BigSignedInteger b(other, other.IsNegative());

		BigSignedInteger resultNumeratorSigned;
		if (this->sign == 1)
		{
			resultNumeratorSigned = a + b;
		}
		else
		{
			resultNumeratorSigned = a - b;
		}

		// Determine the sign and absolute values
		sign = resultNumeratorSigned.IsNegative() ? -1 : 1;
		numerator = static_cast<BigInteger>(resultNumeratorSigned.Abs());
		// Denominator remains unchanged

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator-=( const BigFraction& other )
	{
		if ( this->IsNaN() || other.IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( this->IsZero() )
		{
			if ( other.IsZero() )
			{
				this->sign = 1;
				this->numerator = 0;
				this->denominator = 1;
				return *this;
			}
			else if ( other.IsNegative() )
			{
				this->sign = 1;
				this->numerator = other.numerator.Abs();
				this->denominator = other.denominator.Abs();
				return *this;
			}
			else
			{
				this->sign = -1;
				this->numerator = other.numerator.Abs();
				this->denominator = other.denominator.Abs();
				return *this;
			}
		}
		else if ( other.IsZero() )
		{
			if ( this->IsNegative() )
			{
				this->sign = -1;
				return *this;
			}
			else
			{
				this->sign = 1;
				return *this;
			}
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger a(this->numerator * other.denominator, this->sign < 0);
		BigSignedInteger b(other.numerator * this->denominator, other.sign < 0);
		BigSignedInteger resultNumeratorSigned = a - b;
		BigInteger resultDenominator = this->denominator * other.denominator;

		// Determine the sign and absolute values
		sign = resultNumeratorSigned.IsNegative() ? -1 : 1;
		numerator = static_cast<BigInteger>(resultNumeratorSigned.Abs());
		denominator = resultDenominator.Abs();

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator-=( const BigInteger& other )
	{
		if ( this->IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( this->IsZero() )
		{
			if ( other.IsZero() )
			{
				this->sign = 1;
				this->numerator = 0;
				this->denominator = 1;
				return *this;
			}
			else if ( other.IsNegative() )
			{
				this->sign = 1;
				this->numerator = other.Abs();
				this->denominator = 1;
				return *this;
			}
			else
			{
				this->sign = -1;
				this->numerator = other;
				this->denominator = 1;
				return *this;
			}
		}
		else if ( other.IsZero() )
		{
			return *this;
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger a(this->numerator, this->sign < 0);
		BigSignedInteger b(other, other.IsNegative());

		BigSignedInteger resultNumeratorSigned;
		if ((this->sign > 0 && other.IsNegative()) || (this->sign < 0 && !other.IsNegative()))
		{
			resultNumeratorSigned = a + b.Abs();
		}
		else
		{
			resultNumeratorSigned = a - b;
		}

		// Determine the sign and absolute values
		sign = resultNumeratorSigned.IsNegative() ? -1 : 1;
		numerator = static_cast<BigInteger>(resultNumeratorSigned.Abs());
		// Denominator remains unchanged

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator*=( const BigFraction& other )
	{
		if ( this->IsNaN() || other.IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( other.IsZero() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}
		if ( ( other.numerator > 0 && other.denominator > 0 ) && ( other.numerator == other.denominator ) )
		{
			return *this;
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger signed_numerator(this->numerator, this->sign < 0);
		BigSignedInteger signed_denominator(this->denominator, this->sign < 0);
		BigSignedInteger other_signed_numerator(other.numerator, other.IsNegative());
		BigSignedInteger other_signed_denominator(other.denominator, other.IsNegative());

		signed_numerator *= other_signed_numerator;
		signed_denominator *= other_signed_denominator;

		this->sign = (signed_numerator.IsNegative() != signed_denominator.IsNegative()) ? -1 : 1;
		this->numerator = static_cast<BigInteger>(signed_numerator.Abs());
		this->denominator = static_cast<BigInteger>(signed_denominator.Abs());

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator*=( const BigInteger& other )
	{
		if ( other.IsZero() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}
		if ( other == 1 )
		{
			return *this;
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger signed_numerator(this->numerator, this->sign < 0);
		BigSignedInteger signed_other(other, other.IsNegative());

		signed_numerator *= signed_other;

		this->numerator = static_cast<BigInteger>(signed_numerator.Abs());
		this->sign = signed_numerator.IsNegative() ? -1 : 1;

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator/=( const BigFraction& other )
	{
		if ( this->IsNaN() || other.IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( other.denominator.IsZero() )
		{
			throw std::runtime_error( "Division by zero error." );
		}

		// Use BigSignedInteger for intermediate calculations
		BigSignedInteger signed_numerator(this->numerator, this->sign < 0);
		BigSignedInteger signed_denominator(this->denominator, this->sign < 0);
		BigSignedInteger other_signed_numerator(other.numerator, other.IsNegative());
		BigSignedInteger other_signed_denominator(other.denominator, other.IsNegative());

		signed_numerator *= other_signed_denominator; // Multiply the numerator by the denominator of another fraction
		signed_denominator *= other_signed_numerator; // Multiply the denominator by the numerator of another fraction

		this->sign = (signed_numerator.IsNegative() != signed_denominator.IsNegative()) ? -1 : 1;
		this->numerator = static_cast<BigInteger>(signed_numerator.Abs());
		this->denominator = static_cast<BigInteger>(signed_denominator.Abs());

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction& BigFraction::operator/=( const BigInteger& other )
	{
		if ( this->IsNaN() )
		{
			this->sign = 1;
			this->numerator = 0;
			this->denominator = 1;
			return *this;
		}

		if ( other.IsZero() )
		{
			throw std::runtime_error( "Division by zero error." );
		}
		
		// 使用 BigSignedInteger 进行中间计算
		BigSignedInteger signed_numerator(this->numerator, this->sign < 0);
		BigSignedInteger signed_denominator(this->denominator, this->sign < 0);
		BigSignedInteger other_signed(other, other.IsNegative());

		signed_denominator *= other_signed;

		this->sign = (signed_numerator.IsNegative() != signed_denominator.IsNegative()) ? -1 : 1;
		this->numerator = static_cast<BigInteger>(signed_numerator.Abs());
		this->denominator = static_cast<BigInteger>(signed_denominator.Abs());

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}

		return *this;
	}

	BigFraction BigFraction::operator+( const BigFraction& other ) const
	{
		BigFraction result( *this );
		result += other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator+( const BigInteger& other ) const
	{
		BigFraction result( *this );
		result += other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator-( const BigFraction& other ) const
	{
		BigFraction result( *this );
		result -= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator-( const BigInteger& other ) const
	{
		BigFraction result( *this );
		result -= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator*( const BigFraction& other ) const
	{
		BigFraction result( *this );
		result *= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator*( const BigInteger& other ) const
	{
		BigFraction result( *this );
		result *= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator/( const BigFraction& other ) const
	{
		BigFraction result( *this );
		result /= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	BigFraction BigFraction::operator/( const BigInteger& other ) const
	{
		BigFraction result( *this );
		result /= other;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;

		return result;
	}

	bool BigFraction::operator<( const BigFraction& other ) const
	{
		BigInteger lhs = numerator * other.denominator;
		BigInteger rhs = other.numerator * denominator;
		return lhs < rhs;
	}

	bool BigFraction::operator<=( const BigFraction& other ) const
	{
		return !( *this > other );
	}

	bool BigFraction::operator>( const BigFraction& other ) const
	{
		BigInteger lhs = numerator * other.denominator;
		BigInteger rhs = other.numerator * denominator;
		return lhs > rhs;
	}

	bool BigFraction::operator>=( const BigFraction& other ) const
	{
		return !( *this < other );
	}

	bool BigFraction::operator==( const BigFraction& other ) const
	{
		return numerator == other.numerator && denominator == other.denominator && sign == other.sign;
	}

	bool BigFraction::operator!=( const BigFraction& other ) const
	{
		return !( *this == other );
	}

	BigFraction BigFraction::Abs() const
	{
		BigFraction result = BigFraction( numerator, denominator, 1 );
		
		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Reciprocal() const
	{
		BigFraction result;

		if ( IsNaN() )
		{
			result = BigFraction( 0, 0 );
		}
		else if ( IsZero() )
		{
			result = BigFraction( 0, ONE );
		}
		else
		{
			result = BigFraction( denominator, numerator, sign );
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Sqrt()
	{
		BigFraction result;

		if ( IsNaN() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( IsZero() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( IsNegative() )
		{
			result = BigFraction( 0, ONE );
		}
		else
		{
			result = BigFraction(BigInteger(numerator).Sqrt(), BigInteger(denominator).Sqrt());
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Cbrt()
	{
		BigFraction result;

		if ( IsNaN() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( IsZero() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( IsNegative() )
		{
			result = BigFraction( 0, ONE );
		}
		else
		{
			result = BigFraction(BigInteger(numerator).Cbrt(), BigInteger(denominator).Cbrt());
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	// Logarithm base e
	BigFraction BigFraction::Log( const BigInteger& value ) const
	{
		if ( value.IsZero() )
		{
			throw std::invalid_argument( "Logarithm of non-positive value is undefined." );
		}

		BigFraction result = LogCF( value );

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Log() const
	{
		if ( IsZero() || IsNegative() )
		{
			throw std::invalid_argument( "Logarithm of non-positive value is undefined." );
		}
		BigFraction numeratorLog = LogCF( GetNumerator() );
		BigFraction denominatorLog = LogCF( GetDenominator() );
		BigFraction result = numeratorLog - denominatorLog;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	// Logarithm base 10
	BigFraction BigFraction::Log10( const BigInteger& fraction ) const
	{
		static const BigFraction LOG_10_E = Log( BigFraction::TEN );
		BigFraction result = LogCF( fraction ) / LOG_10_E;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Log10() const
	{
		static const BigFraction LOG_10_E = Log( BigFraction::TEN );
		BigFraction result = Log() / LOG_10_E;

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	// Exponential function using Taylor series
	BigFraction BigFraction::Exp( const BigInteger& value ) const
	{
		BigFraction result(ONE, ONE);
		BigFraction term(ONE, ONE);
		BigInteger i = ONE;

		switch ( this->PrecisionMode )
		{
			case DecimalPrecisionMode::Fixed:
			{
				for(uint64_t round = this->FixedPrecisionCount + 1; round > 0; round--)
				{
					term = term * BigFraction(value, i);
					result += term;
					i += ONE;
				}

				break;
			}
			
			case DecimalPrecisionMode::Full:
			{
				BigFraction precision = GetFullPrecision();
				while (term > precision)
				{
					term = term * BigFraction(value, i);
					result += term;
					i += ONE;
				}

				break;
			}
			default:
				break;
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Exp( const BigFraction& value ) const
	{
		BigFraction result(ONE, ONE);
		BigFraction term(ONE, ONE);
		BigInteger i = ONE;
		

		switch ( this->PrecisionMode )
		{
			case DecimalPrecisionMode::Fixed:
			{
				for(uint64_t round = this->FixedPrecisionCount + 1; round > 0; round--)
				{
					term = term * (value / i);
					result += term;
					i += ONE;
				}

				break;
			}
			
			case DecimalPrecisionMode::Full:
			{
				BigFraction precision = GetFullPrecision();
				while (term > precision)
				{
					term = term * (value / i);
					result += term;
					i += ONE;
				}

				break;
			}
			default:
				break;
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Power( const BigInteger& exponent ) const
	{
		BigFraction result;

		if ( exponent.IsZero() )
		{
			result = BigFraction( ONE, ONE );
		}
		else if ( exponent > 0 )
		{
			BigInteger numeratorPower = numerator;
			numeratorPower.BigPower( exponent );
			BigInteger denominatorPower = denominator;
			denominatorPower.BigPower( exponent );
			result = BigFraction( numeratorPower, denominatorPower );
		}
		else
		{
			result = Reciprocal().Power( exponent.Abs() );
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction BigFraction::Power( const BigFraction& exponent ) const
	{
		BigFraction result;

		if ( IsNaN() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( IsZero() )
		{
			result = BigFraction( ONE, ONE );
		}
		else if ( exponent.IsZero() )
		{
			result = BigFraction( ONE, ONE );;
		}
		else if ( numerator.IsZero() )
		{
			result = BigFraction( 0, ONE );
		}
		else if ( !exponent.IsNaN() )
		{
			result = this->Power( exponent.Round() );
		}
		else
		{
			BigFraction newNumerator = Log( numerator ) * exponent;
			BigFraction newDenominator = Log( denominator ) * exponent;

			BigFraction resultNumerator = Exp( newNumerator );
			BigFraction resultDenominator = Exp( newDenominator );

			result = BigFraction( resultNumerator.GetNumerator(), resultDenominator.GetNumerator() );
		}

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	// Nth Root for BigFraction
	BigFraction BigFraction::NthRoot( const BigInteger& n ) const
	{
		if ( IsZero() || IsNegative() )
		{
			throw std::invalid_argument( "Nth root of zero or negative value is undefined." );
		}

		BigInteger numeratorRoot = NthRoot( numerator, n );
		BigInteger denominatorRoot = NthRoot( denominator, n );
		BigFraction result = BigFraction( numeratorRoot, denominatorRoot );

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	// Nth Root for BigFraction with fraction parameter
	BigFraction BigFraction::NthRoot( const BigFraction& fraction, const BigInteger& n ) const
	{
		if ( fraction.IsZero() || fraction.IsNegative() )
		{
			throw std::invalid_argument( "Nth root of zero or negative value is undefined." );
		}

		BigInteger numeratorRoot = NthRoot( fraction.GetNumerator(), n );
		BigInteger denominatorRoot = NthRoot( fraction.GetDenominator(), n );
		BigFraction result = BigFraction( numeratorRoot, denominatorRoot );

		result.PrecisionMode = this->PrecisionMode;
		result.FixedPrecisionCount = this->FixedPrecisionCount;
		return result;
	}

	BigFraction::BigInteger BigFraction::Floor() const
	{
		if ( IsNaN() )
		{
			return BigInteger( 0 );
		}
		else if ( IsZero() )
		{
			return BigInteger( 0 );
		}
		else
		{
			BigInteger result = ( numerator * sign ) / denominator;
			if ( sign == -1 )
			{
				result -= 1;
			}
			return result;
		}
	}

	BigFraction::BigInteger BigFraction::Ceil() const
	{
		if ( IsNaN() )
		{
			return BigInteger( 0 );
		}
		else if ( IsZero() )
		{
			return BigInteger( 0 );
		}
		else
		{
			BigInteger result = ( numerator * sign ) / denominator;
			if ( sign == 1 )
			{
				result += 1;
			}
			return result;
		}
	}

	BigFraction::BigInteger BigFraction::Round() const
	{
		if ( IsNaN() )
		{
			return BigInteger( 0 );
		}
		else if ( IsZero() )
		{
			return BigInteger( 0 );
		}
		else
		{
			BigInteger numeratorTwice = ( numerator << 1 );
			BigInteger denominatorTwice = ( denominator << 1 );
			BigInteger result = ( numeratorTwice + denominator ) / ( denominatorTwice * sign );
			return result;
		}
	}

	void BigFraction::FromDouble( double value, uint64_t maxIterations )
	{
		if ( std::isnan( value ) )
		{
			sign = 1;
			numerator = 0;
			denominator = ONE;
			return;
		}

		if ( std::isinf( value ) )
		{
			sign = value < 0 ? -1 : 1;
			denominator = 0;
			numerator = ONE;
			return;
		}

		sign = value < 0 ? -1 : 1;
		value = std::fabs( value );

		double integerPart;
		double fractionPart = std::modf( value, &integerPart );

		numerator = static_cast<BigInteger>( integerPart );
		denominator = ONE;

		uint64_t iterations = 0;
		while ( fractionPart != 0.0 && iterations < maxIterations )
		{
			fractionPart *= 10;
			double newIntPart;
			fractionPart = std::modf( fractionPart, &newIntPart );
			numerator = numerator * TEN + static_cast<BigInteger>( newIntPart );
			denominator *= TEN;
			iterations++;
		}

		if ( simplify_reduced )
		{
			ReduceSimplify();
		}
	}

	void BigFraction::FromFloat( float value, uint64_t maxIterations )
	{
		FromDouble( static_cast<double>( value ), maxIterations );
	}

	BigFraction::operator BigInteger() const
	{
		if ( IsNaN() || IsZero() )
		{
			return 0;
		}

		if ( denominator == 1 )
		{
			return numerator;
		}

		return numerator / denominator;
	}

	BigFraction::operator double() const
	{
		return sign * static_cast<double>( numerator.ToUnsignedInt() ) / static_cast<double>( denominator.ToUnsignedInt() );
	}

	BigFraction::operator float() const
	{
		return static_cast<float>( static_cast<double>( *this ) );
	}

	bool BigFraction::IsPerfectPower( const BigInteger& N )
	{
		if ( N <= 1 )
			return false;

		for ( BigInteger b = TWO; b < N.Log2() + 1; ++b )
		{
			BigFraction exponent( 1, b, 1 );
			BigFraction base( N, 1, 1 );
			BigFraction a = base.Power( exponent );

			if ( !a.IsNaN() )
			{
				return true;
			}
		}

		return false;
	}

	std::istream& operator>>( std::istream& is, BigFraction& fraction )
	{
		std::string input;
		is >> input;
		fraction.ComputeAndFromDecimalString( input );
		return is;
	}

	std::ostream& operator<<( std::ostream& os, const BigFraction& fraction )
	{
		std::string output;
		output = fraction.ComputeAndToDecimalString();
		os << output;
		return os;
	}

	void BigFraction::ReduceSimplify()
	{
		if ( this->numerator.IsZero() )
		{
			this->sign = 1;
			this->denominator = 1;
			return;
		}
		if ( this->denominator.IsZero() )
		{
			std::invalid_argument( "Denominator cannot be zero." );
		}
		this->sign = numerator.IsNegative() ? -1 : 1;
		this->numerator = this->numerator.Abs();
		BigInteger gcd = BigInteger::GCD( numerator, denominator );
		this->numerator /= gcd;
		this->denominator /= gcd;
	}

	/*
		Logarithm of a BigInteger using continued fractions
		The function uses the series expansion of the natural logarithm:
		\[
		\ln(1+x) = 2 \left( \frac{x}{1} - \frac{x^3}{3} + \frac{x^5}{5} - \cdots \right)
		\]
		The function uses \( x = \frac{value + 1}{1} \), ensuring \( |x| > 0 \).
	*/
	BigFraction BigFraction::LogCF( const BigInteger& value ) const
	{
		if (value <= 0)
		{
			throw std::invalid_argument("Logarithm is undefined for non-positive values.");
		}
		
		// Initialize x as (value - 1) / (value + 1)
		BigFraction x(value - ONE, value + ONE);
		
		// Variables to store the current and previous terms
		BigFraction result(0, ONE);
		BigFraction newTerm = x;
		BigFraction oldTerm(0, ONE);

		// Initialize variables for the loop
		BigFraction xPower = x;
		BigInteger n = 1;
		bool round_flag = true;
		
		BigFraction term(0, ONE);

		switch (this->PrecisionMode)
		{
			case DecimalPrecisionMode::Fixed:
			{
				// Main loop to calculate
				for (uint64_t round = this->FixedPrecisionCount + 1; round > 0; round--)
				{
					oldTerm = newTerm;
					n += 2;
					xPower *= x.Power(2);  // xPower *= x^2
					term = BigFraction(xPower, n);

					if (round_flag)
					{
						newTerm -= term;
					}
					else
					{
						newTerm += term;
					}

					round_flag = !round_flag;  // Toggle round_flag for the next term
				}

				break;
			}
			
			case DecimalPrecisionMode::Full:
			{
				// Set precision for the calculation
				BigFraction precision = GetFullPrecision(); // 10^-n precision (1 / n)

				BigFraction diff(x, ONE);

				// Main loop to calculate
				while (diff > precision)
				{
					oldTerm = newTerm;
					n += 2;
					xPower *= x.Power(2);  // xPower *= x^2
					term = BigFraction(xPower, n);

					if (round_flag)
					{
						newTerm -= term;
					}
					else
					{
						newTerm += term;
					}

					round_flag = !round_flag;  // Toggle round_flag for the next term

					diff = (newTerm - oldTerm).Abs();
				}

				break;
			}
			
			default:
				break;
		}

		// Scale the result by 2
		result = newTerm * BigFraction(2, 1);

		return result;
	}

	/*
		Method to find the nth root of a BigInteger using Newton-Raphson method
		x = ((n - 1) * x + value / x_power.BigPower(n - ONE)) / n;
	*/
	// Method to find the nth root of a BigInteger using Newton-Raphson method
	TwilightDream::BigInteger::BigInteger BigFraction::NthRoot(const BigInteger& value, const BigInteger& nth) const
	{
		if (value.IsZero() || value == ONE)
		{
			return BigFraction(value, ONE);
		}

		// Initial estimate x = value / nth
		BigFraction x(value, nth);

		// Set precision based on mode
		switch (this->PrecisionMode)
		{
			case DecimalPrecisionMode::Fixed:
			{
				for (uint64_t round = this->FixedPrecisionCount + 1; round > 0; --round)
				{
					// Newton-Raphson iteration formula
					BigFraction xPower = x.Power(nth - ONE);
					BigInteger numerator = (nth - ONE) * x.GetNumerator() * x.GetDenominator() + value * xPower.GetDenominator();
					BigInteger denominator = nth * xPower.GetNumerator();
					BigFraction newX(numerator, denominator);

					x = newX;
				}
				break;
			}

			case DecimalPrecisionMode::Full:
			{
				BigFraction precision = GetFullPrecision();
				while (true)
				{
					// Newton-Raphson iteration formula
					BigFraction xPower = x.Power(nth - ONE);
					BigInteger numerator = (nth - ONE) * x.GetNumerator() * x.GetDenominator() + value * xPower.GetDenominator();
					BigInteger denominator = nth * xPower.GetNumerator();
					BigFraction newX(numerator, denominator);

					if ((newX - x).Abs() < precision)
					{
						x = newX;
						break;
					}

					x = newX;
				}
				break;
			}

			default:
				throw std::invalid_argument("Unknown PrecisionMode");
		}
	}

}  // namespace TwilightDream::BigFraction