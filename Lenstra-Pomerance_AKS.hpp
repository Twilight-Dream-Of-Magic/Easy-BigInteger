#pragma once

#include "BigInteger.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

/**
	 * @brief Deterministic Lenstra-Pomerance Gaussian-period primality test.
	 *
	 * This is NOT the elementary classroom implementation which performs
	 *
	 *     (X + a)^n == X^n + a  mod (X^r - 1, n).
	 *
	 * That version is historically important, but it is not the lowest proven
	 * asymptotic-complexity member of the AKS family.  This implementation uses
	 * Gaussian periods and pseudofields, following:
	 *
	 * H. W. Lenstra, Jr. and Carl Pomerance,
	 * "Primality testing with Gaussian periods",
	 * Journal of the European Mathematical Society 21 (2019), 1229-1269.
	 * DOI: 10.4171/JEMS/861
	 * Preprint: https://math.dartmouth.edu/~carlp/aks111216.pdf
	 *
	 * The paper proves deterministic running time
	 *
	 *     O~((log n)^6),
	 *
	 * where O~ suppresses polylogarithmic factors.  The implementation keeps the
	 * published BigInteger internals untouched and builds the complete auxiliary
	 * algebra only in PrimeNumberTester.cpp.
	 *
	 * -------------------------------------------------------------------------
	 * Erika-style executable pseudocode -- Algorithm 3.3
	 * -------------------------------------------------------------------------
	 *
	 * Input: integer n > 1.
	 *
	 * 1. Handle the finite exact base case.
	 *
	 * 2. Determine whether n is a proper power m^k, k > 1.
	 *    If it is, output COMPOSITE.
	 *
	 * 3. Choose
	 *
	 *        D > max
	 *            {
	 *                (log n)^2 / (3 * (log 2)^2),
	 *                (log n)^(46/25)
	 *            }.
	 *
	 *    Construct a period system P for n with
	 *
	 *        D <= degree(P) < 2D.
	 *
	 *    See Algorithm 3.1 below.
	 *
	 * 4. Put d = degree(P), and
	 *
	 *        b ~= sqrt(d / 3) * log_2(n).
	 *
	 *    Trial-divide n through max(d, b).  If a divisor is found, return the
	 *    exact result immediately.
	 *
	 * 5. Construct a pseudofield (A, alpha) of characteristic n and degree d
	 *    from the Gaussian periods belonging to P.  See Algorithm 8.3 below.
	 *
	 * 6. For every a = 1, 2, ..., b, verify
	 *
	 *        alpha^n + a == (alpha + a)^n  in A.
	 *
	 *    If one identity fails, output COMPOSITE.  Otherwise output PRIME.
	 *
	 * -------------------------------------------------------------------------
	 * Period-system pseudocode -- Algorithm 3.1
	 * -------------------------------------------------------------------------
	 *
	 * 1. Factor every integer in [1, 2D) with a sieve.
	 *
	 * 2. For every prime r < D^(6/11), r does not divide n, collect prime
	 *    divisors q of r - 1 satisfying
	 *
	 *        q < D^(3/11),
	 *        n^((r - 1) / q) != 1 mod r,
	 *
	 *    and use each q only once.
	 *
	 * 3. Find the least square-free d in [D, 2D) composed only of collected q.
	 *    Return the corresponding pairs (r, q).
	 *
	 * The existence theorem contains a very large effectively-computable
	 * threshold c_4 which the paper does not turn into a practical constant.
	 * Therefore the code first executes the strict paper bounds and, only when
	 * they contain no period system for a practical small input, expands the
	 * conductor search while preserving every period-pair condition.  That
	 * fallback changes no correctness statement; it merely does not claim that
	 * the strict asymptotic existence bound covered that particular small input.
	 *
	 * -------------------------------------------------------------------------
	 * Gaussian-period pseudofield pseudocode -- Algorithm 8.3
	 * -------------------------------------------------------------------------
	 *
	 * For every period pair (r, q):
	 *
	 * 1. In (Z/nZ)[zeta_r], construct
	 *
	 *        eta_(r,q) = sum_{rho in Delta^q} zeta_r^rho.
	 *
	 * 2. Form its q conjugates and the characteristic polynomial
	 *
	 *        f_(r,q)(Y) = product_tau (Y - tau(eta_(r,q))).
	 *
	 * 3. Compute
	 *
	 *        Y^n mod f_(r,q)
	 *
	 *    and verify, after evaluation at eta_(r,q), that it equals the
	 *    n-th cyclotomic automorphism sigma_n(eta_(r,q)).
	 *
	 * 4. Tensor all component pseudofields.  Proposition 7.4 computes the
	 *    tensor characteristic polynomial through formal power series and the
	 *    logarithmic derivative / Hadamard-product identity.  If a required
	 *    small integer is not invertible modulo n, the discovered gcd proves
	 *    COMPOSITE.
	 *
	 * Engineering details:
	 *
	 * - Dense polynomial products use Kronecker substitution, reducing one
	 *   coefficient convolution to one already-published BigInteger multiply.
	 * - Formal-series inversion and exponential use Newton doubling.
	 * - Quotient-ring reduction uses reversed-polynomial division with a
	 *   precomputed Newton inverse.
	 * - Exponentiation uses a left-to-right sliding window.
	 * - Independent final Frobenius identities are distributed over the available
	 *   hardware worker pool.
	 *
	 * No fake shortcut: Gaussian periods, component pseudofields, tensor
	 * products, and the final degree-d Frobenius test are all executed here.
	 * For study and regression testing, the exact finite machine-word base case
	 * can be forced through the complete Gaussian-period path by defining TWILIGHT_DREAM_FORCE_LENSTRA_POMERANCE_GAUSSIAN_PERIOD_PATH.
	 */
class AKS_Test
{
private:
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	/*
		 * ---------------------------------------------------------------------
		 * Object ownership and mathematical representation
		 * ---------------------------------------------------------------------
		 *
		 * These are deliberately NON-STATIC private member functions.
		 *
		 * The previous static-member version only moved free functions into the
		 * class scope; it did not express a real owner/collaborator relationship.
		 * In the current design every arithmetic engine receives `AKS_Test&` and
		 * calls the exact object which created it.  Consequently:
		 *
		 *     AKS_Test
		 *         owns the complete primality-proof procedure;
		 *
		 *     ReciprocalBasedModularReductionEngine
		 *         performs coefficient reduction for that AKS_Test object;
		 *
		 *     KroneckerSubstitutionConvolutionEngine
		 *         converts polynomial convolution into BigInteger multiplication;
		 *
		 *     ModularPolynomialArithmeticEngine
		 *         implements formal power series over Z/nZ;
		 *
		 *     PolynomialQuotientRing
		 *         represents (Z/nZ)[Y] / (f(Y));
		 *
		 *     CyclotomicPrimeRing
		 *         represents (Z/nZ)[zeta_r] with Phi_r(zeta_r) = 0.
		 *
		 * The AKS_Test object itself intentionally stores no shared mutable test
		 * state.  One instance may therefore be reused, while all per-candidate
		 * algebra remains local to operator().
		 */
	class ReciprocalBasedModularReductionEngine;
	class KroneckerSubstitutionConvolutionEngine;
	class ModularPolynomialArithmeticEngine;
	class PolynomialQuotientRing;
	class CyclotomicPrimeRing;
	struct CyclotomicPrimeRingElement;
	struct GaussianPeriodData;

	/**
		 * @brief One Gaussian-period pair (r, q) from Algorithm 3.1.
		 *
		 * `conductor` is the prime r.  `degree` is the prime q dividing r - 1.
		 * The accepted pair satisfies
		 *
		 *     n^((r - 1) / q) != 1 (mod r),
		 *
		 * so the q conjugates of the Gaussian period are not collapsed by the
		 * n-th cyclotomic automorphism.
		 */
	struct PeriodPair
	{
		std::uint64_t conductor = 0;
		std::uint64_t degree = 0;
	};

	/**
		 * @brief Product of pairwise-coprime component degrees.
		 *
		 * The tensor pseudofield degree is
		 *
		 *     d = product(q_i).
		 *
		 * `strict_paper_bounds` records whether the practical search succeeded
		 * inside the asymptotic conductor and degree bounds stated in Algorithm
		 * 3.1, rather than inside the correctness-preserving adaptive extension.
		 */
	struct PeriodSystem
	{
		std::vector<PeriodPair> pairs;
		std::uint64_t			degree = 1;
		std::uint64_t			target_degree = 0;
		bool					strict_paper_bounds = false;
	};

	/* Machine-word arithmetic used only for conductors, sieve indices and
		 * the exact finite base case.  The candidate n remains a BigInteger. */
	std::uint64_t CalculateMachineWordProductModulo( std::uint64_t left, std::uint64_t right, std::uint64_t modulo );

	std::uint64_t CalculateMachineWordPowerModulo( std::uint64_t base, std::uint64_t exponent, std::uint64_t modulo );

	bool IsMachineWordPrime( std::uint64_t number );

	/* Exact sieve and finite-group helpers used by period-system search. */
	std::vector<std::uint32_t> BuildSmallestPrimeFactorTable( std::size_t maximum );
	std::vector<std::uint32_t> GeneratePrimeNumbersWithSieve( std::size_t maximum );
	std::vector<std::uint64_t> FindDistinctPrimeFactorsOfMachineWord( std::uint64_t number );
	std::uint64_t			   FindPrimitiveRootModuloPrime( std::uint64_t prime );
	std::size_t				   CalculateCeilingBinaryLogarithm( std::size_t number );

	/* BigInteger adapter operations.  They use only the already-published
		 * public interface and never modify BigInteger's internal algorithms. */
	std::uint64_t CalculateRemainderByMachineWord( const BigInteger& number, std::uint64_t modulo );

	std::vector<std::uint64_t> ExportMachineWords( const BigInteger& number );
	BigInteger				   ImportMachineWords( std::vector<std::uint64_t> machine_words );

	BigInteger CalculateRemainderWithNewtonDivision( const BigInteger& value, const BigInteger& modulo );

	/**
		 * @brief Obtain the object-bound reciprocal reducer for one modulus.
		 *
		 * The implementation uses a thread-local cache because polynomial
		 * arithmetic repeatedly reduces coefficients modulo the same candidate n.
		 * The cache key includes `this`; it is not a process-global arithmetic
		 * singleton and does not turn these member functions back into disguised
		 * static utilities.
		 */
	const ReciprocalBasedModularReductionEngine& GetReciprocalBasedModularReductionEngine( const BigInteger& modulo );

	BigInteger CalculateRemainderModulo( const BigInteger& value, const BigInteger& modulo );

	BigInteger DivideAndCalculateRemainder( const BigInteger& dividend, const BigInteger& divisor, BigInteger& remainder );

	/* Canonical arithmetic in Z/nZ.  Every result is represented in [0,n). */
	BigInteger CalculateModularSum( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

	BigInteger CalculateModularDifference( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

	BigInteger CalculateModularAdditiveInverse( const BigInteger& value, const BigInteger& modulo );

	BigInteger CalculateModularProduct( const BigInteger& left, const BigInteger& right, const BigInteger& modulo );

	/**
		 * @brief Compute value^(-1) in Z/nZ, or expose a compositeness factor.
		 *
		 * Extended Euclid is written with canonical unsigned residues.  If the
		 * gcd is not one, `discovered_factor` receives gcd(value,n); the failed
		 * inversion is itself a deterministic certificate that n is composite.
		 */
	std::optional<BigInteger> CalculateModularMultiplicativeInverse( const BigInteger& value, const BigInteger& modulo, BigInteger* discovered_factor = nullptr );

	/* Exact proper-power rejection from Algorithm 3.3, Step 2. */
	int CompareIntegerPowerWithLimit( BigInteger base, std::uint32_t exponent, const BigInteger& limit );

	BigInteger CalculateIntegerRoot( const BigInteger& number, std::uint32_t exponent );

	bool IsPerfectPowerExact( const BigInteger& number );
	bool IsPerfectPower( const BigInteger& number );

	/* Rigorous integer envelopes for the paper's real-valued degree bounds. */
	long double CalculateBinaryLogarithmUpperBound( const BigInteger& number );
	BigInteger	CalculateSmallMachineWordPower( std::uint64_t base, unsigned exponent );

	std::uint64_t CalculateExclusiveRationalPowerBound( std::uint64_t base, unsigned numerator, unsigned denominator );

	/**
		 * @brief Construct one degree-q Gaussian-period pseudofield component.
		 *
		 * For a period pair (r,q), this constructs
		 *
		 *     eta = sum_{rho in Delta^q} zeta_r^rho
		 *
		 * inside (Z/nZ)[zeta_r], enumerates its q conjugates, and multiplies
		 *
		 *     product_tau (Y - tau(eta))
		 *
		 * to obtain the monic characteristic polynomial f_(r,q)(Y).
		 */
	GaussianPeriodData ConstructGaussianPeriodPseudofieldComponent( const BigInteger& modulo, std::uint64_t conductor, std::uint64_t degree );

	CyclotomicPrimeRingElement EvaluatePolynomialAtGaussianPeriod( const CyclotomicPrimeRing& ring, const std::vector<BigInteger>& polynomial, const CyclotomicPrimeRingElement& period );

	/* Algorithm 3.1: select D, collect period pairs and reach D <= d < 2D. */
	std::uint64_t CalculateTheoreticalPseudofieldDegree( const BigInteger& number );

	std::optional<PeriodSystem> TryConstructPeriodSystem( const BigInteger& number, std::uint64_t target_degree, std::uint64_t conductor_limit_exclusive, std::uint64_t period_degree_limit_exclusive, bool strict_paper_bounds );

	PeriodSystem ConstructPeriodSystem( const BigInteger& number, std::uint64_t target_degree );

	/* Algorithm 3.3, Steps 4 and 6. */
	std::uint64_t CalculateFrobeniusIdentityTestBound( std::uint64_t degree, const BigInteger& number );

	std::optional<BigInteger> FindTrialDivisor( const BigInteger& number, std::uint64_t maximum );

	/**
		 * @brief Execute the complete Lenstra-Pomerance proof procedure.
		 *
		 * This function performs proper-power rejection, period-system search,
		 * component Gaussian-period verification, tensor-pseudofield construction,
		 * and every final Frobenius identity.  It never falls back to the elementary
		 * X^r - 1 AKS congruence.
		 */
	bool ExecuteLenstraPomeranceGaussianPeriodPrimalityTest( const BigInteger& number );

public:
	/**
		 * @param number Unsigned integer to test.
		 * @return true exactly when number is prime.
		 *
		 * @throws std::overflow_error If the mathematically required auxiliary
		 *         degree cannot be represented by this process address space.
		 * @throws std::runtime_error If the machine cannot allocate the required
		 *         pseudofield or an adaptive period system cannot be constructed
		 *         within a representable search interval.
		 */
	bool operator()( const BigInteger& number );
};

inline AKS_Test AKS_Test_Instance;