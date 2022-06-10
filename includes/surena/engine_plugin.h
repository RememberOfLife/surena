#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "surena/engine.h"

// returns the capi version used to build the plugin
typedef uint64_t (*plugin_get_engine_capi_version_t)();
uint64_t plugin_get_engine_capi_version();

// writes the plugin static pointers to the engine methods this plugin brings to methods
// if methods is NULL then count returns the number of methods this may write
// otherwise count returns the number of methods written
// this may only be called once for the plugin
typedef void (*plugin_get_engine_methods_t)(uint32_t* count, const engine_methods** methods);
void plugin_get_engine_methods(uint32_t* count, const engine_methods** methods);

//TODO plugin_cleanup

#ifdef __cplusplus
}
#endif
