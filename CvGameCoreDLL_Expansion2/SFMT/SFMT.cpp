/**
 * @file  SFMT.cpp
 * @brief SIMD oriented Fast Mersenne Twister(SFMT)
 *
 * @author Mutsuo Saito (Hiroshima University)
 * @author Makoto Matsumoto (Hiroshima University)
 *
 * Copyright (C) 2006, 2007 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University.
 * Copyright (C) 2012 Mutsuo Saito, Makoto Matsumoto, Hiroshima
 * University and The University of Tokyo.
 * Copyright (C) 2013 Mutsuo Saito, Makoto Matsumoto and Hiroshima
 * University.
 * All rights reserved.
 *
 * The 3-clause BSD License is applied to this software, see
 * LICENSE.txt
 */

// C++'ified for CvGameCoreDLL by Delnar_Ersike

#include "CvGameCoreDLLPCH.h"

#include <string.h>
#include <assert.h>
#include "SFMT.h"
#include "SFMT-params.h"
#include "SFMT-common.h"

/**
 * parameters used by sse2.
 */
static const w128_t sse2_param_mask = {{SFMT_MSK1, SFMT_MSK2,
                                        SFMT_MSK3, SFMT_MSK4}};
/*----------------
  STATIC FUNCTIONS
  ----------------*/

#include "SFMT-sse2-msc.h"

/**
 * This function simulate a 64-bit index of LITTLE ENDIAN
 * in BIG ENDIAN machine.
 */
inline int SFMersenneTwister::idxof(int i) {
    return i;
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
uint32_t SFMersenneTwister::func1(uint32_t x)
{
    return (x ^ (x >> 27)) * (uint32_t)1664525UL;
}

/**
 * This function represents a function used in the initialization
 * by init_by_array
 * @param x 32-bit integer
 * @return 32-bit integer
 */
uint32_t SFMersenneTwister::func2(uint32_t x)
{
    return (x ^ (x >> 27)) * (uint32_t)1566083941UL;
}

/**
 * This function certificate the period of 2^{MEXP}
 */
void SFMersenneTwister::period_certification()
{
    int inner = 0;
	int i = 0;
    int j;
    uint32_t work;
	uint32_t *psfmt32 = &m_sfmt.state[0].u[0];
    const uint32_t parity[4] = {SFMT_PARITY1, SFMT_PARITY2,
                                SFMT_PARITY3, SFMT_PARITY4};

    for (; i < 4; i++)
        inner ^= psfmt32[i] & parity[i];
    for (i = 16; i > 0; i >>= 1)
        inner ^= inner >> i;
    inner &= 1;
    /* check OK */
    if (inner == 1)
	{
        return;
    }
    /* check NG, and modification */
    for (i = 0; i < 4; i++)
	{
        work = 1;
        for (j = 0; j < 32; j++)
		{
            if ((work & parity[i]) != 0) {
                psfmt32[i] ^= work;
                return;
            }
            work = work << 1;
        }
    }
}

bool SFMersenneTwister::operator==(const SFMersenneTwister& source) const
{
	if (m_sfmt.idx != source.m_sfmt.idx)
		return false;

	__m128i miCompareResult;
	const w128_t * pLHSstate = m_sfmt.state;
	const w128_t * pRHSstate = source.m_sfmt.state;

	for (uint32_t uiI = 0; uiI < SFMT_N; uiI++)
	{
		miCompareResult = _mm_cmpeq_epi8(pLHSstate[uiI].si, pRHSstate[uiI].si);
		if (_mm_movemask_epi8(miCompareResult) != 0xffff)
		{
			return false;
		}
	}
	return true;
}

bool SFMersenneTwister::operator!=(const SFMersenneTwister& source) const
{
	return !(*this == source);
}

/*----------------
  PUBLIC FUNCTIONS
  ----------------*/
/**
 * This function returns the identification string.
 * The string shows the word size, the Mersenne exponent,
 * and all parameters of this generator.
 */
const char* SFMersenneTwister::sfmt_get_idstring()
{
    return SFMT_IDSTR;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array32() function.
 * @return minimum size of array used for fill_array32() function.
 */
int SFMersenneTwister::sfmt_get_min_array_size32()
{
    return SFMT_N32;
}

/**
 * This function returns the minimum size of array used for \b
 * fill_array64() function.
 * @return minimum size of array used for fill_array64() function.
 */
int SFMersenneTwister::sfmt_get_min_array_size64()
{
    return SFMT_N64;
}

/**
 * This function generates pseudorandom 32-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 624 and a
 * multiple of four.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param array an array where pseudorandom 32-bit integers are filled
 * by this function.  The pointer to the array must be \b "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 32-bit pseudorandom integers to be
 * generated.  size must be a multiple of 4, and greater than or equal
 * to (MEXP / 128 + 1) * 4.
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void SFMersenneTwister::sfmt_fill_array32(uint32_t *array, int size)
{
	assert(m_sfmt.idx == SFMT_N32);
    assert(size % 4 == 0);
    assert(size >= SFMT_N32);

    gen_rand_array((w128_t *)array, size / 4);
	m_sfmt.idx = SFMT_N32;
}

/**
 * This function generates pseudorandom 64-bit integers in the
 * specified array[] by one call. The number of pseudorandom integers
 * is specified by the argument size, which must be at least 312 and a
 * multiple of two.  The generation by this function is much faster
 * than the following gen_rand function.
 *
 * For initialization, init_gen_rand or init_by_array must be called
 * before the first call of this function. This function can not be
 * used after calling gen_rand function, without initialization.
 *
 * @param array an array where pseudorandom 64-bit integers are filled
 * by this function.  The pointer to the array must be "aligned"
 * (namely, must be a multiple of 16) in the SIMD version, since it
 * refers to the address of a 128-bit integer.  In the standard C
 * version, the pointer is arbitrary.
 *
 * @param size the number of 64-bit pseudorandom integers to be
 * generated.  size must be a multiple of 2, and greater than or equal
 * to (MEXP / 128 + 1) * 2
 *
 * @note \b memalign or \b posix_memalign is available to get aligned
 * memory. Mac OSX doesn't have these functions, but \b malloc of OSX
 * returns the pointer to the aligned memory block.
 */
void SFMersenneTwister::sfmt_fill_array64(uint64_t *array, int size)
{
	assert(m_sfmt.idx == SFMT_N32);
    assert(size % 2 == 0);
    assert(size >= SFMT_N64);

    gen_rand_array((w128_t *)array, size / 2);
	m_sfmt.idx = SFMT_N32;
}

/**
 * This function initializes the internal state array with a 32-bit
 * integer seed.
 *
 * @param seed a 32-bit integer used as the seed.
 */
void SFMersenneTwister::sfmt_init_gen_rand(uint32_t seed)
{
	int i = 1;

	uint32_t *psfmt32 = &m_sfmt.state[0].u[0];

    psfmt32[0] = seed;
    for (; i < SFMT_N32; i++)
	{
        psfmt32[i] = 1812433253UL * (psfmt32[i - 1] ^ (psfmt32[i - 1] >> 30)) + i;
    }
	m_sfmt.idx = SFMT_N32;
    period_certification();
}

/**
 * This function initializes the internal state array,
 * with an array of 32-bit integers used as the seeds
 * @param init_key the array of 32-bit integers, used as a seed.
 * @param key_length the length of init_key.
 */
void SFMersenneTwister::sfmt_init_by_array(uint32_t *init_key, int key_length)
{
    int count;
    int lag;
    int size = SFMT_N * 4;
	uint32_t *psfmt32 = &m_sfmt.state[0].u[0];

    if (size >= 623) {
        lag = 11;
    } else if (size >= 68) {
        lag = 7;
    } else if (size >= 39) {
        lag = 5;
    } else {
        lag = 3;
    }
    int mid = (size - lag) / 2;

	memset(&m_sfmt, 0x8b, sizeof(sfmt_t));
    if (key_length + 1 > SFMT_N32) {
        count = key_length + 1;
    } else {
        count = SFMT_N32;
    }
	uint32_t r = func1(psfmt32[0] ^ psfmt32[mid]
              ^ psfmt32[SFMT_N32 - 1]);
    psfmt32[mid] += r;
    r += key_length;
    psfmt32[mid + lag] += r;
    psfmt32[0] = r;

    count--;
	int i = 1;
	int j = 0;
    for (; (j < count) && (j < key_length); j++) {
        r = func1(psfmt32[i] ^ psfmt32[(i + mid) % SFMT_N32]
                  ^ psfmt32[(i + SFMT_N32 - 1) % SFMT_N32]);
        psfmt32[(i + mid) % SFMT_N32] += r;
        r += init_key[j] + i;
        psfmt32[(i + mid + lag) % SFMT_N32] += r;
        psfmt32[i] = r;
        i = (i + 1) % SFMT_N32;
    }
    for (; j < count; j++) {
        r = func1(psfmt32[i] ^ psfmt32[(i + mid) % SFMT_N32]
                  ^ psfmt32[(i + SFMT_N32 - 1) % SFMT_N32]);
        psfmt32[(i + mid) % SFMT_N32] += r;
        r += i;
        psfmt32[(i + mid + lag) % SFMT_N32] += r;
        psfmt32[i] = r;
        i = (i + 1) % SFMT_N32;
    }
    for (j = 0; j < SFMT_N32; j++) {
        r = func2(psfmt32[i] + psfmt32[(i + mid) % SFMT_N32]
                  + psfmt32[(i + SFMT_N32 - 1) % SFMT_N32]);
        psfmt32[(i + mid) % SFMT_N32] ^= r;
        r -= i;
        psfmt32[(i + mid + lag) % SFMT_N32] ^= r;
        psfmt32[i] = r;
        i = (i + 1) % SFMT_N32;
    }

	m_sfmt.idx = SFMT_N32;
    period_certification();
}
