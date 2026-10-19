#pragma once

#include <stdint.h>

#if defined(_WIN32)
#  if defined(ENGINE_COMPUTE_BUILD)
#    define ENGINE_API __declspec(dllexport)
#  else
#    define ENGINE_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EngineHandle EngineHandle;

typedef enum EngineStatus {
    ENGINE_STATUS_OK = 0,
    ENGINE_STATUS_INVALID_ARGUMENT = 1,
    ENGINE_STATUS_OUT_OF_MEMORY = 2,
    ENGINE_STATUS_INITIALIZATION_FAILED = 3,
    ENGINE_STATUS_INVALID_STATE = 4,
    ENGINE_STATUS_STEP_FAILED = 5,
    ENGINE_STATUS_NOT_IMPLEMENTED = 6,
    ENGINE_STATUS_INTERNAL_ERROR = 7
} EngineStatus;

typedef struct EngineCreateOptions {
    uint32_t struct_size;
    uint32_t initial_particle_count;
} EngineCreateOptions;

/*
 * Creates an engine. On every failure, *out_handle is set to NULL.
 * options->struct_size must equal sizeof(EngineCreateOptions).
 */
ENGINE_API EngineStatus EngineCreate(
    const EngineCreateOptions* options,
    EngineHandle** out_handle);

/* Performs one logical fixed-timestep tick; wall-clock delta is not accepted. */
ENGINE_API EngineStatus EngineStep(EngineHandle* handle);

/* Null handles are accepted. Destruction is best-effort and never throws. */
ENGINE_API EngineStatus EngineDestroy(EngineHandle* handle);

/*
 * Copies the calling thread's most recent native diagnostic into buffer.
 * out_required_capacity includes the trailing NUL. buffer may be NULL only
 * when buffer_capacity is zero. Diagnostics are best-effort, not stable API text.
 */
ENGINE_API EngineStatus EngineGetLastError(
    char* buffer,
    uint32_t buffer_capacity,
    uint32_t* out_required_capacity);

#ifdef __cplusplus
}
#endif
