/*
 * mul.cpp
 *
 *  Created on: 22 авг. 2018 г.
 *      Author: sadko
 */

#include <dsp/dsp.h>
#include <test/ptest.h>
#include <core/sugar.h>

#define MIN_RANK 8
#define MAX_RANK 16

namespace native
{
    void complex_mul(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
    void pcomplex_mul(float *dst, const float *src1, const float *src2, size_t count);
}

IF_ARCH_X86(
    namespace sse
    {
        void complex_mul(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
        void pcomplex_mul(float *dst, const float *src1, const float *src2, size_t count);
    }

    namespace sse3
    {
        void pcomplex_mul(float *dst, const float *src1, const float *src2, size_t count);
    }

    IF_ARCH_X86_64(
        namespace sse3
        {
            void x64_pcomplex_mul(float *dst, const float *src1, const float *src2, size_t count);
        }

        namespace avx
        {
            void x64_complex_mul(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
            void x64_complex_mul_fma3(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
            void x64_pcomplex_mul(float *dst, const float *src1, const float *src2, size_t count);
            void x64_pcomplex_mul_fma3(float *dst, const float *src1, const float *src2, size_t count);
        }
    )
)

IF_ARCH_ARM(
    namespace neon_d32
    {
        void complex_mul3(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
        void complex_mul3_x12(float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);

        void packed_complex_mul3(float *dst, const float *src1, const float *src2, size_t count);
    }
)

typedef void (* complex_mul_t) (float *dst_re, float *dst_im, const float *src1_re, const float *src1_im, const float *src2_re, const float *src2_im, size_t count);
typedef void (* packed_complex_mul_t) (float *dst, const float *src1, const float *src2, size_t count);

//-----------------------------------------------------------------------------
// Performance test for complex multiplication
PTEST_BEGIN("dsp.complex", mul, 5, 1000)

    void call(const char *label, float *dst, const float *src1, const float *src2, size_t count, packed_complex_mul_t mul)
    {
        if (!PTEST_SUPPORTED(mul))
            return;

        char buf[80];
        sprintf(buf, "%s x %d", label, int(count));
        printf("Testing %s numbers...\n", buf);

        PTEST_LOOP(buf,
            mul(dst, src1, src2, count);
        );
    }

    void call(const char *label, float *dst, const float *src1, const float *src2, size_t count, complex_mul_t mul)
    {
        if (!PTEST_SUPPORTED(mul))
            return;

        char buf[80];
        sprintf(buf, "%s x %d", label, int(count));
        printf("Testing %s numbers...\n", buf);

        PTEST_LOOP(buf,
            mul(dst, &dst[count], src1, &src1[count], src2, &src2[count], count);
        );
    }

    PTEST_MAIN
    {
        size_t buf_size = 1 << MAX_RANK;
        uint8_t *data   = NULL;
        float *ptr      = alloc_aligned<float>(data, buf_size *6, 64);

        float *out      = ptr;
        ptr            += buf_size*2;
        float *in1      = ptr;
        ptr            += buf_size*2;
        float *in2      = ptr;
        ptr            += buf_size*2;

        for (size_t i=0; i < (1 << (MAX_RANK + 1)); ++i)
        {
            in1[i]          = float(rand()) / RAND_MAX;
            in2[i]          = float(rand()) / RAND_MAX;
        }

        for (size_t i=MIN_RANK; i <= MAX_RANK; ++i)
        {
            size_t count = 1 << i;

            call("native:complex_mul", out, in1, in2, count, native::complex_mul);
            IF_ARCH_X86(call("sse:complex_mul", out, in1, in2, count, sse::complex_mul));
            IF_ARCH_X86_64(call("x64_avx:complex_mul", out, in1, in2, count, avx::x64_complex_mul));
            IF_ARCH_X86_64(call("x64_fma3:complex_mul", out, in1, in2, count, avx::x64_complex_mul_fma3));
            IF_ARCH_ARM(call("neon_d32:complex_mul", out, in1, in2, count, neon_d32::complex_mul3));
            IF_ARCH_ARM(call("neon_d32:complex_mul_x12", out, in1, in2, count, neon_d32::complex_mul3_x12));

            call("native:pcomplex_mul", out, in1, in2, count, native::pcomplex_mul);
            IF_ARCH_X86(call("sse:pcomplex_mul", out, in1, in2, count, sse::pcomplex_mul));
            IF_ARCH_X86(call("sse3:pcomplex_mul", out, in1, in2, count, sse3::pcomplex_mul));
            IF_ARCH_X86_64(call("x64_sse3:pcomplex_mul", out, in1, in2, count, sse3::x64_pcomplex_mul));
            IF_ARCH_X86_64(call("x64_avx:pcomplex_mul", out, in1, in2, count, avx::x64_pcomplex_mul));
            IF_ARCH_X86_64(call("x64_fma3:pcomplex_mul", out, in1, in2, count, avx::x64_pcomplex_mul_fma3));
            IF_ARCH_ARM(call("neon_d32:pcomplex_mul", out, in1, in2, count, neon_d32::packed_complex_mul3));

            PTEST_SEPARATOR;
        }

        free_aligned(data);
    }
PTEST_END


