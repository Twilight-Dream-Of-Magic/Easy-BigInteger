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

#ifndef TWILIGHT_DREAM_SIMPLIFIED_QUADRATIC_FROBENIUS_PRIMALITY_TESTER_WITH_THIRD_ROOTS_OF_UNITY_HPP
#define TWILIGHT_DREAM_SIMPLIFIED_QUADRATIC_FROBENIUS_PRIMALITY_TESTER_WITH_THIRD_ROOTS_OF_UNITY_HPP

#include "BigInteger.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace TwilightDream
{
	/**
	 * @brief Seysen's Simplified Quadratic Frobenius probable-prime test with
	 *        the additional third-root-of-unity consistency test.
	 *
	 * Primary reference:
	 *
	 *     Martin Seysen,
	 *     "A Simplified Quadratic Frobenius Primality Test",
	 *     IACR Cryptology ePrint Archive, Report 2005/462, 2005.
	 *
	 * This class owns the complete algorithm described by Algorithm MR2,
	 * Algorithm SQFT3round and Algorithm SQFT3 in the paper.  It is deliberately
	 * separate from PrimeNumberTester: PrimeNumberTester only exposes the public
	 * project-level entry point and delegates the actual mathematics to this
	 * algorithm object.
	 *
	 * The implementation uses the quadratic extension ring
	 *
	 *     R(n,c) = (Z/nZ)[x] / (x^2-c)
	 *
	 * and performs the following proof-obligation chain:
	 *
	 *     1. trial division by every prime smaller than 200;
	 *     2. the paper's Miller-Rabin basis-two or small-nonresidue preparation;
	 *     3. construction and verification of a primitive eighth root of unity;
	 *     4. random sampling constrained by Jacobi(N(z),n)=-1;
	 *     5. the Frobenius identity z^n=conjugate(z);
	 *     6. the eighth-root membership condition;
	 *     7. extraction of a non-trivial third root of unity when one appears;
	 *     8. cross-round consistency with the remembered third root or its inverse.
	 *
	 * A false return value means that one of these checks proved compositeness.
	 * A true return value means probable prime; this is not a deterministic proof.
	 */
	class SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity
	{
	private:
		using BigInteger = TwilightDream::BigInteger::BigInteger;
		using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

		/**
		 * @brief One canonical element a*x+b of R(n,c).
		 *
		 * Both coefficients are always stored in the interval [0,n).  The ring
		 * modulus n and the quadratic parameter c are supplied explicitly to every
		 * arithmetic operation, so elements cannot silently migrate between rings.
		 */
		struct QuadraticExtensionRingElement
		{
			BigInteger x_coefficient;
			BigInteger constant_coefficient;
		};

		/**
		 * @brief Values produced by the preparatory Miller-Rabin/nonresidue stage.
		 *
		 * `signed_quadratic_nonresidue` preserves the mathematical integer c used by
		 * the Jacobi symbol.  `quadratic_nonresidue_modulo_number` is its canonical
		 * residue in Z/nZ.  The cached eighth roots avoid recomputing epsilon^3 in
		 * every probabilistic round.
		 */
		struct SimplifiedQuadraticFrobeniusPrecomputation
		{
			BigSignedInteger			  signed_quadratic_nonresidue;
			BigInteger					  quadratic_nonresidue_modulo_number;
			QuadraticExtensionRingElement primitive_eighth_root_of_unity;
			QuadraticExtensionRingElement primitive_eighth_root_of_unity_cubed;
		};

		enum class QuadraticExtensionRingElementSamplingResult : std::uint8_t
		{
			SuccessfullySampled,
			CompositeNumberDetected
		};

		BigInteger CalculateUnsignedRemainderWithoutDivision( const BigInteger& value, const BigInteger& modulo );

		BigInteger CalculateSignedRemainderWithoutDivision( const BigSignedInteger& value, const BigInteger& modulo );

		BigInteger CalculateModularSum( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

		BigInteger CalculateModularDifference( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

		BigInteger CalculateModularAdditiveInverse( const BigInteger& value, const BigInteger& modulo );

		BigInteger CalculateModularProductWithoutDivision( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

		BigInteger CalculateModularPowerWithoutDivision( const BigInteger& base, const BigInteger& exponent, const BigInteger& modulo );

		int CalculateJacobiSymbol( const BigSignedInteger& numerator, const BigInteger& odd_denominator );

		bool IsPerfectSquare( const BigInteger& number );

		BigInteger GenerateUniformRandomIntegerBelow( const BigInteger& upper_exclusive );

		QuadraticExtensionRingElement AddQuadraticExtensionRingElements( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right, const BigInteger& modulo );

		QuadraticExtensionRingElement MultiplyQuadraticExtensionRingElements( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo );

		QuadraticExtensionRingElement SquareQuadraticExtensionRingElement( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo );

		QuadraticExtensionRingElement RaiseQuadraticExtensionRingElementToPower( QuadraticExtensionRingElement base, const BigInteger& exponent, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo );

		QuadraticExtensionRingElement CalculateQuadraticExtensionRingElementConjugate( const QuadraticExtensionRingElement& value, const BigInteger& modulo );

		QuadraticExtensionRingElement CalculateQuadraticExtensionRingElementAdditiveInverse( const QuadraticExtensionRingElement& value, const BigInteger& modulo );

		BigInteger CalculateQuadraticExtensionRingElementNorm( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo );

		bool AreQuadraticExtensionRingElementsEqual( const QuadraticExtensionRingElement& left, const QuadraticExtensionRingElement& right );

		bool IsQuadraticExtensionRingMultiplicativeIdentity( const QuadraticExtensionRingElement& value );

		bool DoesQuadraticExtensionRingElementSatisfyThirdCyclotomicPolynomial( const QuadraticExtensionRingElement& value, const BigInteger& quadratic_nonresidue_modulo_number, const BigInteger& modulo );

		bool PerformMillerRabinBaseTwoOrSmallQuadraticNonresiduePretest( const BigInteger& number, SimplifiedQuadraticFrobeniusPrecomputation& precomputation );

		QuadraticExtensionRingElementSamplingResult SampleQuadraticExtensionRingElementWithQuadraticNonresidueNorm( const BigInteger& number, const BigInteger& quadratic_nonresidue_modulo_number, QuadraticExtensionRingElement& sampled_element );

		bool PerformSimplifiedQuadraticFrobeniusRoundWithThirdRootsOfUnity( const BigInteger& number, const SimplifiedQuadraticFrobeniusPrecomputation& precomputation, QuadraticExtensionRingElement& remembered_third_root_of_unity );

	public:
		/**
		 * @brief Execute Algorithm SQFT3 from Seysen's paper.
		 *
		 * @param Number Non-negative integer to test.
		 * @param TestingRounds Number of independent strengthened Frobenius rounds.
		 *        It must be at least one.
		 * @return false if compositeness is detected; true if Number passes every
		 *         probable-prime condition.
		 * @throws std::invalid_argument If TestingRounds is zero.
		 * @throws std::runtime_error If defensive rejection-sampling limits are
		 *         exhausted before a mathematically required value is obtained.
		 */
		bool TestProbablePrimality( const BigInteger& Number, std::size_t TestingRounds );
	};
}  // namespace TwilightDream

#endif
