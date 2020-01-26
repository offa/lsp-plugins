/*
 * float.h
 *
 *  Created on: 06 авг. 2016 г.
 *      Author: sadko
 */

#ifndef DSP_ARCH_X86_SSE2_FLOAT_H_
#define DSP_ARCH_X86_SSE2_FLOAT_H_

#ifndef DSP_ARCH_X86_SSE2_IMPL
    #error "This header should not be included directly"
#endif /* DSP_ARCH_X86_SSE2_IMPL */

namespace sse2
{
    #define U4VEC(x)        x, x, x, x
    IF_ARCH_X86(
        static uint32_t XC_SAT[] __lsp_aligned16 =
        {
            U4VEC(0x7fffffff),            // X_ABS
            U4VEC(0x80000000),            // X_SIGN
            U4VEC(0x7f800000),            // X_P_INF
            U4VEC(FLOAT_SAT_P_NAN_I),     // SX_P_NAN
            U4VEC(FLOAT_SAT_P_INF_I)      // SX_P_INF
        };
    )

#define MULTIPLE_SATURATION_BODY(dst, src) \
    /* Prepare values */ \
    __ASM_EMIT("movaps          0x00 + %[XC], %%xmm6") \
    __ASM_EMIT("movaps          0x10 + %[XC], %%xmm7") \
    __ASM_EMIT("xor             %[off], %[off]") \
    __ASM_EMIT("sub             $4, %[count]") \
    __ASM_EMIT("jb              2f") \
    \
    /* x4 blocks */ \
    __ASM_EMIT("1:") \
    __ASM_EMIT("movups          0x00(%[" src "], %[off]), %%xmm0")  /* xmm0 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm1")                    /* xmm1 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm2")                    /* xmm2 = s */ \
    __ASM_EMIT("andps           %%xmm6, %%xmm1")                    /* xmm1 = abs(s)  */ \
    __ASM_EMIT("andps           %%xmm7, %%xmm2")                    /* xmm2 = sign(s) */ \
    __ASM_EMIT("movaps          %%xmm1, %%xmm3")                    /* xmm3 = abs(s)  */ \
    __ASM_EMIT("pcmpgtd         0x20 + %[XC], %%xmm1")              /* xmm1 = abs(s) > +Inf */ \
    __ASM_EMIT("pcmpeqd         0x20 + %[XC], %%xmm3")              /* xmm3 = abs(s) == +Inf */ \
    __ASM_EMIT("movaps          %%xmm1, %%xmm4")                    /* xmm4 = abs(s) > +Inf */ \
    __ASM_EMIT("movaps          %%xmm3, %%xmm5")                    /* xmm5 = abs(s) == +Inf */ \
    __ASM_EMIT("andps           0x30 + %[XC], %%xmm4")              /* xmm4 = P_NAN & (abs(s) > +Inf) */ \
    __ASM_EMIT("andps           0x40 + %[XC], %%xmm5")              /* xmm4 = P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("orps            %%xmm3, %%xmm1")                    /* xmm1 = abs(s) >= +Inf */ \
    __ASM_EMIT("orps            %%xmm5, %%xmm4")                    /* xmm4 = P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("andnps          %%xmm0, %%xmm1")                    /* xmm1 = s & (abs(s) < +Inf) */ \
    __ASM_EMIT("orps            %%xmm4, %%xmm1")                    /* xmm1 = s & (abs(s) < +Inf) | P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("orps            %%xmm2, %%xmm1")                    /* xmm1 = sign(s) | (s & (abs(s) < +Inf) | P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf)) */ \
    __ASM_EMIT("movups          %%xmm1, 0x00(%[" dst "], %[off])") \
    __ASM_EMIT("add             $0x10, %[off]") \
    __ASM_EMIT("sub             $4, %[count]") \
    __ASM_EMIT("jae             1b") \
    __ASM_EMIT("2:") \
    /* x1 blocks */ \
    __ASM_EMIT("add             $3, %[count]") \
    __ASM_EMIT("jl              4f") \
    __ASM_EMIT("3:") \
    __ASM_EMIT("movss           0x00(%[" src "], %[off]), %%xmm0")  /* xmm0 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm1")                    /* xmm1 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm2")                    /* xmm2 = s */ \
    __ASM_EMIT("andps           %%xmm6, %%xmm1")                    /* xmm1 = abs(s)  */ \
    __ASM_EMIT("andps           %%xmm7, %%xmm2")                    /* xmm2 = sign(s) */ \
    __ASM_EMIT("movaps          %%xmm1, %%xmm3")                    /* xmm3 = abs(s)  */ \
    __ASM_EMIT("pcmpgtd         0x20 + %[XC], %%xmm1")              /* xmm1 = abs(s) > +Inf */ \
    __ASM_EMIT("pcmpeqd         0x20 + %[XC], %%xmm3")              /* xmm3 = abs(s) == +Inf */ \
    __ASM_EMIT("movaps          %%xmm1, %%xmm4")                    /* xmm4 = abs(s) > +Inf */ \
    __ASM_EMIT("movaps          %%xmm3, %%xmm5")                    /* xmm5 = abs(s) == +Inf */ \
    __ASM_EMIT("andps           0x30 + %[XC], %%xmm4")              /* xmm4 = P_NAN & (abs(s) > +Inf) */ \
    __ASM_EMIT("andps           0x40 + %[XC], %%xmm5")              /* xmm4 = P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("orps            %%xmm3, %%xmm1")                    /* xmm1 = abs(s) >= +Inf */ \
    __ASM_EMIT("orps            %%xmm5, %%xmm4")                    /* xmm4 = P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("andnps          %%xmm0, %%xmm1")                    /* xmm1 = s & (abs(s) < +Inf) */ \
    __ASM_EMIT("orps            %%xmm4, %%xmm1")                    /* xmm1 = s & (abs(s) < +Inf) | P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf) */ \
    __ASM_EMIT("orps            %%xmm2, %%xmm1")                    /* xmm1 = sign(s) | (s & (abs(s) < +Inf) | P_NAN & (abs(s) > +Inf) | P_INF & (abs(s) == +Inf)) */ \
    __ASM_EMIT("movss           %%xmm1, 0x00(%[" dst "], %[off])") \
    __ASM_EMIT("add             $0x04, %[off]") \
    __ASM_EMIT("dec             %[count]") \
    __ASM_EMIT("jge             3b") \
    __ASM_EMIT("4:") \

    void copy_saturated(float *dst, const float *src, size_t count)
    {
        size_t off;

        ARCH_X86_ASM
        (
            MULTIPLE_SATURATION_BODY("dst", "src")
            : [count] "+r" (count), [off] "=&r" (off)
            : [dst] "r" (dst), [src] "r" (src),
              [XC] "o" (XC_SAT)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

    void saturate(float *dst, size_t count)
    {
        size_t off;

        ARCH_X86_ASM
        (
            MULTIPLE_SATURATION_BODY("dst", "dst")
            : [count] "+r" (count), [off] "=&r" (off)
            : [dst] "r" (dst),
              [XC] "o" (XC_SAT)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

#undef MULTIPLE_SATURATION_BODY

#define LIMIT_SAT_BODY \
    /* xmm0 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm1")                /* xmm1 = s */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm2")                /* xmm2 = s */ \
    __ASM_EMIT("movaps          %%xmm1, %%xmm3")                /* xmm3 = s */ \
    __ASM_EMIT("movaps          %%xmm2, %%xmm4")                /* xmm4 = s */ \
    __ASM_EMIT("cmpps           $5, 0x00 + %[XC], %%xmm1")      /* xmm1 = [ s >= -1 ] */ \
    __ASM_EMIT("cmpps           $2, 0x10 + %[XC], %%xmm2")      /* xmm2 = [ s <= 1 ] */ \
    __ASM_EMIT("andps           0x20 + %[XC], %%xmm3")          /* xmm3 = sign(s) */ \
    __ASM_EMIT("andps           %%xmm2, %%xmm1")                /* xmm1 = [ s >= -1 ] & [ s <= 1 ] */ \
    __ASM_EMIT("andps           0x30 + %[XC], %%xmm4")          /* xmm4 = abs(s) */ \
    __ASM_EMIT("andps           %%xmm1, %%xmm0")                /* xmm0 = s & [ s >= -1 ] & [ s <= 1 ] */ \
    __ASM_EMIT("pcmpgtd         0x40 + %[XC], %%xmm4")          /* xmm4 = abs(s) > +Inf */ \
    __ASM_EMIT("andnps          0x10 + %[XC], %%xmm4")          /* xmm4 = 1 & (abs(s) <= +Inf) */ \
    __ASM_EMIT("orps            %%xmm3, %%xmm4")                /* xmm4 = (1 & (abs(s) == +Inf)) | sign(s) */ \
    __ASM_EMIT("andnps          %%xmm4, %%xmm1")                /* xmm1 = ((1 & (abs(s) == +Inf)) | sign(s)) & ([ s < -1 ] | [ s > 1 ]) */ \
    __ASM_EMIT("orps            %%xmm1, %%xmm0")                /* xmm0 = (s & [ s >= -1 ] & [ s <= 1 ]) | (((1 & (abs(s) <= +Inf)) | sign(s)) & ([ s < -1 ] | [ s > 1 ])) */

#define U4VEC(x)        x, x, x, x
IF_ARCH_X86(
    static uint32_t XLIM_SAT[] __lsp_aligned16 =
    {
        U4VEC(0xbf800000),      // -1
        U4VEC(0x3f800000),      // +1
        U4VEC(0x80000000),      // sign
        U4VEC(0x7fffffff),      // abs
        U4VEC(0x7f800000)       // +Inf
    };
)

#undef U4VEC

    void limit_saturate1(float *dst, size_t count)
    {
        ARCH_X86_ASM
        (
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jb          2f")

            // 4x blocks
            __ASM_EMIT("1:")
            __ASM_EMIT("movups      (%[dst]), %%xmm0")
            LIMIT_SAT_BODY
            __ASM_EMIT("movups      %%xmm0, (%[dst])")
            __ASM_EMIT("add         $0x10, %[dst]")
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jae         1b")

            __ASM_EMIT("2:")
            __ASM_EMIT("add         $4, %[count]")
            __ASM_EMIT("jle         10f")

            // 1x - 3x block
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          4f")
            __ASM_EMIT("movss       (%[dst]), %%xmm0")
            __ASM_EMIT("add         $4, %[dst]")
            __ASM_EMIT("4:")
            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          6f")
            __ASM_EMIT("movlhps     %%xmm0, %%xmm0")
            __ASM_EMIT("movlps      (%[dst]), %%xmm0")
            __ASM_EMIT("6:")

            LIMIT_SAT_BODY

            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          8f")
            __ASM_EMIT("movlps      %%xmm0, (%[dst])")
            __ASM_EMIT("movhlps     %%xmm0, %%xmm0")
            __ASM_EMIT("8:")
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          10f")
            __ASM_EMIT("sub         $4, %[dst]")
            __ASM_EMIT("movss       %%xmm0, (%[dst])")

            __ASM_EMIT("10:")
            : [dst] "+r" (dst), [count] "+r" (count)
            : [XC] "o" (XLIM_SAT)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

    void limit_saturate2(float *dst, const float *src, size_t count)
    {
        ARCH_X86_ASM
        (
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jb          2f")

            // 4x blocks
            __ASM_EMIT("1:")
            __ASM_EMIT("movups      (%[src]), %%xmm0")
            LIMIT_SAT_BODY
            __ASM_EMIT("movups      %%xmm0, (%[dst])")
            __ASM_EMIT("add         $0x10, %[src]")
            __ASM_EMIT("add         $0x10, %[dst]")
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jae         1b")

            __ASM_EMIT("2:")
            __ASM_EMIT("add         $4, %[count]")
            __ASM_EMIT("jle         10f")

            // 1x - 3x block
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          4f")
            __ASM_EMIT("movss       (%[src]), %%xmm0")
            __ASM_EMIT("add         $4, %[src]")
            __ASM_EMIT("add         $4, %[dst]")
            __ASM_EMIT("4:")
            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          6f")
            __ASM_EMIT("movlhps     %%xmm0, %%xmm0")
            __ASM_EMIT("movlps      (%[src]), %%xmm0")
            __ASM_EMIT("6:")

            LIMIT_SAT_BODY

            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          8f")
            __ASM_EMIT("movlps      %%xmm0, (%[dst])")
            __ASM_EMIT("movhlps     %%xmm0, %%xmm0")
            __ASM_EMIT("8:")
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          10f")
            __ASM_EMIT("sub         $4, %[dst]")
            __ASM_EMIT("movss       %%xmm0, (%[dst])")

            __ASM_EMIT("10:")
            : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
            : [XC] "o" (XLIM_SAT)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

#undef LIMIT_SAT_BODY

#define U4VEC(x)        x, x, x, x
IF_ARCH_X86(
    static uint32_t XLIM[] __lsp_aligned16 =
    {
        U4VEC(0x7fffffff),      // abs
        U4VEC(0x7f800000)       // +Inf
    };
)

#undef U4VEC

#define LIMIT_BODY \
    /* xmm0 = s1, xmm6 = min, xmm7 = max */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm1")                /* xmm1 = s */ \
    __ASM_EMIT("movaps          %%xmm6, %%xmm3")                /* xmm3 = min */ \
    __ASM_EMIT("movaps          %%xmm0, %%xmm2")                /* xmm2 = s */ \
    __ASM_EMIT("movaps          %%xmm7, %%xmm4")                /* xmm4 = max */ \
    __ASM_EMIT("movaps          %%xmm6, %%xmm5")                /* xmm5 = min */ \
    __ASM_EMIT("cmpps           $1, %%xmm6, %%xmm1")            /* xmm1 = [s < min] */ \
    __ASM_EMIT("cmpps           $6, %%xmm7, %%xmm2")            /* xmm2 = [s > max] */ \
    __ASM_EMIT("andps           %%xmm1, %%xmm3")                /* xmm3 = [s < min] & min */ \
    __ASM_EMIT("andps           %%xmm2, %%xmm4")                /* xmm4 = [s > max] & max */ \
    __ASM_EMIT("orps            %%xmm2, %%xmm1")                /* xmm1 = [s < min] | [ s > max] */ \
    __ASM_EMIT("orps            %%xmm3, %%xmm4")                /* xmm4 = ([s < min] & min) | ([s > max] & max) */ \
    __ASM_EMIT("andnps          %%xmm0, %%xmm1")                /* xmm1 = ([s >= min] | [s <= max]) & s */ \
    __ASM_EMIT("andps           0x00 + %[XC], %%xmm0")          /* xmm0 = abs(s) */ \
    __ASM_EMIT("orps            %%xmm4, %%xmm1")                /* xmm1 = (([s >= min] | [s <= max]) & s) | ([s < min] & min) | ([s > max] & max) */ \
    __ASM_EMIT("pcmpgtd         0x10 + %[XC], %%xmm0")          /* xmm0 = [abs(s) > +Inf] */ \
    __ASM_EMIT("andps           %%xmm0, %%xmm5")                /* xmm5 = [abs(s) > +Inf] & min */ \
    __ASM_EMIT("andnps          %%xmm1, %%xmm0")                /* xmm0 = [abs(s) <= +Inf] & (([s >= min] | [s <= max]) & s) | ([s < min] & min) | ([s > max] & max) */ \
    __ASM_EMIT("orps            %%xmm5, %%xmm0")                /* xmm5 = ([abs(s) > +Inf] & min) | ([abs(s) <= +Inf] & (([s >= min] | [s <= max]) & s) | ([s < min] & min) | ([s > max] & max)) */ \

    void limit1(float *dst, float min, float max, size_t count)
    {
        ARCH_X86_ASM
        (
            __ASM_EMIT("movss       %[min], %%xmm6")
            __ASM_EMIT("movss       %[max], %%xmm7")
            __ASM_EMIT("shufps      $0x00, %%xmm6, %%xmm6")
            __ASM_EMIT("shufps      $0x00, %%xmm7, %%xmm7")

            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jb          2f")

            // 4x blocks
            __ASM_EMIT("1:")
            __ASM_EMIT("movups      (%[dst]), %%xmm0")
            LIMIT_BODY
            __ASM_EMIT("movups      %%xmm0, (%[dst])")
            __ASM_EMIT("add         $0x10, %[dst]")
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jae         1b")

            __ASM_EMIT("2:")
            __ASM_EMIT("add         $4, %[count]")
            __ASM_EMIT("jle         10f")

            // 1x - 3x block
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          4f")
            __ASM_EMIT("movss       (%[dst]), %%xmm0")
            __ASM_EMIT("add         $4, %[dst]")
            __ASM_EMIT("4:")
            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          6f")
            __ASM_EMIT("movlhps     %%xmm0, %%xmm0")
            __ASM_EMIT("movlps      (%[dst]), %%xmm0")
            __ASM_EMIT("6:")

            LIMIT_BODY

            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          8f")
            __ASM_EMIT("movlps      %%xmm0, (%[dst])")
            __ASM_EMIT("movhlps     %%xmm0, %%xmm0")
            __ASM_EMIT("8:")
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          10f")
            __ASM_EMIT("sub         $4, %[dst]")
            __ASM_EMIT("movss       %%xmm0, (%[dst])")

            __ASM_EMIT("10:")
            : [dst] "+r" (dst), [count] "+r" (count)
            : [XC] "o" (XLIM),
              [min] "m" (min),
              [max] "m" (max)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

    void limit2(float *dst, const float *src, float min, float max, size_t count)
    {
        ARCH_X86_ASM
        (
            __ASM_EMIT("movss       %[min], %%xmm6")
            __ASM_EMIT("movss       %[max], %%xmm7")
            __ASM_EMIT("shufps      $0x00, %%xmm6, %%xmm6")
            __ASM_EMIT("shufps      $0x00, %%xmm7, %%xmm7")

            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jb          2f")

            // 4x blocks
            __ASM_EMIT("1:")
            __ASM_EMIT("movups      (%[src]), %%xmm0")
            LIMIT_BODY
            __ASM_EMIT("movups      %%xmm0, (%[dst])")
            __ASM_EMIT("add         $0x10, %[src]")
            __ASM_EMIT("add         $0x10, %[dst]")
            __ASM_EMIT("sub         $4, %[count]")
            __ASM_EMIT("jae         1b")

            __ASM_EMIT("2:")
            __ASM_EMIT("add         $4, %[count]")
            __ASM_EMIT("jle         10f")

            // 1x - 3x block
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          4f")
            __ASM_EMIT("movss       (%[src]), %%xmm0")
            __ASM_EMIT("add         $4, %[src]")
            __ASM_EMIT("add         $4, %[dst]")
            __ASM_EMIT("4:")
            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          6f")
            __ASM_EMIT("movlhps     %%xmm0, %%xmm0")
            __ASM_EMIT("movlps      (%[src]), %%xmm0")
            __ASM_EMIT("6:")

            LIMIT_BODY

            __ASM_EMIT("test        $2, %[count]")
            __ASM_EMIT("jz          8f")
            __ASM_EMIT("movlps      %%xmm0, (%[dst])")
            __ASM_EMIT("movhlps     %%xmm0, %%xmm0")
            __ASM_EMIT("8:")
            __ASM_EMIT("test        $1, %[count]")
            __ASM_EMIT("jz          10f")
            __ASM_EMIT("sub         $4, %[dst]")
            __ASM_EMIT("movss       %%xmm0, (%[dst])")

            __ASM_EMIT("10:")
            : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
            : [XC] "o" (XLIM),
              [min] "m" (min),
              [max] "m" (max)
            : "cc", "memory",
              "%xmm0", "%xmm1", "%xmm2", "%xmm3",
              "%xmm4", "%xmm5", "%xmm6", "%xmm7"
        );
    }

}

#endif /* DSP_ARCH_X86_SSE2_FLOAT_H_ */
