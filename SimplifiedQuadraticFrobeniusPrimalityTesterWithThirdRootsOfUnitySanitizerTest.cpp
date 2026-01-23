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

		/*
		 * This deliberately smaller suite is intended for AddressSanitizer and
		 * UndefinedBehaviorSanitizer.  It still enters every Algorithm MR2 residue
		 * branch and performs multiple complete quadratic-extension rounds, while
		 * avoiding the very slow 256-bit vector used by the normal regression test.
		 */
		for ( std::uint64_t number = 0; number <= 1000; ++number )
		{
			const bool expected_result = IsPrimeByMachineWordTrialDivision( number );
			const bool actual_result = prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( number ), 2 );
			RequireCondition( actual_result == expected_result, "Sanitizer reference mismatch at " + std::to_string( number ) + "." );
		}

		const std::vector<std::pair<std::uint64_t, std::uint64_t>> prime_numbers_by_residue_class { { 1000000009ULL, 1ULL }, { 1000000123ULL, 3ULL }, { 1000000021ULL, 5ULL }, { 1000000007ULL, 7ULL } };

		for ( const auto& [ prime_number, expected_residue_class ] : prime_numbers_by_residue_class )
		{
			RequireCondition( ( prime_number & 7ULL ) == expected_residue_class, "The sanitizer prime vector has an incorrect residue class." );
			RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( prime_number ), 3 ), "A sanitizer prime vector was rejected." );
		}

		RequireCondition( !prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( BigInteger( "1000036000099" ), 3 ), "A large semiprime was accepted by the sanitizer suite." );

		const BigInteger direct_class_prime( 1000000009ULL );
		RequireCondition( prime_number_tester.SimplifiedQuadraticFrobeniusPrimalityTestWithThirdRootsOfUnity( direct_class_prime, 2 ) == dedicated_frobenius_primality_tester.TestProbablePrimality( direct_class_prime, 2 ), "The dispatcher and the dedicated class disagreed in the sanitizer suite." );

		std::cout << "The reduced sanitizer suite for the Simplified Quadratic Frobenius test passed.\n";
		return 0;
	}
	catch ( const std::exception& exception )
	{
		std::cerr << "Sanitizer test failure: " << exception.what() << '\n';
		return 1;
	}
}
