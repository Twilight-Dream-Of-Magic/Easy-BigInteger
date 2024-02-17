#include "BigInteger.hpp"
#include "BigIntegerTest.hpp"
#include "BigFraction.hpp"
#include "BinaryCipherTest.hpp"
#include "CryptographyAsymmetricKey.hpp"

#include <iomanip>
#include <cassert>

class HardPoly1305
{
private:
	using BigSignedInteger = TwilightDream::BigInteger::BigSignedInteger;
	BigSignedInteger p;	   // Poly1305 算法使用的质数
	BigSignedInteger p2;	   // HardPoly1305 算法使用的质数
	BigSignedInteger r = 0;  // 秘密状态 r
	BigSignedInteger s = 0;  // 秘密状态 s
	BigSignedInteger clamp_bit_mask = 0; //比特掩码 - 修剪数值
	BigSignedInteger hash_max_number = BigSignedInteger( 1 ) << 128;

	BigSignedInteger generate_unpredictable_value( const BigSignedInteger& hash_value, const BigSignedInteger& key )
	{
		const BigSignedInteger a = BigSignedInteger(hash_value) + BigSignedInteger(key);
		const BigSignedInteger b = BigSignedInteger(key) - BigSignedInteger(hash_value);

		// 复杂公式计算 alpha
		// ((hash + key)^2 + (key - hash)^3) mod p2
		BigSignedInteger alpha = ( ( a * a ) + ( b * b * b ) );

		std::cout << "Alpha number: " << alpha.ToString( 10 ) << '\n';

		// 应用 模数 p2
		if ( alpha < 0 )
		{
			std::cout << "Small Alpha number: " << alpha.ToString( 10 ) << '\n';
			alpha = p2 + alpha;
		}
		else if ( alpha >= p2 )
		{
			std::cout << "Big Alpha number: " << alpha.ToString( 10 ) << '\n';
			alpha = alpha % p2;
		}

		size_t key_bit_length = key.BitLength();
		std::cout << "key_bit_length: " << key_bit_length << '\n';
		size_t hash_value_bit_length = hash_value.BitLength();
		std::cout << "hash_value_bit_length: " << hash_value_bit_length << '\n';

		// 计算 u0, u1, u2, u3
		const BigSignedInteger& u0 = a;
		const BigSignedInteger u1 = hash_value - key;
		uint64_t   left_shift_amount = ( u0 % key_bit_length ).ToUnsignedInt();
		uint64_t   right_shift_amount = ( u1 % hash_value_bit_length ).ToUnsignedInt();
		BigSignedInteger u2 = key << left_shift_amount;
		BigSignedInteger u3 = hash_value >> right_shift_amount;

		// 计算 u4
		BigSignedInteger u4 = hash_value * ( u2 + u3 + alpha );

		// 应用 模数 p 规约大小
		return u4 % p;
	}

	// 提取子数组函数，类似于 std::span 和 与Python数据切片range
	std::vector<uint8_t> SubByteArray( const std::vector<uint8_t>& data, ptrdiff_t start, ptrdiff_t end, ptrdiff_t step = 1 )
	{
		if ( step == 0 )
		{
			// 步长不能为0，返回空数组
			return {};
		}

		// 处理负索引
		ptrdiff_t dataSize = static_cast<ptrdiff_t>( data.size() );
		if ( start < 0 )
		{
			start += dataSize;
		}
		if ( end < 0 )
		{
			end += dataSize;
		}

		// 确保索引在有效范围内
		start = std::max<ptrdiff_t>( 0, std::min<ptrdiff_t>( start, dataSize ) );
		end = std::max<ptrdiff_t>( 0, std::min<ptrdiff_t>( end, dataSize ) );

		std::vector<uint8_t> sub_array;
		if ( step > 0 )
		{
			for ( ptrdiff_t i = start; i < end; i += step )
			{
				sub_array.push_back( data[ i ] );
			}
		}
		else
		{
			for ( ptrdiff_t i = start; i > end; i += step )
			{
				sub_array.push_back( data[ i ] );
			}
		}
		return sub_array;
	}

public:
	HardPoly1305()
	:
	p( "1361129467683753853853498429727072845819", 10 ),
	p2( "115792089237316195423570985008687907853269984665640564039457584007913129451867", 10 ),
	clamp_bit_mask( "0FFFFFFC0FFFFFFC0FFFFFFC0FFFFFFF", 16 )
	{}

	std::vector<uint8_t> mix_key_and_message( const std::vector<uint8_t>& message, const std::vector<uint8_t>& key )
	{
		// Mix the message and key
		std::vector<uint8_t> mixed_data( message.size(), 0 );
		size_t				 key_index = 0;

		for ( size_t i = 0; i < message.size(); ++i )
		{
			mixed_data[ i ] = ( message[ i ] + key[ key_index ] ) % 256;
			key_index = ( key_index + 1 ) % key.size();
		}

		if(mixed_data.size() < 32)
		{
			//mixed_data = mixed_data concatenation key

			mixed_data.reserve(32);
			for ( size_t i = mixed_data.size(); i < key.size(); i++ )
			{
				mixed_data.push_back(key[i]);
			}
		}

		return mixed_data;
	}

	std::vector<uint8_t> hard_poly1305_core( const std::vector<uint8_t>& mixed_data, const std::vector<uint8_t>& key )
	{
		// 初始化混合消息状态
		BigSignedInteger mixed_number = 0;

		// 获取中间字节部分的密钥
		BigSignedInteger key_number = 0;
		key_number.ImportData( false, SubByteArray( key, 7, 23 ) );
		// 对原始密钥中间字节部分进行倍增 (Poly1305 算法以前的步骤)
		std::cout << "----------------------------------------\n";
		std::cout << "Middle key byte number `int.from_bytes(key[7:23], 'little')`:\n";
		key_number.Print( 10 );
		std::cout << "Middle key byte number `int.from_bytes(key[7:23], 'little') * 5`:\n";
		key_number *= 5;
		key_number.Print( 10 );
		std::cout << "----------------------------------------\n";
		
		// 初始化(Aaccumulator) hash_value 为 0
		BigSignedInteger hash_value = 0;

		// 初始化秘密状态 Secret status
		// (Poly1305 算法以前的步骤)
		r.ImportData( false, SubByteArray( key, 0, 15 ) );
		s.ImportData( false, SubByteArray( key, 16, 31 ) );
		std::cout << "Secret r: ";
		r.Print( 10 );
		std::cout << "Secret s: ";
		s.Print( 10 );

		// 计算 loop_count
		size_t loop_count = ( mixed_data.size() + 15 ) / 16 + 1;
		std::cout << "loop_count: " << loop_count << '\n';

		std::vector<uint8_t> mixed_data_span;

		// HardPoly1305 算法核心计算循环
		for ( size_t i = 1; i <= loop_count - 1; ++i )
		{
			// 将 r 进行 clamp
			// (Poly1305 算法以前的步骤)
			r &= clamp_bit_mask;

			// 获取 mixed_data 的字节切片
			mixed_data_span = SubByteArray( mixed_data, ( i - 1 ) * 16, i * 16 );

			// 重新分配 bytes 大小并拼接 Byte 0x01
			mixed_data_span.push_back( 0x01 );

			// 计算 mixed_number
			mixed_number.ImportData( false, mixed_data_span );

			// 更新 hash_value
			// (Poly1305 算法以前的步骤)
			hash_value += mixed_number;
			// 首先 应用 秘密部分 r 进行倍增 然后 应用 模数 p
			hash_value = (r * hash_value) % p;
			// 最后 应用 秘密部分 s
			hash_value += s;

			// 更新秘密状态 r 和 s
			// HardPoly1305 算法的步骤
			r = generate_unpredictable_value( hash_value, mixed_number );
			s = generate_unpredictable_value( hash_value, key_number );
		}

		// 规约大小 hash = hash (mod 2^128)
		hash_value = hash_value % hash_max_number;

		// 返回结果
		std::vector<uint8_t> hash_result;
		bool is_negative = false;
		hash_value.ExportData( is_negative, hash_result, 16 );

		std::cout << "----------------------------------------\n";
		std::cout << "Tag/Hash bytes data `int.to_bytes(16, 'little')`:\n";
		for ( const auto& byte : hash_result )
		{
			std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<unsigned int>( byte );
		}
		std::cout << '\n';
		std::cout << "tag/hash byte length: " << hash_result.size() << '\n';
		std::cout << "----------------------------------------" << std::endl;

		return hash_result;
	}
};

// 生成随机字节序列
inline std::vector<uint8_t> generate_random_bytes( size_t size )
{
	std::vector<uint8_t>			bytes( size );
	std::random_device				rd;
	std::mt19937					gen( rd() );
	std::uniform_int_distribution<> dis( 0, 255 );
	for ( size_t i = 0; i < size; ++i )
	{
		bytes[ i ] = static_cast<uint8_t>( dis( gen ) );
	}
	return bytes;
}

// 测试 HardPoly1305 类
inline void test_hard_poly1305()
{
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	// 创建 HardPoly1305 对象
	HardPoly1305 hard_poly1305;

// 生成消息和随机密钥
#if 0
	std::vector<uint8_t> message = generate_random_bytes( 32 );
#else
	std::string			 string_message = std::string( "Hello, world!" );
	std::vector<uint8_t> message( string_message.begin(), string_message.end() );
#endif

	//std::vector<uint8_t> key = generate_random_bytes( 32 );
	std::vector<uint8_t> key( 32, 'A' );

	// 计算混合消息数据 将消息与密钥混合
	std::vector<uint8_t> mixed_data = hard_poly1305.mix_key_and_message( message, key );

	auto format_flags = std::cout.flags();

	// 输出密钥、消息和标签
	std::cout << "Message: ";
	for ( const auto& byte : message )
	{
		std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<unsigned int>( byte );
	}
	std::cout << '\n';
	std::cout.flags( format_flags );
	std::cout << "message byte length: " << message.size() << std::endl;

	std::cout << "Key: ";
	for ( const auto& byte : key )
	{
		std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<unsigned int>( byte );
	}
	std::cout << '\n';
	std::cout.flags( format_flags );
	std::cout << "key byte length: " << key.size() << std::endl;

	std::cout << "MixData: ";
	for ( const auto& byte : mixed_data )
	{
		std::cout << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<unsigned int>( byte );
	}
	std::cout << '\n';
	std::cout.flags( format_flags );
	std::cout << "mixed_data byte length: " << mixed_data.size() << std::endl;

	// 计算消息的标签
	std::vector<uint8_t> tag = hard_poly1305.hard_poly1305_core( key, mixed_data );

	std::cout.flags( format_flags );
}

auto main( int argument_cout, char* argument_vector[] ) -> int
{
	TwilightDream::BigFraction::BigFraction TestBigFraction(12,48);
	TestBigFraction.PrecisionMode = TwilightDream::BigFraction::DecimalPrecisionMode::Fixed;
	TestBigFraction.FixedPrecisionCount = 100;
	std::cout << TestBigFraction.Power(3) << std::endl;
	std::cout << TestBigFraction.Sqrt() << std::endl;
	std::cout << TestBigFraction.Cbrt() << std::endl;
	std::cout << TestBigFraction.Log() << std::endl;
	std::cout << TestBigFraction.Log10() << std::endl;
	std::cout << TestBigFraction.NthRoot(4) << std::endl;

	TestBigFraction.PrecisionMode = TwilightDream::BigFraction::DecimalPrecisionMode::Full;
	TwilightDream::BigInteger::BigInteger FullPrecision("100000", 10);
	TestBigFraction.SetFullPrecision(FullPrecision);
	std::cout << TestBigFraction.Power(3) << std::endl;
	std::cout << TestBigFraction.Sqrt() << std::endl;
	std::cout << TestBigFraction.Cbrt() << std::endl;
	std::cout << TestBigFraction.Log() << std::endl;
	std::cout << TestBigFraction.Log10() << std::endl;
	std::cout << TestBigFraction.NthRoot(3) << std::endl;

	TwilightDream::BigInteger::Test::test_all();
	TwilightDream::BigInteger::Test::test_custom_data();

	using PrimeNumberTester = TwilightDream::PrimeNumberTester;
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	//BigInteger BigPrime(std::string("16158503035655503650357438344334975980222051334857742016065172713762327569433945446598600705761456731844358980460949009747059779575245460547544076193224141560315438683650498045875098875194826053398028819192033784138396109321309878080919047169238085235290822926018152521443787945770532904303776199561965192760957166694834171210342487393282284747428088017663161029038902829665513096354230157075129296432088558362971801859230928678799175576150822952201848806616643615613562842355410104862578550863465661734839271290328348967522998634176499319107762583194718667771801067716614802322659239302476074096777926805529798115243"), 10);
	//PrimeNumberTester NumberTester;
	//std::cout << std::boolalpha << NumberTester.IsPrime(BigPrime) << std::endl;

	TwilightDream::CryptographyAsymmetric::RSA::SelfSanityCheck(4096, 10);

	BinaryCipher BinaryCipherInstance;
	BinaryCipherInstance.Test();
	
	BinaryCipherNaive BinaryCipherNaiveInstance;
	BinaryCipherNaiveInstance.Test();

	//FIXME
	//大整数应该不会计算错误。因为已经全部都测试过了。那唯一可能的是我算法写错了，或者还有什么我们的别的不知道的原因
	//Large integers shouldn't be miscalculated. That's because it's all been tested. So the only possibility is that I wrote the algorithm wrong, or something else we don't know.
	//test_hard_poly1305();
	
	return 0;
}