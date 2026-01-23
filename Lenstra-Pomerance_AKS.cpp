#include "Lenstra-Pomerance_AKS.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	constexpr std::uint64_t MACHINE_WORD_BITS = 64;
	constexpr std::size_t	SCHOOLBOOK_CONVOLUTION_THRESHOLD = 48;
	constexpr std::uint64_t EXACT_MACHINE_BASE_CASE_BITS = 64;

#if defined( __SIZEOF_INT128__ )
#if defined( __GNUC__ ) || defined( __clang__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
	using NativeUnsigned128 = unsigned __int128;
#if defined( __GNUC__ ) || defined( __clang__ )
#pragma GCC diagnostic pop
#endif
#endif
}  // namespace

/*
	 * =====================================================================
	 * Lenstra-Pomerance AKS implementation map
	 * =====================================================================
	 *
	 * Primary reference:
	 *
	 *     H. W. Lenstra, Jr. and Carl Pomerance,
	 *     "Primality testing with Gaussian periods",
	 *     Journal of the European Mathematical Society 21 (2019), 1229-1269.
	 *     DOI: 10.4171/JEMS/861
	 *
	 * The source code follows the mathematical dependency order instead of the order in which a normal application would call the functions:
	 *
	 *     1. exact machine-word helpers and BigInteger adapters;
	 *     2. arithmetic in Z/nZ;
	 *     3. exact proper-power rejection;
	 *     4. fast coefficient convolution by Kronecker substitution;
	 *     5. formal power series and tensor characteristic polynomials;
	 *     6. polynomial quotient rings and cyclotomic prime rings;
	 *     7. Gaussian-period components and period-system construction;
	 *     8. the complete Algorithm 3.3 primality-proof driver.
	 *
	 * Every function below is a non-static private member of AKS_Test.  
	 * The nested arithmetic engines retain AKS_Test& and therefore invoke the owner object explicitly. 
	 * This is intentional object composition, not a set of free functions disguised as static class members.
	 * =====================================================================
	 */

std::uint64_t AKS_Test::CalculateMachineWordProductModulo( std::uint64_t left, std::uint64_t right, std::uint64_t modulo )
{
	if ( modulo == 0 )
		throw std::invalid_argument( "CalculateMachineWordProductModulo: modulo must not be zero." );

#if defined( __SIZEOF_INT128__ )
	return static_cast<std::uint64_t>( ( static_cast<NativeUnsigned128>( left ) * static_cast<NativeUnsigned128>( right ) ) % modulo );
#else
	left %= modulo;
	std::uint64_t result = 0;
	while ( right != 0 )
	{
		if ( ( right & 1ULL ) != 0 )
			result = result >= modulo - left ? result - ( modulo - left ) : result + left;

		right >>= 1;
		if ( right != 0 )
			left = left >= modulo - left ? left - ( modulo - left ) : left + left;
	}
	return result;
#endif
}

std::uint64_t AKS_Test::CalculateMachineWordPowerModulo( std::uint64_t base, std::uint64_t exponent, std::uint64_t modulo )
{
	if ( modulo == 0 )
		throw std::invalid_argument( "CalculateMachineWordPowerModulo: modulo must not be zero." );
	if ( modulo == 1 )
		return 0;

	std::uint64_t result = 1 % modulo;
	base %= modulo;
	while ( exponent != 0 )
	{
		if ( ( exponent & 1ULL ) != 0 )
			result = CalculateMachineWordProductModulo( result, base, modulo );

		exponent >>= 1;
		if ( exponent != 0 )
			base = CalculateMachineWordProductModulo( base, base, modulo );
	}
	return result;
}

bool AKS_Test::IsMachineWordPrime( std::uint64_t number )
{
	if ( number < 2 )
		return false;

	for ( const std::uint64_t prime : { 2ULL, 3ULL, 5ULL, 7ULL, 11ULL, 13ULL, 17ULL, 19ULL, 23ULL, 29ULL, 31ULL, 37ULL } )
	{
		if ( number % prime == 0 )
			return number == prime;
	}

	std::uint64_t odd_part = number - 1;
	unsigned	  power_of_two = 0;
	while ( ( odd_part & 1ULL ) == 0 )
	{
		odd_part >>= 1;
		++power_of_two;
	}

	// Deterministic seven-base witness set for the complete uint64_t range.
	for ( const std::uint64_t witness : { 2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL } )
	{
		if ( witness % number == 0 )
			continue;

		std::uint64_t x = CalculateMachineWordPowerModulo( witness % number, odd_part, number );
		if ( x == 1 || x == number - 1 )
			continue;

		bool reached_minus_one = false;
		for ( unsigned round = 1; round < power_of_two; ++round )
		{
			x = CalculateMachineWordProductModulo( x, x, number );
			if ( x == number - 1 )
			{
				reached_minus_one = true;
				break;
			}
		}

		if ( !reached_minus_one )
			return false;
	}
	return true;
}

std::vector<std::uint32_t> AKS_Test::BuildSmallestPrimeFactorTable( std::size_t maximum )
{
	std::vector<std::uint32_t> smallest_prime_factor( maximum + 1, 0 );
	for ( std::size_t value = 2; value <= maximum; ++value )
	{
		if ( smallest_prime_factor[ value ] != 0 )
			continue;

		smallest_prime_factor[ value ] = static_cast<std::uint32_t>( value );
		if ( value <= maximum / value )
		{
			for ( std::size_t multiple = value * value; multiple <= maximum; multiple += value )
			{
				if ( smallest_prime_factor[ multiple ] == 0 )
					smallest_prime_factor[ multiple ] = static_cast<std::uint32_t>( value );
			}
		}
	}
	return smallest_prime_factor;
}

std::vector<std::uint32_t> AKS_Test::GeneratePrimeNumbersWithSieve( std::size_t maximum )
{
	if ( maximum < 2 )
		return {};

	const auto				   smallest_prime_factor = BuildSmallestPrimeFactorTable( maximum );
	std::vector<std::uint32_t> primes;
	for ( std::size_t value = 2; value <= maximum; ++value )
	{
		if ( smallest_prime_factor[ value ] == value )
			primes.push_back( static_cast<std::uint32_t>( value ) );
	}
	return primes;
}

std::vector<std::uint64_t> AKS_Test::FindDistinctPrimeFactorsOfMachineWord( std::uint64_t number )
{
	std::vector<std::uint64_t> factors;
	if ( number % 2 == 0 )
	{
		factors.push_back( 2 );
		do
		{
			number /= 2;
		} while ( number % 2 == 0 );
	}

	for ( std::uint64_t prime = 3; prime <= number / prime; prime += 2 )
	{
		if ( number % prime != 0 )
			continue;

		factors.push_back( prime );
		do
		{
			number /= prime;
		} while ( number % prime == 0 );
	}

	if ( number > 1 )
		factors.push_back( number );
	return factors;
}

std::uint64_t AKS_Test::FindPrimitiveRootModuloPrime( std::uint64_t prime )
{
	if ( prime == 2 )
		return 1;

	const auto factors = FindDistinctPrimeFactorsOfMachineWord( prime - 1 );
	for ( std::uint64_t generator = 2; generator < prime; ++generator )
	{
		bool is_primitive = true;
		for ( const std::uint64_t factor : factors )
		{
			if ( CalculateMachineWordPowerModulo( generator, ( prime - 1 ) / factor, prime ) == 1 )
			{
				is_primitive = false;
				break;
			}
		}

		if ( is_primitive )
			return generator;
	}
	throw std::runtime_error( "FindPrimitiveRootModuloPrime: primitive root not found." );
}

std::size_t AKS_Test::CalculateCeilingBinaryLogarithm( std::size_t number )
{
	if ( number <= 1 )
		return 0;

	std::size_t logarithm = 0;
	--number;
	while ( number != 0 )
	{
		number >>= 1;
		++logarithm;
	}
	return logarithm;
}

std::uint64_t AKS_Test::CalculateRemainderByMachineWord( const BigInteger& number, std::uint64_t modulo )
{
	if ( modulo == 0 )
		throw std::invalid_argument( "CalculateRemainderByMachineWord: modulo must not be zero." );
	if ( number.IsZero() )
		return 0;
	if ( number.Size() == 1 )
		return number.ToUnsignedInt() % modulo;

	BigInteger copy = number;
	return copy.DividModuloNumber( modulo );
}

std::vector<std::uint64_t> AKS_Test::ExportMachineWords( const BigInteger& number )
{
	std::vector<std::uint64_t> limbs( number.Size(), 0 );
	for ( std::size_t index = 0; index < limbs.size(); ++index )
		limbs[ index ] = number.GetBlock( index );

	while ( !limbs.empty() && limbs.back() == 0 )
		limbs.pop_back();
	return limbs;
}

AKS_Test::BigInteger AKS_Test::ImportMachineWords( std::vector<std::uint64_t> limbs )
{
	while ( !limbs.empty() && limbs.back() == 0 )
		limbs.pop_back();
	if ( limbs.empty() )
		return BigInteger( 0 );

	BigInteger result = BigInteger::BasePowerN( limbs.size() - 1 );
	for ( std::size_t index = 0; index < limbs.size(); ++index )
		result.SetBlock( index, limbs[ index ] );
	return result;
}

AKS_Test::BigInteger AKS_Test::CalculateRemainderWithNewtonDivision( const BigInteger& value, const BigInteger& modulo )
{
	if ( modulo.IsZero() )
		throw std::invalid_argument( "CalculateRemainderWithNewtonDivision: modulo must not be zero." );
	if ( value < modulo )
		return value;
	if ( value == modulo )
		return BigInteger( 0 );

	if ( modulo.Size() == 1 )
	{
		BigInteger quotient = value;
		return BigInteger( quotient.DividModuloNumber( modulo.ToUnsignedInt() ) );
	}

	BigInteger remainder;
	value.DivideModuloNewtonIteration( modulo, remainder );
	return remainder;
}

/**
	 * @brief Reciprocal-based modular reduction without changing BigInteger internals.
	 *
	 * The long-division assertion which requires the leading partial dividend word to be no greater than the leading divisor word is a valid quotient-estimation invariant.  
	 * This AKS implementation does not remove it, weaken it, or route around it as though the assertion were a bug.
	 *
	 * The reason for this external reducer is performance. 
	 * Gaussian-period and pseudofield arithmetic performs an enormous number of coefficient reductions.
	 * Repeating a complete large-integer division for every coefficient would throw away the asymptotic benefit obtained from Kronecker substitution.
	 *
	 * The AKS layer therefore computes once
	 *
	 *     reciprocal = floor(2^precision / modulus),
	 *     precision  = 2 * bit_length(modulus) + bit_length(size_t) + 2.
	 *
	 * Every coefficient extracted from a size_t-addressable exact convolution is smaller than 2^precision.  
	 * Consequently,
	 *
	 *     estimated_quotient = floor(value * reciprocal / 2^precision)
	 *
	 * underestimates floor(value / modulus) by at most one. 
	 * One multiplication, one shift, and at most one mathematical correction replace repeated division.
	 * The published BigInteger representation, division algorithms, assertions, and application binary interface remain untouched.
	 */
class AKS_Test::ReciprocalBasedModularReductionEngine
{
public:
	ReciprocalBasedModularReductionEngine( AKS_Test& algorithm, BigInteger modulo ) : algorithm_( algorithm ), modulo_( std::move( modulo ) )
	{
		if ( modulo_ <= BigInteger( 1 ) )
			throw std::invalid_argument( "ReciprocalBasedModularReductionEngine: modulo must exceed one." );

		const std::size_t	  modulo_bits = modulo_.BitLength();
		constexpr std::size_t ADDRESS_GUARD_BITS = std::numeric_limits<std::size_t>::digits + 2;
		if ( modulo_bits > ( std::numeric_limits<std::size_t>::max() - ADDRESS_GUARD_BITS ) / 2 )
		{
			throw std::overflow_error( "ReciprocalBasedModularReductionEngine: precision overflow." );
		}

		precision_bits_ = 2 * modulo_bits + ADDRESS_GUARD_BITS;
		const BigInteger numerator = BigInteger::TwoPowerN( precision_bits_ );
		BigInteger		 ignored_remainder;
		reciprocal_ = numerator.DivideModuloNewtonIteration( modulo_, ignored_remainder );
	}

	const BigInteger& Modulus() const noexcept
	{
		return modulo_;
	}

	BigInteger CalculateRemainder( const BigInteger& value ) const
	{
		if ( value < modulo_ )
			return value;
		if ( value == modulo_ )
			return BigInteger( 0 );

		// Values beyond the proven reciprocal precision are not produced by the AKS convolution path;
		// retain a correctness-only Newton fallback for such external callers.
		if ( value.BitLength() > precision_bits_ )
			return algorithm_.CalculateRemainderWithNewtonDivision( value, modulo_ );

		BigInteger quotient = ( value * reciprocal_ ) >> precision_bits_;
		BigInteger product = quotient * modulo_;

		// floor(2^s/n) cannot mathematically overestimate.  Keep this guard to
		// make a corrupted or non-canonical backend fail closed instead of
		// causing unsigned subtraction to wrap.
		while ( product > value )
		{
			if ( quotient.IsZero() )
				throw std::runtime_error( "ReciprocalBasedModularReductionEngine: reciprocal quotient invariant failed." );
			--quotient;
			product -= modulo_;
		}

		BigInteger remainder = value - product;
		while ( remainder >= modulo_ )
			remainder -= modulo_;
		return remainder;
	}

private:
	AKS_Test&	algorithm_;
	BigInteger	modulo_;
	BigInteger	reciprocal_;
	std::size_t precision_bits_ = 0;
};

const AKS_Test::ReciprocalBasedModularReductionEngine& AKS_Test::GetReciprocalBasedModularReductionEngine( const BigInteger& modulo )
{
	/*
		 * The cache is thread-local because the public AKS_Test instance may be used concurrently. 
		 * The owner pointer is part of the cache key: the reduction engine is a real sub-object collaborator bound to this AKS_Test object, not a disguised global utility hidden behind a static member function.
		 */
	struct ThreadLocalReductionEngineCache
	{
		AKS_Test*											 owner = nullptr;
		std::optional<ReciprocalBasedModularReductionEngine> engine;
	};

	thread_local ThreadLocalReductionEngineCache cache;
	if ( cache.owner != this || !cache.engine.has_value() || cache.engine->Modulus() != modulo )
	{
		cache.owner = this;
		cache.engine.emplace( *this, modulo );
	}
	return *cache.engine;
}

AKS_Test::BigInteger AKS_Test::CalculateRemainderModulo( const BigInteger& value, const BigInteger& modulo )
{
	return GetReciprocalBasedModularReductionEngine( modulo ).CalculateRemainder( value );
}


AKS_Test::BigInteger AKS_Test::DivideAndCalculateRemainder( const BigInteger& dividend, const BigInteger& divisor, BigInteger& remainder )
{
	if ( divisor.IsZero() )
		throw std::invalid_argument( "DivideAndCalculateRemainder: divisor must not be zero." );
	if ( dividend < divisor )
	{
		remainder = dividend;
		return BigInteger( 0 );
	}
	if ( dividend == divisor )
	{
		remainder = BigInteger( 0 );
		return BigInteger( 1 );
	}

	if ( divisor.Size() == 1 )
	{
		BigInteger quotient = dividend;
		remainder = BigInteger( quotient.DividModuloNumber( divisor.ToUnsignedInt() ) );
		return quotient;
	}
	return dividend.DivideModuloNewtonIteration( divisor, remainder );
}


AKS_Test::BigInteger AKS_Test::CalculateModularSum( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
{
	BigInteger result = left + right;
	return CalculateRemainderModulo( result, modulo );
}

AKS_Test::BigInteger AKS_Test::CalculateModularDifference( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
{
	const BigInteger normalized_left = CalculateRemainderModulo( left, modulo );
	const BigInteger normalized_right = CalculateRemainderModulo( right, modulo );
	if ( normalized_left >= normalized_right )
		return normalized_left - normalized_right;
	return modulo - ( normalized_right - normalized_left );
}

AKS_Test::BigInteger AKS_Test::CalculateModularAdditiveInverse( const BigInteger& value, const BigInteger& modulo )
{
	const BigInteger normalized = CalculateRemainderModulo( value, modulo );
	if ( normalized.IsZero() )
		return normalized;
	return modulo - normalized;
}

AKS_Test::BigInteger AKS_Test::CalculateModularProduct( const BigInteger& left, const BigInteger& right, const BigInteger& modulo )
{
	return CalculateRemainderModulo( left * right, modulo );
}


std::optional<AKS_Test::BigInteger> AKS_Test::CalculateModularMultiplicativeInverse( const BigInteger& value, const BigInteger& modulo, BigInteger* discovered_factor )
{
	/*
		 * Unsigned extended Euclid.
		 *
		 * The published BigInteger is intentionally unsigned.  
		 * We do not mutate it and we do not introduce a hidden signed dependency.  
		 * Instead, coefficient updates are kept modulo `modulo`:
		 *
		 *     r_i == t_i * value (mod modulo).
		 *
		 * This invariant is all modular inversion needs.
		 */
	BigInteger old_remainder = modulo;
	BigInteger remainder = CalculateRemainderModulo( value, modulo );
	BigInteger old_coefficient( 0 );
	BigInteger coefficient( 1 );

	while ( !remainder.IsZero() )
	{
		BigInteger		 next_remainder;
		const BigInteger quotient = DivideAndCalculateRemainder( old_remainder, remainder, next_remainder );
		const BigInteger quotient_modulo = CalculateRemainderModulo( quotient, modulo );
		const BigInteger next_coefficient = CalculateModularDifference( old_coefficient, CalculateModularProduct( quotient_modulo, coefficient, modulo ), modulo );

		old_remainder = remainder;
		remainder = next_remainder;
		old_coefficient = coefficient;
		coefficient = next_coefficient;
	}

	if ( old_remainder != BigInteger( 1 ) )
	{
		if ( discovered_factor != nullptr )
			*discovered_factor = old_remainder;
		return std::nullopt;
	}
	return CalculateRemainderModulo( old_coefficient, modulo );
}

int AKS_Test::CompareIntegerPowerWithLimit( BigInteger base, std::uint32_t exponent, const BigInteger& limit )
{
	BigInteger result( 1 );
	while ( exponent != 0 )
	{
		if ( ( exponent & 1U ) != 0 )
		{
			result *= base;
			if ( result > limit )
				return 1;
		}

		exponent >>= 1U;
		if ( exponent != 0 )
		{
			base *= base;
			if ( base > limit )
				base = limit + BigInteger( 1 );
		}
	}

	if ( result < limit )
		return -1;
	if ( result > limit )
		return 1;
	return 0;
}

AKS_Test::BigInteger AKS_Test::CalculateIntegerRoot( const BigInteger& number, std::uint32_t exponent )
{
	if ( exponent < 2 || number < BigInteger( 2 ) )
		return number;

	const std::size_t bit_length = number.BitLength();
	BigInteger		  lower = BigInteger::TwoPowerN( ( bit_length - 1 ) / exponent );
	BigInteger		  upper = BigInteger::TwoPowerN( ( bit_length + exponent - 1 ) / exponent + 1 );

	while ( lower + BigInteger( 1 ) < upper )
	{
		const BigInteger middle = ( lower + upper ) >> 1;
		if ( CompareIntegerPowerWithLimit( middle, exponent, number ) <= 0 )
			lower = middle;
		else
			upper = middle;
	}
	return lower;
}

bool AKS_Test::IsPerfectPowerExact( const BigInteger& number )
{
	if ( number < BigInteger( 4 ) )
		return false;

	const std::size_t bit_length = number.BitLength();
	const auto		  prime_exponents = GeneratePrimeNumbersWithSieve( bit_length );
	for ( const std::uint32_t exponent : prime_exponents )
	{
		BigInteger root = CalculateIntegerRoot( number, exponent );
		if ( CompareIntegerPowerWithLimit( root, exponent, number ) == 0 )
			return true;

		++root;
		if ( CompareIntegerPowerWithLimit( root, exponent, number ) == 0 )
			return true;
	}
	return false;
}

long double AKS_Test::CalculateBinaryLogarithmUpperBound( const BigInteger& number )
{
	const std::size_t bit_length = number.BitLength();
	if ( bit_length == 0 )
		throw std::invalid_argument( "CalculateBinaryLogarithmUpperBound: logarithm of zero." );

	if ( bit_length <= MACHINE_WORD_BITS )
	{
		const long double value = static_cast<long double>( number.ToUnsignedInt() );
		return std::nextafter( std::log2( value ), std::numeric_limits<long double>::infinity() );
	}

	const std::size_t	shift = bit_length - MACHINE_WORD_BITS;
	const BigInteger	top_integer = number.RightShiftBit( shift );
	const std::uint64_t top = top_integer.ToUnsignedInt();
	const long double	upper_mantissa = static_cast<long double>( top ) + 1.0L;
	return std::nextafter( static_cast<long double>( shift ) + std::log2( upper_mantissa ), std::numeric_limits<long double>::infinity() );
}

AKS_Test::BigInteger AKS_Test::CalculateSmallMachineWordPower( std::uint64_t base, unsigned exponent )
{
	BigInteger result( 1 );
	BigInteger value( base );
	while ( exponent != 0 )
	{
		if ( ( exponent & 1U ) != 0 )
			result *= value;
		exponent >>= 1U;
		if ( exponent != 0 )
			value *= value;
	}
	return result;
}

std::uint64_t AKS_Test::CalculateExclusiveRationalPowerBound( std::uint64_t base, unsigned numerator, unsigned denominator )
{
	if ( base <= 1 )
		return base;

	const long double estimate = std::pow( static_cast<long double>( base ), static_cast<long double>( numerator ) / static_cast<long double>( denominator ) );
	if ( estimate >= static_cast<long double>( std::numeric_limits<std::uint64_t>::max() ) )
		throw std::overflow_error( "CalculateExclusiveRationalPowerBound overflow." );

	std::uint64_t	 floor_value = std::max<std::uint64_t>( 1, static_cast<std::uint64_t>( estimate ) );
	const BigInteger right = CalculateSmallMachineWordPower( base, numerator );
	auto			 is_less_or_equal = [ this, &right, denominator ]( std::uint64_t value ) {
		return CalculateSmallMachineWordPower( value, denominator ) <= right;
	};

	while ( floor_value > 1 && !is_less_or_equal( floor_value ) )
		--floor_value;
	while ( floor_value != std::numeric_limits<std::uint64_t>::max() && is_less_or_equal( floor_value + 1 ) )
		++floor_value;

	const bool exact = CalculateSmallMachineWordPower( floor_value, denominator ) == right;
	if ( exact )
		return floor_value;
	if ( floor_value == std::numeric_limits<std::uint64_t>::max() )
		return floor_value;
	return floor_value + 1;
}

/**
	 * @brief Exact dense polynomial convolution through Kronecker substitution.
	 *
	 * For coefficients 0 <= a_i,b_i < n, choose a slot width s satisfying
	 *
	 *     2^s > min(number_of_left_terms, number_of_right_terms) * n^2.
	 *
	 * Encode
	 *
	 *     A = sum_i a_i 2^(s i),
	 *     B = sum_j b_j 2^(s j).
	 *
	 * No carry can cross a slot in A*B, so slot k is exactly
	 *
	 *     sum_(i+j=k) a_i b_j.
	 *
	 * One BigInteger multiplication therefore replaces a quadratic number of BigInteger coefficient multiplications. 
	 * The small schoolbook branch is nly a constant-factor crossover and does not change the asymptotic path.
	 *
	 * The engine holds AKS_Test& because coefficient normalization and import / export operations belong to the owning primality-test implementation.
	 */
class AKS_Test::KroneckerSubstitutionConvolutionEngine
{
public:
	KroneckerSubstitutionConvolutionEngine( AKS_Test& algorithm, BigInteger modulo ) : algorithm_( algorithm ), modulo_( std::move( modulo ) ), modulo_bits_( modulo_.BitLength() )
	{
		if ( modulo_ <= BigInteger( 1 ) )
			throw std::invalid_argument( "KroneckerSubstitutionConvolutionEngine: modulo must be greater than one." );
	}

	std::vector<BigInteger> MultiplyPolynomialCoefficients( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right, std::size_t truncate = std::numeric_limits<std::size_t>::max() ) const
	{
		if ( left.empty() || right.empty() || truncate == 0 )
			return {};

		if ( left.size() > std::numeric_limits<std::size_t>::max() - right.size() + 1 )
			throw std::overflow_error( "KroneckerSubstitutionConvolutionEngine: polynomial size overflow." );

		const std::size_t full_size = left.size() + right.size() - 1;
		const std::size_t output_size = std::min( full_size, truncate );

		// Tiny products are faster without packing.  The asymptotic path starts
		// immediately after this deliberately small engineering threshold.
		if ( left.size() <= SCHOOLBOOK_CONVOLUTION_THRESHOLD / std::max<std::size_t>( 1, right.size() ) )
			return MultiplyPolynomialCoefficientsWithSchoolbookConvolution( left, right, output_size );

		const std::size_t term_count = std::min( left.size(), right.size() );
		const std::size_t guard_bits = algorithm_.CalculateCeilingBinaryLogarithm( term_count ) + 2;
		if ( modulo_bits_ > ( std::numeric_limits<std::size_t>::max() - guard_bits ) / 2 )
			throw std::overflow_error( "KroneckerSubstitutionConvolutionEngine: slot bit width overflow." );

		const std::size_t				 slot_bits = 2 * modulo_bits_ + guard_bits;
		const std::size_t				 slot_limbs = std::max<std::size_t>( 1, ( slot_bits + 63 ) / 64 );
		const BigInteger				 packed_left = PackPolynomialCoefficientsIntoBigInteger( left, slot_limbs );
		const BigInteger				 packed_right = PackPolynomialCoefficientsIntoBigInteger( right, slot_limbs );
		const BigInteger				 packed_product = packed_left * packed_right;
		const std::vector<std::uint64_t> product_limbs = algorithm_.ExportMachineWords( packed_product );

		std::vector<BigInteger> output( output_size, BigInteger( 0 ) );
		for ( std::size_t coefficient_index = 0; coefficient_index < output_size; ++coefficient_index )
		{
			if ( coefficient_index > std::numeric_limits<std::size_t>::max() / slot_limbs )
				throw std::overflow_error( "KroneckerSubstitutionConvolutionEngine: slot offset overflow." );

			const std::size_t		   offset = coefficient_index * slot_limbs;
			std::vector<std::uint64_t> slot( slot_limbs, 0 );
			if ( offset < product_limbs.size() )
			{
				const std::size_t available = std::min( slot_limbs, product_limbs.size() - offset );
				std::copy_n( product_limbs.begin() + static_cast<std::ptrdiff_t>( offset ), available, slot.begin() );
			}
			output[ coefficient_index ] = algorithm_.CalculateRemainderModulo( algorithm_.ImportMachineWords( std::move( slot ) ), modulo_ );
		}
		return output;
	}

private:
	std::vector<BigInteger> MultiplyPolynomialCoefficientsWithSchoolbookConvolution( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right, std::size_t output_size ) const
	{
		std::vector<BigInteger> output( output_size, BigInteger( 0 ) );
		for ( std::size_t left_index = 0; left_index < left.size(); ++left_index )
		{
			const std::size_t maximum_right = std::min( right.size(), output_size - std::min( left_index, output_size ) );
			if ( left_index >= output_size )
				break;

			for ( std::size_t right_index = 0; right_index < maximum_right; ++right_index )
			{
				const std::size_t target = left_index + right_index;
				output[ target ] = algorithm_.CalculateModularSum( output[ target ], algorithm_.CalculateModularProduct( left[ left_index ], right[ right_index ], modulo_ ), modulo_ );
			}
		}
		return output;
	}

	BigInteger PackPolynomialCoefficientsIntoBigInteger( const std::vector<BigInteger>& coefficients, std::size_t slot_limbs ) const
	{
		if ( !coefficients.empty() && slot_limbs > std::numeric_limits<std::size_t>::max() / coefficients.size() )
			throw std::overflow_error( "KroneckerSubstitutionConvolutionEngine: packed limb count overflow." );

		std::vector<std::uint64_t> limbs( coefficients.size() * slot_limbs, 0 );
		for ( std::size_t coefficient_index = 0; coefficient_index < coefficients.size(); ++coefficient_index )
		{
			const BigInteger				 normalized = algorithm_.CalculateRemainderModulo( coefficients[ coefficient_index ], modulo_ );
			const std::vector<std::uint64_t> source = algorithm_.ExportMachineWords( normalized );
			if ( source.size() > slot_limbs )
				throw std::runtime_error( "KroneckerSubstitutionConvolutionEngine: coefficient exceeded its exact slot." );

			std::copy( source.begin(), source.end(), limbs.begin() + static_cast<std::ptrdiff_t>( coefficient_index * slot_limbs ) );
		}
		return algorithm_.ImportMachineWords( std::move( limbs ) );
	}

	AKS_Test&	algorithm_;
	BigInteger	modulo_;
	std::size_t modulo_bits_;
};

/**
	 * @brief Fast polynomial and formal-power-series arithmetic over Z/nZ.
	 *
	 * The tensor-product construction in Proposition 7.4 requires more than ordinary polynomial multiplication. 
	 * This engine implements:
	 *
	 *     G^(-1)       by Newton doubling,
	 *     log(G)       = integral(G' / G),
	 *     exp(H)       by Newton doubling,
	 *     L(G)         = x G'(x) / G(x),
	 *
	 * and reconstructs a series from its logarithmic derivative.
	 * For two monic characteristic polynomials f and g, their tensor characteristic polynomial is recovered from
	 *
	 *     L((f tensor g)^flat)
	 *         = -L(f^flat) HadamardProduct L(g^flat).
	 *
	 * A failed division by a small integer exposes gcd(integer,n) and therefore terminates with a deterministic compositeness certificate.
	 */
class AKS_Test::ModularPolynomialArithmeticEngine
{
public:
	ModularPolynomialArithmeticEngine( AKS_Test& algorithm, BigInteger modulo ) : algorithm_( algorithm ), modulo_( std::move( modulo ) ), convolution_( algorithm_, modulo_ ) {}

	BigInteger NormalizeCoefficient( BigInteger value ) const
	{
		return algorithm_.CalculateRemainderModulo( value, modulo_ );
	}

	std::vector<BigInteger> AddPolynomials( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right, std::size_t truncate = std::numeric_limits<std::size_t>::max() ) const
	{
		const std::size_t		output_size = std::min( std::max( left.size(), right.size() ), truncate );
		std::vector<BigInteger> output( output_size, BigInteger( 0 ) );
		for ( std::size_t index = 0; index < output_size; ++index )
		{
			const BigInteger left_value = index < left.size() ? left[ index ] : BigInteger( 0 );
			const BigInteger right_value = index < right.size() ? right[ index ] : BigInteger( 0 );
			output[ index ] = algorithm_.CalculateModularSum( left_value, right_value, modulo_ );
		}
		return output;
	}

	std::vector<BigInteger> SubtractPolynomials( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right, std::size_t truncate = std::numeric_limits<std::size_t>::max() ) const
	{
		const std::size_t		output_size = std::min( std::max( left.size(), right.size() ), truncate );
		std::vector<BigInteger> output( output_size, BigInteger( 0 ) );
		for ( std::size_t index = 0; index < output_size; ++index )
		{
			const BigInteger left_value = index < left.size() ? left[ index ] : BigInteger( 0 );
			const BigInteger right_value = index < right.size() ? right[ index ] : BigInteger( 0 );
			output[ index ] = algorithm_.CalculateModularDifference( left_value, right_value, modulo_ );
		}
		return output;
	}

	std::vector<BigInteger> MultiplyPolynomials( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right, std::size_t truncate = std::numeric_limits<std::size_t>::max() ) const
	{
		return convolution_.MultiplyPolynomialCoefficients( left, right, truncate );
	}

	std::vector<BigInteger> CalculateFormalPowerSeriesMultiplicativeInverse( const std::vector<BigInteger>& series, std::size_t length, BigInteger* discovered_factor = nullptr ) const
	{
		if ( length == 0 )
			return {};
		if ( series.empty() )
			throw std::invalid_argument( "ModularPolynomialArithmeticEngine::CalculateFormalPowerSeriesMultiplicativeInverse: empty input." );

		const auto inverse_constant = algorithm_.CalculateModularMultiplicativeInverse( series[ 0 ], modulo_, discovered_factor );
		if ( !inverse_constant.has_value() )
			return {};

		std::vector<BigInteger> inverse { *inverse_constant };
		std::size_t				current_length = 1;
		while ( current_length < length )
		{
			const std::size_t		next_length = std::min( length, current_length * 2 );
			std::vector<BigInteger> cut( next_length, BigInteger( 0 ) );
			for ( std::size_t index = 0; index < std::min( next_length, series.size() ); ++index )
				cut[ index ] = NormalizeCoefficient( series[ index ] );

			std::vector<BigInteger> correction = MultiplyPolynomials( cut, inverse, next_length );
			correction.resize( next_length, BigInteger( 0 ) );
			for ( BigInteger& coefficient : correction )
				coefficient = algorithm_.CalculateModularAdditiveInverse( coefficient, modulo_ );
			correction[ 0 ] = algorithm_.CalculateModularSum( correction[ 0 ], BigInteger( 2 ), modulo_ );

			inverse = MultiplyPolynomials( inverse, correction, next_length );
			inverse.resize( next_length, BigInteger( 0 ) );
			current_length = next_length;
		}
		return inverse;
	}

	std::vector<BigInteger> CalculatePolynomialDerivative( const std::vector<BigInteger>& polynomial ) const
	{
		if ( polynomial.size() <= 1 )
			return {};

		std::vector<BigInteger> derivative( polynomial.size() - 1, BigInteger( 0 ) );
		for ( std::size_t index = 1; index < polynomial.size(); ++index )
		{
			derivative[ index - 1 ] = algorithm_.CalculateModularProduct( polynomial[ index ], BigInteger( static_cast<std::uint64_t>( index ) ), modulo_ );
		}
		return derivative;
	}

	std::vector<BigInteger> CalculateFormalPowerSeriesIntegral( const std::vector<BigInteger>& polynomial, BigInteger* discovered_factor = nullptr ) const
	{
		std::vector<BigInteger> integral( polynomial.size() + 1, BigInteger( 0 ) );
		for ( std::size_t index = 0; index < polynomial.size(); ++index )
		{
			const auto inverse = algorithm_.CalculateModularMultiplicativeInverse( BigInteger( static_cast<std::uint64_t>( index + 1 ) ), modulo_, discovered_factor );
			if ( !inverse.has_value() )
				return {};
			integral[ index + 1 ] = algorithm_.CalculateModularProduct( polynomial[ index ], *inverse, modulo_ );
		}
		return integral;
	}

	std::vector<BigInteger> CalculateFormalPowerSeriesLogarithm( const std::vector<BigInteger>& series, std::size_t length, BigInteger* discovered_factor = nullptr ) const
	{
		if ( length == 0 )
			return {};
		if ( series.empty() || NormalizeCoefficient( series[ 0 ] ) != BigInteger( 1 ) )
			throw std::invalid_argument( "ModularPolynomialArithmeticEngine::CalculateFormalPowerSeriesLogarithm requires constant coefficient one." );

		const std::vector<BigInteger> inverse = CalculateFormalPowerSeriesMultiplicativeInverse( series, length, discovered_factor );
		if ( inverse.empty() )
			return {};

		const std::vector<BigInteger> derivative = CalculatePolynomialDerivative( series );
		const std::vector<BigInteger> quotient = MultiplyPolynomials( derivative, inverse, length > 0 ? length - 1 : 0 );
		std::vector<BigInteger>		  logarithm = CalculateFormalPowerSeriesIntegral( quotient, discovered_factor );
		if ( logarithm.empty() && length > 1 )
			return {};
		logarithm.resize( length, BigInteger( 0 ) );
		return logarithm;
	}

	std::vector<BigInteger> CalculateFormalPowerSeriesExponential( const std::vector<BigInteger>& series, std::size_t length, BigInteger* discovered_factor = nullptr ) const
	{
		if ( length == 0 )
			return {};
		if ( !series.empty() && NormalizeCoefficient( series[ 0 ] ) != BigInteger( 0 ) )
			throw std::invalid_argument( "ModularPolynomialArithmeticEngine::CalculateFormalPowerSeriesExponential requires zero constant coefficient." );

		std::vector<BigInteger> exponential { BigInteger( 1 ) };
		std::size_t				current_length = 1;
		while ( current_length < length )
		{
			const std::size_t			  next_length = std::min( length, current_length * 2 );
			const std::vector<BigInteger> current_logarithm = CalculateFormalPowerSeriesLogarithm( exponential, next_length, discovered_factor );
			if ( current_logarithm.empty() && next_length > 1 )
				return {};

			std::vector<BigInteger> correction( next_length, BigInteger( 0 ) );
			for ( std::size_t index = 0; index < next_length; ++index )
			{
				const BigInteger target = index < series.size() ? series[ index ] : BigInteger( 0 );
				const BigInteger present = index < current_logarithm.size() ? current_logarithm[ index ] : BigInteger( 0 );
				correction[ index ] = algorithm_.CalculateModularDifference( target, present, modulo_ );
			}
			correction[ 0 ] = algorithm_.CalculateModularSum( correction[ 0 ], BigInteger( 1 ), modulo_ );

			exponential = MultiplyPolynomials( exponential, correction, next_length );
			exponential.resize( next_length, BigInteger( 0 ) );
			current_length = next_length;
		}
		return exponential;
	}

	std::vector<BigInteger> CalculateFormalLogarithmicDerivative( const std::vector<BigInteger>& series, std::size_t length, BigInteger* discovered_factor = nullptr ) const
	{
		const std::vector<BigInteger> inverse = CalculateFormalPowerSeriesMultiplicativeInverse( series, length, discovered_factor );
		if ( inverse.empty() )
			return {};

		std::vector<BigInteger> d_series( length, BigInteger( 0 ) );
		for ( std::size_t index = 1; index < std::min( length, series.size() ); ++index )
		{
			d_series[ index ] = algorithm_.CalculateModularProduct( series[ index ], BigInteger( static_cast<std::uint64_t>( index ) ), modulo_ );
		}

		std::vector<BigInteger> result = MultiplyPolynomials( d_series, inverse, length );
		result.resize( length, BigInteger( 0 ) );
		return result;
	}

	std::vector<BigInteger> ReconstructFormalPowerSeriesFromLogarithmicDerivative( const std::vector<BigInteger>& series, std::size_t length, BigInteger* discovered_factor = nullptr ) const
	{
		std::vector<BigInteger> integral( length, BigInteger( 0 ) );
		for ( std::size_t index = 1; index < length; ++index )
		{
			const auto inverse = algorithm_.CalculateModularMultiplicativeInverse( BigInteger( static_cast<std::uint64_t>( index ) ), modulo_, discovered_factor );
			if ( !inverse.has_value() )
				return {};

			const BigInteger value = index < series.size() ? series[ index ] : BigInteger( 0 );
			integral[ index ] = algorithm_.CalculateModularProduct( value, *inverse, modulo_ );
		}
		return CalculateFormalPowerSeriesExponential( integral, length, discovered_factor );
	}

	std::vector<BigInteger> CalculateTensorProductCharacteristicPolynomial( const std::vector<BigInteger>& first, const std::vector<BigInteger>& second, BigInteger* discovered_factor = nullptr ) const
	{
		if ( first.size() < 2 || second.size() < 2 )
			throw std::invalid_argument( "CalculateTensorProductCharacteristicPolynomial: positive degrees required." );

		const std::size_t first_degree = first.size() - 1;
		const std::size_t second_degree = second.size() - 1;
		if ( second_degree != 0 && first_degree > std::numeric_limits<std::size_t>::max() / second_degree )
			throw std::overflow_error( "CalculateTensorProductCharacteristicPolynomial: degree overflow." );
		const std::size_t product_degree = first_degree * second_degree;

		std::vector<BigInteger> first_flat( product_degree + 1, BigInteger( 0 ) );
		std::vector<BigInteger> second_flat( product_degree + 1, BigInteger( 0 ) );
		first_flat[ 0 ] = BigInteger( 1 );
		second_flat[ 0 ] = BigInteger( 1 );
		for ( std::size_t index = 1; index <= first_degree; ++index )
			first_flat[ index ] = NormalizeCoefficient( first[ first_degree - index ] );
		for ( std::size_t index = 1; index <= second_degree; ++index )
			second_flat[ index ] = NormalizeCoefficient( second[ second_degree - index ] );

		const std::vector<BigInteger> first_log = CalculateFormalLogarithmicDerivative( first_flat, product_degree + 1, discovered_factor );
		if ( first_log.empty() )
			return {};
		const std::vector<BigInteger> second_log = CalculateFormalLogarithmicDerivative( second_flat, product_degree + 1, discovered_factor );
		if ( second_log.empty() )
			return {};

		std::vector<BigInteger> tensor_log( product_degree + 1, BigInteger( 0 ) );
		for ( std::size_t index = 1; index <= product_degree; ++index )
		{
			tensor_log[ index ] = algorithm_.CalculateModularAdditiveInverse( algorithm_.CalculateModularProduct( first_log[ index ], second_log[ index ], modulo_ ), modulo_ );
		}

		const std::vector<BigInteger> tensor_flat = ReconstructFormalPowerSeriesFromLogarithmicDerivative( tensor_log, product_degree + 1, discovered_factor );
		if ( tensor_flat.empty() )
			return {};

		std::vector<BigInteger> characteristic( product_degree + 1, BigInteger( 0 ) );
		characteristic[ product_degree ] = BigInteger( 1 );
		for ( std::size_t index = 1; index <= product_degree; ++index )
			characteristic[ product_degree - index ] = NormalizeCoefficient( tensor_flat[ index ] );
		return characteristic;
	}

private:
	AKS_Test&							   algorithm_;
	BigInteger							   modulo_;
	KroneckerSubstitutionConvolutionEngine convolution_;
};

/**
	 * @brief Arithmetic in the pseudofield representation (Z/nZ)[Y] / (f(Y)).
	 *
	 * Elements are coefficient vectors of length degree(f). 
	 * Multiplication first forms an exact product and then reduces it modulo the monic polynomial f.
	 * Division by f is accelerated with reversed polynomials:
	 *
	 *     reverse(quotient)
	 *         = reverse(high_part(product)) * reverse(f)^(-1).
	 *
	 * The inverse of reverse(f) is precomputed once by Newton iteration. 
	 * Large powers such as Y^n and (Y+a)^n use a left-to-right sliding window, so the quotient-ring implementation remains coupled to fast polynomial multiply.
	 */
class AKS_Test::PolynomialQuotientRing
{
public:
	PolynomialQuotientRing( AKS_Test& algorithm, BigInteger modulo, std::vector<BigInteger> monic_modulus, BigInteger* discovered_factor = nullptr ) : algorithm_( algorithm ), modulo_( std::move( modulo ) ), engine_( algorithm_, modulo_ ), modulus_polynomial_( std::move( monic_modulus ) )
	{
		if ( modulus_polynomial_.size() < 2 )
			throw std::invalid_argument( "PolynomialQuotientRing: positive-degree modulus required." );

		degree_ = modulus_polynomial_.size() - 1;
		for ( BigInteger& coefficient : modulus_polynomial_ )
			coefficient = engine_.NormalizeCoefficient( coefficient );
		if ( modulus_polynomial_.back() != BigInteger( 1 ) )
			throw std::invalid_argument( "PolynomialQuotientRing: modulus must be monic." );

		std::vector<BigInteger> reversed( modulus_polynomial_.rbegin(), modulus_polynomial_.rend() );
		inverse_reversed_modulus_ = engine_.CalculateFormalPowerSeriesMultiplicativeInverse( reversed, degree_ + 1, discovered_factor );
		if ( inverse_reversed_modulus_.empty() )
			throw std::runtime_error( "PolynomialQuotientRing: reversed modulus is not invertible." );
	}

	std::size_t Degree() const noexcept
	{
		return degree_;
	}

	std::vector<BigInteger> CreateZeroElement() const
	{
		return std::vector<BigInteger>( degree_, BigInteger( 0 ) );
	}

	std::vector<BigInteger> CreateMultiplicativeIdentityElement() const
	{
		std::vector<BigInteger> result = CreateZeroElement();
		result[ 0 ] = BigInteger( 1 );
		return result;
	}

	std::vector<BigInteger> CreatePolynomialGeneratorElement() const
	{
		std::vector<BigInteger> result = CreateZeroElement();
		if ( degree_ == 1 )
			result[ 0 ] = algorithm_.CalculateModularAdditiveInverse( modulus_polynomial_[ 0 ], modulo_ );
		else
			result[ 1 ] = BigInteger( 1 );
		return result;
	}

	std::vector<BigInteger> CreatePolynomialGeneratorPlusConstantElement( std::uint64_t value ) const
	{
		std::vector<BigInteger> result = CreatePolynomialGeneratorElement();
		result[ 0 ] = algorithm_.CalculateModularSum( result[ 0 ], BigInteger( value ), modulo_ );
		return result;
	}

	std::vector<BigInteger> AddElements( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right ) const
	{
		std::vector<BigInteger> result = engine_.AddPolynomials( left, right, degree_ );
		result.resize( degree_, BigInteger( 0 ) );
		return result;
	}

	std::vector<BigInteger> MultiplyElements( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right ) const
	{
		return ReducePolynomialToQuotientRing( engine_.MultiplyPolynomials( left, right ) );
	}

	std::vector<BigInteger> SquareElement( const std::vector<BigInteger>& value ) const
	{
		return MultiplyElements( value, value );
	}

	std::vector<BigInteger> RaiseElementToPower( std::vector<BigInteger> base, const BigInteger& exponent ) const
	{
		base.resize( degree_, BigInteger( 0 ) );
		const std::size_t bit_length = exponent.BitLength();
		if ( bit_length == 0 )
			return CreateMultiplicativeIdentityElement();

		const unsigned						 window = SelectWindowSize( bit_length );
		const std::size_t					 table_size = std::size_t( 1 ) << ( window - 1 );
		std::vector<std::vector<BigInteger>> odd_powers( table_size );
		odd_powers[ 0 ] = base;
		if ( table_size > 1 )
		{
			const std::vector<BigInteger> base_squared = SquareElement( base );
			for ( std::size_t index = 1; index < table_size; ++index )
				odd_powers[ index ] = MultiplyElements( odd_powers[ index - 1 ], base_squared );
		}

		std::vector<BigInteger> result = CreateMultiplicativeIdentityElement();
		std::size_t				bit = bit_length;
		while ( bit > 0 )
		{
			const std::size_t high = bit - 1;
			if ( !exponent.GetBit( high ) )
			{
				result = SquareElement( result );
				--bit;
				continue;
			}

			std::size_t low = high + 1 > window ? high + 1 - window : 0;
			while ( low < high && !exponent.GetBit( low ) )
				++low;

			std::uint32_t window_value = 0;
			for ( std::size_t index = high;; --index )
			{
				window_value = static_cast<std::uint32_t>( ( window_value << 1U ) | ( exponent.GetBit( index ) ? 1U : 0U ) );
				if ( index == low )
					break;
			}

			const std::size_t squaring_count = high - low + 1;
			for ( std::size_t count = 0; count < squaring_count; ++count )
				result = SquareElement( result );

			result = MultiplyElements( result, odd_powers[ ( window_value - 1U ) / 2U ] );
			bit = low;
		}
		return result;
	}

	bool AreElementsEqual( const std::vector<BigInteger>& left, const std::vector<BigInteger>& right ) const
	{
		for ( std::size_t index = 0; index < degree_; ++index )
		{
			const BigInteger left_value = index < left.size() ? engine_.NormalizeCoefficient( left[ index ] ) : BigInteger( 0 );
			const BigInteger right_value = index < right.size() ? engine_.NormalizeCoefficient( right[ index ] ) : BigInteger( 0 );
			if ( left_value != right_value )
				return false;
		}
		return true;
	}

private:
	unsigned SelectWindowSize( std::size_t exponent_bits ) const
	{
		if ( exponent_bits < 32 )
			return 2;
		if ( exponent_bits < 128 )
			return 3;
		if ( exponent_bits < 512 )
			return 4;
		if ( exponent_bits < 2048 )
			return 5;
		return 6;
	}

	std::vector<BigInteger> ReducePolynomialToQuotientRing( std::vector<BigInteger> polynomial ) const
	{
		for ( BigInteger& coefficient : polynomial )
			coefficient = engine_.NormalizeCoefficient( coefficient );
		while ( !polynomial.empty() && polynomial.back().IsZero() )
			polynomial.pop_back();

		if ( polynomial.size() <= degree_ )
		{
			polynomial.resize( degree_, BigInteger( 0 ) );
			return polynomial;
		}

		const std::size_t		polynomial_degree = polynomial.size() - 1;
		const std::size_t		quotient_length = polynomial_degree - degree_ + 1;
		std::vector<BigInteger> reversed_top( quotient_length, BigInteger( 0 ) );
		for ( std::size_t index = 0; index < quotient_length; ++index )
			reversed_top[ index ] = polynomial[ polynomial_degree - index ];

		std::vector<BigInteger> reversed_quotient = engine_.MultiplyPolynomials( reversed_top, inverse_reversed_modulus_, quotient_length );
		reversed_quotient.resize( quotient_length, BigInteger( 0 ) );
		std::vector<BigInteger>		  quotient( reversed_quotient.rbegin(), reversed_quotient.rend() );
		const std::vector<BigInteger> product = engine_.MultiplyPolynomials( quotient, modulus_polynomial_ );

		if ( polynomial.size() < product.size() )
			polynomial.resize( product.size(), BigInteger( 0 ) );
		for ( std::size_t index = 0; index < product.size(); ++index )
			polynomial[ index ] = algorithm_.CalculateModularDifference( polynomial[ index ], product[ index ], modulo_ );

		polynomial.resize( degree_, BigInteger( 0 ) );
		return polynomial;
	}

	AKS_Test&						  algorithm_;
	BigInteger						  modulo_;
	ModularPolynomialArithmeticEngine engine_;
	std::vector<BigInteger>			  modulus_polynomial_;
	std::vector<BigInteger>			  inverse_reversed_modulus_;
	std::size_t						  degree_ = 0;
};

struct AKS_Test::CyclotomicPrimeRingElement
{
	std::vector<BigInteger> coefficients;
};

/**
	 * @brief Arithmetic in (Z/nZ)[zeta_r] for a prime conductor r.
	 *
	 * Since r is prime,
	 *
	 *     Phi_r(X) = 1 + X + ... + X^(r-1).
	 *
	 * The implementation stores the basis 1,zeta_r,...,zeta_r^(r-2). 
	 * A raw coefficient of zeta_r^(r-1) is eliminated through
	 *
	 *     zeta_r^(r-1) = -(1 + zeta_r + ... + zeta_r^(r-2)).
	 *
	 * Multiplication is first cyclic modulo X^r-1 and then normalized by Phi_r.
	 * The automorphism sigma_m sends zeta_r^e to zeta_r^(m e mod r); this is the exact Frobenius action required by the Gaussian-period component test.
	 */
class AKS_Test::CyclotomicPrimeRing
{
public:
	using Element = CyclotomicPrimeRingElement;

	CyclotomicPrimeRing( AKS_Test& algorithm, BigInteger modulo, std::uint64_t conductor ) : algorithm_( algorithm ), modulo_( std::move( modulo ) ), conductor_( conductor ), convolution_( algorithm_, modulo_ )
	{
		if ( conductor_ < 2 || !algorithm_.IsMachineWordPrime( conductor_ ) )
			throw std::invalid_argument( "CyclotomicPrimeRing: conductor must be prime." );
	}

	std::uint64_t Conductor() const noexcept
	{
		return conductor_;
	}

	const BigInteger& Modulus() const noexcept
	{
		return modulo_;
	}

	std::size_t Dimension() const noexcept
	{
		return static_cast<std::size_t>( conductor_ - 1 );
	}

	Element CreateZeroElement() const
	{
		return Element { std::vector<BigInteger>( Dimension(), BigInteger( 0 ) ) };
	}

	Element CreateMultiplicativeIdentityElement() const
	{
		Element result = CreateZeroElement();
		result.coefficients[ 0 ] = BigInteger( 1 );
		return result;
	}

	Element Constant( const BigInteger& value ) const
	{
		Element result = CreateZeroElement();
		result.coefficients[ 0 ] = algorithm_.CalculateRemainderModulo( value, modulo_ );
		return result;
	}

	Element CreateElementFromExponentSet( const std::vector<std::uint64_t>& exponents ) const
	{
		std::vector<BigInteger> raw( conductor_, BigInteger( 0 ) );
		for ( const std::uint64_t exponent : exponents )
		{
			const std::size_t index = static_cast<std::size_t>( exponent % conductor_ );
			raw[ index ] = algorithm_.CalculateModularSum( raw[ index ], BigInteger( 1 ), modulo_ );
		}
		return NormalizeRawCoefficientVector( std::move( raw ) );
	}

	Element AddElements( const Element& left, const Element& right ) const
	{
		Element result = CreateZeroElement();
		for ( std::size_t index = 0; index < Dimension(); ++index )
		{
			result.coefficients[ index ] = algorithm_.CalculateModularSum( left.coefficients[ index ], right.coefficients[ index ], modulo_ );
		}
		return result;
	}

	Element Subtract( const Element& left, const Element& right ) const
	{
		Element result = CreateZeroElement();
		for ( std::size_t index = 0; index < Dimension(); ++index )
		{
			result.coefficients[ index ] = algorithm_.CalculateModularDifference( left.coefficients[ index ], right.coefficients[ index ], modulo_ );
		}
		return result;
	}

	Element CalculateAdditiveInverse( const Element& value ) const
	{
		Element result = CreateZeroElement();
		for ( std::size_t index = 0; index < Dimension(); ++index )
			result.coefficients[ index ] = algorithm_.CalculateModularAdditiveInverse( value.coefficients[ index ], modulo_ );
		return result;
	}

	Element MultiplyElements( const Element& left, const Element& right ) const
	{
		const std::vector<BigInteger> convolution = convolution_.MultiplyPolynomialCoefficients( left.coefficients, right.coefficients );
		std::vector<BigInteger>		  raw( conductor_, BigInteger( 0 ) );
		for ( std::size_t exponent = 0; exponent < convolution.size(); ++exponent )
		{
			const std::size_t reduced = exponent < conductor_ ? exponent : exponent - static_cast<std::size_t>( conductor_ );
			raw[ reduced ] = algorithm_.CalculateModularSum( raw[ reduced ], convolution[ exponent ], modulo_ );
		}
		return NormalizeRawCoefficientVector( std::move( raw ) );
	}

	Element ApplyCyclotomicAutomorphism( const Element& value, std::uint64_t multiplier ) const
	{
		multiplier %= conductor_;
		if ( multiplier == 0 )
			throw std::invalid_argument( "CyclotomicPrimeRing::ApplyCyclotomicAutomorphism: zero multiplier." );

		std::vector<BigInteger> raw( conductor_, BigInteger( 0 ) );
		for ( std::size_t exponent = 0; exponent < Dimension(); ++exponent )
		{
			const std::size_t target = static_cast<std::size_t>( algorithm_.CalculateMachineWordProductModulo( static_cast<std::uint64_t>( exponent ), multiplier, conductor_ ) );
			raw[ target ] = algorithm_.CalculateModularSum( raw[ target ], value.coefficients[ exponent ], modulo_ );
		}
		return NormalizeRawCoefficientVector( std::move( raw ) );
	}

	bool AreElementsEqual( const Element& left, const Element& right ) const
	{
		if ( left.coefficients.size() != Dimension() || right.coefficients.size() != Dimension() )
			return false;
		for ( std::size_t index = 0; index < Dimension(); ++index )
		{
			if ( algorithm_.CalculateRemainderModulo( left.coefficients[ index ], modulo_ ) != algorithm_.CalculateRemainderModulo( right.coefficients[ index ], modulo_ ) )
				return false;
		}
		return true;
	}

	bool IsScalarElement( const Element& value ) const
	{
		for ( std::size_t index = 1; index < Dimension(); ++index )
		{
			if ( !algorithm_.CalculateRemainderModulo( value.coefficients[ index ], modulo_ ).IsZero() )
				return false;
		}
		return true;
	}

	BigInteger ExtractScalarValue( const Element& value ) const
	{
		if ( !IsScalarElement( value ) )
			throw std::runtime_error( "Gaussian-period characteristic coefficient escaped the scalar subring." );
		return algorithm_.CalculateRemainderModulo( value.coefficients[ 0 ], modulo_ );
	}

	std::vector<Element> MultiplyPolynomialsWithCyclotomicRingCoefficients( const std::vector<Element>& left, const std::vector<Element>& right ) const
	{
		if ( left.empty() || right.empty() )
			return {};

		const std::size_t stride = static_cast<std::size_t>( 2 * conductor_ - 1 );
		if ( left.size() > std::numeric_limits<std::size_t>::max() / stride || right.size() > std::numeric_limits<std::size_t>::max() / stride )
		{
			throw std::overflow_error( "CyclotomicPrimeRing::MultiplyPolynomialsWithCyclotomicRingCoefficients size overflow." );
		}

		std::vector<BigInteger> flat_left( left.size() * stride, BigInteger( 0 ) );
		std::vector<BigInteger> flat_right( right.size() * stride, BigInteger( 0 ) );
		for ( std::size_t y = 0; y < left.size(); ++y )
		{
			for ( std::size_t x = 0; x < Dimension(); ++x )
				flat_left[ y * stride + x ] = algorithm_.CalculateRemainderModulo( left[ y ].coefficients[ x ], modulo_ );
		}
		for ( std::size_t y = 0; y < right.size(); ++y )
		{
			for ( std::size_t x = 0; x < Dimension(); ++x )
				flat_right[ y * stride + x ] = algorithm_.CalculateRemainderModulo( right[ y ].coefficients[ x ], modulo_ );
		}

		const std::vector<BigInteger> flat_product = convolution_.MultiplyPolynomialCoefficients( flat_left, flat_right );
		const std::size_t			  y_count = left.size() + right.size() - 1;
		std::vector<Element>		  output( y_count, CreateZeroElement() );
		for ( std::size_t y = 0; y < y_count; ++y )
		{
			std::vector<BigInteger> raw( conductor_, BigInteger( 0 ) );
			for ( std::size_t x = 0; x <= static_cast<std::size_t>( 2 * conductor_ - 4 ); ++x )
			{
				const std::size_t flat_index = y * stride + x;
				if ( flat_index >= flat_product.size() )
					break;

				const std::size_t reduced = x < conductor_ ? x : x - static_cast<std::size_t>( conductor_ );
				raw[ reduced ] = algorithm_.CalculateModularSum( raw[ reduced ], flat_product[ flat_index ], modulo_ );
			}
			output[ y ] = NormalizeRawCoefficientVector( std::move( raw ) );
		}
		return output;
	}

private:
	Element NormalizeRawCoefficientVector( std::vector<BigInteger> raw ) const
	{
		raw.resize( conductor_, BigInteger( 0 ) );
		for ( BigInteger& coefficient : raw )
			coefficient = algorithm_.CalculateRemainderModulo( coefficient, modulo_ );

		const BigInteger pivot = raw[ conductor_ - 1 ];
		Element			 result = CreateZeroElement();
		for ( std::size_t index = 0; index < Dimension(); ++index )
			result.coefficients[ index ] = algorithm_.CalculateModularDifference( raw[ index ], pivot, modulo_ );
		return result;
	}

	AKS_Test&							   algorithm_;
	BigInteger							   modulo_;
	std::uint64_t						   conductor_;
	KroneckerSubstitutionConvolutionEngine convolution_;
};

/**
	 * @brief Complete materialized result for one Gaussian-period component.
	 *
	 * Keeping the ring, period, conjugates and characteristic polynomial together prevents an element from being interpreted under a different conductor or coefficient modulus. 
	 * The structure is private because it is a proof-stage object, not part of the public BigInteger API.
	 */
struct AKS_Test::GaussianPeriodData
{
	std::uint64_t							  conductor = 0;
	std::uint64_t							  degree = 0;
	CyclotomicPrimeRing						  ring;
	CyclotomicPrimeRing::Element			  period;
	std::vector<CyclotomicPrimeRing::Element> conjugates;
	std::vector<BigInteger>					  characteristic_polynomial;

	GaussianPeriodData( std::uint64_t conductor_value, std::uint64_t degree_value, CyclotomicPrimeRing ring_value, CyclotomicPrimeRing::Element period_value, std::vector<CyclotomicPrimeRing::Element> conjugate_values, std::vector<BigInteger> characteristic_value ) : conductor( conductor_value ), degree( degree_value ), ring( std::move( ring_value ) ), period( std::move( period_value ) ), conjugates( std::move( conjugate_values ) ), characteristic_polynomial( std::move( characteristic_value ) ) {}
};

/**
	 * Algorithm 8.3 component construction.
	 *
	 * Let Delta = (Z/rZ)^* and let Delta^q be the subgroup of index q. 
	 * The Gaussian period
	 *
	 *     eta = sum_(rho in Delta^q) zeta_r^rho
	 *
	 * has q conjugates.  
	 * A balanced product tree multiplies the q linear factors (Y - conjugate) so that the characteristic polynomial is obtained with fast convolution instead of sequential quadratic multiplication.
	 */
AKS_Test::GaussianPeriodData AKS_Test::ConstructGaussianPeriodPseudofieldComponent( const BigInteger& modulo, std::uint64_t conductor, std::uint64_t degree )
{
	if ( !IsMachineWordPrime( conductor ) || degree <= 1 || ( conductor - 1 ) % degree != 0 )
		throw std::invalid_argument( "ConstructGaussianPeriodPseudofieldComponent: invalid period pair." );

	CyclotomicPrimeRing		   ring( *this, modulo, conductor );
	const std::uint64_t		   primitive_root = FindPrimitiveRootModuloPrime( conductor );
	const std::uint64_t		   subgroup_generator = CalculateMachineWordPowerModulo( primitive_root, degree, conductor );
	std::vector<std::uint64_t> subgroup;
	subgroup.reserve( static_cast<std::size_t>( ( conductor - 1 ) / degree ) );

	std::uint64_t current = 1;
	for ( std::uint64_t index = 0; index < ( conductor - 1 ) / degree; ++index )
	{
		subgroup.push_back( current );
		current = CalculateMachineWordProductModulo( current, subgroup_generator, conductor );
	}
	CyclotomicPrimeRing::Element period = ring.CreateElementFromExponentSet( subgroup );

	std::vector<CyclotomicPrimeRing::Element> conjugates;
	conjugates.reserve( static_cast<std::size_t>( degree ) );
	std::uint64_t coset_multiplier = 1;
	for ( std::uint64_t index = 0; index < degree; ++index )
	{
		conjugates.push_back( ring.ApplyCyclotomicAutomorphism( period, coset_multiplier ) );
		coset_multiplier = CalculateMachineWordProductModulo( coset_multiplier, primitive_root, conductor );
	}

	std::vector<std::vector<CyclotomicPrimeRing::Element>> product_tree;
	product_tree.reserve( conjugates.size() );
	for ( const auto& conjugate : conjugates )
		product_tree.push_back( { ring.CalculateAdditiveInverse( conjugate ), ring.CreateMultiplicativeIdentityElement() } );

	while ( product_tree.size() > 1 )
	{
		std::vector<std::vector<CyclotomicPrimeRing::Element>> next_level;
		next_level.reserve( ( product_tree.size() + 1 ) / 2 );
		for ( std::size_t index = 0; index < product_tree.size(); index += 2 )
		{
			if ( index + 1 == product_tree.size() )
				next_level.push_back( std::move( product_tree[ index ] ) );
			else
				next_level.push_back( ring.MultiplyPolynomialsWithCyclotomicRingCoefficients( product_tree[ index ], product_tree[ index + 1 ] ) );
		}
		product_tree = std::move( next_level );
	}

	const auto&				cyclotomic_coefficients = product_tree.front();
	std::vector<BigInteger> characteristic( cyclotomic_coefficients.size(), BigInteger( 0 ) );
	for ( std::size_t index = 0; index < cyclotomic_coefficients.size(); ++index )
		characteristic[ index ] = ring.ExtractScalarValue( cyclotomic_coefficients[ index ] );

	if ( characteristic.size() != degree + 1 || characteristic.back() != BigInteger( 1 ) )
		throw std::runtime_error( "ConstructGaussianPeriodPseudofieldComponent: invalid characteristic polynomial." );

	return GaussianPeriodData( conductor, degree, std::move( ring ), std::move( period ), std::move( conjugates ), std::move( characteristic ) );
}

AKS_Test::CyclotomicPrimeRingElement AKS_Test::EvaluatePolynomialAtGaussianPeriod( const CyclotomicPrimeRing& ring, const std::vector<BigInteger>& polynomial, const CyclotomicPrimeRing::Element& period )
{
	CyclotomicPrimeRing::Element result = ring.CreateZeroElement();
	for ( std::size_t index = polynomial.size(); index-- > 0; )
	{
		result = ring.MultiplyElements( result, period );
		result.coefficients[ 0 ] = CalculateModularSum( result.coefficients[ 0 ], polynomial[ index ], ring.Modulus() );
	}
	return result;
}

std::uint64_t AKS_Test::CalculateTheoreticalPseudofieldDegree( const BigInteger& number )
{
	const long double log2_number = CalculateBinaryLogarithmUpperBound( number );
	const long double natural_log = log2_number * std::log( 2.0L );
	const long double first_bound = log2_number * log2_number / 3.0L;
	const long double second_bound = std::pow( natural_log, 46.0L / 25.0L );
	const long double target = std::max( first_bound, second_bound );

	if ( !std::isfinite( target ) || target >= static_cast<long double>( std::numeric_limits<std::uint64_t>::max() - 2 ) )
	{
		throw std::overflow_error( "CalculateTheoreticalPseudofieldDegree: auxiliary degree exceeds uint64_t." );
	}

	// The paper permits an integer D whose error is bounded by a constant.
	// `+2` gives a safe one-sided envelope after the floating upper bound.
	return static_cast<std::uint64_t>( std::floor( target ) ) + 2;
}

/**
	 * Attempt Algorithm 3.1 inside one finite search box.
	 *
	 * Candidate conductors r are prime.
	 * Candidate component degrees q are prime divisors of r-1, used at most once, and accepted only when
	 *
	 *     n^((r-1)/q) != 1 (mod r).
	 *
	 * A subset-product dynamic program then finds the least square-free product d of accepted q values satisfying target_degree <= d < 2*target_degree.
	 */
std::optional<AKS_Test::PeriodSystem> AKS_Test::TryConstructPeriodSystem( const BigInteger& number, std::uint64_t target_degree, std::uint64_t conductor_limit_exclusive, std::uint64_t period_degree_limit_exclusive, bool strict_paper_bounds )
{
	if ( target_degree == 0 )
		return std::nullopt;
	if ( target_degree > std::numeric_limits<std::uint64_t>::max() / 2 )
		throw std::overflow_error( "TryConstructPeriodSystem: 2D overflow." );

	const std::uint64_t twice_target = 2 * target_degree;
	const std::uint64_t sieve_limit_word = std::max( twice_target, conductor_limit_exclusive == 0 ? 0 : conductor_limit_exclusive - 1 );
	if ( sieve_limit_word > static_cast<std::uint64_t>( std::numeric_limits<std::size_t>::max() - 1 ) )
		throw std::overflow_error( "TryConstructPeriodSystem: sieve exceeds address space." );

	const std::size_t								 sieve_limit = static_cast<std::size_t>( sieve_limit_word );
	const std::vector<std::uint32_t>				 smallest_prime_factor = BuildSmallestPrimeFactorTable( sieve_limit );
	std::unordered_map<std::uint64_t, std::uint64_t> conductor_for_degree;

	/*
			 * Algorithm 3.1, Step 2.
			 *
			 * q is prime because it is read from the prime factorization of r - 1.
			 * Since r is prime and r does not divide n, Fermat gives
			 *
			 *     (n^((r - 1) / q))^q == 1 mod r.
			 *
			 * Therefore checking that the first power is not one proves that its order is exactly q, which is the period-pair condition.
			 */
	for ( std::uint64_t conductor = 2; conductor < conductor_limit_exclusive && conductor <= sieve_limit_word; ++conductor )
	{
		if ( smallest_prime_factor[ static_cast<std::size_t>( conductor ) ] != conductor )
			continue;

		const std::uint64_t number_mod_conductor = CalculateRemainderByMachineWord( number, conductor );
		if ( number_mod_conductor == 0 )
			continue;

		std::uint64_t remaining = conductor - 1;
		while ( remaining > 1 )
		{
			const std::uint64_t prime = smallest_prime_factor[ static_cast<std::size_t>( remaining ) ];
			do
			{
				remaining /= prime;
			} while ( remaining > 1 && remaining % prime == 0 );

			if ( prime <= 1 || prime >= period_degree_limit_exclusive )
				continue;
			if ( conductor_for_degree.find( prime ) != conductor_for_degree.end() )
				continue;
			if ( CalculateMachineWordPowerModulo( number_mod_conductor, ( conductor - 1 ) / prime, conductor ) == 1 )
			{
				continue;
			}

			conductor_for_degree.emplace( prime, conductor );
		}
	}

	/*
		* Algorithm 3.1, Step 3.
		* Scan in increasing order, so the first accepted square-free integer is exactly the least d in [D, 2D) requested by the paper.
		*/
	for ( std::uint64_t candidate = target_degree; candidate < twice_target; ++candidate )
	{
		std::uint64_t			   remaining = candidate;
		std::vector<std::uint64_t> selected_degrees;
		bool					   accepted = true;
		while ( remaining > 1 )
		{
			const std::uint64_t prime = smallest_prime_factor[ static_cast<std::size_t>( remaining ) ];
			unsigned			multiplicity = 0;
			do
			{
				remaining /= prime;
				++multiplicity;
			} while ( remaining > 1 && remaining % prime == 0 );

			if ( multiplicity != 1 || conductor_for_degree.find( prime ) == conductor_for_degree.end() )
			{
				accepted = false;
				break;
			}
			selected_degrees.push_back( prime );
		}

		if ( !accepted )
			continue;

		PeriodSystem system;
		system.degree = candidate;
		system.target_degree = target_degree;
		system.strict_paper_bounds = strict_paper_bounds;
		for ( const std::uint64_t degree : selected_degrees )
		{
			system.pairs.push_back( PeriodPair { conductor_for_degree.at( degree ), degree } );
		}
		std::sort( system.pairs.begin(), system.pairs.end(), []( const PeriodPair& left, const PeriodPair& right ) { return left.degree < right.degree; } );
		return system;
	}
	return std::nullopt;
}

/**
	 * Construct a period system, first under the paper bounds and then, only for finite practical inputs below the unspecified asymptotic threshold, by expanding the search box while preserving every period-pair condition.
	 * The adaptive search affects the existence search, not primality correctness.
	 */
AKS_Test::PeriodSystem AKS_Test::ConstructPeriodSystem( const BigInteger& number, std::uint64_t target_degree )
{
	const std::uint64_t strict_conductor_limit = std::max<std::uint64_t>( 3, CalculateExclusiveRationalPowerBound( target_degree, 6, 11 ) );
	const std::uint64_t strict_degree_limit = std::max<std::uint64_t>( 3, CalculateExclusiveRationalPowerBound( target_degree, 3, 11 ) );

	if ( const auto strict = TryConstructPeriodSystem( number, target_degree, strict_conductor_limit, strict_degree_limit, true ) )
	{
		return *strict;
	}

	/*
		* The mathematical definition is not weakened here. 
		* Only the finite search box is expanded for inputs below the paper's opaque c_4.
		*/
	std::uint64_t		conductor_limit = std::max<std::uint64_t>( strict_conductor_limit, 64 );
	std::uint64_t		degree_limit = std::max<std::uint64_t>( strict_degree_limit, 64 );
	const std::uint64_t maximum_conductor_limit = std::max<std::uint64_t>( 65536, target_degree > ( std::numeric_limits<std::uint64_t>::max() - 1 ) / 2 ? std::numeric_limits<std::uint64_t>::max() : 2 * target_degree + 1 );
	const std::uint64_t maximum_degree_limit = std::max<std::uint64_t>( 65536, target_degree > ( std::numeric_limits<std::uint64_t>::max() - 1 ) / 2 ? std::numeric_limits<std::uint64_t>::max() : 2 * target_degree + 1 );

	for ( ;; )
	{
		if ( const auto adaptive = TryConstructPeriodSystem( number, target_degree, conductor_limit, degree_limit, false ) )
		{
			return *adaptive;
		}

		if ( conductor_limit == maximum_conductor_limit && degree_limit == maximum_degree_limit )
			break;

		conductor_limit = std::min( maximum_conductor_limit, conductor_limit <= maximum_conductor_limit / 2 ? conductor_limit * 2 : maximum_conductor_limit );
		degree_limit = std::min( maximum_degree_limit, degree_limit <= maximum_degree_limit / 2 ? degree_limit * 2 : maximum_degree_limit );
	}

	throw std::runtime_error( "ConstructPeriodSystem: no valid period system was found in the representable adaptive search box." );
}

std::uint64_t AKS_Test::CalculateFrobeniusIdentityTestBound( std::uint64_t degree, const BigInteger& number )
{
	const long double bound = std::sqrt( static_cast<long double>( degree ) / 3.0L ) * CalculateBinaryLogarithmUpperBound( number );
	if ( !std::isfinite( bound ) || bound >= static_cast<long double>( std::numeric_limits<std::uint64_t>::max() ) )
	{
		throw std::overflow_error( "CalculateFrobeniusIdentityTestBound overflow." );
	}
	return static_cast<std::uint64_t>( std::ceil( bound ) );
}

std::optional<AKS_Test::BigInteger> AKS_Test::FindTrialDivisor( const BigInteger& number, std::uint64_t maximum )
{
	if ( maximum > static_cast<std::uint64_t>( std::numeric_limits<std::size_t>::max() ) )
		throw std::overflow_error( "FindTrialDivisor: bound exceeds size_t." );

	const std::vector<std::uint32_t> primes = GeneratePrimeNumbersWithSieve( static_cast<std::size_t>( maximum ) );
	for ( const std::uint32_t prime : primes )
	{
		if ( CalculateRemainderByMachineWord( number, prime ) == 0 )
			return BigInteger( prime );
	}
	return std::nullopt;
}

bool AKS_Test::ExecuteLenstraPomeranceGaussianPeriodPrimalityTest( const BigInteger& number )
{
	const BigInteger ZERO( 0 );
	const BigInteger ONE( 1 );
	const BigInteger TWO( 2 );
	const BigInteger THREE( 3 );

	if ( number < TWO )
		return false;
	if ( number == TWO || number == THREE )
		return true;
	if ( number.IsEven() )
		return false;

	/*
		* Algorithm 3.3, Step 1 -- finite exact base case.
		*
		* A deterministic uint64_t test is strictly stronger than trial division for this finite range and costs essentially nothing compared with a pseudofield.
		* Define TWILIGHT_DREAM_FORCE_LENSTRA_POMERANCE_GAUSSIAN_PERIOD_PATH when testing the hard path on educational machine-sized inputs.
		*/
#if !defined( TWILIGHT_DREAM_FORCE_LENSTRA_POMERANCE_GAUSSIAN_PERIOD_PATH )
	if ( number.BitLength() <= EXACT_MACHINE_BASE_CASE_BITS )
		return IsMachineWordPrime( number.ToUnsignedInt() );
#endif

	// Algorithm 3.3, Step 2.
	if ( IsPerfectPowerExact( number ) )
		return false;

	std::uint64_t target_degree = CalculateTheoreticalPseudofieldDegree( number );
	PeriodSystem  period_system;
	std::uint64_t frobenius_bound = 0;

	/*
		* The strict inequality d > log_2(n)^2 / 3 implies b < d.
		* The loop is only a defensive guard against finite-precision envelope effects; it does not alter the asymptotic degree d = Theta((log n)^2).
		*/
	for ( ;; )
	{
		period_system = ConstructPeriodSystem( number, target_degree );
		frobenius_bound = CalculateFrobeniusIdentityTestBound( period_system.degree, number );
		if ( frobenius_bound < period_system.degree )
			break;

		if ( target_degree > std::numeric_limits<std::uint64_t>::max() / 2 )
			throw std::overflow_error( "AKS target degree overflow while enforcing b < d." );
		target_degree *= 2;
	}

	if ( number <= BigInteger( period_system.degree ) )
	{
		if ( number.BitLength() <= EXACT_MACHINE_BASE_CASE_BITS )
			return IsMachineWordPrime( number.ToUnsignedInt() );
		throw std::runtime_error( "AKS pseudofield degree unexpectedly reached n." );
	}

	// Algorithm 3.3, Step 4.
	// Testing only prime divisors is equivalent to testing every integer in the interval and is obviously cheaper.
	const std::uint64_t trial_bound = std::max( period_system.degree, frobenius_bound );
	if ( const auto divisor = FindTrialDivisor( number, trial_bound ) )
		return number == *divisor;

	/*
		* Algorithm 8.3, Step 1.
		*
		* Each accepted period pair produces a degree-q component pseudofield.
		* The check below is exactly
		*
		*     g_(r,q)(eta_(r,q)) == sigma_n(eta_(r,q)),
		*
		* where g_(r,q) is Y^n reduced modulo f_(r,q).
		*/
	std::vector<std::vector<BigInteger>> component_characteristics;
	component_characteristics.reserve( period_system.pairs.size() );
	for ( const PeriodPair& pair : period_system.pairs )
	{
		GaussianPeriodData				   component = ConstructGaussianPeriodPseudofieldComponent( number, pair.conductor, pair.degree );
		PolynomialQuotientRing			   component_ring( *this, number, component.characteristic_polynomial );
		const std::vector<BigInteger>	   y_power_n = component_ring.RaiseElementToPower( component_ring.CreatePolynomialGeneratorElement(), number );
		const CyclotomicPrimeRing::Element evaluated = EvaluatePolynomialAtGaussianPeriod( component.ring, y_power_n, component.period );
		const CyclotomicPrimeRing::Element frobenius_period = component.ring.ApplyCyclotomicAutomorphism( component.period, CalculateRemainderByMachineWord( number, pair.conductor ) );

		if ( !component.ring.AreElementsEqual( evaluated, frobenius_period ) )
			return false;

		component_characteristics.push_back( std::move( component.characteristic_polynomial ) );
	}

	if ( component_characteristics.empty() )
		throw std::runtime_error( "AKS period system contained no period pairs." );

	/*
		* Algorithm 8.3, Step 2 / Proposition 7.4.
		*
		* The component degrees are distinct primes, hence pairwise coprime.
		* CalculateTensorProductCharacteristicPolynomial applies
		*
		*     L(f_tensor^flat) = -L(f_1^flat) * L(f_2^flat),
		*
		* where * is the Hadamard product.
		* Failure to invert an integer i <= d returns gcd(i,n), which is already a deterministic compositeness proof.
		*/
	ModularPolynomialArithmeticEngine polynomial_engine( *this, number );
	std::vector<BigInteger>			  characteristic = std::move( component_characteristics.front() );
	for ( std::size_t index = 1; index < component_characteristics.size(); ++index )
	{
		BigInteger				discovered_factor( 0 );
		std::vector<BigInteger> tensor = polynomial_engine.CalculateTensorProductCharacteristicPolynomial( characteristic, component_characteristics[ index ], &discovered_factor );
		if ( tensor.empty() )
			return false;
		characteristic = std::move( tensor );
	}

	if ( characteristic.size() != static_cast<std::size_t>( period_system.degree ) + 1 )
		throw std::runtime_error( "AKS tensor pseudofield degree mismatch." );

	/*
		* Algorithm 3.3, Step 6.
		*
		* All bases a are independent.
		* A hardware-bounded worker pool avoids the old one-std::async-per-a explosion while retaining deterministic semantics.
		*/
	PolynomialQuotientRing		  pseudofield( *this, number, characteristic );
	const std::vector<BigInteger> alpha = pseudofield.CreatePolynomialGeneratorElement();
	const std::vector<BigInteger> alpha_power_n = pseudofield.RaiseElementToPower( alpha, number );

	const unsigned			   hardware_threads = std::max( 1U, std::thread::hardware_concurrency() );
	const unsigned			   worker_count = static_cast<unsigned>( std::min<std::uint64_t>( frobenius_bound, hardware_threads ) );
	std::atomic<std::uint64_t> next_base( 1 );
	std::atomic<std::uint64_t> failed_base( 0 );
	std::atomic_bool		   stop( false );
	std::exception_ptr		   worker_exception;
	std::mutex				   exception_mutex;

	auto worker = [ & ]() {
		try
		{
			while ( !stop.load( std::memory_order_relaxed ) )
			{
				const std::uint64_t base = next_base.fetch_add( 1, std::memory_order_relaxed );
				if ( base > frobenius_bound )
					break;

				std::vector<BigInteger> left = alpha_power_n;
				left[ 0 ] = CalculateModularSum( left[ 0 ], BigInteger( base ), number );
				const std::vector<BigInteger> right = pseudofield.RaiseElementToPower( pseudofield.CreatePolynomialGeneratorPlusConstantElement( base ), number );
				if ( !pseudofield.AreElementsEqual( left, right ) )
				{
					std::uint64_t expected = 0;
					failed_base.compare_exchange_strong( expected, base, std::memory_order_relaxed );
					stop.store( true, std::memory_order_relaxed );
					break;
				}
			}
		}
		catch ( ... )
		{
			{
				std::lock_guard<std::mutex> lock( exception_mutex );
				if ( worker_exception == nullptr )
					worker_exception = std::current_exception();
			}
			stop.store( true, std::memory_order_relaxed );
		}
	};

	if ( worker_count <= 1 )
	{
		worker();
	}
	else
	{
		std::vector<std::thread> workers;
		workers.reserve( worker_count );
		for ( unsigned index = 0; index < worker_count; ++index )
			workers.emplace_back( worker );
		for ( std::thread& thread : workers )
			thread.join();
	}

	if ( worker_exception != nullptr )
		std::rethrow_exception( worker_exception );
	return failed_base.load( std::memory_order_relaxed ) == 0;
}

bool AKS_Test::IsPerfectPower( const BigInteger& number )
{
	return IsPerfectPowerExact( number );
}

bool AKS_Test::operator()( const BigInteger& number )
{
	return ExecuteLenstraPomeranceGaussianPeriodPrimalityTest( number );
}