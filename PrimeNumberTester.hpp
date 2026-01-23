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

#ifndef TWILIGHT_DREAM_PRIME_NUMBER_TESTER_HPP
#define TWILIGHT_DREAM_PRIME_NUMBER_TESTER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "Lenstra-Pomerance_AKS.hpp"
#include "SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.hpp"

namespace TwilightDream
{
	struct PrimeNumberTester
	{
		using BigInteger = TwilightDream::BigInteger::BigInteger;

	private:
		/*
		 * PrimeNumberTester owns the algorithm object and delegates the public project-level call to it.  
		 * The quadratic-extension ring, Jacobi-symbol preparation, roots of unity and probabilistic round state remain inside the dedicated class instead of being mixed into this general dispatcher.
		 */
		SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnityInstance;

	public:
		/**
		 * @brief Execute Seysen's Simplified Quadratic Frobenius probable-prime test with the third-root-of-unity consistency test.
		 *
		 * This function is intentionally only a dispatcher.  The complete
		 * implementation is owned by
		 * SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity.
		 *
		 * @param Number Non-negative integer to test.
		 * @param TestingRounds Number of strengthened Frobenius rounds. It must  be at least one.
		 * @return false when compositeness is detected; true when Number passes every probable-prime condition.
		 */
		bool SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( const BigInteger& Number, std::size_t TestingRounds );

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
		bool MillerRabin( const BigInteger& n, int k );

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
		bool MillerRabinWithMontgomery( const BigInteger& n, int k );

		bool IsPrime_SlowAlgorithm(const BigInteger& Number);

		bool IsPrime_FastAlgorithm(const BigInteger& Number);

		bool IsPrime(const BigInteger& Number);
	};
}

#endif
