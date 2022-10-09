//
// Created by lujun on 19-6-28.
//
// This contains sin, cos and sqrt calculates
// each one has it's own function.
// All function can be found in main function.
// If you don't want to use it, then comment it.
#include "riscv_math.h"
#include <stdint.h>
#include <stdlib.h>
#include "../common.h"
#include <nuclei_sdk_hal.h>

#include "../HelperFunctions/math_helper.c"
#include "../HelperFunctions/ref_helper.c"

#include <stdio.h>
#define DELTAF32 (0.05f)
#define DELTAQ31 (63)
#define DELTAQ15 (1)
#define DELTAQ7 (1)

int test_flag_error = 0;

BENCH_DECLARE_VAR();

extern riscv_status ref_sqrt_q31(q31_t in, q31_t *pOut);
extern riscv_status ref_sqrt_q15(q15_t in, q15_t *pOut);
extern riscv_status ref_divide_q15(q15_t numerator, q15_t denominator, q15_t *quotient, int16_t *shift);
extern void ref_vexp_f32(const float32_t * pSrc, float32_t * pDst, uint32_t blockSize);
extern void ref_vlog_f32(const float32_t * pSrc, float32_t * pDst, uint32_t blockSize);

#define BLOCK_TESTSZ        256

static int DSP_SQRT(void)
{
    float32_t f32_pOUT[BLOCK_TESTSZ], f32_pOUT_ref[BLOCK_TESTSZ];
    float32_t f32_pIN[BLOCK_TESTSZ];
    q31_t q31_pOUT, q31_pOUT_ref;
    q15_t q15_pOUT, q15_pOUT_ref;
    volatile int i = 0;

    for (int i = 0; i < BLOCK_TESTSZ; i++) {
        f32_pIN[i] = (float32_t) abs(rand());
    }

    // f32_sqrt
    BENCH_START(riscv_sqrt_f32);
    for (i = 0; i < BLOCK_TESTSZ; i++) {
        riscv_sqrt_f32(f32_pIN[i], &f32_pOUT[i]);
    }
    BENCH_END(riscv_sqrt_f32);
    for (i = 0; i < BLOCK_TESTSZ; i++) {
        f32_pOUT_ref[i] = sqrtf(f32_pIN[i]);
    }
    for (i = 0; i < BLOCK_TESTSZ; i++) {
        if (fabs(f32_pOUT[i] - f32_pOUT_ref[i]) > DELTAF32) {
            BENCH_ERROR(riscv_sqrt_f32);
            printf("expect: %f, actual: %f\n", f32_pOUT_ref, f32_pOUT);
            test_flag_error = 1;
        }
    }
    BENCH_STATUS(riscv_sqrt_f32);

    // q31_sqrt
    // TODO optimize this case not using a fixed value
    BENCH_START(riscv_sqrt_q31);
    for (i = 0; i < BLOCK_TESTSZ; i++) {
        riscv_sqrt_q31(100, &q31_pOUT);
    }
    BENCH_END(riscv_sqrt_q31);
    ref_sqrt_q31(100, &q31_pOUT_ref);
    if (labs(q31_pOUT - q31_pOUT_ref) > DELTAQ31) {
        BENCH_ERROR(riscv_sqrt_q31);
        printf("expect: %x, actual: %x\n", q31_pOUT_ref, q31_pOUT);
        test_flag_error = 1;
    }
    BENCH_STATUS(riscv_sqrt_q31);

    // q15_sqrt
    // TODO optimize this case not using a fixed value
    BENCH_START(riscv_sqrt_q15);
    for (i = 0; i < BLOCK_TESTSZ; i++) {
        riscv_sqrt_q15(100, &q15_pOUT);
    }
    BENCH_END(riscv_sqrt_q15);
    ref_sqrt_q15(100, &q15_pOUT_ref);
    if (abs(q15_pOUT - q15_pOUT_ref) > DELTAQ15) {
        BENCH_ERROR(riscv_sqrt_q15);
        printf("expect: %x, actual: %x\n", q15_pOUT_ref, q15_pOUT);
        test_flag_error = 1;
    }
    BENCH_STATUS(riscv_sqrt_q15);
}

static int DSP_DIVIDE(void)
{
    q15_t q15_pOUT, q15_pOUT_ref;
    int16_t shift;

    // TODO optimize this case not using a fixed value
    BENCH_START(riscv_divide_q15);
    for (volatile int i = 0; i < BLOCK_TESTSZ; i ++) {
        riscv_divide_q15(4203, 2490, &q15_pOUT, &shift);
    }
    BENCH_END(riscv_divide_q15);
    ref_divide_q15(4203, 2490, &q15_pOUT_ref, &shift);
    if (abs(q15_pOUT - q15_pOUT_ref) > DELTAQ15) {
        BENCH_ERROR(riscv_divide_q15);
        printf("expect: %x, actual: %x\n", q15_pOUT_ref, q15_pOUT);
        test_flag_error = 1;
    }
    BENCH_STATUS(riscv_divide_q15);
}

static int DSP_EXP(void)
{
    float32_t f32_pOUT[BLOCK_TESTSZ], f32_pOUT_ref[BLOCK_TESTSZ];
    float32_t f32_pIN[BLOCK_TESTSZ];

    for (int i = 0; i < BLOCK_TESTSZ; i++) {
        f32_pIN[i] = (float32_t) rand();
    }

    // TODO optimize this case not using a fixed value
    BENCH_START(riscv_vexp_f32);
    riscv_vexp_f32(f32_pIN, f32_pOUT, BLOCK_TESTSZ);
    BENCH_END(riscv_vexp_f32);
    ref_vexp_f32(f32_pIN, f32_pOUT_ref, BLOCK_TESTSZ);
    for (int i = 0; i < BLOCK_TESTSZ; i++) {
        if (fabs(f32_pOUT[i] - f32_pOUT_ref[i]) > DELTAF32) {
            BENCH_ERROR(riscv_vexp_f32);
            printf("index: %d, expect: %f, actual: %f\n", i, f32_pOUT_ref[i], f32_pOUT[i]);
            test_flag_error = 1;
        }
    }
    BENCH_STATUS(riscv_vexp_f32);
}

static int DSP_LOG(void)
{
    float32_t f32_pOUT[BLOCK_TESTSZ], f32_pOUT_ref[BLOCK_TESTSZ];
    float32_t f32_pIN[BLOCK_TESTSZ];

    for (int i = 0; i < BLOCK_TESTSZ; i++) {
        f32_pIN[i] = (float32_t) rand();
    }

    // TODO optimize this case not using a fixed value
    BENCH_START(riscv_vlog_f32);
    riscv_vlog_f32(f32_pIN, f32_pOUT, BLOCK_TESTSZ);
    BENCH_END(riscv_vlog_f32);
    ref_vlog_f32(f32_pIN, f32_pOUT_ref, BLOCK_TESTSZ);
    for (int i = 0; i < BLOCK_TESTSZ; i++)
        if (fabs(f32_pOUT[i] - f32_pOUT_ref[i]) > DELTAF32) {
            BENCH_ERROR(riscv_vlog_f32);
            printf("index: %d, expect: %f, actual: %f\n", i, f32_pOUT_ref[i], f32_pOUT[i]);
            test_flag_error = 1;
        }
    BENCH_STATUS(riscv_vlog_f32);
}

int main()
{
    BENCH_INIT();

    srand(__RV_CSR_READ(mcycle));

    DSP_SQRT();
    DSP_DIVIDE();
    DSP_EXP();
    DSP_LOG();

    if (test_flag_error) {
        printf("test error apprears, please recheck.\n");
        return 1;
    } else {
        printf("all test are passed. Well done!\n");
    }
    return 0;
}
