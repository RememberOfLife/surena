#pragma once

#include <stdint.h>

#include "surena/util/serialization.h"

#ifdef __cplusplus
extern "C" {
#endif

//TODO rename to timestamp_?
// general purpose timing function that counts up monotonically
uint64_t surena_get_ms64(); // milliseconds
uint64_t surena_get_ns64(); // nanoseconds

static const uint32_t TIMESTAMP_MAX_FRACTION = 999999999;

// represents either: a point in time, if precision allows, to the nanosecond
// or: a duration of time
typedef struct timestamp_s {
    uint64_t time; // unix timestamp in seconds
    uint32_t fraction; // nanosecond fraction 0-999.999.999 (TIMESTAMP_MAX_FRACTION)
} timestamp;

timestamp timestamp_get();
/*
// usage example to extract local time info from this:
timestamp ts = timestamp_get(NULL);
struct tm* info;
info = localtime((const time_t*)&ts.time);
printf("%d-%d-%d %02d:%02d:%02d.%09d", 1900 + info->tm_year, info->tm_mon, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec, ts.fraction);
*/

// lhs < rhs ==> -1
// lhs = rhs ==> 0
// lhs > rhs ==> 1
int timestamp_compare(timestamp lhs, timestamp rhs);

// returns the absolute difference between ts1 and ts2, regardless of which is greater, as a duration
timestamp timestamp_diff(timestamp ts1, timestamp ts2);

// recommended use: timestamp + duration
timestamp timestamp_add(timestamp ts1, timestamp ts2);

extern const serialization_layout sl_timestamp[];

#ifdef __cplusplus
}
#endif
