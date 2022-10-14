#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "surena/util/raw_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO simplify rawstream encoding process for byte based primitives (int/uint/float etc) by just using one, "encode bytes" func, on a union type!

raw_stream rs_init(void* buf)
{
    return (raw_stream){
        .begin = buf,
        .end = buf,
    };
}

size_t rs_get_size(raw_stream* rs)
{
    return rs->end - rs->begin;
}

void rs_w_bool(raw_stream* rs, bool v)
{
    rs->end[0] = (uint8_t)v;
    rs->end++;
}

bool rs_r_bool(raw_stream* rs)
{
    bool r = (bool)(rs->end[0]);
    rs->end++;
    return r;
}

void rs_w_int8(raw_stream* rs, int8_t v)
{
    rs->end[0] = (uint8_t)v;
    rs->end++;
}

int8_t rs_r_int8(raw_stream* rs)
{
    int8_t r = (int8_t)(rs->end[0]);
    rs->end++;
    return r;
}

void rs_w_int16(raw_stream* rs, int16_t v)
{
    rs->end[0] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 2;
}

int16_t rs_r_int16(raw_stream* rs)
{
    int16_t r = 0;
    r |= (int16_t)(rs->end[0]) << 8;
    r |= (int16_t)(rs->end[1]) << 0;
    rs->end += 2;
    return r;
}

void rs_w_int32(raw_stream* rs, int32_t v)
{
    rs->end[0] = (uint8_t)((v >> 24) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 16) & 0xFF);
    rs->end[2] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[3] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 4;
}

int32_t rs_r_int32(raw_stream* rs)
{
    int32_t r = 0;
    r |= (int32_t)(rs->end[0]) << 24;
    r |= (int32_t)(rs->end[1]) << 16;
    r |= (int32_t)(rs->end[2]) << 8;
    r |= (int32_t)(rs->end[3]) << 0;
    rs->end += 4;
    return r;
}

void rs_w_int64(raw_stream* rs, int64_t v)
{
    rs->end[0] = (uint8_t)((v >> 56) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 48) & 0xFF);
    rs->end[2] = (uint8_t)((v >> 40) & 0xFF);
    rs->end[3] = (uint8_t)((v >> 32) & 0xFF);
    rs->end[4] = (uint8_t)((v >> 24) & 0xFF);
    rs->end[5] = (uint8_t)((v >> 16) & 0xFF);
    rs->end[6] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[7] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 8;
}

int64_t rs_r_int64(raw_stream* rs)
{
    int64_t r = 0;
    r |= (int64_t)(rs->end[0]) << 56;
    r |= (int64_t)(rs->end[1]) << 48;
    r |= (int64_t)(rs->end[2]) << 40;
    r |= (int64_t)(rs->end[3]) << 32;
    r |= (int64_t)(rs->end[4]) << 24;
    r |= (int64_t)(rs->end[5]) << 16;
    r |= (int64_t)(rs->end[6]) << 8;
    r |= (int64_t)(rs->end[7]) << 0;
    rs->end += 8;
    return r;
}

void rs_w_uint8(raw_stream* rs, uint8_t v)
{
    rs->end[0] = (uint8_t)v;
    rs->end++;
}

uint8_t rs_r_uint8(raw_stream* rs)
{
    uint8_t r = (uint8_t)(rs->end[0]);
    rs->end++;
    return r;
}

void rs_w_uint16(raw_stream* rs, uint16_t v)
{
    rs->end[0] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 2;
}

uint16_t rs_r_uint16(raw_stream* rs)
{
    uint16_t r = 0;
    r |= (uint16_t)(rs->end[0]) << 8;
    r |= (uint16_t)(rs->end[1]) << 0;
    rs->end += 2;
    return r;
}

void rs_w_uint32(raw_stream* rs, uint32_t v)
{
    rs->end[0] = (uint8_t)((v >> 24) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 16) & 0xFF);
    rs->end[2] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[3] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 4;
}

uint32_t rs_r_uint32(raw_stream* rs)
{
    uint32_t r = 0;
    r |= (uint32_t)(rs->end[0]) << 24;
    r |= (uint32_t)(rs->end[1]) << 16;
    r |= (uint32_t)(rs->end[2]) << 8;
    r |= (uint32_t)(rs->end[3]) << 0;
    rs->end += 4;
    return r;
}

void rs_w_uint64(raw_stream* rs, uint64_t v)
{
    rs->end[0] = (uint8_t)((v >> 56) & 0xFF);
    rs->end[1] = (uint8_t)((v >> 48) & 0xFF);
    rs->end[2] = (uint8_t)((v >> 40) & 0xFF);
    rs->end[3] = (uint8_t)((v >> 32) & 0xFF);
    rs->end[4] = (uint8_t)((v >> 24) & 0xFF);
    rs->end[5] = (uint8_t)((v >> 16) & 0xFF);
    rs->end[6] = (uint8_t)((v >> 8) & 0xFF);
    rs->end[7] = (uint8_t)((v >> 0) & 0xFF);
    rs->end += 8;
}

uint64_t rs_r_uint64(raw_stream* rs)
{
    uint64_t r = 0;
    r |= (uint64_t)(rs->end[0]) << 56;
    r |= (uint64_t)(rs->end[1]) << 48;
    r |= (uint64_t)(rs->end[2]) << 40;
    r |= (uint64_t)(rs->end[3]) << 32;
    r |= (uint64_t)(rs->end[4]) << 24;
    r |= (uint64_t)(rs->end[5]) << 16;
    r |= (uint64_t)(rs->end[6]) << 8;
    r |= (uint64_t)(rs->end[7]) << 0;
    rs->end += 8;
    return r;
}

void rs_w_size(raw_stream* rs, size_t v)
{
    rs_w_uint64(rs, (uint64_t)(v));
}

size_t rs_r_size(raw_stream* rs)
{
    return (size_t)rs_r_uint64(rs);
}

void rs_w_float(raw_stream* rs, float v)
{
    union {
        float f;
        uint8_t u8[sizeof(float)];
    } f32 = {.f = v};

    for (int i = 0; i < sizeof(float); i++) {
        rs_w_uint8(rs, f32.u8[i]);
    }
}

float rs_r_float(raw_stream* rs)
{
    union {
        float f;
        uint8_t u8[sizeof(float)];
    } f32;

    for (int i = 0; i < sizeof(float); i++) {
        f32.u8[i] = rs_r_uint8(rs);
    }
    return f32.f;
}

void rs_w_double(raw_stream* rs, double v)
{
    union {
        double d;
        uint8_t u8[sizeof(double)];
    } f64 = {.d = v};

    for (int i = 0; i < sizeof(double); i++) {
        rs_w_uint8(rs, f64.u8[i]);
    }
}

double rs_r_double(raw_stream* rs)
{
    union {
        double d;
        uint8_t u8[sizeof(double)];
    } f64;

    for (int i = 0; i < sizeof(double); i++) {
        f64.u8[i] = rs_r_uint8(rs);
    }
    return f64.d;
}

void rs_w_string(raw_stream* rs, char* str)
{
    size_t len = strlen(str) + 1;
    memcpy(rs->end, str, len);
    rs->end += len;
}

size_t rs_r_string_len(raw_stream* rs)
{
    return strlen((char*)rs->end) + 1;
}

void rs_r_string(raw_stream* rs, char* str)
{
    size_t len = strlen((char*)rs->end) + 1;
    memcpy(str, rs->end, len);
    rs->end += len;
}

void rs_w_raw(raw_stream* rs, void* buf, size_t len)
{
    rs_w_size(rs, len);
    memcpy(rs->end, buf, len);
    rs->end += len;
}

size_t rs_r_raw_len(raw_stream* rs)
{
    size_t len = rs_r_size(rs);
    rs->end -= sizeof(uint64_t); // this function should not alter the stream position
    return len;
}

void rs_r_raw(raw_stream* rs, void* buf)
{
    size_t len = rs_r_size(rs);
    memcpy(buf, rs->end, len);
    rs->end += len;
}

#ifdef __cplusplus
}
#endif
