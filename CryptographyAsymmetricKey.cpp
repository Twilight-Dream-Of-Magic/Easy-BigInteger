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

#include "CryptographyAsymmetricKey.hpp"

namespace TwilightDream::CryptographyAsymmetric
{
	void RSA::GeneratePrimeNumbers(std::vector<FindPrimeState>& primes, size_t primes_index)
	{
		BigInteger PrimeNumber = BigInteger::RandomGenerateNBit(bit_count);
		PrimeNumber = MIN + (PrimeNumber % RANGE_INTEGER);
		
		//Prime numbers cannot be found inside an even number except two.
		if(PrimeNumber.IsEven())
		{
			PrimeNumber |= ONE;
		}

		bool IsPrime = Tester.IsPrime(PrimeNumber);

		while ( !IsPrime )
		{
			/*
				https://crypto.stackexchange.com/questions/1970/how-are-primes-generated-for-rsa
				Generate a random 512 bit odd number, say p
				Test to see if p is prime; if it is, return p; this is expected to occur after testing about Log(p)/2∼177 candidates
				Otherwise set p=p+2, goto 2
			*/
			PrimeNumber += TWO;

			if (PrimeNumber > MAX)
			{
				PrimeNumber = MIN;
				if (PrimeNumber.IsEven())
				{
					PrimeNumber += ONE;
				}
			}
			IsPrime = Tester.IsPrime(PrimeNumber);
		}

		primes[primes_index].IsPrime = IsPrime;
		primes[primes_index].Number = PrimeNumber;
	}

	std::optional<RSA::FindPrimeState> RSA::GeneratePrimesInParallelFunctions()
	{
		std::vector<std::future<void>> futures;
		const size_t				   max_thread_count = 4; //std::thread::hardware_concurrency();
		std::vector<FindPrimeState>	   prime_map( max_thread_count, FindPrimeState() );

		for ( size_t i = 0; i < prime_map.size(); ++i )
		{
			futures.push_back( std::async( std::launch::async, &RSA::GeneratePrimeNumbers, this, std::ref( prime_map ), i ) );
		}

		// Wait for every prime-search worker and propagate any worker exception.
		for ( auto& Future : futures )
		{
			Future.get();
		}

		for ( const auto& state : prime_map )
		{
			if ( state.IsPrime )
			{
				//std::cout << "Generate BigInteger Is Prime: True" << std::endl;
				return state;
			}
			else
			{
				//std::cout << "Generate BigInteger Is Prime: False" << std::endl;
				continue;
			}
		}

		return std::nullopt;  // Return std::nullopt if no prime is found
	}

	RSA::BigInteger RSA::GeneratePrimeNumber( size_t bit_count )
	{
		if ( bit_count == 0 )
			return BigInteger( 0 );

		this->bit_count = bit_count;
		MIN = BigInteger(TWO).Power(bit_count - 1);
		MAX = BigInteger(TWO).Power(bit_count) - ONE;
		RANGE_INTEGER = MAX - MIN + ONE;

		std::optional<RSA::FindPrimeState> state_data = GeneratePrimesInParallelFunctions();
		
		if(state_data.has_value())
			return state_data.value().Number;
		else
			return BigInteger(0);
	}

	RSA::RSA_AlgorithmNumbers RSA::GenerateKeys( size_t bit_count, bool is_pkcs )
	{
		if(bit_count == 0 || bit_count == 1)
		{
			throw std::invalid_argument("Invalid bit_count values.");
		}

		// Generate two large prime numbers
		BigInteger PrimeNumberA = 0;
		BigInteger PrimeNumberB = 0;

		GeneratePrimesAgain:
		do
		{
			PrimeNumberA = GeneratePrimeNumber(bit_count / 2);
			PrimeNumberB = GeneratePrimeNumber(bit_count - bit_count / 2);
		}
		while (PrimeNumberA.IsZero() || PrimeNumberB.IsZero() || PrimeNumberA == PrimeNumberB);
		
		// Calculate n = p * q
		BigInteger AlgorithmModulus = PrimeNumberA * PrimeNumberB;
		
		if(AlgorithmModulus.BitLength() != bit_count)
		{
			goto GeneratePrimesAgain;
		}

		// Calculate totient(n) = phi(n) = (p - 1) * (q - 1)
		BigInteger Totient_PhiFunctionValue = (PrimeNumberA - ONE) * (PrimeNumberB - ONE);
		
		//std::cout << "---------------------------------------------------------------------------------------------------\n";
		//std::cout << "RSA Algorithm: Number Prime A is: " << PrimeNumberA.ToString(10) << "\n\n";
		//std::cout << "RSA Algorithm: Number Prime B is: " << PrimeNumberB.ToString(10) << "\n\n";
		//std::cout << "RSA Algorithm: Number Modulus is: " << AlgorithmModulus.ToString(10) << "\n\n";
		//std::cout << "RSA Algorithm: Number Totient_PhiFunctionValue is: " << Totient_PhiFunctionValue.ToString(10) << "\n\n\n";

		BigInteger EncryptExponent = 0;
		//Enable security and performance optimization?
		if(is_pkcs)
		{
			EncryptExponent = 65537;
			if (BigInteger::GCD(EncryptExponent, Totient_PhiFunctionValue) != ONE)
			{
				throw std::runtime_error("PKCS exponent 65537 is not coprime with phi(n).");
			}
		}
		else
		{
			BigInteger min = 2;
			BigInteger max = (BigInteger {1} << bit_count - 1) - 1;
			BigInteger range_integer = max - min + 1;

			// Ensure that e is odd and greater than 2
			while ( EncryptExponent <= 2 )
			{
				// Choosing exponents for RSA encryption
				// encryption_exponents = RandomNumberRange(2, 2^{bit\_count} - 1)
				EncryptExponent = BigInteger::RandomGenerateNBit(bit_count + 1) % range_integer + min;

				// Check if e is not 3
				// encryption_exponents equal 3 is not safe
				if(EncryptExponent == THREE)
				{
					continue;
				}

				if(EncryptExponent.IsEven())
				{
					EncryptExponent.SetBit(0);
				}
			}

			// Check if gcd(e, totient(n)) = 1, result is false then repeat this loop.
			do 
			{
				if (BigInteger::GCD(EncryptExponent, Totient_PhiFunctionValue) != ONE)
				{
					EncryptExponent += TWO;
					//std::cout << "Updated Temporary Number EncryptExponent is:" << EncryptExponent.ToString(10) << "\n";
					continue;
				}

				break;
			} while (true);

			//std::cout << "The large odd number EncryptExponent is computed." << "\n";
		}

		// Choosing exponents for RSA decryption
		// Need calculate d such that decryption_exponents = encryption_exponents^{-1} (mod totient(n))
		BigSignedInteger DecryptExponent = BigSignedInteger::ModuloInverse(EncryptExponent, Totient_PhiFunctionValue);

		//std::cout << "The large odd number DecryptExponent is computed." << "\n\n\n";
		//std::cout << "RSA Algorithm: EncryptExponent is: " << EncryptExponent.ToString(10) << "\n\n";
		//std::cout << "RSA Algorithm: DecryptExponent is: " << DecryptExponent.ToString(10) << "\n\n";
		//std::cout << "---------------------------------------------------------------------------------------------------\n";

		RSA::RSA_AlgorithmNumbers AlgorithmNumbers = RSA::RSA_AlgorithmNumbers();
		AlgorithmNumbers.EncryptExponent = EncryptExponent;
		AlgorithmNumbers.DecryptExponent = static_cast<BigInteger>(DecryptExponent);
		AlgorithmNumbers.AlgorithmModulus = AlgorithmModulus;

		return AlgorithmNumbers;
	}

	void RSA::Encryption(BigInteger& PlainMessage, const BigInteger& EncryptExponent, const BigInteger& AlgorithmModulus )
	{
		if(PlainMessage >= AlgorithmModulus)
		{
			return;
		}
		//std::cout << "---------------------------------\n";
		//std::cout << "PlainMessage: " << PlainMessage.ToString( 10 ) << "\n";
		//std::cout << "EncryptExponent: " << EncryptExponent.ToString( 10 ) << "\n";
		//std::cout << "AlgorithmModulus: " << AlgorithmModulus.ToString( 10 ) << "\n";
		PlainMessage.PowerWithModulo(EncryptExponent, AlgorithmModulus);
		//std::cout << "CipherText: " << PlainMessage.ToString( 10 ) << "\n";
		//std::cout << "---------------------------------\n";
	}

	void RSA::Decryption(BigInteger& CipherMessage, const BigInteger& DecryptExponent, const BigInteger& AlgorithmModulus )
	{
		if(CipherMessage >= AlgorithmModulus)
		{
			return;
		}
		//std::cout << "---------------------------------\n";
		//std::cout << "CipherText: " << CipherMessage.ToString( 10 ) << "\n";
		//std::cout << "DecryptExponent: " << DecryptExponent.ToString( 10 ) << "\n";
		//std::cout << "AlgorithmModulus: " << AlgorithmModulus.ToString( 10 ) << "\n";
		CipherMessage.PowerWithModulo(DecryptExponent, AlgorithmModulus);
		//std::cout << "PlainMessage: " << CipherMessage.ToString( 10 ) << "\n";
		//std::cout << "---------------------------------\n";

	}
	
	void FastRSA::GeneratePrimeNumbers(std::vector<FindPrimeState>& primes, size_t primes_index)
	{
		BigInteger PrimeNumber = BigInteger::RandomGenerateNBit(bit_count) % RANGE_INTEGER + MIN;

		if (PrimeNumber.IsEven())
		{
			PrimeNumber += ONE;
		}

		bool isPrime = Tester.IsPrime(PrimeNumber);

		while (!isPrime)
		{
			PrimeNumber += TWO;
			if (PrimeNumber > MAX)
			{
				PrimeNumber = MIN;
				if (PrimeNumber.IsEven())
				{
					PrimeNumber += ONE;
				}
			}
			isPrime = Tester.IsPrime(PrimeNumber);
		}

		primes[primes_index].IsPrime = true;
		primes[primes_index].Number = PrimeNumber;
	}

	std::optional<FastRSA::FindPrimeState> FastRSA::GeneratePrimesInParallelFunctions()
	{
		std::vector<std::future<void>> futures;
		const size_t max_thread_count = 4;
		std::vector<FindPrimeState> prime_map(max_thread_count, FindPrimeState());

		for (size_t i = 0; i < prime_map.size(); ++i)
		{
			futures.push_back(
				std::async(
					std::launch::async,
					&FastRSA::GeneratePrimeNumbers,
					this,
					std::ref(prime_map),
					i
				)
			);
		}

		for ( auto& Future : futures )
		{
			Future.get();
		}

		for (const auto& state : prime_map)
		{
			if (state.IsPrime)
			{
				return state;
			}
		}

		return std::nullopt;
	}

	FastRSA::BigInteger FastRSA::GeneratePrimeNumber(size_t bit_count)
	{
		if ( bit_count == 0 )
		{
			return BigInteger( 0 );
		}

		this->bit_count = bit_count;
		MIN = BigInteger(TWO).Power(bit_count - 1);
		MAX = BigInteger(TWO).Power(bit_count) - ONE;
		RANGE_INTEGER = MAX - MIN + ONE;

		std::optional<FindPrimeState> state_data = GeneratePrimesInParallelFunctions();
		if (state_data.has_value())
		{
			return state_data.value().Number;
		}

		return BigInteger(0);
	}

	FastRSA::KeyPair FastRSA::GenerateKeys(size_t bit_count, bool use_pkcs_public_exponent)
	{
		if (bit_count <= 1)
		{
			throw std::invalid_argument("Invalid bit_count values.");
		}

		BigInteger p = 0;
		BigInteger q = 0;

		GeneratePrimesAgain:
		do
		{
			p = GeneratePrimeNumber(bit_count / 2);
			q = GeneratePrimeNumber(bit_count - bit_count / 2);
		}
		while (p.IsZero() || q.IsZero() || p == q);

		BigInteger n = p * q;
		
		if(n.BitLength() != bit_count)
		{
			goto GeneratePrimesAgain;
		}
		BigInteger phi = (p - ONE) * (q - ONE);

		BigInteger e = 0;
		if (use_pkcs_public_exponent)
		{
			e = 65537;
			if (BigInteger::GCD(e, phi) != ONE)
			{
				throw std::runtime_error("PKCS exponent 65537 is not coprime with phi(n).");
			}
		}
		else
		{
			BigInteger min = 2;
			BigInteger max = (BigInteger{1} << (bit_count - 1)) - 1;
			BigInteger range_integer = max - min + 1;

			while (e <= 2)
			{
				e = BigInteger::RandomGenerateNBit(bit_count + 1) % range_integer + min;

				if (e == THREE)
				{
					continue;
				}

				if (e.IsEven())
				{
					e.SetBit(0);
				}
			}

			while (BigInteger::GCD(e, phi) != ONE)
			{
				e += TWO;
			}
		}

		BigSignedInteger d_signed = BigSignedInteger::ModuloInverse(
			BigSignedInteger(e),
			BigSignedInteger(phi)
		);
		BigInteger d = NormalizeModulo(d_signed, phi);

		KeyPair keyPair;
		keyPair.PublicExponent = e;
		keyPair.PrivateExponent = d;
		keyPair.Modulus = n;

		keyPair.PrimeP = p;
		keyPair.PrimeQ = q;
		keyPair.DP = d % (p - ONE);
		keyPair.DQ = d % (q - ONE);
		keyPair.QInverseModP = ComputeQInverseModP(q, p);

		return keyPair;
	}

	void FastRSA::Encrypt(BigInteger& message, const BigInteger& publicExponent, const BigInteger& modulus) const
	{
		if (modulus.IsZero())
		{
			throw std::invalid_argument("modulus cannot be zero.");
		}

		if (message >= modulus)
		{
			message %= modulus;
		}

		message.PowerWithModulo(publicExponent, modulus);
	}

	void FastRSA::DecryptSlow(BigInteger& cipher, const BigInteger& privateExponent, const BigInteger& modulus) const
	{
		if (modulus.IsZero())
		{
			throw std::invalid_argument("modulus cannot be zero.");
		}

		if (cipher >= modulus)
		{
			cipher %= modulus;
		}

		cipher.PowerWithModulo(privateExponent, modulus);
	}

	void FastRSA::DecryptCRT(BigInteger& cipher, const KeyPair& keyPair) const
	{
		if (keyPair.Modulus.IsZero() || keyPair.PrimeP.IsZero() || keyPair.PrimeQ.IsZero())
		{
			throw std::invalid_argument("invalid CRT key.");
		}

		if (cipher >= keyPair.Modulus)
		{
			cipher %= keyPair.Modulus;
		}

		const BigInteger originalCipher = cipher;

		BigInteger mp = cipher % keyPair.PrimeP;
		mp.PowerWithModulo(keyPair.DP, keyPair.PrimeP);

		BigInteger mq = cipher % keyPair.PrimeQ;
		mq.PowerWithModulo(keyPair.DQ, keyPair.PrimeQ);

		BigInteger diff = NonNegativeModuloDifference(mp, mq, keyPair.PrimeP);

		BigInteger h = keyPair.QInverseModP;
		h *= diff;
		h %= keyPair.PrimeP;

		BigInteger message = mq + h * keyPair.PrimeQ;

		// Optional fault / correctness verification
		BigInteger verify = message;
		verify.PowerWithModulo(keyPair.PublicExponent, keyPair.Modulus);
		if (verify != originalCipher)
		{
			throw std::runtime_error("FastRSA CRT verification failed.");
		}

		cipher = std::move(message);
	}
		
	bool FastRSA::ValidateCRTKeyPair(const KeyPair& keyPair) const
	{
		if (keyPair.PrimeP.IsZero() || keyPair.PrimeQ.IsZero() || keyPair.Modulus.IsZero())
		{
			return false;
		}

		if (keyPair.PrimeP == keyPair.PrimeQ)
		{
			return false;
		}

		if (keyPair.PrimeP * keyPair.PrimeQ != keyPair.Modulus)
		{
			return false;
		}

		BigInteger phi = (keyPair.PrimeP - ONE) * (keyPair.PrimeQ - ONE);

		if (BigInteger::GCD(keyPair.PublicExponent, phi) != ONE)
		{
			return false;
		}

		if ((keyPair.PrivateExponent * keyPair.PublicExponent) % phi != ONE)
		{
			return false;
		}

		if (keyPair.DP != keyPair.PrivateExponent % (keyPair.PrimeP - ONE))
		{
			return false;
		}

		if (keyPair.DQ != keyPair.PrivateExponent % (keyPair.PrimeQ - ONE))
		{
			return false;
		}

		if ((keyPair.PrimeQ * keyPair.QInverseModP) % keyPair.PrimeP != ONE)
		{
			return false;
		}

		return true;
	}
}  // namespace TwilightDream::CryptographyAsymmetric

namespace TwilightDream::CryptographyAsymmetric::SM2
{
	bool EllipticCurvePoint::CheckCurveFormula() const
	{
		using EllitpticCurvePrimeField = TwilightDream::CryptographyAsymmetric::SM2::EllipticCurvePrimeField;

		EllitpticCurvePrimeField x = this->X();
		EllitpticCurvePrimeField y = this->Y();

		// Check the curve equation in Affine coordinates: y^2 (mod prime) = x^3 + ax + b (mod prime)
		return y.power(2) == x.power(3) + EllitpticCurvePrimeField(EllipticCurve::a) * x + EllitpticCurvePrimeField(EllipticCurve::b);
	}

	EllipticCurvePoint EllipticCurvePoint::ConvertAffineToJaccobian() const
	{
		// Handle special case: invalid point (0, 0, 0)
		if (IsInvalidPoint(*this))
		{
			return EllipticCurvePoint(); // Return the invalid point (0, 0, 0)
		}

		// Check if it is the point at infinity
		if (this->IsInfinityPoint())
		{
			// In Jacobian coordinates, the point at infinity is typically represented as (1, 1, 0) 
			// or any point with Z = 0
			return MakeJaccobianInfinity();
		}

		// For an affine point (x, y), convert it to a Jacobian point (x, y, 1)
		return EllipticCurvePoint(x, y, EllipticCurvePrimeField(1));
	}

	EllipticCurvePoint EllipticCurvePoint::ConvertJaccobianToAffine() const
	{
		// Handle special case: invalid point (0, 0, 0)
		if (IsInvalidPoint(*this))
		{
			return EllipticCurvePoint(); // Return the invalid point (0, 0, 0)
		}

		BigSignedInteger x = this->x.GetValue();
		BigSignedInteger y = this->y.GetValue();
		BigSignedInteger z = this->z.GetValue();

		// Use Fermat's Little Theorem to calculate the modular inverse in the prime field
		// (z^{-1} ≡ z^(p-2) mod p)
		BigSignedInteger count = EllipticCurvePrimeField::Prime;
		count -= 2;
		BigSignedInteger z_inverse = z;
		while (!count.IsZero())
		{
			if (count & 1)
			{
				z_inverse = (z_inverse * z) % EllipticCurvePrimeField::Prime;
			}
			z_inverse = (z_inverse * z_inverse) % EllipticCurvePrimeField::Prime;
			count >>= 1;
		}

		BigSignedInteger z_inverse_square = (z_inverse * z_inverse) % EllipticCurvePrimeField::Prime;
		BigSignedInteger z_inverse_cube = (z_inverse_square * z_inverse) % EllipticCurvePrimeField::Prime;

		BigSignedInteger x_affine = (x * z_inverse_square) % EllipticCurvePrimeField::Prime;
		BigSignedInteger y_affine = (y * z_inverse_cube) % EllipticCurvePrimeField::Prime;
		BigSignedInteger z_affine = (this->z.GetValue() * z) % EllipticCurvePrimeField::Prime;

		if (z_affine == 1)
		{
			return EllipticCurvePoint(x_affine, y_affine, EllipticCurvePrimeField(1), false); // Return affine point
		}
		else
		{
			return EllipticCurvePoint(); // Return invalid point (0, 0, 0)
		}
	}

	EllipticCurvePoint EllipticCurvePoint::operator+(const EllipticCurvePoint& other) const
	{
		// First, check if either point is the point at infinity
		if (this->IsInfinityPoint())
			return other; // If *this is the point at infinity, return the other point
		if (other.IsInfinityPoint())
			return *this; // If the other point is the point at infinity, return *this

		EllipticCurvePrimeField x1 = this->x;
		EllipticCurvePrimeField y1 = this->y;
		EllipticCurvePrimeField z1 = this->z;
		EllipticCurvePrimeField x2 = other.x;
		EllipticCurvePrimeField y2 = other.y;
		EllipticCurvePrimeField z2 = other.z;

		// Compute intermediate values
		EllipticCurvePrimeField w1 = x1 * z2;
		EllipticCurvePrimeField w2 = x2 * z1;
		EllipticCurvePrimeField w3 = w1 - w2;
		EllipticCurvePrimeField w4 = y1 * z2;
		EllipticCurvePrimeField w5 = y2 * z1;
		EllipticCurvePrimeField w6 = w4 - w5;

		if (w3.GetValue().IsZero())
		{
			if (w6.GetValue().IsZero())
			{
				// If this == other, return the result of point doubling
				return this->Twice();
			}
			// If this == -other, return the point at infinity
			return EllipticCurvePoint();
		}

		EllipticCurvePrimeField w7 = w1 + w2;
		EllipticCurvePrimeField w8 = z1 * z2;
		EllipticCurvePrimeField w9 = w3.power(2);
		EllipticCurvePrimeField w10 = w3 * w9;
		EllipticCurvePrimeField w11 = w8 * w6.power(2) - w7 * w9;

		// Compute the resulting coordinates
		EllipticCurvePrimeField x3 = w3 * w11;
		EllipticCurvePrimeField y3 = w6 * (w9 * w1 - w11) - w4 * w10;
		EllipticCurvePrimeField z3 = w10 * w8;

		return EllipticCurvePoint(x3.GetValue(), y3.GetValue(), z3.GetValue());
	}

	EllipticCurvePoint EllipticCurvePoint::Twice() const
	{
		// Handle special case: the point at infinity
		if (this->IsInfinityPoint())
			return *this;

		// Handle special case: vertical tangent (y = 0)
		if (this->y.GetValue().IsZero())
			return EllipticCurvePoint();

		EllipticCurvePrimeField x1 = x;
		EllipticCurvePrimeField y1 = y;
		EllipticCurvePrimeField z1 = z;

		// Compute intermediate values
		EllipticCurvePrimeField w1 = (x1.power(2) * EllipticCurvePrimeField(3) + EllipticCurvePrimeField(EllipticCurve::a) * z1.power(2));
		EllipticCurvePrimeField w2 = (y1 << 1) * z1;
		EllipticCurvePrimeField w3 = y1.power(2);
		EllipticCurvePrimeField w4 = w3 * x1 * z1;
		EllipticCurvePrimeField w5 = w2.power(2);
		EllipticCurvePrimeField w6 = w1.power(2) - (w4 << 3);

		// Compute resulting coordinates
		EllipticCurvePrimeField x3 = w2 * w6;
		EllipticCurvePrimeField y3 = w1 * ((w4 << 2) - w6) - ((w5 << 1) * w3);
		EllipticCurvePrimeField z3 = w2 * w5;

		return EllipticCurvePoint(x3.GetValue(), y3.GetValue(), z3.GetValue());
	}

	EllipticCurvePoint EllipticCurvePoint::operator*(const BigInteger& scalar) const
	{
		// Handle special case: the point at infinity
		if (this->IsInfinityPoint())
			return *this;

		// Handle special case: scalar is zero
		if (scalar.IsZero())
			return EllipticCurvePoint();

		// Compute k3 = 3 * scalar
		BigInteger k3 = scalar * 3;

		size_t k3_bit_length = k3.BitLength();
		size_t k_bit_length = scalar.BitLength();

		// Define the negation of the point 
		EllipticCurvePoint negate_point = -(*this);

		// Initialize the result point
		EllipticCurvePoint Q = *this;

		bool k3_bit = false;
		bool k_bit = false;
		// Iterate through the bits of k3, from the second most significant to the least significant
		for (int64_t i = k3_bit_length - 2; i >= 0; i--)
		{
			// Double the current point
			Q = Q.Twice();

			// Get the current bits of k3 and scalar
			k3_bit = k3.GetBit(i);
			
			// If k3_bit_length > k_bit_length and i > k_bit_length - 1, then k_bit is always false.
			// because for a bit block longer than k_bit, the "non-existent" most significant bit of k_bit is always zero.
			if(k3_bit_length > k_bit_length && (i > k_bit_length - 1))
			{
				k_bit = false;
			}
			else
			{
				k_bit = scalar.GetBit(i);
			}

			// Add the appropriate point based on the bit difference
			if (k3_bit != k_bit)
			{
				Q = Q + (k3_bit ? *this : negate_point);
			}
		}

		return Q;
	}

	bool EllipticCurvePoint::operator==(const EllipticCurvePoint& other) const
	{
		// Check if the two points are the same object in memory
		if (this == &other)
		{
			return true;
		}

		// Check if either point is at infinity
		if (this->IsInfinityPoint())
			return other.IsInfinityPoint();
		if (other.IsInfinityPoint())
			return this->IsInfinityPoint();

		// Compare the Y-coordinates using cross-multiplication
		EllipticCurvePrimeField u = other.y * this->z - this->y * other.z;
		if (!u.GetValue().IsZero())
			return false;

		// Compare the X-coordinates using cross-multiplication
		EllipticCurvePrimeField v = other.x * this->z - this->x * other.z;
		return v.GetValue().IsZero();
	}

	bool EllipticCurvePoint::operator!=( const EllipticCurvePoint& other ) const
	{
		return !(*this == other);
	}

	bool EllipticCurvePoint::IsOnCurve(const EllipticCurvePoint& point)
	{
		// Check if the point is invalid
		if (IsInvalidPoint(point))
		{
			return false;
		}

		// Internally converts projective coordinates to affine coordinates using X() and Y(),
		// and validates the point against the elliptic curve equation in affine form.
		return point.CheckCurveFormula();
	}
	
	void SM2KeyPair::GenerateKeyPair()
	{
		// Curve order n
		// min_random_k = 1
		// max_random_k = n - 2
		const BigInteger& min = 1;
		const BigInteger& max = EllipticCurve::n - 2;

		// Generate a random private key in the range [1, n-2]
		BigInteger range = max - min + 1;
		BigInteger random;

		TryAgain:
		do
		{
			random = BigInteger::RandomGenerateNBit(range.BitLength());
		}
		while (random >= range);
		privateKey = random + min;

		// Compute the public key P = [d]G
		publicKey = G * privateKey;

		// Test if the public key point lies on the curve
		if (!EllipticCurvePoint::IsOnCurve(publicKey))
		{
			std::cout << "Key pair generation verification not passed: public key is not on SM2 algorithm curve!" << "\n";
			goto TryAgain;
		}

		std::cout << "Key pair generation verification passed: public key is on the SM2 algorithm curve! \n";
	}

	/*
	
	Papers: Efficient and Secure Elliptic Curve Cryptography Implementation of Curve P-256
	
	inline constexpr bool EllipticCurvePoint::IsOptimized = false;

	EllipticCurvePoint EllipticCurvePoint::operator+( const EllipticCurvePoint& other ) const
	{
		if (this->IsInfinityPoint())
			return other;
		if (other.IsInfinityPoint())
			return *this;

		if(*this == other)
			return this->Twice();

		if constexpr(!IsOptimized)
		{
			const EllipticCurvePoint& P_affine = *this;
			const EllipticCurvePoint& Q_affine = other;

			// 如果 x1 == x2 且 y1 == -y2，则返回无穷远点
			if (P_affine.x == Q_affine.x && P_affine.y == -Q_affine.y)
			{
				return EllipticCurvePoint(EllipticCurvePrimeField("0", 10), EllipticCurvePrimeField("0", 10), EllipticCurvePrimeField(1), true);
			}

			// 点加公式
			EllipticCurvePrimeField lambda = (Q_affine.y - P_affine.y) * (Q_affine.x - P_affine.x).inverse();
			EllipticCurvePrimeField x3 = lambda * lambda - P_affine.x - Q_affine.x;
			EllipticCurvePrimeField y3 = lambda * (P_affine.x - x3) - P_affine.y;
			return EllipticCurvePoint(x3, y3);
		}
		else
		{
			// 提取点 this 和点 other 的坐标
			const EllipticCurvePrimeField& X1 = this->x;
			const EllipticCurvePrimeField& Y1 = this->y;
			const EllipticCurvePrimeField& Z1 = this->z;

			const EllipticCurvePrimeField& X2 = other.x;
			const EllipticCurvePrimeField& Y2 = other.y;
			const EllipticCurvePrimeField& Z2 = other.z;

			// 临时变量
			EllipticCurvePrimeField U1 = X1;
			EllipticCurvePrimeField S1 = Y1;
			EllipticCurvePrimeField U2 = X2;
			EllipticCurvePrimeField S2 = Y2;

			if (Z1 != EllipticCurvePrimeField(BigInteger(1)))
			{
				U1 = X1 * (Z2.power(BigInteger(2)));
				S1 = Y1 * (Z2.power(BigInteger(3)));
			}

			if (Z2 != EllipticCurvePrimeField(BigInteger(1)))
			{
				U2 = X2 * (Z1.power(BigInteger(2)));
				S2 = Y2 * (Z1.power(BigInteger(3)));
			}

			// H = U2 - U1
			EllipticCurvePrimeField H = U2 - U1;
			// R = S2 - S1
			EllipticCurvePrimeField R = S2 - S1;

			if (H == EllipticCurvePrimeField("0", 10))
			{
				if (R == EllipticCurvePrimeField("0", 10))
				{
					// P == Q，进行点倍加
					return this->Twice();
				}
				else
				{
					// P == -Q，返回无穷远点
					return EllipticCurvePoint();
				}
			}

			// HSquared = H**2
			EllipticCurvePrimeField HSquared = H.power(BigInteger(2));
			// G = HSquared * H
			EllipticCurvePrimeField G = HSquared * H;
			// V = HSquared * U1
			EllipticCurvePrimeField V = HSquared * U1;

			// X3 = R**2 - G - 2V
			EllipticCurvePrimeField RSquared = R.power(BigInteger(2));
			EllipticCurvePrimeField X3 = RSquared - G - (V * EllipticCurvePrimeField("2", 10));

			// Y3 = R * (V - X3) - S1 * G
			EllipticCurvePrimeField V_minus_X3 = V - X3;
			EllipticCurvePrimeField Y3 = (R * V_minus_X3) - (S1 * G);

			// Z3 = H * Z1 * Z2
			EllipticCurvePrimeField Z3 = H * Z1 * Z2;

			return EllipticCurvePoint(X3, Y3, Z3);
		}
	}

	EllipticCurvePoint EllipticCurvePoint::Twice() const
	{
		if (this->IsInfinityPoint())
			return *this;

		if constexpr(!IsOptimized)
		{
			const EllipticCurvePoint& P_affine = *this;

			// 点倍加公式
			EllipticCurvePrimeField lambda = (EllipticCurvePrimeField(3) * P_affine.x * P_affine.x + P256V1Curve::a) * (P_affine.y * EllipticCurvePrimeField(2)).inverse();
			EllipticCurvePrimeField x3 = lambda * lambda - P_affine.x * EllipticCurvePrimeField(2);
			EllipticCurvePrimeField y3 = lambda * (P_affine.x - x3) - P_affine.y;
			return EllipticCurvePoint(x3, y3);
		}
		else
		{
			const EllipticCurvePrimeField& X1 = this->x;
			const EllipticCurvePrimeField& Y1 = this->y;
			const EllipticCurvePrimeField& Z1 = this->z;

			if (Y1 == EllipticCurvePrimeField(BigInteger(0)))
				return EllipticCurvePoint(); // 无穷远点

			// S = 4 * X1 * Y1**2
			EllipticCurvePrimeField Y1Squared = Y1.power(BigInteger(2));
			EllipticCurvePrimeField S = X1 * Y1Squared;
			S *= EllipticCurvePrimeField("4", 10);

			// M = 3 * X1**2 + a * Z1**4
			EllipticCurvePrimeField X1Squared = X1.power(BigInteger(2));
			EllipticCurvePrimeField M = X1Squared * EllipticCurvePrimeField("3", 10);
			EllipticCurvePrimeField Z1Squared = Z1.power(BigInteger(2));
			EllipticCurvePrimeField Z1Fourth = Z1Squared.power(BigInteger(2));
			EllipticCurvePrimeField a_times_Z1Fourth = P256V1Curve::a * Z1Fourth.GetValue() % EllipticCurvePrimeField::EllipticCurvePrimeFieldPrime;
			M += a_times_Z1Fourth;

			// X3 = M**2 - 2 * S
			EllipticCurvePrimeField MSquared = M.power(BigInteger(2));
			EllipticCurvePrimeField X3 = MSquared - (S * EllipticCurvePrimeField("2", 10));

			// Y3 = M * (S - X3) - 8 * Y1**4
			EllipticCurvePrimeField S_minus_X3 = S - X3;
			EllipticCurvePrimeField Y1Fourth = Y1Squared.power(BigInteger(2));
			EllipticCurvePrimeField Y3 = (M * S_minus_X3) - (Y1Fourth * EllipticCurvePrimeField("8", 10));

			// Z3 = 2 * Y1 * Z1
			EllipticCurvePrimeField Z3 = Y1 * EllipticCurvePrimeField("2", 10) * Z1;

			return EllipticCurvePoint(X3, Y3, Z3);
		}
	}

	*/

}  // namespace TwilightDream::CryptographyAsymmetric::SM2
