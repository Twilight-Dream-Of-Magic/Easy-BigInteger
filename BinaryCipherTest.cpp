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

#include "BinaryCipherTest.hpp"

using BigInteger = TwilightDream::BigInteger::BigInteger;

/*
	This byte-substitution box: Strict avalanche criterion is satisfied !
	ByteDataSecurityTestData Transparency Order Is: 7.85956
	ByteDataSecurityTestData Nonlinearity Is: 112
	ByteDataSecurityTestData Propagation Characteristics Is: 8
	ByteDataSecurityTestData Delta Uniformity Is: 4
	ByteDataSecurityTestData Robustness Is: 0.984375
	ByteDataSecurityTestData Signal To Noise Ratio/Differential Power Analysis Is: 10.3062
	ByteDataSecurityTestData Absolute Value Indicatorer Is: 32
	ByteDataSecurityTestData Sum Of Square Value Indicator Is: 67584
	ByteDataSecurityTestData Algebraic Degree Is: 8
	ByteDataSecurityTestData Algebraic Immunity Degree Is: 4
*/
constexpr std::array<std::uint8_t, 256> ByteSubstitutionBox
{
	0xE2, 0x4E, 0x54, 0xFC, 0x94, 0xC2, 0x4A, 0xCC, 0x62, 0x0D, 0x6A, 0x46, 0x3C, 0x4D, 0x8B, 0xD1,
	0x5E, 0xFA, 0x64, 0xCB, 0xB4, 0x97, 0xBE, 0x2B, 0xBC, 0x77, 0x2E, 0x03, 0xD3, 0x19, 0x59, 0xC1,
	0x1D, 0x06, 0x41, 0x6B, 0x55, 0xF0, 0x99, 0x69, 0xEA, 0x9C, 0x18, 0xAE, 0x63, 0xDF, 0xE7, 0xBB,
	0x00, 0x73, 0x66, 0xFB, 0x96, 0x4C, 0x85, 0xE4, 0x3A, 0x09, 0x45, 0xAA, 0x0F, 0xEE, 0x10, 0xEB,
	0x2D, 0x7F, 0xF4, 0x29, 0xAC, 0xCF, 0xAD, 0x91, 0x8D, 0x78, 0xC8, 0x95, 0xF9, 0x2F, 0xCE, 0xCD,
	0x08, 0x7A, 0x88, 0x38, 0x5C, 0x83, 0x2A, 0x28, 0x47, 0xDB, 0xB8, 0xC7, 0x93, 0xA4, 0x12, 0x53,
	0xFF, 0x87, 0x0E, 0x31, 0x36, 0x21, 0x58, 0x48, 0x01, 0x8E, 0x37, 0x74, 0x32, 0xCA, 0xE9, 0xB1,
	0xB7, 0xAB, 0x0C, 0xD7, 0xC4, 0x56, 0x42, 0x26, 0x07, 0x98, 0x60, 0xD9, 0xB6, 0xB9, 0x11, 0x40,
	0xEC, 0x20, 0x8C, 0xBD, 0xA0, 0xC9, 0x84, 0x04, 0x49, 0x23, 0xF1, 0x4F, 0x50, 0x1F, 0x13, 0xDC,
	0xD8, 0xC0, 0x9E, 0x57, 0xE3, 0xC3, 0x7B, 0x65, 0x3B, 0x02, 0x8F, 0x3E, 0xE8, 0x25, 0x92, 0xE5,
	0x15, 0xDD, 0xFD, 0x17, 0xA9, 0xBF, 0xD4, 0x9A, 0x7E, 0xC5, 0x39, 0x67, 0xFE, 0x76, 0x9D, 0x43,
	0xA7, 0xE1, 0xD0, 0xF5, 0x68, 0xF2, 0x1B, 0x34, 0x70, 0x05, 0xA3, 0x8A, 0xD5, 0x79, 0x86, 0xA8,
	0x30, 0xC6, 0x51, 0x4B, 0x1E, 0xA6, 0x27, 0xF6, 0x35, 0xD2, 0x6E, 0x24, 0x16, 0x82, 0x5F, 0xDA,
	0xE6, 0x75, 0xA2, 0xEF, 0x2C, 0xB2, 0x1C, 0x9F, 0x5D, 0x6F, 0x80, 0x0A, 0x72, 0x44, 0x9B, 0x6C,
	0x90, 0x0B, 0x5B, 0x33, 0x7D, 0x5A, 0x52, 0xF3, 0x61, 0xA1, 0xF7, 0xB0, 0xD6, 0x3F, 0x7C, 0x6D,
	0xED, 0x14, 0xE0, 0xA5, 0x3D, 0x22, 0xB3, 0xF8, 0x89, 0xDE, 0x71, 0x1A, 0xAF, 0xBA, 0xB5, 0x81
};

void BinaryCipher::InitialWithKey( std::vector<uint8_t> Keys )
{
	if ( Keys.empty() )
	{
		throw std::invalid_argument( "Empty key vector provided." );
	}

	Random0 = Bitset512SizeZero;
	Random1 = Bitset512SizeZero;
	Random2 = Bitset512SizeZero;
	Random3 = Bitset512SizeZero;
	Random4 = Bitset512SizeZero;
	Random5 = Bitset512SizeZero;
	Random6 = Bitset512SizeZero;
	Random7 = Bitset512SizeZero;

	uint8_t CounterA = 0;
	uint8_t CounterB = 0;

	std::string		 BinaryString;
	uint8_t			 ByteData = 0x00;
	constexpr size_t ByteBitCount = std::numeric_limits<uint8_t>::digits;
	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random0.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random1.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random2.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random3.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random4.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random5.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random6.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	Random7.FromString( BinaryString, 2 );
	BinaryString.clear();
	CounterB = 0;

	size_t KeyIndex = 0;
	auto   XORWithKeyBytes = []( BigInteger& Random, const std::vector<uint8_t>& Keys, size_t& KeyIndex ) -> void 
	{
		// Loop over each element of the key vector and perform an arithmetic operation on the corresponding Random member.
		for ( size_t i = 0; i < 512; ++i )
		{
			Random.SetBit( Random.GetBit( i ) ^ ( ( Keys[ KeyIndex ] >> ( i % 8 ) ) & 1 ), i );

			// Update the key index to loop through the elements of the key vector
			if(((i + 1) & 8) == 0 && KeyIndex < Keys.size())
			{
				++KeyIndex;
			}
		}
	};

	XORWithKeyBytes( Random0, Keys, KeyIndex );
	XORWithKeyBytes( Random1, Keys, KeyIndex );
	XORWithKeyBytes( Random2, Keys, KeyIndex );
	XORWithKeyBytes( Random3, Keys, KeyIndex );
	XORWithKeyBytes( Random4, Keys, KeyIndex );
	XORWithKeyBytes( Random5, Keys, KeyIndex );
	XORWithKeyBytes( Random6, Keys, KeyIndex );
	XORWithKeyBytes( Random7, Keys, KeyIndex );
}

//Reference web site: https://bisqwit.iki.fi/story/howto/bitmath/
BigInteger BinaryCipher::AdditionBits512( const  BigInteger& a, const  BigInteger& b )
{
	BigInteger sum(Bitset512SizeZero);
	bool carry = false;
	for ( int i = 0; i < 512; ++i )
	{
		bool bit_a = a.GetBit(i);
		bool bit_b = b.GetBit(i);

		// Perform binary addition of the current bit, taking into account rounding
		if (carry)
		{
			sum.SetBit(bit_a != bit_b, i); // The result of an iso-or of 2 bits as a sum of the current bits
			carry = bit_a || bit_b; // Binary addition of the incoming bits as an or-operation of the two current bits
		}
		else
		{
			sum.SetBit(bit_a != bit_b, i); // The result of an exclusive-or of 2 bits as a sum of the current bits
			carry = bit_a && bit_b; // Binary addition of the incoming bits as an and-operation of the two current bits
		}
	}
	return sum;
}

void BinaryCipher::KeyExpansion( const BigInteger& BitsKey, BigInteger& BitsExpansionKey )
{
	//BitsKey is 256 Bit
	//BitsExpansionKey is 512 Bit

	size_t length_bits = BitsKey.BitLength();

	/*Key Padding*/
	if(length_bits < 264)
	{
		std::string binary_string = BitsKey.ToBinaryString(512, false);
		binary_string.append("1");

		// Calculate the number of zeros needed to be added
		const uint32_t k = (448 - length_bits - 1 + 512) % 512;

		// Append k zeros to the binary string
		binary_string.append(k, '0');

		// Append a 64-bit binary representation of the original message length
		std::bitset<64> length_bitset(length_bits);
		binary_string.append(length_bitset.to_string());

		// Ensure the length of binary_string is a multiple of 512
		while (binary_string.length() % 512 != 0)
		{
			binary_string.append("0");
		}

		// Now binary_string has the padded message
		// You need to convert the binary string back to BigInteger and assign it to BitsExpansionKey
		BitsExpansionKey.FromString(binary_string, 2);
		std::cout << "Padded Key:" << BitsExpansionKey.ToBinaryString(512) << std::endl;
	}

	BigInteger TemporaryState( BitsExpansionKey );
	BigInteger TemporaryState2 = BigInteger( Bitset512SizeZero );

	/*PRF*/
	BigInteger RandomA( Bitset512SizeZero ), RandomB( Bitset512SizeZero ), RandomC( Bitset512SizeZero ), RandomD( Bitset512SizeZero ), RandomE( Bitset512SizeZero ), RandomF( Bitset512SizeZero ), RandomG( Bitset512SizeZero ), RandomH( Bitset512SizeZero ), RandomI( Bitset512SizeZero );
	BigInteger Choose( Bitset512SizeZero ), Majority( Bitset512SizeZero ), Choose2( Bitset512SizeZero ), Majority2( Bitset512SizeZero ), Choose3( Bitset512SizeZero ), Majority3( Bitset512SizeZero );

	/*
		The 79 and 11 is prime number
		
		FindNextPrime(79) = 83
		248 = (79 - 11) * (83 + 11) mod 512
		
		FindNextPrime(248) = 251
		FindNextPrime(11) = 13
		88 = (248 - 13) * (251 + 13) mod 512
		
		FindNextPrime(88) = 89
		FindNextPrime(13) = 17
		358 = (88 - 17) * (89 + 17) mod 512
		
		FindNextPrime(358) = 359
		FindNextPrime(17) = 19
		142 = (358 - 19) * (359 + 19) mod 512
		
		FindNextPrime(142) = 149
		FindNextPrime(19) = 23
		500 = (142 - 23) * (149 + 23) mod 512
		
		FindNextPrime(500) = 503
		FindNextPrime(23) = 29
		204 = (500 - 29) * (503 + 29) mod 512
		
		FindNextPrime(204) = 211
		FindNextPrime(29) = 31
		394 = (204 - 31) * (211 + 31) mod 512
		
		FindNextPrime(394) = 397
		FindNextPrime(31) = 37
		314 = (394 - 37) * (397 + 37) mod 512
	*/

	for ( int round = 16; round > 0; round-- )
	{
		Random0 = BigInteger::BitRotateRight(TemporaryState, 314, 512);
		Random1 = BigInteger::BitRotateLeft(TemporaryState, 248, 512);
		Random2 = BigInteger::BitRotateLeft(TemporaryState, 88, 512);
		Random3 = BigInteger::BitRotateRight(TemporaryState, 358, 512);
		Random4 = BigInteger::BitRotateLeft(TemporaryState, 142, 512);
		Random5 = BigInteger::BitRotateRight(TemporaryState, 500, 512);
		Random6 = BigInteger::BitRotateRight(TemporaryState, 204, 512);
		Random7 = BigInteger::BitRotateLeft(TemporaryState, 394, 512);

		RandomA = Random0 ^ Random5;
		RandomB = BigInteger::BitRotateRight(( Random1 ^ Random7 ), 7, 512);
		RandomC = Random2 ^ Random3 ^ Random4;
		RandomD = BigInteger::BitRotateLeft(( Random1 ^ Random4 ), 163, 512);
		RandomE = Random2 ^ Random6;
		RandomF = BigInteger::BitRotateRight(( Random3 ^ Random4 ^ Random5 ), 439, 512);
		RandomG = Random0 ^ Random1 ^ Random3 ^ Random5;
		RandomH = BigInteger::BitRotateLeft(( Random0 ^ Random2 ^ Random4 ^ Random6 ), 257, 512);

		//ALL XOR
		RandomI = Random0 ^ Random1 ^ Random2 ^ Random3 ^ Random4 ^ Random5 ^ Random6 ^ Random7;

		Choose = ( RandomA & RandomB ) ^ ( ~RandomA ^ RandomC );
		Majority = ( RandomA & RandomB ) ^ ( RandomA & RandomC ) ^ ( RandomB & RandomC );
		Choose2 = ( RandomD & RandomE ) ^ ( ~RandomD ^ RandomF );
		Majority2 = ( RandomD & RandomE ) ^ ( RandomD & RandomF ) ^ ( RandomE & RandomF );
		Choose3 = ( RandomG & RandomH ) ^ ( ~RandomG ^ RandomI );
		Majority3 = ( RandomG & RandomH ) ^ ( RandomG & RandomI ) ^ ( RandomH & RandomI );

		auto&& NumberA = ( Choose3 ^ Majority2 );
		auto&& NumberB = ( Choose | Majority3 ) ^ ( Choose2 & Majority );
		
		#ifdef USE_BIG_INTEGER_ARITHMATIC_VERSION

		TemporaryState2 = (NumberA + NumberB) % Number2Power512;
		TemporaryState = (TemporaryState + TemporaryState2) % Number2Power512;
		
		#else

		TemporaryState2 = AdditionBits512( NumberA, NumberB );
		TemporaryState = AdditionBits512( TemporaryState, TemporaryState2 );

		#endif
	}

	#ifdef USE_BIG_INTEGER_ARITHMATIC_VERSION
	BitsExpansionKey = (BitsExpansionKey + TemporaryState) % Number2Power512;
	#else
	BitsExpansionKey = AdditionBits512( BitsExpansionKey, TemporaryState );
	#endif
}

int LeadingZeros( uint32_t x )
{
	if ( x == 0 )
	{
		return 32;	// 如果输入为0，返回32，因为它的所有位都是零
	}

	int count = 0;
	while ( ( x & 0x80000000 ) == 0 )
	{
		x <<= 1;
		++count;
	}

	return count;
}

//Reference web site: https://bisqwit.iki.fi/story/howto/bitmath/
std::bitset<512> AdditionBits512( const std::bitset<512>& a, const std::bitset<512>& b )
{
	std::bitset<512> sum;
	bool carry = false;
	for ( int i = 0; i < 512; ++i )
	{
		bool bit_a = a[i];
		bool bit_b = b[i];

		// Perform binary addition of the current bit, taking into account rounding
		if (carry)
		{
			sum[i] = bit_a != bit_b; // The result of an iso-or of 2 bits as a sum of the current bits
			carry = bit_a || bit_b; // Binary addition of the incoming bits as an or-operation of the two current bits
		}
		else
		{
			sum[i] = bit_a != bit_b; // The result of an exclusive-or of 2 bits as a sum of the current bits
			carry = bit_a && bit_b; // Binary addition of the incoming bits as an and-operation of the two current bits
		}
	}
	return sum;
}

void BinaryCipherNaive::InitialWithKey( std::vector<uint8_t> Keys )
{
	if ( Keys.empty() )
	{
		throw std::invalid_argument( "Empty key vector provided." );
	}

	RandomBits0.reset();
	RandomBits1.reset();
	RandomBits2.reset();
	RandomBits3.reset();
	RandomBits4.reset();
	RandomBits5.reset();
	RandomBits6.reset();
	RandomBits7.reset();

	uint8_t CounterA = 0;
	uint8_t CounterB = 0;

	std::string		 BinaryString;
	uint8_t			 ByteData = 0x00;
	constexpr size_t ByteBitCount = std::numeric_limits<uint8_t>::digits;
	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits0 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits1 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits2 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits3 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits4 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits5 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits6 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;
	++CounterA;
	ByteData ^= CounterA;

	while ( CounterB < 512 / ByteBitCount )
	{
		ByteData = ByteSubstitutionBox[ ByteSubstitutionBox[ ByteData ] ];
		BinaryString.append( std::bitset<ByteBitCount>( ByteData ).to_string() );
		++CounterB;
	}
	RandomBits7 = std::bitset<512>( BinaryString );
	BinaryString.clear();
	CounterB = 0;

	size_t KeyIndex = 0;
	auto   XORWithKeyBytes = []( std::bitset<512>& Random, const std::vector<uint8_t>& Keys, size_t& KeyIndex ) -> void 
	{
		// Loop over each element of the key vector and perform an arithmetic operation on the corresponding Random member.
		for ( size_t i = 0; i < 512; ++i )
		{
			Random[ i ] = ( Random[ i ] ^ ( ( Keys[ KeyIndex ] >> ( i & 8 ) ) & 1 ) );

			// Update the key index to loop through the elements of the key vector
			if(((i + 1) & 8) == 0 && KeyIndex < Keys.size())
			{
				++KeyIndex;
			}
		}
	};

	XORWithKeyBytes( RandomBits0, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits1, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits2, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits3, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits4, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits5, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits6, Keys, KeyIndex );
	XORWithKeyBytes( RandomBits7, Keys, KeyIndex );
}

void BinaryCipherNaive::KeyExpansion( const std::bitset<256>& BitsKey, std::bitset<512>& BitsExpansionKey )
{
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	/* Key Padding */
	std::string binary_string = BitsKey.to_string();
	std::size_t length_bits = binary_string.size();

	if(length_bits < 264)
	{
		// Append "1" to the binary string
		binary_string.push_back('1');

		// Calculate the number of zeros needed to be added
		const uint32_t k = (448 - length_bits - 1 + 512) % 512;

		// Append k zeros to the binary string
		binary_string.append(k, '0');

		// Append a 64-bit binary representation of the original message length
		std::bitset<64> length_bits(BitsKey.size());
		binary_string.append(length_bits.to_string());

		// Ensure the length of binary_string is a multiple of 512
		while (binary_string.length() % 512 != 0)
		{
			binary_string.push_back('0');
		}

		// Now binary_string has the padded message
		// Copy the result to BitsExpansionKey
		BitsExpansionKey = std::bitset<512>(binary_string);
		std::cout << "Padded Key:" << BitsExpansionKey.to_string() << std::endl;
	}

	/*PRF*/
	std::bitset<512> RandomBitsA, RandomBitsB, RandomBitsC, RandomBitsD, RandomBitsE, RandomBitsF, RandomBitsG, RandomBitsH, RandomBitsI;
	std::bitset<512> Choose, Majority, Choose2, Majority2, Choose3, Majority3;

	std::bitset<512> TemporaryState = BitsExpansionKey;
	std::bitset<512> TemporaryState2;

	/*
		The 79 and 11 is prime number
		
		FindNextPrime(79) = 83
		248 = (79 - 11) * (83 + 11) mod 512
		
		FindNextPrime(248) = 251
		FindNextPrime(11) = 13
		88 = (248 - 13) * (251 + 13) mod 512
		
		FindNextPrime(88) = 89
		FindNextPrime(13) = 17
		358 = (88 - 17) * (89 + 17) mod 512
		
		FindNextPrime(358) = 359
		FindNextPrime(17) = 19
		142 = (358 - 19) * (359 + 19) mod 512
		
		FindNextPrime(142) = 149
		FindNextPrime(19) = 23
		500 = (142 - 23) * (149 + 23) mod 512
		
		FindNextPrime(500) = 503
		FindNextPrime(23) = 29
		204 = (500 - 29) * (503 + 29) mod 512
		
		FindNextPrime(204) = 211
		FindNextPrime(29) = 31
		394 = (204 - 31) * (211 + 31) mod 512
		
		FindNextPrime(394) = 397
		FindNextPrime(31) = 37
		314 = (394 - 37) * (397 + 37) mod 512
    */

	//#define DEBUG

#if defined( DEBUG )
	std::cout << "BitsExpansionKeyCopy: " << TemporaryState << '\n';
	std::cout << std::endl;
#endif

	for ( int round = 16; round > 0; round-- )
	{
#if defined( DEBUG )
		std::cout << "Round: " << round << std::endl;
#endif

		RandomBits0 = ( TemporaryState >> 314 ) | ( TemporaryState << 512 - 314 );
		RandomBits1 = ( TemporaryState << 248 ) | ( TemporaryState >> 512 - 248 );
		RandomBits2 = ( TemporaryState << 88 ) | ( TemporaryState >> 512 - 88 );
		RandomBits3 = ( TemporaryState >> 358 ) | ( TemporaryState << 512 - 358 );
		RandomBits4 = ( TemporaryState << 142 ) | ( TemporaryState >> 512 - 142 );
		RandomBits5 = ( TemporaryState >> 500 ) | ( TemporaryState << 512 - 500 );
		RandomBits6 = ( TemporaryState >> 204 ) | ( TemporaryState << 512 - 204 );
		RandomBits7 = ( TemporaryState << 394 ) | ( TemporaryState >> 512 - 394 );

#if defined( DEBUG )
		std::cout << "Initial Random Bits:" << '\n';
		std::cout << "RandomBits0: " << RandomBits0 << '\n';
		std::cout << "RandomBits1: " << RandomBits1 << '\n';
		std::cout << "RandomBits2: " << RandomBitsC << '\n';
		std::cout << "RandomBits3: " << RandomBitsD << '\n';
		std::cout << "RandomBits4: " << RandomBitsE << '\n';
		std::cout << "RandomBits5: " << RandomBitsF << '\n';
		std::cout << "RandomBits6: " << RandomBitsG << '\n';
		std::cout << "RandomBits7: " << RandomBitsH << std::endl;
#endif

		RandomBitsA = RandomBits0 ^ RandomBits5;
		RandomBitsB = ( RandomBits1 ^ RandomBits7 ) >> 7 | ( RandomBits1 ^ RandomBits7 ) << 512 - 7;
		RandomBitsC = RandomBits2 ^ RandomBits3 ^ RandomBits4;
		RandomBitsD = ( RandomBits1 ^ RandomBits4 ) << 163 | ( RandomBits1 ^ RandomBits4 ) >> 512 - 163;
		RandomBitsE = RandomBits2 ^ RandomBits6;
		RandomBitsF = ( RandomBits3 ^ RandomBits4 ^ RandomBits5 ) >> 439 | ( RandomBits3 ^ RandomBits4 ^ RandomBits5 ) << 512 - 439;
		RandomBitsG = RandomBits0 ^ RandomBits1 ^ RandomBits3 ^ RandomBits5;
		RandomBitsH = ( RandomBits0 ^ RandomBits2 ^ RandomBits4 ^ RandomBits6 ) << 257 | ( RandomBits0 ^ RandomBits2 ^ RandomBits4 ^ RandomBits6 ) >> 512 - 257;

		//ALL XOR
		RandomBitsI = RandomBits0 ^ RandomBits1 ^ RandomBits2 ^ RandomBits3 ^ RandomBits4 ^ RandomBits5 ^ RandomBits6 ^ RandomBits7;

#if defined( DEBUG )
		std::cout << "After XOR operations:" << '\n';
		std::cout << "RandomBitsA: " << RandomBitsA << '\n';
		std::cout << "RandomBitsB: " << RandomBitsB << '\n';
		std::cout << "RandomBitsC: " << RandomBitsC << '\n';
		std::cout << "RandomBitsD: " << RandomBitsD << '\n';
		std::cout << "RandomBitsE: " << RandomBitsE << '\n';
		std::cout << "RandomBitsF: " << RandomBitsF << '\n';
		std::cout << "RandomBitsG: " << RandomBitsG << '\n';
		std::cout << "RandomBitsH: " << RandomBitsH << '\n';
		std::cout << "RandomBitsI: " << RandomBitsI << std::endl;
#endif

		Choose = ( RandomBitsA & RandomBitsB ) ^ ( ~RandomBitsA ^ RandomBitsC );
		Majority = ( RandomBitsA & RandomBitsB ) ^ ( RandomBitsA & RandomBitsC ) ^ ( RandomBitsB & RandomBitsC );
		Choose2 = ( RandomBitsD & RandomBitsE ) ^ ( ~RandomBitsD ^ RandomBitsF );
		Majority2 = ( RandomBitsD & RandomBitsE ) ^ ( RandomBitsD & RandomBitsF ) ^ ( RandomBitsE & RandomBitsF );
		Choose3 = ( RandomBitsG & RandomBitsH ) ^ ( ~RandomBitsG ^ RandomBitsI );
		Majority3 = ( RandomBitsG & RandomBitsH ) ^ ( RandomBitsG & RandomBitsI ) ^ ( RandomBitsH & RandomBitsI );

#if defined( DEBUG )
		std::cout << "After Choose and Maj operations:" << '\n';
		std::cout << "Choose: " << Choose << '\n';
		std::cout << "Majority: " << Majority << '\n';
		std::cout << "Choose2: " << Choose2 << '\n';
		std::cout << "Majority2: " << Majority2 << '\n';
		std::cout << "Choose3: " << Choose << '\n';
		std::cout << "Majority3: " << Majority << std::endl;
#endif
		auto&& NumberA = ( Choose3 ^ Majority2 );
		auto&& NumberB = ( Choose | Majority3 ) ^ ( Choose2 & Majority );
		
		TemporaryState2 = AdditionBits512( NumberA, NumberB );
		TemporaryState = AdditionBits512( TemporaryState, TemporaryState2 );

#if defined( DEBUG )
		std::cout << "After final operation:" << '\n';
		std::cout << "BitsExpansionKeyCopy: " << TemporaryState << '\n';
		std::cout << '\n';
		std::cout << "#############################################################" << '\n';
		std::cout << std::endl;
#endif
	}

	BitsExpansionKey = AdditionBits512( BitsExpansionKey, TemporaryState );
}

#ifdef USE_BIG_INTEGER_ARITHMATIC_VERSION
#undef USE_BIG_INTEGER_ARITHMATIC_VERSION
#endif	// USE_BIG_INTEGER_ARITHMATIC_VERSION