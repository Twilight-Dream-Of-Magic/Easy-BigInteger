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

#include "HardPoly1305.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

// ---------------------------------------------------------------------
// 工具函数实现
// ---------------------------------------------------------------------

std::vector<uint8_t> SubByteArray( const std::vector<uint8_t>& data,
								   ptrdiff_t start,
								   ptrdiff_t end,
								   ptrdiff_t step )
{
	if ( step == 0 )
	{
		return {};
	}

	ptrdiff_t dataSize = static_cast<ptrdiff_t>( data.size() );

	if ( start < 0 )
	{
		start += dataSize;
	}
	if ( end < 0 )
	{
		end += dataSize;
	}

	start = std::max<ptrdiff_t>( 0, start );
	end = std::min<ptrdiff_t>( end, dataSize );

	if ( start >= end )
	{
		return {};
	}

	std::vector<uint8_t> sub_array;
	if ( step > 0 )
	{
		auto first = data.begin() + start;
		auto last  = data.begin() + end;
		std::copy_if(
			first,
			last,
			std::back_inserter( sub_array ),
			[ n = 0, step ]( const uint8_t& ) mutable
			{
				return n++ % step == 0;
			} );
	}
	else
	{
		auto first = data.rbegin() + ( dataSize - end );
		auto last  = data.rbegin() + ( dataSize - start );
		std::copy_if(
			first,
			last,
			std::back_inserter( sub_array ),
			[ n = 0, step ]( const uint8_t& ) mutable
			{
				return n++ % -step == 0;
			} );
	}

	return sub_array;
}

std::string BytesToHexString( const std::vector<uint8_t>& bytes )
{
	std::ostringstream oss;
	for ( const auto& byte : bytes )
	{
		oss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<int>( byte );
	}
	return oss.str();
}

std::vector<uint8_t> generate_random_bytes( size_t size )
{
	std::vector<uint8_t>		  bytes( size );
	std::random_device			  rd;
	std::mt19937				  gen( rd() );
	std::uniform_int_distribution dis( 0, 255 );
	for ( size_t i = 0; i < size; ++i )
	{
		bytes[ i ] = static_cast<uint8_t>( dis( gen ) );
	}
	return bytes;
}

// ---------------------------------------------------------------------
// [Sec 1.1] 从 master_key 派生 HardPoly1305KeyParams
// ---------------------------------------------------------------------
HardPoly1305KeyParams HardPoly1305::derive_key_parameters( const std::vector<uint8_t>& master_key ) const
{
	if ( master_key.size() < 32 )
	{
		throw std::invalid_argument( "HardPoly1305 V2-Lite requires master_key >= 32 bytes" );
	}

	// 只取前 32 字节
	std::vector<uint8_t> key32( master_key.begin(), master_key.begin() + 32 );

	// k_h <- key[0:16] (LE) mod p
	std::vector<uint8_t> key_lo( key32.begin(), key32.begin() + 16 );
	std::vector<uint8_t> key_hi( key32.begin() + 16, key32.begin() + 32 );

	BigSignedInteger k_high; //Key Bit high part
	BigSignedInteger k_low; //Key Bit low part (math : k_x)
	BigSignedInteger k_mix;
	BigSignedInteger k_mix2;

	k_high.ImportData( false, key_lo ); // little-endian
	k_high %= p;

	// k_x <- key[16:32] (LE) mod p
	k_low.ImportData( false, key_hi );
	k_low %= p;

	// k_mix <- key[0:32] (LE) mod p2
	k_mix.ImportData( false, key32 );
	k_mix %= p2;

	// k_mix2 = (3 * k_mix + GOLDEN_RATIO_CONST) mod p2
	BigSignedInteger golden_const( "9E3779B97F4A7C15", 16 );
	k_mix2 = ( k_mix * BigSignedInteger( 3 ) + golden_const ) % p2;

	return HardPoly1305KeyParams( k_high, k_low, k_mix, k_mix2 );
}

// ---------------------------------------------------------------------
// [Sec 1.4] h_core: u_i = h_core(h_{i-1}, X_i, params)
//   1) F1 上的三次多项式 alpha
//   2) 256-bit bit 域 + NOT-AND + ROTL_256(127) + XOR
//   3) 回到 F2 上得到 u
// ---------------------------------------------------------------------
BigSignedInteger HardPoly1305::h_core(
	const BigSignedInteger& hash_value,
	const BigSignedInteger& block_value,
	const HardPoly1305KeyParams& params ) const
{
	// 1) F1 = Z_{p} 上的高次多项式 alpha
	BigSignedInteger h_f1 = hash_value % p;
	BigSignedInteger x_f1 = block_value % p;

	//A = (h_f1 + x_f1 + params.k_h)^{2}
	BigSignedInteger t1		= h_f1 + x_f1 + params.k_h;
	BigSignedInteger t1_square = t1 * t1;

	//B = (x_f1 - h_f1 + params.k_x)^{3}
	BigSignedInteger t2		= x_f1 - h_f1 + params.k_x;
	BigSignedInteger t2_cube = t2 * t2 * t2;

	//C = A + B (mod PrimeNumber) 
	BigSignedInteger alpha = ( t1_square + t2_cube ) % p;

	// 2) bit 域 + ARX-like
	// alpha_bits = C mod 2^{256}
	// key_mix = params.k_mix mod 2^{256}
	// key_mix2 = params.k_mix2 mod 2^{256}
	BigSignedInteger alpha_bits = alpha & bit_256_mask;
	BigSignedInteger key_mix			= params.k_mix & bit_256_mask;
	BigSignedInteger key_mix2		= params.k_mix2 & bit_256_mask;

	// NOT-AND: ~(alpha_bits & km2) & 2^256-1
	BigSignedInteger t_and = alpha_bits & key_mix2;
	BigSignedInteger t_not = ~t_and;
	t_not &= bit_256_mask;

	// tmp = km XOR t_not
	BigSignedInteger tmp = key_mix ^ t_not;

	// 256-bit 循环左移 127 位
	BigSignedInteger tmp_left  = ( tmp << 127 ) & bit_256_mask;
	BigSignedInteger tmp_right = tmp >> ( 256 - 127 );
	BigSignedInteger rotated_left = tmp_left | tmp_right;

	tmp ^= rotated_left;

	// 3) 回到 F2 = Z_{p2}
	// ARX-like (Modular Addition)
	BigSignedInteger u = ( hash_value + tmp ) % p2;

	return u;
}

// ---------------------------------------------------------------------
// [Sec 1.5] derive_r_s_from_u: 从 256-bit u_i 导出 (r_i, s_i)
//   - u 按小端导出为 32 字节：u_bytes[0:16] -> r_raw, [16:32] -> s_raw
//   - r_i = clamp(r_raw)
//   - s_i = s_raw & (2^128-1)
// ---------------------------------------------------------------------
void HardPoly1305::derive_r_s_from_u(
	const BigSignedInteger& u_value,
	BigSignedInteger& r_out,
	BigSignedInteger& s_out ) const
{
	// 先限制到 256 bit
	BigSignedInteger u_256 = u_value & bit_256_mask;

	// 导出为 32 字节小端
	std::vector<uint8_t> u_bytes;
	bool				 is_negative = false;
	u_256.ExportData( is_negative, u_bytes, 32, false );

	// 低 16 字节 -> r_raw
	std::vector<uint8_t> r_bytes( u_bytes.begin(), u_bytes.begin() + 16 );
	BigSignedInteger		 r_raw;
	r_raw.ImportData( false, r_bytes );
	r_out = r_raw & clamp_bit_mask;

	// 高 16 字节 -> s_raw
	std::vector<uint8_t> s_bytes( u_bytes.begin() + 16, u_bytes.begin() + 32 );
	BigSignedInteger		 s_raw;
	s_raw.ImportData( false, s_bytes );

	// s_i 只保留 128 bit: s_i = s_raw & (2^128 - 1)
	BigSignedInteger mask128 = ( ( BigSignedInteger( 1 ) << 128 ) - BigSignedInteger( 1 ) );
	s_out				 = s_raw & mask128;
}

// ---------------------------------------------------------------------
// [Sec 1.2] 消息 & 密钥混合：mixed(M, K)
// ---------------------------------------------------------------------
std::vector<uint8_t> HardPoly1305::mix_key_and_message( const std::vector<uint8_t>& message,
														const std::vector<uint8_t>& key )
{
	if ( key.empty() )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: key must not be empty" );
	}
	if ( key.size() < 32 )
	{
		throw std::invalid_argument( "HardPoly1305::mix_key_and_message: key length must be >= 32" );
	}

	std::vector<uint8_t> mixed_data( message.size(), 0 );
	size_t				  key_index = 0;

	for ( size_t i = 0; i < message.size(); ++i )
	{
		mixed_data[ i ] = static_cast<uint8_t>( ( message[ i ] + key[ key_index ] ) & 0xFF );
		++key_index;
		if ( key_index == key.size() )
		{
			key_index = 0;
		}
	}

	// 保证“纠缠长度”至少 32 字节：如果 mixed < 32，则补 key[mixed_len:]
	if ( mixed_data.size() < 32 && key.size() > mixed_data.size() )
	{
		size_t start = mixed_data.size();
		for ( size_t i = start; i < key.size(); ++i )
		{
			mixed_data.push_back( key[ i ] );
		}
	}

	return mixed_data;
}

// ---------------------------------------------------------------------
// [Sec 1.6] HardPoly1305 V2-Lite 主算法
//
// 输入：
//   mixed_data = mixed(message, key)   （先调用 mix_key_and_message）
//   key        = master_key (>= 32 bytes)
//
// 流程：
//   1) params = derive_key_parameters(key)
//   2) 将 mixed_data 按 16 字节分块，每块编码 X_i = LE(block || 0x01)
//   3) h_0 = 0; 对每个块：
//        u_i        = h_core(h_{i-1}, X_i, params)
//        (r_i,s_i)  = derive_r_s_from_u(u_i)
//        h_i        = r_i * (h_{i-1}+X_i) + s_i (mod p)
//   4) tag = h_t mod 2^128，以 16 字节小端返回
// ---------------------------------------------------------------------
std::vector<uint8_t> HardPoly1305::hard_poly1305_core( const std::vector<uint8_t>& mixed_data,
													   const std::vector<uint8_t>& key )
{
	if ( key.size() < 32 )
	{
		throw std::invalid_argument( "HardPoly1305::hard_poly1305_core: key length must be >= 32" );
	}

	// [Sec 1.1] 派生内部参数
	HardPoly1305KeyParams params = derive_key_parameters( key );

	// [Sec 1.6] 迭代 Poly1305 形状的随机系数多项式 MAC
	BigSignedInteger hash_value = 0; // h_0 = 0

	const size_t total_len = mixed_data.size();

	for ( size_t offset = 0; offset < total_len; offset += 16 )
	{
		// 当前块的 [offset, offset+16)，不足 16 的最后一块如实取长度
		ptrdiff_t start = static_cast<ptrdiff_t>( offset );
		ptrdiff_t end   = static_cast<ptrdiff_t>( std::min( offset + 16, total_len ) );

		std::vector<uint8_t> block_bytes = SubByteArray( mixed_data, start, end, 1 );
		block_bytes.push_back( 0x01 ); // encode_block_with_one: append 0x01 (little endian high bit)

		// LE 导入为整数 X_i
		BigSignedInteger block_value;
		block_value.ImportData( false, block_bytes );

		// u_i = h_core(h_{i-1}, X_i, params)
		BigSignedInteger u_value = h_core( hash_value, block_value, params );

		// (r_i, s_i) 从 u_i 导出
		BigSignedInteger r_i;
		BigSignedInteger s_i;
		derive_r_s_from_u( u_value, r_i, s_i );

		// h_i = r_i * (h_{i-1} + X_i) + s_i (mod p)
		BigSignedInteger hash_plus_block = ( hash_value + block_value ) % p;
		hash_value					= ( r_i * hash_plus_block + s_i ) % p;
	}

	// 截断到 128 bit：tag = h_t mod 2^128
	BigSignedInteger tag_value = hash_value % hash_max_number;

	std::vector<uint8_t> tag_bytes;
	bool				 is_negative = false;
	tag_value.ExportData( is_negative, tag_bytes, 16, false ); // 16 字节小端

	return tag_bytes;
}

// ---------------------------------------------------------------------
// 简单自测：跟 Python 版一样做个 smoke test
// ---------------------------------------------------------------------
void test_hard_poly1305()
{
	using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

	HardPoly1305 hard_poly1305;

	// 固定 key（和 Python 自测保持一致风格）
	std::vector<uint8_t> key( 32 );
	for ( size_t i = 0; i < key.size(); ++i )
	{
		key[ i ] = static_cast<uint8_t>( i );
	}

	std::vector<std::vector<uint8_t>> message_list;

	message_list.push_back( {} );
	{
		const char* s = "Hello, HardPoly1305!";
		message_list.emplace_back( s, s + std::strlen( s ) );
	}
	message_list.emplace_back( 16, 'A' );
	message_list.emplace_back( 31, 'A' );
	message_list.emplace_back( 32, 'A' );
	message_list.emplace_back( 100, 'A' );

	std::cout << "HardPoly1305 V2-Lite quick self-test (C++ version)\n";

	for ( size_t i = 0; i < message_list.size(); ++i )
	{
		const auto& m = message_list[ i ];
		auto		 mixed_data = hard_poly1305.mix_key_and_message( m, key );
		auto		 tag		  = hard_poly1305.hard_poly1305_core( mixed_data, key );

		std::cout << "[" << i << "] len=" << m.size() << ", tag=" << BytesToHexString( tag ) << "\n";
	}

	// 一致性检查：同一 (msg, key) 重复调用必须得到相同 tag
	std::vector<uint8_t> rand_key = generate_random_bytes( 32 );
	std::vector<uint8_t> rand_msg = generate_random_bytes( 123 );

	auto mixed1 = hard_poly1305.mix_key_and_message( rand_msg, rand_key );
	auto mixed2 = hard_poly1305.mix_key_and_message( rand_msg, rand_key );

	auto t1 = hard_poly1305.hard_poly1305_core( mixed1, rand_key );
	auto t2 = hard_poly1305.hard_poly1305_core( mixed2, rand_key );

	if ( t1 != t2 )
	{
		std::cerr << "Self-test failed: tags for same (msg,key) are different.\n";
	}
	else
	{
		std::cout << "Self-test passed.\n";
	}
}