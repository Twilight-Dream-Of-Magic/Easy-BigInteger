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

#if !defined(HARD_POLY1305_WITH_BIGINTEGER)
#define HARD_POLY1305_WITH_BIGINTEGER

#include "BigInteger.hpp"
#include <vector>
#include <string>

// 提取子数组函数，类似于 std::span 和 Python 的切片
extern std::vector<uint8_t> SubByteArray( const std::vector<uint8_t>& data,
										  ptrdiff_t start,
										  ptrdiff_t end,
										  ptrdiff_t step );

// 将字节数组转换为 16 进制字符串表示
extern std::string BytesToHexString( const std::vector<uint8_t>& bytes );

// 生成随机字节序列
extern std::vector<uint8_t> generate_random_bytes( size_t size );

// ---------------------------------------------------------------------
// HardPoly1305 V2-Lite：从 master_key 派生出的内部参数
//
// - k_h, k_x : F1 = Z_{p1} 上的偏移参数（参与 α 的多项式）
// - k_mix, k_mix2 : F2 = Z_{p2} 上的混合参数（参与 bit 域 + ARX）
// ---------------------------------------------------------------------
struct HardPoly1305KeyParams
{
	using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

	BigSignedInteger k_h;
	BigSignedInteger k_x;
	BigSignedInteger k_mix;
	BigSignedInteger k_mix2;

	HardPoly1305KeyParams() = default;

	HardPoly1305KeyParams(
		const BigSignedInteger& kh,
		const BigSignedInteger& kx,
		const BigSignedInteger& km,
		const BigSignedInteger& km2
	)
		: k_h( kh )
		, k_x( kx )
		, k_mix( km )
		, k_mix2( km2 )
	{}
};

class HardPoly1305
{
private:
	using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;

	// Poly1305 原始模数 p = 2^130 - 5
	BigSignedInteger p;

	// 第二个大素数 p2 = 2^256 - 188069
	BigSignedInteger p2;

	// 标准 Poly1305 clamp 掩码
	BigSignedInteger clamp_bit_mask = BigSignedInteger( "0FFFFFFC0FFFFFFC0FFFFFFC0FFFFFFF", 16 );

	// 2^128，用来做最终截断
	BigSignedInteger hash_max_number = ( BigSignedInteger( 1 ) << 128 );

	// 2^256 - 1，bit 域掩码，限制到 256 bit
	BigSignedInteger bit_256_mask = ( ( BigSignedInteger( 1 ) << 256 ) - BigSignedInteger( 1 ) );

	// --------- V2-Lite 内部工具函数 ----------

	// [Sec 1.1] 从 32 字节 master_key 派生内部参数
	HardPoly1305KeyParams derive_key_parameters( const std::vector<uint8_t>& master_key ) const;

	// [Sec 1.4] 小 h：u_i = h_core(h_{i-1}, X_i, params)
	BigSignedInteger h_core(
		const BigSignedInteger& hash_value,
		const BigSignedInteger& block_value,
		const HardPoly1305KeyParams& params
	) const;

	// [Sec 1.5] 从 256-bit u_i 导出 (r_i, s_i)
	void derive_r_s_from_u(
		const BigSignedInteger& u_value,
		BigSignedInteger& r_out,
		BigSignedInteger& s_out
	) const;

public:
	HardPoly1305()
		: p( "1361129467683753853853498429727072845819", 10 ), // 2^130 - 5
		  p2( "115792089237316195423570985008687907853269984665640564039457584007913129451867", 10 ) // 2^256 - 188069 (Safe Prime)
	{
	}

	// [Sec 1.2] 消息 & 密钥混合：mixed(M, K)
	std::vector<uint8_t> mix_key_and_message( const std::vector<uint8_t>& message,
											  const std::vector<uint8_t>& key );

	// [Sec 1.6] HardPoly1305 V2-Lite 主算法核心（输入：mixed(M,K) 与 master_key）
	std::vector<uint8_t> hard_poly1305_core( const std::vector<uint8_t>& mixed_data,
											 const std::vector<uint8_t>& key );
};

// 测试 HardPoly1305 类
extern void test_hard_poly1305();

inline std::vector<uint8_t> hardpoly1305_v2_lite_tag( const std::vector<uint8_t>& message, const std::vector<uint8_t>& master_key )
{
	HardPoly1305 mac;
	auto		 mixed = mac.mix_key_and_message( message, master_key );
	return mac.hard_poly1305_core( mixed, master_key );
}

#endif	// HARD_POLY1305_WITH_BIGINTEGER
