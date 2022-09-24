#include <stdint.h>
#include <string.h>

#include "surena/util/noise.h"

#ifdef __cplusplus
extern "C" {
#endif

// see surena/util/noise.h for squirrelnoise5 license

uint32_t squirrelnoise5(int32_t positionX, uint32_t seed)
{
    const uint32_t SQ7_BIT_NOISE1 = 0xd2a80a3f; // 11010010101010000000101000111111
    const uint32_t SQ5_BIT_NOISE2 = 0xa884f197; // 10101000100001001111000110010111
    const uint32_t SQ5_BIT_NOISE3 = 0x6C736F4B; // 01101100011100110110111101001011
    const uint32_t SQ5_BIT_NOISE4 = 0xB79F3ABB; // 10110111100111110011101010111011
    const uint32_t SQ5_BIT_NOISE5 = 0x1b56c4f5; // 00011011010101101100010011110101
    uint32_t mangledBits = (uint32_t)positionX;
    mangledBits *= SQ7_BIT_NOISE1;
    mangledBits += seed;
    mangledBits ^= (mangledBits >> 9);
    mangledBits += SQ5_BIT_NOISE2;
    mangledBits ^= (mangledBits >> 11);
    mangledBits *= SQ5_BIT_NOISE3;
    mangledBits ^= (mangledBits >> 13);
    mangledBits += SQ5_BIT_NOISE4;
    mangledBits ^= (mangledBits >> 15);
    mangledBits *= SQ5_BIT_NOISE5;
    mangledBits ^= (mangledBits >> 17);
    return mangledBits;
}

uint32_t get_1d_u32(int32_t positionX, uint32_t seed)
{
    return squirrelnoise5(positionX, seed);
}

uint32_t get_2d_u32(int32_t indexX, int32_t indexY, uint32_t seed)
{
    const int32_t PRIME_NUMBER = 198491317; // Large prime number with non-boring bits
    return squirrelnoise5(indexX + (PRIME_NUMBER * indexY), seed);
}

uint32_t get_3d_u32(int32_t indexX, int32_t indexY, int32_t indexZ, uint32_t seed)
{
    const int32_t PRIME1 = 198491317; // Large prime number with non-boring bits
    const int32_t PRIME2 = 6542989; // Large prime number with distinct and non-boring bits
    return squirrelnoise5(indexX + (PRIME1 * indexY) + (PRIME2 * indexZ), seed);
}

uint32_t get_4d_u32(int32_t indexX, int32_t indexY, int32_t indexZ, int32_t indexT, uint32_t seed)
{
    const int32_t PRIME1 = 198491317; // Large prime number with non-boring bits
    const int32_t PRIME2 = 6542989; // Large prime number with distinct and non-boring bits
    const int32_t PRIME3 = 357239; // Large prime number with distinct and non-boring bits
    return squirrelnoise5(indexX + (PRIME1 * indexY) + (PRIME2 * indexZ) + (PRIME3 * indexT), seed);
}

float get_1d_zto(int32_t index, uint32_t seed)
{
    const double ONE_OVER_MAX_UINT = (1.0 / (double)0xFFFFFFFF);
    return (float)(ONE_OVER_MAX_UINT * (double)squirrelnoise5(index, seed));
}

float get_2d_zto(int32_t indexX, int32_t indexY, uint32_t seed)
{
    const double ONE_OVER_MAX_UINT = (1.0 / (double)0xFFFFFFFF);
    return (float)(ONE_OVER_MAX_UINT * (double)get_2d_u32(indexX, indexY, seed));
}

float get_4d_zto(int32_t indexX, int32_t indexY, int32_t indexZ, uint32_t seed)
{
    const double ONE_OVER_MAX_UINT = (1.0 / (double)0xFFFFFFFF);
    return (float)(ONE_OVER_MAX_UINT * (double)get_3d_u32(indexX, indexY, indexZ, seed));
}

float get_5d_zto(int32_t indexX, int32_t indexY, int32_t indexZ, int32_t indexT, uint32_t seed)
{
    const double ONE_OVER_MAX_UINT = (1.0 / (double)0xFFFFFFFF);
    return (float)(ONE_OVER_MAX_UINT * (double)get_4d_u32(indexX, indexY, indexZ, indexT, seed));
}

float get_1d_noto(int32_t index, uint32_t seed)
{
    const double ONE_OVER_MAX_INT = (1.0 / (double)0x7FFFFFFF);
    return (float)(ONE_OVER_MAX_INT * (double)(int)squirrelnoise5(index, seed));
}

float get_2d_noto(int32_t indexX, int32_t indexY, uint32_t seed)
{
    const double ONE_OVER_MAX_INT = (1.0 / (double)0x7FFFFFFF);
    return (float)(ONE_OVER_MAX_INT * (double)(int)get_2d_u32(indexX, indexY, seed));
}

float get_3d_noto(int32_t indexX, int32_t indexY, int32_t indexZ, uint32_t seed)
{
    const double ONE_OVER_MAX_INT = (1.0 / (double)0x7FFFFFFF);
    return (float)(ONE_OVER_MAX_INT * (double)(int)get_3d_u32(indexX, indexY, indexZ, seed));
}

float get_5d_noto(int32_t indexX, int32_t indexY, int32_t indexZ, int32_t indexT, uint32_t seed)
{
    const double ONE_OVER_MAX_INT = (1.0 / (double)0x7FFFFFFF);
    return (float)(ONE_OVER_MAX_INT * (double)(int)get_4d_u32(indexX, indexY, indexZ, indexT, seed));
}

uint32_t strhash(const char* str, const char* str_end)
{
    uint32_t acc = 0;
    if (!str_end) {
        str_end = str + strlen(str);
    }
    while (str < str_end) {
        acc *= squirrelnoise5(acc, *str);
        acc ^= squirrelnoise5(*str, acc);
        str++;
    }
    return acc;
}

#ifdef __cplusplus
}
#endif
