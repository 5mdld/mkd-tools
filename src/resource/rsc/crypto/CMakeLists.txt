target_sources(mkd PRIVATE resource_crypto.cpp)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_QUIET TRUE)
unset(resource_crypto_has_neon CACHE)
check_cxx_source_compiles([[
    #include <arm_neon.h>
    int main() {
        const auto bytes = vdupq_n_u8(0);
        return vgetq_lane_u8(vqtbl1q_u8(bytes, bytes), 0);
    }
]] resource_crypto_has_neon)

unset(resource_crypto_has_ssse3 CACHE)
check_cxx_source_compiles([[
    #include <tmmintrin.h>
    #if !defined(__SSSE3__) && !defined(__AVX__)
    #error SSSE3 must be part of the compilation target
    #endif
    int main(int argc, char**) {
        const auto bytes = _mm_cvtsi32_si128(argc);
        return _mm_cvtsi128_si32(_mm_shuffle_epi8(bytes, bytes));
    }
]] resource_crypto_has_ssse3)

if (resource_crypto_has_neon)
    target_sources(mkd PRIVATE resource_crypto_neon.cpp)
elseif (resource_crypto_has_ssse3)
    target_sources(mkd PRIVATE resource_crypto_sse.cpp)
else ()
    target_sources(mkd PRIVATE resource_crypto_scalar.cpp)
endif ()
