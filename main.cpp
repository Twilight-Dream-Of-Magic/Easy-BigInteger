#include "BigInteger.hpp"
#include "BigIntegerTest.hpp"
#include "BigFraction.hpp"
#include "BinaryCipherTest.hpp"
#include "CryptographyAsymmetricKey.hpp"
#include "HardPoly1305.hpp"

#include <iomanip>
#include <cassert>

namespace SM2Tests
{
	using namespace TwilightDream::CryptographyAsymmetric;

	inline void TestPrintPointCompute()
	{
		using BigInteger = TwilightDream::BigInteger::BigInteger;

		SM2::EllipticCurvePoint Point = SM2::G;
		SM2::EllipticCurvePoint NegationPoint = -SM2::G;
		SM2::EllipticCurvePoint DoublePoint = Point.Twice();
		SM2::EllipticCurvePoint AddPoint = Point + DoublePoint;

		std::cout << "Original Generator G Point: " << Point << std::endl;
		std::cout << "Negation of Generator G Point: " << NegationPoint << std::endl;
		std::cout << "G + G Double Point: " << DoublePoint << std::endl;
		std::cout << "G + 2G Added Point: " << AddPoint << std::endl;

		std::cout << "Original G Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(Point) << std::endl;
		std::cout << "Negation of G Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(NegationPoint) << std::endl;
		std::cout << "G + G Double Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(DoublePoint) << std::endl;
		std::cout << "G + 2G Added Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(AddPoint) << std::endl;
		
		BigInteger k1 = 1;
		BigInteger k2 = 2;
		BigInteger k3 = 3;
		BigInteger k4 = 4;
		SM2::EllipticCurvePoint ScalarMultiplied_1_Point = Point * k1;
		SM2::EllipticCurvePoint ScalarMultiplied_2_Point = Point * k2;
		SM2::EllipticCurvePoint ScalarMultiplied_3_Point = Point * k3;
		SM2::EllipticCurvePoint ScalarMultiplied_4_Point = Point * k4;

		std::cout << "[1]G Scalar with Multiplied Point: " << ScalarMultiplied_1_Point << std::endl;
		std::cout << "[2]G Scalar with Multiplied Point: " << ScalarMultiplied_2_Point << std::endl;
		std::cout << "[3]G Scalar with Multiplied Point: " << ScalarMultiplied_3_Point << std::endl;
		std::cout << "[4]G Scalar with Multiplied Point: " << ScalarMultiplied_4_Point << std::endl;

		std::cout << "[1]G Scalar with Multiplied Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(ScalarMultiplied_1_Point) << std::endl;
		std::cout << "[2]G Scalar with Multiplied Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(ScalarMultiplied_2_Point) << std::endl;
		std::cout << "[3]G Scalar with Multiplied Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(ScalarMultiplied_3_Point) << std::endl;
		std::cout << "[4]G Scalar with Multiplied Point is on the curve? : " << std::boolalpha << SM2::EllipticCurvePoint::IsOnCurve(ScalarMultiplied_4_Point) << std::endl;
	
		//Result must be true
		std:: cout << "DoublePoint == ScalarMultiplied_1_Point:" << std::boolalpha << (DoublePoint == ScalarMultiplied_1_Point) << std::endl;
		//Result must be false
		std:: cout << "DoublePoint == ScalarMultiplied_2_Point:" << std::boolalpha << (DoublePoint == ScalarMultiplied_2_Point) << std::endl;

		std::cout << "All Point compute test passed! \n";
	}

	inline void TestPointCompute()
	{
		SM2::EllipticCurvePoint A = SM2::G + SM2::G;
		SM2::EllipticCurvePoint B = SM2::G.Twice();
		assert( A == B );
		std::cout << "Point computation test passed: G + G == 2G.\n";

		A = SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G;
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G == 3G.\n";

		A = SM2::G + SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G.Twice();
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G + G == 4G.\n";

		A = SM2::G + SM2::G + SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G.Twice() + SM2::G;
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G + G + G == 5G.\n";

		A = SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G.Twice() + SM2::G.Twice();
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G + G + G + G == 6G.\n";

		A = SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G.Twice() + SM2::G.Twice() + SM2::G;
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G + G + G + G + G == 7G.\n";

		A = SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G + SM2::G;
		B = SM2::G.Twice() + SM2::G.Twice() + SM2::G.Twice() + SM2::G.Twice();
		assert( A == B );
		std::cout << "Point computation test passed: G + G + G + G + G + G + G + G == 8G.\n";
	}

	inline void TestPointGroup()
	{
		SM2::EllipticCurvePoint TestPoint = SM2::G;
		SM2::EllipticCurvePoint TestPoint2 = SM2::G;
		
		if(SM2::EllipticCurvePoint::IsOnCurve(TestPoint))
		{
			std::cout << "G is on the curve.\n";
		}

		std::vector<size_t> FailedTestPoints;
		for ( size_t number = 0; number < 256; number++ )
		{
			TestPoint2 = TestPoint + TestPoint2;
			if(!SM2::EllipticCurvePoint::IsOnCurve(TestPoint2))
			{
				FailedTestPoints.push_back(number);
				std::cout << "Addition of Point " << number + 2 << "G is not on the curve.\n";
			}
			else
			{
				std::cout << "Emmm......  Addition of Point " << number + 2 << "G is on the curve!\n";
			}
		}

		if(!FailedTestPoints.empty())
		{
			FailedTestPoints.clear();
			assert(false);
		}

		TestPoint = SM2::G;
		TestPoint2 = SM2::G;
		for( size_t number = 0; number < 256; number++ )
		{
			TestPoint = TestPoint.Twice();
			if(!SM2::EllipticCurvePoint::IsOnCurve(TestPoint))
			{
				FailedTestPoints.push_back(number);
				std::cout << "Doubling of Point " << number + 2 << "G is not on the curve.\n";
			}
			else
			{
				std::cout << "Emmm......  Doubling of Point " << number + 2 << "G is on the curve!\n";
			}
		}

		if(!FailedTestPoints.empty())
		{
			FailedTestPoints.clear();
			assert(false);
		}

		std::cout << "Point group test passed: G^256 is on the curve.\n";
	}

	inline void TestKeyPair()
	{
		// 生成密钥对
		SM2::SM2KeyPair keyPair;

		// 获取私钥和公钥
		const auto& privateKey = keyPair.GetPrivateKey();
		const auto& publicKey = keyPair.GetPublicKey();
		const auto& n = SM2::EllipticCurve::n;

		// 测试私钥范围
		assert(privateKey > 0 && privateKey < (n - 2));
		std::cout << "Private key range test passed: 1 <= d <= n-2.\n";

		// 打印密钥对
		std::cout << "Generated key pair:\n";
		std::cout << "  Private key: " << privateKey.ToString(16) << "\n";
		std::cout << "  Public key: " << publicKey << "\n";
	}

	inline void TestInfinityPoint()
	{
		SM2::EllipticCurvePoint Infinity;
		SM2::EllipticCurvePoint G = SM2::G;
		// P + O = P
		SM2::EllipticCurvePoint G_plus_O = G + Infinity;
		assert( G_plus_O == G );
		// O + P = P
		SM2::EllipticCurvePoint O_plus_G = Infinity + G;
		assert( O_plus_G == G );
		// O + O = O
		SM2::EllipticCurvePoint O_plus_O = Infinity + Infinity;
		assert( O_plus_O == Infinity );
		std::cout << "Infinity point operations test passed.\n";
	}

	inline void TestPointInverse()
	{
		// Calculate -G
		SM2::EllipticCurvePoint minusG = -SM2::G;
		// G + (-G) = O
		SM2::EllipticCurvePoint G_plus_minusG = SM2::G + minusG;
		SM2::EllipticCurvePoint Infinity;
		assert( G_plus_minusG == Infinity );
		std::cout << "Point inverse test passed: G + (-G) == O.\n";
	}

	inline void TestRandomPointsOnCurve()
	{
		// Example: Generate multiple points through scalar multiplication
		SM2::SM2KeyPair keyPair1;
		SM2::SM2KeyPair keyPair2;
		SM2::SM2KeyPair keyPair3;
		
		// 打印密钥对
		std::cout << "Generated key pair:\n";
		std::cout << " Private key: " << keyPair1.GetPrivateKey().ToString(16) << "\n";
		std::cout << " Public key: " << keyPair1.GetPublicKey() << "\n";

		std::cout << "Generated key pair:\n";
		std::cout << " Private key: " << keyPair2.GetPrivateKey().ToString(16) << "\n";
		std::cout << " Public key: " << keyPair2.GetPublicKey() << "\n";

		std::cout << "Generated key pair:\n";
		std::cout << " Private key: " << keyPair3.GetPrivateKey().ToString(16) << "\n";
		std::cout << " Public key: " << keyPair3.GetPublicKey() << "\n";
		
		std::cout << "All Random points on curve test passed.\n";
	}

	inline void TestBoundaryConditions()
	{
		SM2::EllipticCurvePrimeField Zero( "0", 10 );
		// max = p - 1 (mod p)
		SM2::EllipticCurvePrimeField Max = SM2::EllipticCurvePrimeField( "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFE", 16 );
		// 0 + 0 = 0
		SM2::EllipticCurvePrimeField Sum = Zero + Zero;
		assert( Sum == Zero );
		// Max + 1 = 0 (mod p)
		SM2::EllipticCurvePrimeField One( "1", 10 );
		SM2::EllipticCurvePrimeField Overflow = Max + One;
		assert( Overflow == Zero );
		std::cout << "Boundary conditions test passed.\n";
	}

	inline void TestLargeScalarMultiplication()
	{
		// Use a large scalar, such as n - 1
		const auto&		 n = SM2::EllipticCurve::n;
		SM2::EllipticCurvePoint P = SM2::G * ( n - 1 );
		SM2::EllipticCurvePoint MinusG = SM2::EllipticCurvePoint( SM2::G.X(), -SM2::G.Y() );
		assert( P == MinusG );
		std::cout << "Large scalar multiplication test passed: G * (n - 1) == -G.\n";
	}

	inline void RunAllTests()
	{
		TestPointCompute();
		TestPrintPointCompute();
		TestKeyPair();
		TestInfinityPoint();
		TestPointInverse();
		TestRandomPointsOnCurve();
		TestBoundaryConditions();
		TestLargeScalarMultiplication();

		//NOTE: Do you need to test the correctness of the curve point group? 
		// Correctness test result: Yes! Passed (Please note that this function only needs to be tested when the function fails.)
		//TestPointGroup();
		std::cout << "SM2 All tests passed!\n";
	}

}  // namespace SM2Tests

inline void test_negative_big_fraction()
{
	// Initialize a BigFraction with a negative fraction -3/7
	TwilightDream::BigInteger::BigInteger numerator( 3 );	 // Positive numerator
	TwilightDream::BigInteger::BigInteger denominator( 7 );	 // Positive denominator
	int									  sign = -1;		 // Negative sign for the fraction

	// Create the BigFraction with the negative sign
	TwilightDream::BigFraction::BigFraction negativeFraction( numerator, denominator, sign );

	// Set a very high precision denominator for the final value
	TwilightDream::BigInteger::BigInteger FullPrecision(
		"1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 
		10
	);
	negativeFraction.SetFullPrecision(FullPrecision);

	// Output the fraction to check initialization
	std::cout << "Negative fraction (-3/7): " << negativeFraction << std::endl;

	// Perform operations on the negative fraction
	std::cout << "Negative fraction squared: " << negativeFraction.Power( 2 ) << std::endl;
	std::cout << "Square root of the negative fraction: " << negativeFraction.Sqrt() << std::endl;  // Should handle or raise an error if negative
	std::cout << "Negative fraction multiplied by 2/3: " << ( negativeFraction * TwilightDream::BigFraction::BigFraction( TwilightDream::BigInteger::BigInteger( 2 ), TwilightDream::BigInteger::BigInteger( 3 ), 1 ) ) << std::endl;

	// Check inversion (should handle the sign correctly)
	std::cout << "Reciprocal negative fraction: " << negativeFraction.Reciprocal() << std::endl;
}

auto main( int argument_cout, char* argument_vector[] ) -> int
{
	//SM2Tests::RunAllTests();

	//TEST PASSED
	/*TwilightDream::BigInteger::BigInteger TestNumber( "123456789", 10 );
	TwilightDream::BigInteger::BigInteger TestNumberPower = TwilightDream::BigInteger::BigInteger( TestNumber ).Power( 14 );
	TwilightDream::BigInteger::ShiftingKthRoot kthRootCalculator( 14 );
	TwilightDream::BigInteger::BigInteger	   TestNumberKthRoot = kthRootCalculator( TestNumberPower );
	if ( TestNumberKthRoot == TestNumber )
	{
		std::cout << "True" << std::endl;
	}
	else
	{
		std::cout << "False" << std::endl;
	}*/

	//TEST PASSED
	/*auto pi = TwilightDream::BigFraction::BigFraction::GenerateSrinivasaRamanujanPI();
	pi.PrecisionMode = TwilightDream::BigFraction::DecimalPrecisionMode::Fixed;
	pi.FixedPrecisionCount = 10000;
	std::cout << pi << std::endl;*/

	//TEST PASSED
	//std::cout << TwilightDream::BigInteger::BigInteger::Factorial(10000).ToString(10) << std::endl;
	
	//test_negative_big_fraction();

	TwilightDream::BigFraction::BigFraction TestBigFraction(12,48);
	TestBigFraction.PrecisionMode = TwilightDream::BigFraction::DecimalPrecisionMode::Fixed;
	TestBigFraction.FixedPrecisionCount = 100;
	std::cout << TestBigFraction.Power(3) << std::endl;
	//FIXME
	std::cout << TestBigFraction.Power(TwilightDream::BigFraction::BigFraction(1, 4)) << std::endl;
	std::cout << TestBigFraction.Sqrt() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Cbrt() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Log() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Log10() << std::endl;
	//FIXME
	std::cout << TestBigFraction.NthRoot(4) << std::endl;

	TestBigFraction.PrecisionMode = TwilightDream::BigFraction::DecimalPrecisionMode::Full;
	TwilightDream::BigInteger::BigInteger FullPrecision("1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", 10);
	TestBigFraction.SetFullPrecision(FullPrecision);
	std::cout << TestBigFraction.Power(3) << std::endl;
	std::cout << TestBigFraction.Sqrt() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Cbrt() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Log() << std::endl;
	//FIXME
	std::cout << TestBigFraction.Log10() << std::endl;
	//FIXME
	std::cout << TestBigFraction.NthRoot(4) << std::endl;
	
	TwilightDream::BigInteger::Test::test_all();
	TwilightDream::BigInteger::Test::test_custom_data();

	using PrimeNumberTester = TwilightDream::PrimeNumberTester;
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	//BigInteger BigPrime(std::string("16158503035655503650357438344334975980222051334857742016065172713762327569433945446598600705761456731844358980460949009747059779575245460547544076193224141560315438683650498045875098875194826053398028819192033784138396109321309878080919047169238085235290822926018152521443787945770532904303776199561965192760957166694834171210342487393282284747428088017663161029038902829665513096354230157075129296432088558362971801859230928678799175576150822952201848806616643615613562842355410104862578550863465661734839271290328348967522998634176499319107762583194718667771801067716614802322659239302476074096777926805529798115243"), 10);
	//PrimeNumberTester NumberTester;
	//std::cout << std::boolalpha << NumberTester.IsPrime(BigPrime) << std::endl;

	//TEST PASSED
	//TwilightDream::CryptographyAsymmetric::RSA::SelfSanityCheck(4096, 10);

	BinaryCipher BinaryCipherInstance;
	BinaryCipherInstance.Test();
	
	BinaryCipherNaive BinaryCipherNaiveInstance;
	BinaryCipherNaiveInstance.Test();

	test_hard_poly1305();

	return 0;
}
