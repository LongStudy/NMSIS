/*
 * Copyright (C) 2010-2022 Arm Limited or its affiliates.
 * Copyright (c) 2022 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      NMSIS NN Library
 * Title:        riscv_elementwise_add_s8
 * Description:  Elementwise add
 *
 * $Date:        3 Februari 2022
 * $Revision:    V.2.6.0
 *
 * Target Processor: RISC-V Cores
 *
 * -------------------------------------------------------------------- */

#include "riscv_nnfunctions.h"
#include "riscv_nnsupportfunctions.h"

/**
 *  @ingroup groupNN
 */

/**
 * @addtogroup BasicMath
 * @{
 */

/*
 * s8 elementwise add
 *
 * Refer header file for details.
 *
 */

/* Note: __SHIFT is expected to be <=0 */

riscv_status riscv_elementwise_add_s8(const int8_t *input_1_vect,
                                  const int8_t *input_2_vect,
                                  const int32_t input_1_offset,
                                  const int32_t input_1_mult,
                                  const int32_t input_1_shift,
                                  const int32_t input_2_offset,
                                  const int32_t input_2_mult,
                                  const int32_t input_2_shift,
                                  const int32_t left_shift,
                                  int8_t *output,
                                  const int32_t out_offset,
                                  const int32_t out_mult,
                                  const int32_t out_shift,
                                  const int32_t out_activation_min,
                                  const int32_t out_activation_max,
                                  const int32_t block_size)
{
    int32_t loop_count;
    int32_t input_1;
    int32_t input_2;
    int32_t sum;

#if defined(RISCV_MATH_VECTOR)
    int32_t blkCnt = block_size & (~RVV_OPT_THRESHOLD);                               /* Loop counter */
    size_t l;
    vint32m8_t input_1_m8;
    vint32m8_t input_2_m8;
    vint32m8_t sum0m8;

    for (; (l = vsetvl_e8m2(blkCnt)) > 0; blkCnt -= l) {
        input_1_m8 = vsll_vx_i32m8(vadd_vx_i32m8(vwadd_vx_i32m8(vwadd_vx_i16m4(vle8_v_i8m2(input_1_vect, l), 0, l), 0, l), input_1_offset, l), left_shift, l);
        input_1_vect += l;
        input_2_m8 = vsll_vx_i32m8(vadd_vx_i32m8(vwadd_vx_i32m8(vwadd_vx_i16m4(vle8_v_i8m2(input_2_vect, l), 0, l), 0, l), input_2_offset, l), left_shift, l);
        input_2_vect += l;
        if (input_1_shift < 0) {
            input_1_m8 = vsra_vx_i32m8(vsmul_vx_i32m8(input_1_m8, input_1_mult, l), -input_1_shift, l);
        } else {
            input_1_m8 = vsll_vx_i32m8(vsmul_vx_i32m8(input_1_m8, input_1_mult, l), input_1_shift, l);
        }
        if (input_2_shift < 0) {
            input_2_m8 = vsra_vx_i32m8(vsmul_vx_i32m8(input_2_m8, input_2_mult, l), -input_2_shift, l);
        } else {
            input_2_m8 = vsll_vx_i32m8(vsmul_vx_i32m8(input_2_m8, input_2_mult, l), input_2_shift, l);
        }
        sum0m8 = vadd_vv_i32m8(input_1_m8, input_2_m8, l);
        if (out_shift < 0) {
            sum0m8 = vadd_vx_i32m8(vsra_vx_i32m8(vsmul_vx_i32m8(sum0m8, out_mult, l), -out_shift, l), out_offset, l);
        } else {
            sum0m8 = vadd_vx_i32m8(vsll_vx_i32m8(vsmul_vx_i32m8(sum0m8, out_mult, l), out_shift, l), out_offset, l);
        }

        sum0m8 = vmax_vx_i32m8(sum0m8, out_activation_min, l);
        sum0m8 = vmin_vx_i32m8(sum0m8, out_activation_max, l);
        vse8_v_i8m2(output, vnsra_wx_i8m2(vnsra_wx_i16m4(sum0m8, 0, l), 0, l), l);
        output += l;
    }
    loop_count = block_size & RVV_OPT_THRESHOLD;
#elif defined(RISCV_MATH_DSP)
    int32_t a_1, b_1, a_2, b_2;

    int32_t offset_1_packed, offset_2_packed;

    int8_t r1, r2, r3, r4;

    offset_1_packed = (input_1_offset << 16U) | (input_1_offset & 0x0FFFFL);
    offset_2_packed = (input_2_offset << 16U) | (input_2_offset & 0x0FFFFL);

    loop_count = block_size >> 2;

    while (loop_count > 0)
    {
        /* 4 outputs are calculated in one loop. The order of calculation is follows the order of output sign extension
           intrinsic */
        input_1_vect = read_and_pad_reordered(input_1_vect, &b_1, &a_1);
        input_2_vect = read_and_pad_reordered(input_2_vect, &b_2, &a_2);

        a_1 = __SADD16(a_1, offset_1_packed);
        b_1 = __SADD16(b_1, offset_1_packed);

        a_2 = __SADD16(a_2, offset_2_packed);
        b_2 = __SADD16(b_2, offset_2_packed);

        /* Sum 1 */
        input_1 = (b_1 & 0x0FFFF) << left_shift;

        input_1 = riscv_nn_requantize(input_1, input_1_mult, input_1_shift);

        input_2 = (b_2 & 0x0FFFF) << left_shift;
        input_2 = riscv_nn_requantize(input_2, input_2_mult, input_2_shift);

        sum = input_1 + input_2;
        sum = riscv_nn_requantize(sum, out_mult, out_shift);
        sum += out_offset;
        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);
        r1 = (q7_t)sum;

        /* Sum 3 */
        input_1 = ((b_1 >> 16) & 0x0FFFF) << left_shift;
        input_1 = riscv_nn_requantize(input_1, input_1_mult, input_1_shift);

        input_2 = ((b_2 >> 16) & 0x0FFFF) << left_shift;
        input_2 = riscv_nn_requantize(input_2, input_2_mult, input_2_shift);

        sum = input_1 + input_2;
        sum = riscv_nn_requantize(sum, out_mult, out_shift);
        sum += out_offset;
        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);
        r3 = (q7_t)sum;

        /* Sum 2 */
        input_1 = (a_1 & 0x0FFFF) << left_shift;
        input_1 = riscv_nn_requantize(input_1, input_1_mult, input_1_shift);

        input_2 = (a_2 & 0x0FFFF) << left_shift;
        input_2 = riscv_nn_requantize(input_2, input_2_mult, input_2_shift);

        sum = input_1 + input_2;
        sum = riscv_nn_requantize(sum, out_mult, out_shift);
        sum += out_offset;
        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);
        r2 = (q7_t)sum;

        /* Sum 4 */
        input_1 = ((a_1 >> 16) & 0x0FFFF) << left_shift;
        input_1 = riscv_nn_requantize(input_1, input_1_mult, input_1_shift);

        input_2 = ((a_2 >> 16) & 0x0FFFF) << left_shift;
        input_2 = riscv_nn_requantize(input_2, input_2_mult, input_2_shift);

        sum = input_1 + input_2;
        sum = riscv_nn_requantize(sum, out_mult, out_shift);
        sum += out_offset;
        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);
        r4 = (q7_t)sum;

        riscv_nn_write_q7x4_ia(&output, PACK_Q7x4_32x1(r1, r2, r3, r4));

        loop_count--;
    }

    loop_count = block_size & 0x3;
#else
    loop_count = block_size;
#endif /* defined(RISCV_MATH_VECTOR) */

    while (loop_count > 0)
    {
        /* C = A + B */

        input_1 = (*input_1_vect++ + input_1_offset) << left_shift;
        input_2 = (*input_2_vect++ + input_2_offset) << left_shift;

        input_1 = riscv_nn_requantize(input_1, input_1_mult, input_1_shift);
        input_2 = riscv_nn_requantize(input_2, input_2_mult, input_2_shift);

        sum = input_1 + input_2;
        sum = riscv_nn_requantize(sum, out_mult, out_shift);
        sum += out_offset;

        sum = MAX(sum, out_activation_min);
        sum = MIN(sum, out_activation_max);

        *output++ = (q7_t)sum;

        /* Decrement loop counter */
        loop_count--;
    }


    return (RISCV_MATH_SUCCESS);
}

/**
 * @} end of BasicMath group
 */
