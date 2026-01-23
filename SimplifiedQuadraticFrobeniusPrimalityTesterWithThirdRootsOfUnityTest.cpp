#include "PrimeNumberTester.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	bool IsPrimeByMachineWordTrialDivision( std::uint64_t number )
	{
		if ( number < 2 )
		{
			return false;
		}

		if ( ( number & 1ULL ) == 0 )
		{
			return number == 2;
		}

		for ( std::uint64_t divisor = 3; divisor <= number / divisor; divisor += 2 )
		{
			if ( number % divisor == 0 )
			{
				return false;
			}
		}
		return true;
	}

	void RequireCondition( bool condition, const std::string& failure_message )
	{
		if ( !condition )
		{
			throw std::runtime_error( failure_message );
		}
	}
}  // namespace

int main()
{
	try
	{
		TwilightDream::PrimeNumberTester												prime_number_tester;
		TwilightDream::SimplifiedQuadraticFrobeniusPrimalityTesterWithThirdRootsOfUnity dedicated_frobenius_primality_tester;

		bool zero_testing_rounds_were_rejected = false;
		try
		{
			( void )prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( 211 ), 0 );
		}
		catch ( const std::invalid_argument& )
		{
			zero_testing_rounds_were_rejected = true;
		}
		RequireCondition( zero_testing_rounds_were_rejected, "Zero testing rounds must be rejected." );

		/*
		 * Every composite number not exceeding 5000 has a prime factor below 200.
		 * This range therefore verifies the public small-number behavior and the
		 * complete trial-division front end without introducing probabilistic test
		 * flakiness.
		 */
		for ( std::uint64_t number = 0; number <= 5000; ++number )
		{
			const bool expected_result = IsPrimeByMachineWordTrialDivision( number );
			const bool actual_result = prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( number ), 3 );

			RequireCondition( actual_result == expected_result, "Machine-word reference mismatch at " + std::to_string( number ) + "." );
		}

		/*
		 * These composites have no prime factor below 200, so they must reach the
		 * actual Miller-Rabin/Frobenius machinery instead of being rejected by the
		 * introductory trial divisions.
		 */
		const std::vector<std::string> large_composite_numbers { "1000036000099", "341550071728321", "3825123056546413051", "318665857834031151167461" };

		for ( const std::string& decimal_composite_number : large_composite_numbers )
		{
			RequireCondition( !prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( decimal_composite_number ), 4 ), "A large composite number was accepted: " + decimal_composite_number + "." );
		}

		/*
		 * One known prime from each odd residue class modulo eight exercises every
		 * branch of Algorithm MR2.
		 */
		const std::vector<std::pair<std::string, std::uint64_t>> large_prime_numbers_by_residue_class { { "1267650600228229401496703205953", 1 }, { "1267650600228229401496703205707", 3 }, { "1267650600228229401496703205653", 5 }, { "1267650600228229401496703205823", 7 } };

		for ( const auto& [ decimal_prime_number, expected_residue_class ] : large_prime_numbers_by_residue_class )
		{
			const BigInteger prime_number( decimal_prime_number );
			RequireCondition( ( prime_number.ToUnsignedInt() & 7ULL ) == expected_residue_class, "The fixed prime vector has an incorrect residue class." );
			RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( prime_number, 3 ), "A large prime was rejected in residue class " + std::to_string( expected_residue_class ) + "." );
		}

		/* Mersenne prime 2^127-1 and the prime field modulus used by secp256k1. */
		RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( "170141183460469231731687303715884105727" ), 3 ), "The Mersenne prime 2^127-1 was rejected." );

		RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( "115792089237316195423570985008687907853269984665640564039457584007908834671663" ), 2 ), "The secp256k1 prime field modulus was rejected." );

		/* A large odd perfect square must be rejected before it can mimic n=1 mod 8. */
		RequireCondition( !prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( "1329227995784915948996626364332246081" ), 3 ), "A large odd perfect square was accepted." );

		/* Verify that the project-level dispatcher and the dedicated class agree. */
		const BigInteger delegation_prime( "170141183460469231731687303715884105727" );
		RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( delegation_prime, 2 ) == dedicated_frobenius_primality_tester.TestProbablePrimality( delegation_prime, 2 ), "PrimeNumberTester did not delegate consistently to the dedicated algorithm class." );

		std::cout << "All Simplified Quadratic Frobenius primality tests with third roots of unity passed.\n";
		return 0;
	}
	catch ( const std::exception& exception )
	{
		std::cerr << "Test failure: " << exception.what() << '\n';
		return 1;
	}
}
