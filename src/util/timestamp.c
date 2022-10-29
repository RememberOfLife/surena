#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "surena/util/serialization.h"

#include "surena/util/timestamp.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t surena_get_ms64()
{
    struct timespec record;
    clock_gettime(CLOCK_MONOTONIC_RAW, &record);
    return record.tv_sec * 1000 + record.tv_nsec / 1000000;
}

uint64_t surena_get_ns64()
{
    struct timespec record;
    clock_gettime(CLOCK_MONOTONIC_RAW, &record);
    return record.tv_sec * 1000000000 + record.tv_nsec;
}

// modelled after: https://github.com/protocolbuffers/protobuf/blob/main/src/google/protobuf/timestamp.proto

timestamp timestamp_get()
{
    struct timespec record;
    clock_gettime(CLOCK_REALTIME, &record);
    return (timestamp){
        .time = record.tv_sec,
        .fraction = record.tv_nsec,
    };
}

int timestamp_compare(timestamp lhs, timestamp rhs)
{
    int rv = (lhs.time > rhs.time) - (lhs.time < rhs.time);
    if (rv == 0) {
        rv = (lhs.fraction > rhs.fraction) - (lhs.fraction < rhs.fraction);
    }
    return rv;
}

timestamp timestamp_diff(timestamp ts1, timestamp ts2)
{
    timestamp res;
    int64_t fraction;
    int cmp = timestamp_compare(ts1, ts2);
    if (cmp == 1) {
        // ts1 > ts2
        res.time = ts1.time - ts2.time;
        fraction = (int64_t)ts1.fraction - (int64_t)ts2.fraction;
    } else {
        // ts1 <= ts2
        res.time = ts2.time - ts1.time;
        fraction = (int64_t)ts2.fraction - (int64_t)ts1.fraction;
    }
    if (fraction < 0) {
        if (cmp == 0) {
            fraction = -fraction;
        } else {
            res.time -= 1;
            fraction += (int64_t)TIMESTAMP_MAX_FRACTION + 1;
        }
    }
    res.fraction = (uint32_t)fraction;
    return res;
}

timestamp timestamp_add(timestamp ts1, timestamp ts2)
{
    timestamp res = (timestamp){
        .time = ts1.time + ts2.time,
        .fraction = ts1.fraction + ts2.fraction,
    };
    if (res.fraction > TIMESTAMP_MAX_FRACTION) {
        res.time += 1;
        res.fraction -= TIMESTAMP_MAX_FRACTION + 1;
    }
    return res;
}

const serialization_layout sl_timestamp[] = {
    {SL_TYPE_U64, offsetof(timestamp, time)},
    {SL_TYPE_U32, offsetof(timestamp, fraction)},
    {SL_TYPE_STOP},
};

#ifdef __cplusplus
}
#endif
