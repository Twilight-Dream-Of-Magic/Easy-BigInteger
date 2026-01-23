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

#include "SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace TwilightDream
{
	/*
	 * =====================================================================
	 * Simplified Quadratic Frobenius test with third roots of unity
	 * =====================================================================
	 *
	 * This file implements Algorithm MR2, Algorithm SQFT3round and Algorithm
	 * SQFT3 from Martin Seysen's IACR ePrint 2005/462 paper.  Every helper is a
	 * private, non-static member of the dedicated algorithm class.  The generic
	 * PrimeNumberTester class contains only a delegating project-level entry
	 * point; it does not own this quadratic-extension arithmetic.
	 * =====================================================================
	 */

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateUnsignedRemainderWithoutDivision( const BigInteger& value, const BigInteger& modulo )
	{
		if ( modulo.IsZero() )
		{
			throw std::invalid_argument( "CalculateUnsignedRemainderWithoutDivision: modulo cannot be zero." );
		}
		if ( value < modulo )
		{
			return value;
		}

		// Binary long reduction.  This is intentionally independent of the
		// library's general DivideModulo path: the quadratic Frobenius test repeatedly reduces 2k-bit
		// products modulo k-bit values, and this route stays correct even when
		// no native double-width division is available.
		BigInteger	 remainder( 0 );
		const size_t bit_length = value.BitLength();
		for ( size_t bit_index = bit_length; bit_index > 0; --bit_index )
		{
			remainder += remainder;
			if ( remainder >= modulo )
			{
				remainder -= modulo;
			}

			if ( value.GetBit( bit_index - 1 ) )
			{
				remainder += BigInteger( 1 );
				if ( remainder >= modulo )
				{
					remainder -= modulo;
				}
			}
		}
		return remainder;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateSignedRemainderWithoutDivision( const BigSignedInteger& value, const BigInteger& modulo )
	{
		if ( modulo.IsZero() )
		{
			throw std::invalid_argument( "CalculateSignedRemainderWithoutDivision: modulo cannot be zero." );
		}

		BigInteger magnitude = CalculateUnsignedRemainderWithoutDivision( static_cast<BigInteger>( value ), modulo );
		if ( value.IsNegative() && !magnitude.IsZero() )
		{
			return modulo - magnitude;
		}
		return magnitude;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateModularSum( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
	{
		const BigInteger reduced_left = CalculateUnsignedRemainderWithoutDivision( left, modulo );
		const BigInteger reduced_right = CalculateUnsignedRemainderWithoutDivision( right, modulo );
		BigInteger		 result = reduced_left + reduced_right;
		if ( result >= modulo )
		{
			result -= modulo;
		}
		return result;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateModularDifference( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
	{
		const BigInteger reduced_left = CalculateUnsignedRemainderWithoutDivision( left, modulo );
		const BigInteger reduced_right = CalculateUnsignedRemainderWithoutDivision( right, modulo );
		if ( reduced_left >= reduced_right )
		{
			return reduced_left - reduced_right;
		}
		return modulo - ( reduced_right - reduced_left );
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateModularAdditiveInverse( const BigInteger& value, const BigInteger& modulo )
	{
		const BigInteger reduced_value = CalculateUnsignedRemainderWithoutDivision( value, modulo );
		if ( reduced_value.IsZero() )
		{
			return BigInteger( 0 );
		}
		return modulo - reduced_value;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateModularProductWithoutDivision( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
	{
		BigInteger		 result( 0 );
		BigInteger		 addend = CalculateUnsignedRemainderWithoutDivision( left, modulo );
		const BigInteger multiplier = CalculateUnsignedRemainderWithoutDivision( right, modulo );

		auto add_canonical = [ & ]( const BigInteger& first, const BigInteger& second ) {
			BigInteger sum = first + second;
			if ( sum >= modulo )
			{
				sum -= modulo;
			}
			return sum;
		};

		const size_t multiplier_bits = multiplier.BitLength();
		for ( size_t bit_index = 0; bit_index < multiplier_bits; ++bit_index )
		{
			if ( multiplier.GetBit( bit_index ) )
			{
				result = add_canonical( result, addend );
			}
			if ( bit_index + 1 < multiplier_bits )
			{
				addend = add_canonical( addend, addend );
			}
		}
		return result;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateModularPowerWithoutDivision( const BigInteger& base, const BigInteger& exponent, const BigInteger& modulo )
	{
		BigInteger	 result( 1 );
		BigInteger	 current_power = CalculateUnsignedRemainderWithoutDivision( base, modulo );
		const size_t exponent_bits = exponent.BitLength();

		for ( size_t bit_index = 0; bit_index < exponent_bits; ++bit_index )
		{
			if ( exponent.GetBit( bit_index ) )
			{
				result = CalculateModularProductWithoutDivision( result, current_power, modulo );
			}

			if ( bit_index + 1 < exponent_bits )
			{
				current_power = CalculateModularProductWithoutDivision( current_power, current_power, modulo );
			}
		}

		return result;
	}

	int SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateJacobiSymbol( const BigSignedInteger& numerator, const BigInteger& odd_denominator )
	{
		const BigInteger ZERO( 0 );
		const BigInteger ONE( 1 );

		if ( odd_denominator <= ONE || odd_denominator.IsEven() )
		{
			throw std::invalid_argument( "CalculateJacobiSymbol: denominator must be an odd integer greater than one." );
		}

		BigInteger denominator = odd_denominator;
		BigInteger value = CalculateUnsignedRemainderWithoutDivision( static_cast<BigInteger>( numerator ), denominator );
		int		   symbol = 1;

		// (-1/n) = -1 exactly when n == 3 (mod 4).
		if ( numerator.IsNegative() && ( denominator.ToUnsignedInt() & 3ULL ) == 3ULL )
		{
			symbol = -symbol;
		}

		while ( value != ZERO )
		{
			while ( value.IsEven() )
			{
				value >>= 1;
				const uint64_t denominator_modulo_eight = denominator.ToUnsignedInt() & 7ULL;
				if ( denominator_modulo_eight == 3ULL || denominator_modulo_eight == 5ULL )
				{
					symbol = -symbol;
				}
			}

			if ( value == ONE )
			{
				return symbol;
			}

			if ( ( value.ToUnsignedInt() & 3ULL ) == 3ULL && ( denominator.ToUnsignedInt() & 3ULL ) == 3ULL )
			{
				symbol = -symbol;
			}

			BigInteger remainder = CalculateUnsignedRemainderWithoutDivision( denominator, value );
			denominator = std::move( value );
			value = std::move( remainder );
		}

		return denominator == ONE ? symbol : 0;
	}

	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::IsPerfectSquare( const BigInteger& number )
	{
		if ( number.IsZero() )
		{
			return true;
		}

		// Restoring binary square root; uses only shifts, comparisons and
		// additions/subtractions, so it does not depend on general division.
		BigInteger	 remainder = number;
		BigInteger	 root( 0 );
		const size_t highest_even_bit = ( number.BitLength() - 1 ) & ~size_t( 1 );
		BigInteger	 bit = BigInteger( 1 ).LeftShiftBit( highest_even_bit );

		while ( !bit.IsZero() )
		{
			const BigInteger trial = root + bit;
			if ( remainder >= trial )
			{
				remainder -= trial;
				root = ( root >> 1 ) + bit;
			}
			else
			{
				root >>= 1;
			}
			bit >>= 2;
		}

		return root * root == number;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::GenerateUniformRandomIntegerBelow( const BigInteger& upper_exclusive )
	{
		const BigInteger ZERO( 0 );
		const BigInteger ONE( 1 );

		if ( upper_exclusive <= ONE )
		{
			return ZERO;
		}

		const BigInteger maximum_value = upper_exclusive - ONE;
		const size_t	 bit_length = maximum_value.BitLength();
		const size_t	 byte_count = ( bit_length + 7 ) / 8;
		const size_t	 unused_high_bits = byte_count * 8 - bit_length;

		thread_local std::mt19937_64 generator( std::random_device {}() );

		for ( ;; )
		{
			std::vector<uint8_t> random_bytes( byte_count, 0 );
			uint64_t			 random_word = 0;
			size_t				 bytes_left_in_word = 0;

			for ( size_t index = 0; index < byte_count; ++index )
			{
				if ( bytes_left_in_word == 0 )
				{
					random_word = generator();
					bytes_left_in_word = sizeof( random_word );
				}

				random_bytes[ index ] = static_cast<uint8_t>( random_word & 0xffU );
				random_word >>= 8;
				--bytes_left_in_word;
			}

			if ( unused_high_bits != 0 )
			{
				const uint8_t mask = static_cast<uint8_t>( 0xffU >> unused_high_bits );
				random_bytes.back() &= mask;
			}

			BigInteger candidate;
			candidate.ImportData( std::move( random_bytes ), false );
			if ( candidate < upper_exclusive )
			{
				return candidate;
			}
		}
	}


	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::AddQuadraticExtensionRingElements( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right, const BigInteger& modulo )
	{
		return QuadraticExtensionRingElement { CalculateModularSum( left.x_coefficient, right.x_coefficient, modulo ), CalculateModularSum( left.constant_coefficient, right.constant_coefficient, modulo ) };
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::MultiplyQuadraticExtensionRingElements( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo )
	{
		// (a*x+b)(d*x+e) = (a*e+b*d)x + (a*d*c+b*e), x^2=c.
		const BigInteger x_term_left = CalculateModularProductWithoutDivision( left.x_coefficient, right.constant_coefficient, modulo );
		const BigInteger x_term_right = CalculateModularProductWithoutDivision( left.constant_coefficient, right.x_coefficient, modulo );

		const BigInteger constant_from_x = CalculateModularProductWithoutDivision( CalculateModularProductWithoutDivision( left.x_coefficient, right.x_coefficient, modulo ), quadratic_nonresidue_modulo_number, modulo );
		const BigInteger constant_direct = CalculateModularProductWithoutDivision( left.constant_coefficient, right.constant_coefficient, modulo );

		return QuadraticExtensionRingElement { CalculateModularSum( x_term_left, x_term_right, modulo ), CalculateModularSum( constant_from_x, constant_direct, modulo ) };
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::SquareQuadraticExtensionRingElement( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo )
	{
		// (a*x+b)^2 = 2ab*x + (a^2*c+b^2).
		const BigInteger product_ab = CalculateModularProductWithoutDivision( value.x_coefficient, value.constant_coefficient, modulo );
		const BigInteger x_coefficient = CalculateModularSum( product_ab, product_ab, modulo );
		const BigInteger a_squared_c = CalculateModularProductWithoutDivision( CalculateModularProductWithoutDivision( value.x_coefficient, value.x_coefficient, modulo ), quadratic_nonresidue_modulo_number, modulo );
		const BigInteger b_squared = CalculateModularProductWithoutDivision( value.constant_coefficient, value.constant_coefficient, modulo );

		return QuadraticExtensionRingElement { x_coefficient, CalculateModularSum( a_squared_c, b_squared, modulo ) };
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::RaiseQuadraticExtensionRingElementToPower( QuadraticExtensionRingElement base, const BigInteger& exponent, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo )
	{
		QuadraticExtensionRingElement result { BigInteger( 0 ), BigInteger( 1 ) };
		const size_t				  exponent_bits = exponent.BitLength();

		for ( size_t bit_index = 0; bit_index < exponent_bits; ++bit_index )
		{
			if ( exponent.GetBit( bit_index ) )
			{
				result = MultiplyQuadraticExtensionRingElements( result, base, quadratic_nonresidue_modulo_number, modulo );
			}

			if ( bit_index + 1 < exponent_bits )
			{
				base = SquareQuadraticExtensionRingElement( base, quadratic_nonresidue_modulo_number, modulo );
			}
		}

		return result;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateQuadraticExtensionRingElementConjugate( const QuadraticExtensionRingElement& value, const BigInteger& modulo )
	{
		// For z=a*x+b, conjugation sends x to -x: conjugate(z)=-a*x+b.
		return QuadraticExtensionRingElement { CalculateModularAdditiveInverse( value.x_coefficient, modulo ), value.constant_coefficient };
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElement SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateQuadraticExtensionRingElementAdditiveInverse( const QuadraticExtensionRingElement& value, const BigInteger& modulo )
	{
		return QuadraticExtensionRingElement { CalculateModularAdditiveInverse( value.x_coefficient, modulo ), CalculateModularAdditiveInverse( value.constant_coefficient, modulo ) };
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::BigInteger SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::CalculateQuadraticExtensionRingElementNorm( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo )
	{
		// N(a*x+b) = (a*x+b)(-a*x+b) = b^2-c*a^2.
		const BigInteger b_squared = CalculateModularProductWithoutDivision( value.constant_coefficient, value.constant_coefficient, modulo );
		const BigInteger c_a_squared = CalculateModularProductWithoutDivision( quadratic_nonresidue_modulo_number, CalculateModularProductWithoutDivision( value.x_coefficient, value.x_coefficient, modulo ), modulo );
		return CalculateModularDifference( b_squared, c_a_squared, modulo );
	}

	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::AreQuadraticExtensionRingElementsEqual( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right )
	{
		return left.x_coefficient == right.x_coefficient && left.constant_coefficient == right.constant_coefficient;
	}


	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::IsQuadraticExtensionRingMultiplicativeIdentity( const QuadraticExtensionRingElement& value )
	{
		return value.x_coefficient.IsZero() && value.constant_coefficient == BigInteger( 1 );
	}

	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::DoesQuadraticExtensionRingElementSatisfyThirdCyclotomicPolynomial( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo )
	{
		/*
		 * A non-trivial third root of unity is exactly a root of
		 *
		 *     Phi_3(X) = X^2 + X + 1.
		 *
		 * Evaluating the polynomial inside the quadratic extension ring is stronger
		 * than checking value^3=1 alone: it excludes the trivial root value=1 and
		 * preserves the exact condition used by the strengthened test.
		 */
		const QuadraticExtensionRingElement value_squared = SquareQuadraticExtensionRingElement( value, quadratic_nonresidue_modulo_number, modulo );
		QuadraticExtensionRingElement		polynomial_value = AddQuadraticExtensionRingElements( value_squared, value, modulo );
		polynomial_value.constant_coefficient = CalculateModularSum( polynomial_value.constant_coefficient, BigInteger( 1 ), modulo );
		return polynomial_value.x_coefficient.IsZero() && polynomial_value.constant_coefficient.IsZero();
	}

	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::PerformMillerRabinBaseTwoOrSmallQuadraticNonresiduePretest( const BigInteger& number, SimplifiedQuadraticFrobeniusPrecomputation& precomputation )
	{
		const BigInteger ZERO( 0 );
		const BigInteger ONE( 1 );
		const BigInteger TWO( 2 );
		const BigInteger THREE( 3 );
		const BigInteger FOUR( 4 );
		const BigInteger MINUS_ONE_MODULO_NUMBER = number - ONE;

		const uint64_t number_modulo_eight = number.ToUnsignedInt() & 7ULL;

		if ( number_modulo_eight == 3 || number_modulo_eight == 7 )
		{
			// n == 3 (mod 4): alpha = 2^((n-3)/4), c=-1,
			// epsilon = alpha + alpha*x.
			const BigInteger exponent = ( number - THREE ) >> 2;
			const BigInteger alpha = CalculateModularPowerWithoutDivision( TWO, exponent, number );
			const BigInteger twice_alpha_squared = CalculateModularSum( CalculateModularProductWithoutDivision( alpha, alpha, number ), CalculateModularProductWithoutDivision( alpha, alpha, number ), number );

			if ( twice_alpha_squared != ONE && twice_alpha_squared != MINUS_ONE_MODULO_NUMBER )
			{
				return false;
			}

			precomputation.signed_quadratic_nonresidue = BigSignedInteger( -1 );
			precomputation.quadratic_nonresidue_modulo_number = CalculateSignedRemainderWithoutDivision( precomputation.signed_quadratic_nonresidue, number );
			precomputation.primitive_eighth_root_of_unity = QuadraticExtensionRingElement { alpha, alpha };
		}
		else if ( number_modulo_eight == 5 )
		{
			// n == 5 (mod 8): alpha = 2^((n-1)/4), c=2,
			// epsilon = ((1+alpha)/2)*x.
			const BigInteger exponent = ( number - ONE ) >> 2;
			const BigInteger alpha = CalculateModularPowerWithoutDivision( TWO, exponent, number );
			const BigInteger alpha_squared = CalculateModularProductWithoutDivision( alpha, alpha, number );
			if ( alpha_squared != MINUS_ONE_MODULO_NUMBER )
			{
				return false;
			}

			const BigInteger inverse_of_two = ( number + ONE ) >> 1;
			const BigInteger epsilon_x_coefficient = CalculateModularProductWithoutDivision( CalculateModularSum( ONE, alpha, number ), inverse_of_two, number );

			precomputation.signed_quadratic_nonresidue = BigSignedInteger( 2 );
			precomputation.quadratic_nonresidue_modulo_number = TWO;
			precomputation.primitive_eighth_root_of_unity = QuadraticExtensionRingElement { epsilon_x_coefficient, ZERO };
		}
		else if ( number_modulo_eight == 1 )
		{
			// Every odd square is 1 modulo 8.  Rejecting it is necessary because
			// a square modulus has no Jacobi symbol -1 among its units.
			if ( IsPerfectSquare( number ) )
			{
				return false;
			}

			BigInteger selected_c;
			bool	   found_nonresidue = false;

			auto inspect_candidate = [ & ]( const BigInteger& candidate ) -> bool {
				const int jacobi = CalculateJacobiSymbol( BigSignedInteger( candidate ), number );
				if ( jacobi == -1 )
				{
					selected_c = candidate;
					found_nonresidue = true;
					return true;
				}
				if ( jacobi == 0 )
				{
					// candidate is in [2,n), so Jacobi zero directly proves that
					// 1 < gcd(candidate,n) < n without running general division.
					return true;
				}
				return false;
			};

			// Prefer a small random c, as specified by the preparatory algorithm in the paper, then fall back to the
			// full residue interval if no small non-residue was sampled quickly.
			const BigInteger SMALL_PARAMETER_LIMIT( 65537 );
			const BigInteger small_upper = number < SMALL_PARAMETER_LIMIT ? number : SMALL_PARAMETER_LIMIT;
			const BigInteger small_range = small_upper - TWO;

			for ( size_t attempt = 0; attempt < 256 && !found_nonresidue; ++attempt )
			{
				const BigInteger candidate = GenerateUniformRandomIntegerBelow( small_range ) + TWO;
				if ( inspect_candidate( candidate ) && !found_nonresidue )
				{
					return false;
				}
			}

			const BigInteger full_range = number - TWO;
			for ( size_t attempt = 0; attempt < 8192 && !found_nonresidue; ++attempt )
			{
				const BigInteger candidate = GenerateUniformRandomIntegerBelow( full_range ) + TWO;
				if ( inspect_candidate( candidate ) && !found_nonresidue )
				{
					return false;
				}
			}

			if ( !found_nonresidue )
			{
				throw std::runtime_error( "PerformMillerRabinBaseTwoOrSmallQuadraticNonresiduePretest: unable to sample a Jacobi-symbol-minus-one quadratic parameter." );
			}

			precomputation.signed_quadratic_nonresidue = BigSignedInteger( selected_c );
			precomputation.quadratic_nonresidue_modulo_number = selected_c;

			const BigInteger exponent = ( number - ONE ) >> 3;
			const BigInteger alpha = CalculateModularPowerWithoutDivision( precomputation.quadratic_nonresidue_modulo_number, exponent, number );
			const BigInteger alpha_to_four = CalculateModularPowerWithoutDivision( alpha, FOUR, number );
			if ( alpha_to_four != MINUS_ONE_MODULO_NUMBER )
			{
				return false;
			}

			precomputation.primitive_eighth_root_of_unity = QuadraticExtensionRingElement { ZERO, alpha };
		}
		else
		{
			// The public entry point filters even inputs, so this is defensive only.
			return false;
		}

		if ( CalculateJacobiSymbol( precomputation.signed_quadratic_nonresidue, number ) != -1 )
		{
			return false;
		}

		const QuadraticExtensionRingElement epsilon_to_four = RaiseQuadraticExtensionRingElementToPower( precomputation.primitive_eighth_root_of_unity, FOUR, precomputation.quadratic_nonresidue_modulo_number, number );
		const QuadraticExtensionRingElement minus_one { ZERO, MINUS_ONE_MODULO_NUMBER };
		if ( !AreQuadraticExtensionRingElementsEqual( epsilon_to_four, minus_one ) )
		{
			return false;
		}

		precomputation.primitive_eighth_root_of_unity_cubed = MultiplyQuadraticExtensionRingElements( SquareQuadraticExtensionRingElement( precomputation.primitive_eighth_root_of_unity, precomputation.quadratic_nonresidue_modulo_number, number ), precomputation.primitive_eighth_root_of_unity, precomputation.quadratic_nonresidue_modulo_number, number );
		return true;
	}

	SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::QuadraticExtensionRingElementSamplingResult SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::SampleQuadraticExtensionRingElementWithQuadraticNonresidueNorm( const BigInteger& number, const BigInteger& quadratic_nonresidue_modulo_number, QuadraticExtensionRingElement& sampled_element )
	{

		// Rejection sampling makes z uniform over the subset satisfying
		// Jacobi(N(z),n)=-1.  For a prime n, approximately half of all z qualify.
		for ( size_t attempt = 0; attempt < 8192; ++attempt )
		{
			QuadraticExtensionRingElement candidate { GenerateUniformRandomIntegerBelow( number ), GenerateUniformRandomIntegerBelow( number ) };

			const BigInteger norm = CalculateQuadraticExtensionRingElementNorm( candidate, quadratic_nonresidue_modulo_number, number );
			const int		 jacobi = CalculateJacobiSymbol( BigSignedInteger( norm ), number );
			if ( jacobi == -1 )
			{
				sampled_element = std::move( candidate );
				return QuadraticExtensionRingElementSamplingResult::SuccessfullySampled;
			}

			if ( jacobi == 0 && !norm.IsZero() )
			{
				// 0 < norm < n, hence Jacobi zero exposes a non-trivial gcd.
				return QuadraticExtensionRingElementSamplingResult::CompositeNumberDetected;
			}
		}

		throw std::runtime_error( "SampleQuadraticExtensionRingElementWithQuadraticNonresidueNorm: unable to sample an element whose norm has Jacobi symbol minus one." );
	}


	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::PerformSimplifiedQuadraticFrobeniusRoundWithThirdRootsOfUnity( const BigInteger& number, const SimplifiedQuadraticFrobeniusPrecomputation& precomputation, QuadraticExtensionRingElement& remembered_third_root_of_unity )
	{
		const BigInteger ONE( 1 );

		QuadraticExtensionRingElement sampled_element;
		if ( SampleQuadraticExtensionRingElementWithQuadraticNonresidueNorm( number, precomputation.quadratic_nonresidue_modulo_number, sampled_element ) == QuadraticExtensionRingElementSamplingResult::CompositeNumberDetected )
		{
			return false;
		}

		/*
		 * Frobenius condition.
		 *
		 * For prime n and Jacobi(c/n)=-1, x behaves as an element of the quadratic
		 * field extension and x^n=-x.  Therefore every z=a*x+b must satisfy
		 *
		 *     z^n = -a*x+b = conjugate(z).
		 */
		const QuadraticExtensionRingElement sampled_element_to_number = RaiseQuadraticExtensionRingElementToPower( sampled_element, number, precomputation.quadratic_nonresidue_modulo_number, number );
		const QuadraticExtensionRingElement sampled_element_conjugate = CalculateQuadraticExtensionRingElementConjugate( sampled_element, number );
		if ( !AreQuadraticExtensionRingElementsEqual( sampled_element_to_number, sampled_element_conjugate ) )
		{
			return false;
		}

		/*
		 * Two-primary component condition.
		 *
		 * The norm constraint forces the (n^2-1)/8 power into the four-element set
		 * {+epsilon,-epsilon,+epsilon^3,-epsilon^3} for every prime input.
		 */
		BigInteger							extension_group_order = number * number - ONE;
		BigInteger							eighth_root_exponent = extension_group_order >> 3;
		const QuadraticExtensionRingElement eighth_root_candidate = RaiseQuadraticExtensionRingElementToPower( sampled_element, eighth_root_exponent, precomputation.quadratic_nonresidue_modulo_number, number );
		const QuadraticExtensionRingElement negative_primitive_eighth_root = CalculateQuadraticExtensionRingElementAdditiveInverse( precomputation.primitive_eighth_root_of_unity, number );
		const QuadraticExtensionRingElement negative_primitive_eighth_root_cubed = CalculateQuadraticExtensionRingElementAdditiveInverse( precomputation.primitive_eighth_root_of_unity_cubed, number );

		if ( !AreQuadraticExtensionRingElementsEqual( eighth_root_candidate, precomputation.primitive_eighth_root_of_unity ) && !AreQuadraticExtensionRingElementsEqual( eighth_root_candidate, negative_primitive_eighth_root ) && !AreQuadraticExtensionRingElementsEqual( eighth_root_candidate, precomputation.primitive_eighth_root_of_unity_cubed ) && !AreQuadraticExtensionRingElementsEqual( eighth_root_candidate, negative_primitive_eighth_root_cubed ) )
		{
			return false;
		}

		/*
		 * Three-primary component condition.
		 *
		 * Write n^2-1 = 3^u * r with gcd(r,3)=1.  Starting from z^r, repeated
		 * cubing finds the least i for which z^(3^i r)=1.  If i>0, then
		 * z^(3^(i-1)r) is a non-trivial third root of unity.
		 */
		BigInteger exponent_without_factors_of_three = extension_group_order;
		size_t	   three_adic_valuation = 0;
		for ( ;; )
		{
			BigInteger quotient = exponent_without_factors_of_three;
			if ( quotient.DividModuloNumber( 3 ) != 0 )
			{
				break;
			}
			exponent_without_factors_of_three = std::move( quotient );
			++three_adic_valuation;
		}

		if ( three_adic_valuation == 0 )
		{
			// The public driver has already established gcd(n,6)=1, so n^2-1 must
			// be divisible by three.  Reaching this branch means an arithmetic
			// invariant in the BigInteger backend was violated.
			throw std::runtime_error( "PerformSimplifiedQuadraticFrobeniusRoundWithThirdRootsOfUnity: n squared minus one was not divisible by three." );
		}

		QuadraticExtensionRingElement current_three_primary_component = RaiseQuadraticExtensionRingElementToPower( sampled_element, exponent_without_factors_of_three, precomputation.quadratic_nonresidue_modulo_number, number );

		// The minimum i equals zero.  This round supplies no new non-trivial root,
		// so the remembered cross-round root remains unchanged.
		if ( IsQuadraticExtensionRingMultiplicativeIdentity( current_three_primary_component ) )
		{
			return true;
		}

		QuadraticExtensionRingElement newly_extracted_third_root_of_unity;
		bool						  extracted_nontrivial_third_root = false;
		for ( size_t exponent_level = 1; exponent_level <= three_adic_valuation; ++exponent_level )
		{
			const QuadraticExtensionRingElement cubed_component = MultiplyQuadraticExtensionRingElements( SquareQuadraticExtensionRingElement( current_three_primary_component, precomputation.quadratic_nonresidue_modulo_number, number ), current_three_primary_component, precomputation.quadratic_nonresidue_modulo_number, number );

			if ( IsQuadraticExtensionRingMultiplicativeIdentity( cubed_component ) )
			{
				newly_extracted_third_root_of_unity = current_three_primary_component;
				extracted_nontrivial_third_root = true;
				break;
			}
			current_three_primary_component = cubed_component;
		}

		if ( !extracted_nontrivial_third_root )
		{
			/*
			 * The Frobenius equality already implies z^(n^2-1)=1 for a valid
			 * extension-ring element.  Consequently the least exponent level must
			 * exist.  Failure here is therefore a deterministic rejection.
			 */
			return false;
		}

		if ( IsQuadraticExtensionRingMultiplicativeIdentity( remembered_third_root_of_unity ) )
		{
			if ( !DoesQuadraticExtensionRingElementSatisfyThirdCyclotomicPolynomial( newly_extracted_third_root_of_unity, precomputation.quadratic_nonresidue_modulo_number, number ) )
			{
				return false;
			}
		}
		else
		{
			/*
			 * For a non-trivial third root omega, omega^(-1)=omega^2.  Once one
			 * round has fixed the order-three subgroup, every later non-trivial root
			 * must be either omega or omega^(-1).
			 */
			const QuadraticExtensionRingElement inverse_of_remembered_third_root = SquareQuadraticExtensionRingElement( remembered_third_root_of_unity, precomputation.quadratic_nonresidue_modulo_number, number );
			if ( !AreQuadraticExtensionRingElementsEqual( newly_extracted_third_root_of_unity, remembered_third_root_of_unity ) && !AreQuadraticExtensionRingElementsEqual( newly_extracted_third_root_of_unity, inverse_of_remembered_third_root ) )
			{
				return false;
			}
		}

		remembered_third_root_of_unity = std::move( newly_extracted_third_root_of_unity );
		return true;
	}

	bool SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::TestProbablePrimality( const BigInteger& Number, size_t TestingRounds )
	{
		if ( TestingRounds == 0 )
		{
			throw std::invalid_argument( "SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity::TestProbablePrimality: TestingRounds must be at least one." );
		}

		const BigInteger TWO( 2 );
		if ( Number < TWO )
		{
			return false;
		}

		/*
		 * The strengthened driver in the paper assumes n>200 and first removes
		 * every prime divisor below 200.  Equality is handled explicitly so this
		 * public API remains correct for the complete non-negative input domain.
		 */
		constexpr std::array<std::uint64_t, 46> PRIME_NUMBERS_BELOW_TWO_HUNDRED 
		{
			2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199
		};

		for ( const std::uint64_t prime_number : PRIME_NUMBERS_BELOW_TWO_HUNDRED )
		{
			const BigInteger prime( prime_number );
			if ( Number == prime )
			{
				return true;
			}

			BigInteger remainder_source = Number;
			if ( remainder_source.DividModuloNumber( prime_number ) == 0 )
			{
				return false;
			}
		}

		SimplifiedQuadraticFrobeniusPrecomputation precomputation;
		if ( !PerformMillerRabinBaseTwoOrSmallQuadraticNonresiduePretest( Number, precomputation ) )
		{
			return false;
		}

		QuadraticExtensionRingElement remembered_third_root_of_unity { BigInteger( 0 ), BigInteger( 1 ) };
		for ( size_t round_index = 0; round_index < TestingRounds; ++round_index )
		{
			if ( !PerformSimplifiedQuadraticFrobeniusRoundWithThirdRootsOfUnity( Number, precomputation, remembered_third_root_of_unity ) )
			{
				return false;
			}
		}

		return true;
	}
}  // namespace TwilightDream
