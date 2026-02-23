typedef uint8_t u8;
typedef  int8_t i8;
typedef uint16_t u16;
typedef  int16_t i16;
typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;
typedef  int64_t i64;

#if defined(_MSC_VER) || true
    #include <intrin.h>
    // MSVC Intrinsics also GCC/Clang?
    #define ROTL8(x, n) _rotl8((x), (n))
    #define ROTR8(x, n) _rotr8((x), (n))
    #define ROTL16(x, n) _rotl16((x), (n))
    #define ROTR16(x, n) _rotr16((x), (n))

    #define ROTL32(x, n) _rotl((x), (n))
    #define ROTR32(x, n) _rotr((x), (n))
    #define ROTL64(x, n) _rotl64((x), (n))
    #define ROTR64(x, n) _rotr64((x), (n))
#else
    #define ROTR32(x, n) ((x >> n) | (x << (32 -  n)))
#endif

#define is_power2(x) ((x != 0) && ((x & (x - 1)) == 0))
#define log2_pure_power(x) ({ u32 _p = 0; typeof(x) _y = x; while (_y >>= 1) {_p++;} _p; })
#define highest_bit_index(x) ({ log2_pure_power(x); })

// False for size 0 buffer
#define any(buffer_ptr, end_ptr, _any_ptr_predicate_block) ({ \
    typeof(buffer_ptr) _any_ptr = (buffer_ptr);         \
    const typeof(end_ptr) _e = (end_ptr);               \
    _Bool _run = _any_ptr < _e;                          \
    _Bool _result = false;                               \
    while(_run) {                                       \
        _result = _any_ptr_predicate_block; _any_ptr++; \
        /* continue if case fail & more buffer */       \
        _run = (!_result) & (_any_ptr < _e);            \
    }                                                   \
    _result; /* statement expression return */          \
})

// True for size 0 buffer
#define all(buffer_ptr, end_ptr, _all_ptr_predicate_block) ({ \
    typeof(buffer_ptr) _all_ptr = (buffer_ptr);         \
    const typeof(end_ptr) _e = (end_ptr);               \
    _Bool _run = _all_ptr < _e;                          \
    _Bool _result = true;                                \
    while(_run) {                                       \
        _result = _all_ptr_predicate_block; _all_ptr++; \
        /* continue if case pass & more buffer */       \
        _run = _result & (_all_ptr < _e);               \
    }                                                   \
    _result; /* statement expression return */          \
})
