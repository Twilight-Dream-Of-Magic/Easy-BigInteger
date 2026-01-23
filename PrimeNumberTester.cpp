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

#include "PrimeNumberTester.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TwilightDream
{
	

	bool PrimeNumberTester::SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity(
		const BigInteger& Number,
		std::size_t TestingRounds )
	{
		return SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnityInstance.TestProbablePrimality(
			Number,
			TestingRounds );
	}

	/**
	* @brief Miller-Rabin primality test for integer `n`.
	*
	* This function applies the Miller-Rabin primality test to check if the given
	* integer `n` is likely prime. The test is repeated `k` times for increased accuracy.
	*
	* @param n The integer to test for primality.
	* @param k The number of iterations for the Miller-Rabin test.
	* @return True if `n` is likely prime, false otherwise.
	*/
	bool PrimeNumberTester::MillerRabin( const BigInteger& n, int k )
	{
		const BigInteger ZERO( 0 );
		const BigInteger ONE( 1 );
		const BigInteger TWO( 2 );

		if ( n <= ZERO )
			return false;

		if ( n.IsEven() )
			return false;

		const BigInteger n_minus_one = n - ONE;
		const BigInteger n_minus_two = n - TWO;
		BigInteger		 x = ZERO;

		bool TryAgain = false;

		// Decompose (n - 1) to write it as (2 ** s) * d
		// While d is even, divide it by 2 and increase the exponent.
		BigInteger d = n_minus_one;
		size_t	   s = 0;
		while ( d.IsEven() )
		{
			++s;
			d >>= 1;
		}

		const BigInteger RangeInteger = n_minus_two - TWO + ONE;

		// Perform Miller-Rabin test
		// Test k witnesses.
		for ( size_t i = 0; i < k; ++i )
		{
			// Generate random integer a, where 2 <= a <= (n - 2)
			const BigInteger a = BigInteger::RandomGenerateNBit( n.BitLength() + 1 );
			x = a % RangeInteger + TWO;

			x.PowerWithModulo( d, n );

			if ( x == ONE || x == n_minus_one )
			{
				continue;
			}

			for ( size_t r = 0; r < s; ++r )
			{
				x.PowerWithModulo( TWO, n );

				if ( x == ONE )
				{
					// n is composite.
					return false;
				}
				else if ( x == n_minus_one )
				{
					// Exit inner loop and continue with next witness.
					TryAgain = true;
					break;
				}
			}

			//x != n_minus_one
			if ( !TryAgain )
				return false;
			TryAgain = false;
		}

		//n is *probably* prime
		return true;
	}

	/**
	* @brief Miller-Rabin primality test with Montgomery multiplication for integer `n`.
	*
	* This function applies the Miller-Rabin primality test using Montgomery multiplication
	* to check if the given integer `n` is likely prime. The test is repeated `k` times for
	* increased accuracy.
	*
	* @param n The integer to test for primality.
	* @param k The number of iterations for the Miller-Rabin test.
	* @return True if `n` is likely prime, false otherwise.
	*/
	bool PrimeNumberTester::MillerRabinWithMontgomery( const BigInteger& n, int k )
	{
		const BigInteger ZERO( 0 );
		const BigInteger ONE( 1 );
		const BigInteger TWO( 2 );

		if ( n <= ZERO )
			return false;

		if ( n.IsEven() )
			return false;

		const BigInteger n_minus_one = n - ONE;
		BigInteger		 x = ZERO;

		auto generate_random_number_with_range = []( const BigInteger& start, const BigInteger& end ) {
			BigInteger range = end - start;
			BigInteger result = BigInteger::RandomGenerateNBit( range.BitLength() + 1 );
			return result % range + start;
		};

		// Decompose (n - 1) to write it as (2 ** s) * d
		// While d is even, divide it by 2 and increase the exponent.
		BigInteger s = n_minus_one;
		size_t	   t = s.CountTrailingZeros();
		s >>= t;

		// Perform Miller-Rabin test with Montgomery multiplication
		// Test k witnesses.

		BigInteger							  range_integer = n_minus_one;
		TwilightDream::BigInteger::Montgomery montgomery( n );
		for ( size_t count = 0; count < k; count++ )
		{
			// Generate random integer a, where 2 <= a <= (n - 2)
			BigInteger x = generate_random_number_with_range( 2, range_integer );
			x = montgomery.Power( x, s );
			if ( x != 1 )
			{
				size_t i = 0;
				while ( x != range_integer )
				{
					if ( i == t - 1 )
					{
						return false;
					}
					else
					{
						i++;
						x = x * x % n;
					}
				}
			}
		}
		return true;
	}

	/**********/

	bool PrimeNumberTester::IsPrime_SlowAlgorithm( const BigInteger& Number )
	{
		uint64_t NormalNumber = Number.ToUnsignedInt();

		if ( NormalNumber < 2 )
		{
			return false;
		}

		// Eratosthenes sieve
		std::vector<bool> SieveTable( 10240000, false );

		SieveTable[ 0 ] = SieveTable[ 1 ] = false;

		for ( uint64_t i = 3; i * i <= NormalNumber; i += 2 )
		{
			// If prime[p] is not changed, then it is a prime
			if ( SieveTable[ i / 2 ] == false )
			{
				// Update all multiples of p greater than or equal to the square of it numbers
				// which are multiple of p and are less than p^2 are already been marked.
				for ( uint64_t j = i * 3; j <= NormalNumber; j += 2 * i )
				{
					SieveTable[ j / 2 ] = true;
				}
			}
		}

		return SieveTable[ Number.ToUnsignedInt() ];
	}

	bool PrimeNumberTester::IsPrime_FastAlgorithm( const BigInteger& Number )
	{
		std::vector<BigInteger> SmallPrimes { BigInteger( 2 ), BigInteger( 3 ), BigInteger( 5 ), BigInteger( 7 ), BigInteger( 11 ) };

		// Check for small numbers.
		if ( Number < 13 )
		{
			for ( const auto& SmallPrime : SmallPrimes )
			{
				if ( Number == SmallPrime )
				{
					return true;
				}
			}
			return false;
		}

		size_t bit_size = Number.BitLength();
		/*
			Returns minimum number of rounds for Miller-Rabing primality testing, on number bitsize.

			According to NIST FIPS 186-4, Appendix C, Table C.3, minimum number of rounds of M-R testing
			using an error probability of 2 ** (-100), for different p, q bitsizes are:
			* p, q bitsize: 512; rounds: 7
			* p, q bitsize: 1024; rounds: 4
			* p, q bitsize: 1536; rounds: 3
			See: http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf
		*/
		auto primality_testing_rounds = [ &bit_size ]( const BigInteger& number ) -> size_t {
			// Set number of rounds.
			if ( bit_size >= 1536 )
				return 3;
			if ( bit_size >= 1024 )
				return 4;
			if ( bit_size >= 512 )
				return 7;
			// For smaller bitsizes, set arbitrary number of rounds.
			return 10;
		};

		if ( bit_size > 4096 )
		{
			return AKS_Test_Instance( Number );
		}
		else if ( bit_size > 2048 )
			return MillerRabinWithMontgomery( Number, primality_testing_rounds( Number ) + 1 );
		else
			return MillerRabin( Number, primality_testing_rounds( Number ) + 1 );
	}

	bool PrimeNumberTester::IsPrime( const BigInteger& Number )
	{
		if ( Number < 10240000 )
		{
			return IsPrime_SlowAlgorithm( Number );
		}
		else
		{
			return IsPrime_FastAlgorithm( Number );
		}
	}
}  // namespace TwilightDream
