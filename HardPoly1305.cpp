/*
MIT License

Copyright (c) 2024-2050 Twilight-Dream & With-Sky

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

#include "HardPoly1305.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using TwilightDream::BigInteger::BigInteger;

namespace
{
	/** Convert bytes to a lowercase hexadecimal string for self-test diagnostics. */
	std::string bytes_to_hexadecimal_string( const std::vector<uint8_t>& bytes )
	{
		std::ostringstream stream;
		stream << std::hex << std::setfill( '0' );
		for ( uint8_t byte : bytes )
		{
			stream << std::setw( 2 ) << static_cast<unsigned int>( byte );
		}
		return stream.str();
	}

	/** Parse an even-length hexadecimal string used by deterministic KATs. */
	std::vector<uint8_t> hexadecimal_string_to_bytes( const std::string& hexadecimal_string )
	{
		if ( hexadecimal_string.size() % 2 != 0 )
		{
			throw std::invalid_argument( "hexadecimal_string_to_bytes requires an even number of characters" );
		}

		std::vector<uint8_t> bytes;
		bytes.reserve( hexadecimal_string.size() / 2 );

		auto hexadecimal_digit_value = []( char character ) -> uint8_t {
			if ( character >= '0' && character <= '9' )
			{
				return static_cast<uint8_t>( character - '0' );
			}
			if ( character >= 'a' && character <= 'f' )
			{
				return static_cast<uint8_t>( character - 'a' + 10 );
			}
			if ( character >= 'A' && character <= 'F' )
			{
				return static_cast<uint8_t>( character - 'A' + 10 );
			}
			throw std::invalid_argument( "hexadecimal_string_to_bytes encountered a non-hexadecimal character" );
		};

		for ( std::size_t index = 0; index < hexadecimal_string.size(); index += 2 )
		{
			const uint8_t high = hexadecimal_digit_value( hexadecimal_string[ index ] );
			const uint8_t low = hexadecimal_digit_value( hexadecimal_string[ index + 1 ] );
			bytes.push_back( static_cast<uint8_t>( ( high << 4 ) | low ) );
		}

		return bytes;
	}

	bool expect_equal_tag( const std::string& test_name, const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected )
	{
		if ( actual == expected )
		{
			return true;
		}

		std::cerr << test_name << " failed\n"
				  << "  expected: " << bytes_to_hexadecimal_string( expected ) << "\n"
				  << "  actual:   " << bytes_to_hexadecimal_string( actual ) << std::endl;
		return false;
	}
}  // namespace

// ---- Constant definitions ----

const BigInteger HardPoly1305::P1( "1361129467683753853853498429727072845819" );  // 2^130 - 5

const BigInteger HardPoly1305::P2( "115792089237316195423570985008687907853269984665640564039457584007913129451867" );	// 2^256 - 188069

const BigInteger HardPoly1305::MASK_256 = ( BigInteger( 1 ) << 256 ) - BigInteger( 1 );
const BigInteger HardPoly1305::MASK_128 = ( BigInteger( 1 ) << 128 ) - BigInteger( 1 );

// Easy-BigInteger base-16 parsing expects digits only. Do not add a "0x" prefix.
const BigInteger HardPoly1305::CLAMP_MASK( "0ffffffc0ffffffc0ffffffc0fffffff", 16 );

const BigInteger HardPoly1305::A7_SHIFT_248 = BigInteger( 0xA7 ) << 248;
const BigInteger HardPoly1305::TWO_129 = BigInteger( 1 ) << 129;
const BigInteger HardPoly1305::TWO_193 = BigInteger( 1 ) << 193;

// ---- Static helpers ----

BigInteger HardPoly1305::bytes_to_integer_little_endian( const std::vector<uint8_t>& bytes )
{
	BigInteger integer;
	integer.ImportData( bytes, false );
	return integer;
}

std::vector<uint8_t> HardPoly1305::integer_to_bytes_little_endian( const BigInteger& integer, std::size_t length )
{
	// Easy-BigInteger::ExportData is currently non-const, so export from a copy.
	BigInteger			 export_copy = integer;
	std::vector<uint8_t> bytes;
	export_copy.ExportData( bytes, length, false );
	return bytes;
}

BigInteger HardPoly1305::rotate_left_256( const BigInteger& value, uint32_t shift )
{
	const uint32_t	 normalized_shift = shift % 256;
	const BigInteger normalized_value = value & MASK_256;

	if ( normalized_shift == 0 )
	{
		return normalized_value;
	}

	const uint32_t right_shift = 256 - normalized_shift;
	return ( ( normalized_value << normalized_shift ) | ( normalized_value >> right_shift ) ) & MASK_256;
}

void HardPoly1305::derive_pre_mix_parameters( const std::vector<uint8_t>& key, const BigInteger& encoded_context, BigInteger& key_integer, BigInteger& rotated_key_context, BigInteger& multiplier_mask, BigInteger& additive_mask, BigInteger& reduced_key )
{
	if ( key.size() != 32 )
	{
		throw std::invalid_argument( "HardPoly1305-SP key must contain exactly 32 bytes" );
	}

	key_integer = bytes_to_integer_little_endian( key );
	key_integer &= MASK_256;

	const BigInteger key_context = key_integer ^ encoded_context;
	rotated_key_context = rotate_left_256( key_context, 97 );
	multiplier_mask = rotated_key_context & MASK_128;
	additive_mask = ( rotated_key_context >> 128 ) & MASK_128;
	reduced_key = key_integer % P2;
}

void HardPoly1305::rx_transform( const BigInteger& core_output, BigInteger& lower_output, BigInteger& upper_output )
{
	// Four little-endian 64-bit lanes.
	const uint64_t lane_0 = core_output.GetBlock( 0 );
	const uint64_t lane_1 = core_output.GetBlock( 1 );
	const uint64_t lane_2 = core_output.GetBlock( 2 );
	const uint64_t lane_3 = core_output.GetBlock( 3 );

	auto rotate_left_64 = []( uint64_t value, uint32_t shift ) -> uint64_t {
		return static_cast<uint64_t>( ( value << shift ) | ( value >> ( 64 - shift ) ) );
	};

	const uint64_t rotated_0 = rotate_left_64( lane_0, 7 );
	const uint64_t rotated_1 = rotate_left_64( lane_1, 19 );
	const uint64_t rotated_2 = rotate_left_64( lane_2, 37 );
	const uint64_t rotated_3 = rotate_left_64( lane_3, 53 );

	const uint64_t mixed_0 = rotated_1 ^ rotated_2 ^ rotated_3;
	const uint64_t mixed_1 = rotated_0 ^ rotated_2 ^ rotated_3;
	const uint64_t mixed_2 = rotated_0 ^ rotated_1 ^ rotated_3;
	const uint64_t mixed_3 = rotated_0 ^ rotated_1 ^ rotated_2;

	lower_output = BigInteger( mixed_0 ) | ( BigInteger( mixed_1 ) << 64 );
	upper_output = BigInteger( mixed_2 ) | ( BigInteger( mixed_3 ) << 64 );
}

// ---- Class implementation ----

HardPoly1305::HardPoly1305() : key_stored( 0 ), encoded_context_stored( 0 ), rotated_key_context_stored( 0 ), multiplier_mask_stored( 0 ), additive_mask_stored( 0 ), reduced_key_stored( 0 ), reduced_context_stored( 0 ), context_ready( false ) {}

HardPoly1305::~HardPoly1305()
{
	clear_context();
}

void HardPoly1305::clear_context()
{
	// Best-effort logical clearing. See the header warning about dynamic storage.
	key_stored = BigInteger( 0 );
	encoded_context_stored = BigInteger( 0 );
	rotated_key_context_stored = BigInteger( 0 );
	multiplier_mask_stored = BigInteger( 0 );
	additive_mask_stored = BigInteger( 0 );
	reduced_key_stored = BigInteger( 0 );
	reduced_context_stored = BigInteger( 0 );
	context_ready = false;
}

std::vector<uint8_t> HardPoly1305::mix_key_and_message( const std::vector<uint8_t>& message, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce, const std::vector<uint8_t>& tweak, const std::vector<uint8_t>& theta )
{
	if ( key.size() != 32 )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: key must contain exactly 32 bytes" );
	}
	if ( nonce.size() != 12 )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: nonce must contain exactly 12 bytes" );
	}
	if ( tweak.size() != 12 )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: tweak must contain exactly 12 bytes" );
	}
	if ( theta.size() != 8 )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: theta must contain exactly 8 bytes" );
	}

	// E = IntLE(nonce || tweak || LE64(theta)) XOR (0xA7 << 248).
	std::vector<uint8_t> encoded_context_bytes;
	encoded_context_bytes.reserve( 32 );
	encoded_context_bytes.insert( encoded_context_bytes.end(), nonce.begin(), nonce.end() );
	encoded_context_bytes.insert( encoded_context_bytes.end(), tweak.begin(), tweak.end() );
	encoded_context_bytes.insert( encoded_context_bytes.end(), theta.begin(), theta.end() );

	BigInteger encoded_context = bytes_to_integer_little_endian( encoded_context_bytes );
	encoded_context ^= A7_SHIFT_248;
	encoded_context &= MASK_256;

	BigInteger key_integer;
	BigInteger rotated_key_context;
	BigInteger multiplier_mask;
	BigInteger additive_mask;
	BigInteger reduced_key;

	derive_pre_mix_parameters( key, encoded_context, key_integer, rotated_key_context, multiplier_mask, additive_mask, reduced_key );

	key_stored = key_integer;
	encoded_context_stored = encoded_context;
	rotated_key_context_stored = rotated_key_context;
	multiplier_mask_stored = multiplier_mask;
	additive_mask_stored = additive_mask;
	reduced_key_stored = reduced_key;
	reduced_context_stored = encoded_context % P2;
	context_ready = true;

	// The SP PreMix does not alter the message.
	return message;
}

std::vector<uint8_t> HardPoly1305::hard_poly1305_core( const std::vector<uint8_t>& message, const std::vector<uint8_t>& key_override ) const
{
	if ( !context_ready )
	{
		throw std::runtime_error( "HardPoly1305::hard_poly1305_core called before mix_key_and_message" );
	}

	if ( message.size() > static_cast<std::size_t>( std::numeric_limits<uint64_t>::max() ) )
	{
		throw std::length_error( "HardPoly1305-SP message length does not fit the 64-bit frame length" );
	}

	BigInteger multiplier_mask = multiplier_mask_stored;
	BigInteger additive_mask = additive_mask_stored;
	BigInteger reduced_key = reduced_key_stored;

	// The historical API permits a key override. Re-derive it correctly instead
	// of silently ignoring it.
	if ( !key_override.empty() )
	{
		BigInteger override_key_integer;
		BigInteger override_rotated_key_context;

		derive_pre_mix_parameters( key_override, encoded_context_stored, override_key_integer, override_rotated_key_context, multiplier_mask, additive_mask, reduced_key );
	}

	// Frame(message) = LE64(message length) || message || 0xA7.
	std::vector<uint8_t> frame;
	frame.reserve( 8 + message.size() + 1 );

	const uint64_t message_length = static_cast<uint64_t>( message.size() );
	for ( uint32_t byte_index = 0; byte_index < 8; ++byte_index )
	{
		frame.push_back( static_cast<uint8_t>( ( message_length >> ( 8 * byte_index ) ) & 0xFFU ) );
	}
	frame.insert( frame.end(), message.begin(), message.end() );
	frame.push_back( 0xA7 );

	BigInteger	accumulator( 0 );
	std::size_t block_index = 0;

	for ( std::size_t position = 0; position < frame.size(); position += 16, ++block_index )
	{
		const std::size_t block_length = std::min<std::size_t>( 16, frame.size() - position );

		std::vector<uint8_t> block( frame.begin() + static_cast<std::ptrdiff_t>( position ), frame.begin() + static_cast<std::ptrdiff_t>( position + block_length ) );

		// m = IntLE(block) + 2^(8 * block_length).
		BigInteger message_block = bytes_to_integer_little_endian( block );
		message_block += BigInteger( 1 ) << static_cast<uint32_t>( 8 * block_length );

		const BigInteger block_index_integer( static_cast<uint64_t>( block_index ) );

		// c = m + 2^129 * i + 2^193.
		const BigInteger block_encoding = message_block + TWO_129 * block_index_integer + TWO_193;

		// beta = Kbar + Ebar + 5*i + 0xA7 mod P2.
		const BigInteger beta = ( reduced_key + reduced_context_stored + BigInteger( 5 ) * block_index_integer + BigInteger( 0xA7 ) ) % P2;

		// u = h + c + beta mod P2.
		const BigInteger core_input = ( accumulator + block_encoding + beta ) % P2;

		// A = u^3 + 2*beta mod P2.
		const BigInteger core_input_squared = ( core_input * core_input ) % P2;
		const BigInteger core_output = ( core_input_squared * core_input + BigInteger( 2 ) * beta ) % P2;

		BigInteger lower_rx_output;
		BigInteger upper_rx_output;
		rx_transform( core_output, lower_rx_output, upper_rx_output );

		// SplitClamp.
		const BigInteger multiplier = ( lower_rx_output ^ multiplier_mask ) & CLAMP_MASK;
		const BigInteger additive_value = upper_rx_output ^ additive_mask;

		// h = r * (h + m) + s mod P1.
		accumulator = ( multiplier * ( accumulator + message_block ) + additive_value ) % P1;
	}

	const BigInteger tag = accumulator & MASK_128;
	return integer_to_bytes_little_endian( tag, 16 );
}

// ---- Convenience wrapper ----

std::vector<uint8_t> hardpoly1305_sp_tag( const std::vector<uint8_t>& message, const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce, const std::vector<uint8_t>& tweak, const std::vector<uint8_t>& theta )
{
	HardPoly1305 authenticator;
	authenticator.mix_key_and_message( message, key, nonce, tweak, theta );
	return authenticator.hard_poly1305_core( message );
}

// ---- Deterministic self-tests ----

bool hardpoly1305_sp_self_test()
{
	try
	{
		bool all_tests_passed = true;

		// KAT 1: all-zero key/context and empty message.
		{
			const std::vector<uint8_t> message;
			const std::vector<uint8_t> key( 32, 0 );
			const std::vector<uint8_t> nonce( 12, 0 );
			const std::vector<uint8_t> tweak( 12, 0 );
			const std::vector<uint8_t> theta( 8, 0 );
			const std::vector<uint8_t> expected = hexadecimal_string_to_bytes( "12fe2132b7c4ddb56c01ebcd9e78c5b8" );

			const std::vector<uint8_t> actual = hardpoly1305_sp_tag( message, key, nonce, tweak, theta );

			all_tests_passed &= expect_equal_tag( "HardPoly1305-SP KAT 1", actual, expected );
		}

		// KAT 2: 15-byte message, immediately below a 16-byte message boundary.
		{
			std::vector<uint8_t> message( 15 );
			std::vector<uint8_t> key( 32 );
			std::vector<uint8_t> nonce( 12 );
			std::vector<uint8_t> tweak( 12 );
			std::vector<uint8_t> theta( 8 );

			for ( std::size_t index = 0; index < message.size(); ++index )
			{
				message[ index ] = static_cast<uint8_t>( index );
			}
			for ( std::size_t index = 0; index < key.size(); ++index )
			{
				key[ index ] = static_cast<uint8_t>( index );
			}
			for ( std::size_t index = 0; index < nonce.size(); ++index )
			{
				nonce[ index ] = static_cast<uint8_t>( index );
				tweak[ index ] = static_cast<uint8_t>( 0x20 + index );
			}
			for ( std::size_t index = 0; index < theta.size(); ++index )
			{
				theta[ index ] = static_cast<uint8_t>( 0x40 + index );
			}

			const std::vector<uint8_t> expected = hexadecimal_string_to_bytes( "b2facc0397a2446115d0a91ef6e89101" );
			const std::vector<uint8_t> actual = hardpoly1305_sp_tag( message, key, nonce, tweak, theta );

			all_tests_passed &= expect_equal_tag( "HardPoly1305-SP KAT 2", actual, expected );

			// Verify that the old key_override API now really overrides the key.
			HardPoly1305			   authenticator;
			const std::vector<uint8_t> wrong_initial_key( 32, 0 );
			authenticator.mix_key_and_message( message, wrong_initial_key, nonce, tweak, theta );

			const std::vector<uint8_t> override_actual = authenticator.hard_poly1305_core( message, key );
			all_tests_passed &= expect_equal_tag( "HardPoly1305-SP key override", override_actual, expected );
		}

		// KAT 3: 17-byte message, immediately above a 16-byte message boundary.
		{
			std::vector<uint8_t>	   message( 17 );
			std::vector<uint8_t>	   key( 32 );
			const std::vector<uint8_t> nonce( 12, 0xA5 );
			const std::vector<uint8_t> tweak( 12, 0x5A );
			const std::vector<uint8_t> theta = { 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01 };

			for ( std::size_t index = 0; index < message.size(); ++index )
			{
				message[ index ] = static_cast<uint8_t>( index );
			}
			for ( std::size_t index = 0; index < key.size(); ++index )
			{
				key[ index ] = static_cast<uint8_t>( 0xFF - index );
			}

			const std::vector<uint8_t> expected = hexadecimal_string_to_bytes( "97e58f7c773a40d6af4e05ad3fa7829d" );
			const std::vector<uint8_t> actual = hardpoly1305_sp_tag( message, key, nonce, tweak, theta );

			all_tests_passed &= expect_equal_tag( "HardPoly1305-SP KAT 3", actual, expected );
		}

		// The specification requires exactly 32 key bytes.
		{
			bool rejected_short_key = false;
			bool rejected_long_key = false;

			try
			{
				hardpoly1305_sp_tag( {}, std::vector<uint8_t>( 31, 0 ) );
			}
			catch ( const std::invalid_argument& )
			{
				rejected_short_key = true;
			}

			try
			{
				hardpoly1305_sp_tag( {}, std::vector<uint8_t>( 33, 0 ) );
			}
			catch ( const std::invalid_argument& )
			{
				rejected_long_key = true;
			}

			if ( !rejected_short_key || !rejected_long_key )
			{
				std::cerr << "HardPoly1305-SP exact key-length test failed" << std::endl;
				all_tests_passed = false;
			}
		}

		if ( all_tests_passed )
		{
			std::cout << "HardPoly1305-SP self-test: all tests passed" << std::endl;
		}

		return all_tests_passed;
	}
	catch ( const std::exception& exception )
	{
		std::cerr << "HardPoly1305-SP self-test raised an exception: " << exception.what() << std::endl;
		return false;
	}
}

void test_hard_poly1305()
{
	if ( !hardpoly1305_sp_self_test() )
	{
		throw std::runtime_error( "HardPoly1305-SP self-test failed" );
	}
}