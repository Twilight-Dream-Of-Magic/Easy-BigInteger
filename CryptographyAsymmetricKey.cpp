/*
MIT License

Copyright (c) 2024 Twilight-Dream & With-Sky

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
	void RSA::GeneratePrimesInParallelFunction(std::vector<FindPrimeState>& primes, size_t primes_index)
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
			if(!IsPrime)
			{
				PrimeNumber += TWO;
			}

			//size_t BitSize = PrimeNumber.BitSize();
			//std::cout << "Generate BigInteger Bit Size Is :" << BitSize << std::endl;
			IsPrime = Tester.IsPrime(PrimeNumber);
		}

		primes[primes_index].IsPrime = IsPrime;
		primes[primes_index].Number = PrimeNumber;
	}

	std::optional<RSA::FindPrimeState> RSA::GeneratePrimesInParallelFunctions(size_t bit_count)
	{
		std::vector<std::future<void>> futures;
		const size_t				   max_thread_count = 4; //std::thread::hardware_concurrency();
		std::vector<FindPrimeState>	   prime_map( max_thread_count, FindPrimeState() );

		for ( size_t i = 0; i < prime_map.size(); ++i )
		{
			futures.push_back( std::async( std::launch::async, &RSA::GeneratePrimesInParallelFunction, this, std::ref( prime_map ), i ) );
		}

		size_t counter = 0; // 初始化为 0，因为你可能需要等待所有线程完成
		while (counter < futures.size())
		{
			for (size_t i = 0; i < futures.size(); ++i)
			{
				auto& future = futures[i];
				auto status = future.wait_for(std::chrono::seconds(1));
				if (status == std::future_status::ready)
				{
					if (i == counter)
					{
						++counter;
					}
				}
				else if (status == std::future_status::timeout)
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
			}
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
		this->bit_count = bit_count;
		MIN = BigInteger(2).Power(bit_count); // MIN: 2^{bit\_count}
		MAX = BigInteger(2).Power(bit_count + 1) - ONE; // MAX: 2^{bit\_count + 1} - 1
		RANGE_INTEGER = MAX - MIN + ONE;

		if(this->bit_count == 0)
			return BigInteger(0);

		std::optional<RSA::FindPrimeState> state_data = GeneratePrimesInParallelFunctions(bit_count);
		
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
		BigInteger PrimeNumberA = GeneratePrimeNumber(bit_count / 2);
		if(!PrimeNumberA.IsZero())
		{
			std::cout << "The large prime A has been generated." << "\n";
		}

		BigInteger PrimeNumberB = GeneratePrimeNumber(bit_count / 2);
		if(!PrimeNumberB.IsZero())
		{
			std::cout << "The large prime B has been generated." << "\n";
		}
		
		// Calculate n = p * q
		BigInteger AlgorithmModulus = PrimeNumberA * PrimeNumberB;

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
			EncryptExponent = 65537;
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
}  // namespace TwilightDream::CryptographyAsymmetric
