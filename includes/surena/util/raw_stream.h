#pragma once

#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct raw_stream_s {
    uint8_t* begin;
    uint8_t* end;
} raw_stream;

raw_stream rs_init(void* buf);
size_t rs_get_size(raw_stream* rs);

void rs_w_bool(raw_stream* rs, bool v);
bool rs_r_bool(raw_stream* rs);

void rs_w_int8(raw_stream* rs, int8_t v);
int8_t rs_r_int8(raw_stream* rs);

void rs_w_int16(raw_stream* rs, int16_t v);
int16_t rs_r_int16(raw_stream* rs);

void rs_w_int32(raw_stream* rs, int32_t v);
int32_t rs_r_int32(raw_stream* rs);

void rs_w_int64(raw_stream* rs, int64_t v);
int64_t rs_r_int64(raw_stream* rs);

void rs_w_uint8(raw_stream* rs, uint8_t v);
uint8_t rs_r_uint8(raw_stream* rs);

void rs_w_uint16(raw_stream* rs, uint16_t v);
uint16_t rs_r_uint16(raw_stream* rs);

void rs_w_uint32(raw_stream* rs, uint32_t v);
uint32_t rs_r_uint32(raw_stream* rs);

void rs_w_uint64(raw_stream* rs, uint64_t v);
uint64_t rs_r_uint64(raw_stream* rs);

void rs_w_size(raw_stream* rs, size_t v);
size_t rs_r_size(raw_stream* rs);

void rs_w_float(raw_stream* rs, float v);
float rs_r_float(raw_stream* rs);

void rs_w_double(raw_stream* rs, double v);
double rs_r_double(raw_stream* rs);

void rs_w_string(raw_stream* rs, char* str);
size_t rs_r_string_len(raw_stream* rs); // does NOT alter the stream position; DOES account for the null character
void rs_r_string(raw_stream* rs, char* str);

void rs_w_raw(raw_stream* rs, void* buf, size_t len);
size_t rs_r_raw_len(raw_stream* rs); // does NOT alter the stream position
void rs_r_raw(raw_stream* rs, void* buf);

#ifdef __cplusplus
}
#endif
