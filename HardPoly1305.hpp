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

#ifndef HARD_POLY1305_HPP
#define HARD_POLY1305_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "BigInteger.hpp"

/**
 * @brief Reference implementation of the HardPoly1305-SP SafePrime branch.
 *
 * This class follows Algorithm A.2 from the paper:
 *
 *   HardPoly1305-BEF7 and HardPoly1305-SP:
 *   Algebraic Poly1305 Candidates under Arbitrary Nonce Repetition
 *   and Adaptive Tweaks.
 *
 * Important engineering scope:
 * - This file is the Easy-BigInteger reference implementation.
 * - It is intended for mathematical conformance and known-answer testing.
 * - It is not the fixed-limb, constant-time, production implementation
 *   described by the performance section of the paper.
 *
 * The historical two-stage API is preserved:
 *
 *   mix_key_and_message(...)
 *   hard_poly1305_core(...)
 *
 * HardPoly1305-SP does not alter the message in the PreMix stage, so
 * mix_key_and_message() returns the original message unchanged.
 */
class HardPoly1305
{
private:
	using BigInteger = TwilightDream::BigInteger::BigInteger;

	// P1 = 2^130 - 5, used by the Poly1305 accumulator.
	static const BigInteger P1;

	// P2 = 2^256 - 188069, used by the SafePrime branch.
	static const BigInteger P2;

	static const BigInteger MASK_256;
	static const BigInteger MASK_128;
	static const BigInteger CLAMP_MASK;
	static const BigInteger A7_SHIFT_248;
	static const BigInteger TWO_129;
	static const BigInteger TWO_193;

	// Stored PreMix state produced by mix_key_and_message().
	BigInteger key_stored;
	BigInteger encoded_context_stored;
	BigInteger rotated_key_context_stored;
	BigInteger multiplier_mask_stored;
	BigInteger additive_mask_stored;
	BigInteger reduced_key_stored;
	BigInteger reduced_context_stored;
	bool context_ready;

	/** Convert a little-endian byte vector to a non-negative BigInteger. */
	static BigInteger bytes_to_integer_little_endian( const std::vector<uint8_t>& bytes );

	/** Export the low fixed-length little-endian representation of a BigInteger. */
	static std::vector<uint8_t> integer_to_bytes_little_endian( const BigInteger& integer, std::size_t length );

	/** Rotate a 256-bit word left without calling BigInteger::BitRotateLeft(). */
	static BigInteger rotate_left_256( const BigInteger& value, uint32_t shift );

	/**
	 * Derive q, Q, rho, sigma, and Kbar for one key under an already encoded
	 * public context E.
	 */
	static void derive_pre_mix_parameters(
		const std::vector<uint8_t>& key,
		const BigInteger& encoded_context,
		BigInteger& key_integer,
		BigInteger& rotated_key_context,
		BigInteger& multiplier_mask,
		BigInteger& additive_mask,
		BigInteger& reduced_key );

	/** RX transform from one 256-bit word to two 128-bit words. */
	static void rx_transform( const BigInteger& core_output, BigInteger& lower_output, BigInteger& upper_output );

public:
	HardPoly1305();
	~HardPoly1305();

	/**
	 * @brief PreMix the 256-bit key with the public nonce, tweak, and theta.
	 *
	 * @param message Original message. The SP PreMix returns it unchanged.
	 * @param key Exactly 32 key bytes.
	 * @param nonce Exactly 12 nonce bytes.
	 * @param tweak Exactly 12 tweak bytes.
	 * @param theta Exactly 8 bytes encoding the little-endian 64-bit theta.
	 * @return A copy of message, unchanged.
	 */
	std::vector<uint8_t> mix_key_and_message(
		const std::vector<uint8_t>& message,
		const std::vector<uint8_t>& key,
		const std::vector<uint8_t>& nonce = std::vector<uint8_t>( 12, 0 ),
		const std::vector<uint8_t>& tweak = std::vector<uint8_t>( 12, 0 ),
		const std::vector<uint8_t>& theta = std::vector<uint8_t>( 8, 0 ) );

	/**
	 * @brief Compute the HardPoly1305-SP tag using the stored public context.
	 *
	 * When key_override is empty, the key from mix_key_and_message() is used.
	 * When key_override contains exactly 32 bytes, the PreMix key-dependent
	 * values are genuinely re-derived under the stored public context for this
	 * call. The override is not silently ignored.
	 *
	 * @param message Message to authenticate.
	 * @param key_override Empty, or exactly 32 key bytes.
	 * @return The 16-byte little-endian authentication tag.
	 */
	std::vector<uint8_t> hard_poly1305_core(
		const std::vector<uint8_t>& message,
		const std::vector<uint8_t>& key_override = std::vector<uint8_t>() ) const;

	/**
	 * @brief Clear the stored PreMix state.
	 *
	 * This is a best-effort logical reset. Easy-BigInteger uses dynamic storage,
	 * so this method is not a formal guarantee that every previous heap copy was
	 * securely erased.
	 */
	void clear_context();
};

/** One-call convenience wrapper for HardPoly1305-SP. */
std::vector<uint8_t> hardpoly1305_sp_tag(
	const std::vector<uint8_t>& message,
	const std::vector<uint8_t>& key,
	const std::vector<uint8_t>& nonce = std::vector<uint8_t>( 12, 0 ),
	const std::vector<uint8_t>& tweak = std::vector<uint8_t>( 12, 0 ),
	const std::vector<uint8_t>& theta = std::vector<uint8_t>( 8, 0 ) );

/** Run deterministic known-answer and API-contract tests. */
bool hardpoly1305_sp_self_test();

/** Compatibility test entry point. Throws std::runtime_error on failure. */
void test_hard_poly1305();

#endif // HARD_POLY1305_HPP