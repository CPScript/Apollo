#ifndef APOLLO_TYPES_H
#define APOLLO_TYPES_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef uint64_t size_t;
typedef uint64_t uintptr_t;
typedef int64_t ptrdiff_t;

typedef enum {
    false = 0,
    true = 1
} bool;

#ifndef NULL
#define NULL ((void*)0)
#endif

#define PACKED __attribute__((packed))
#define ALIGNED(x) __attribute__((aligned(x)))
#define SECTION(x) __attribute__((section(x)))
#define USED __attribute__((used))
#define UNUSED __attribute__((unused))

#define MEMORY_BARRIER() __asm__ __volatile__("mfence" ::: "memory")
#define COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")

#define BIT(n) (1ULL << (n))
#define MASK(bits) (BIT(bits) - 1)
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef enum {
    APOLLO_SUCCESS = 0,
    APOLLO_ERROR_INVALID_PARAMETER = -1,
    APOLLO_ERROR_OUT_OF_MEMORY = -2,
    APOLLO_ERROR_NOT_FOUND = -3,
    APOLLO_ERROR_ACCESS_DENIED = -4,
    APOLLO_ERROR_TIMEOUT = -5,
    APOLLO_ERROR_BUSY = -6,
    APOLLO_ERROR_NOT_IMPLEMENTED = -7
} apollo_result_t;

#endif