// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifdef PPLNN_USE_ARMV8_2_FP16

#include "ppl/kernel/arm_server/maxpool/neon/maxpool.h"

#include <arm_neon.h>

#include "ppl/kernel/arm_server/common/math.h"
#include "ppl/kernel/arm_server/common/internal_include.h"

#define CVL() 8

namespace ppl { namespace kernel { namespace arm_server { namespace neon {

static void maxpool2d_n8cx_general_fp16(
    const __fp16 *input,
    __fp16 *output,
    const int64_t src_h,
    const int64_t src_w,
    const int64_t dst_h,
    const int64_t dst_w,
    const int64_t num_channel,
    const int64_t num_batch,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t strd_h,
    const int64_t strd_w,
    const int64_t pad_h,
    const int64_t pad_w,
    const int64_t dltn_h,
    const int64_t dltn_w)
{
    PRAGMA_OMP_PARALLEL()
    {
        const int64_t num_channel_ceil8 = CEIL8(num_channel);
        const float16x8_t vmin          = vdupq_n_f16(-numeric_max<__fp16>());
        for (int64_t n = 0; n < num_batch; n++) {
            const __fp16 *input_b_base = input + n * num_channel_ceil8 * src_h * src_w;
            __fp16 *output_b_base      = output + n * num_channel_ceil8 * dst_h * dst_w;
            PRAGMA_OMP_FOR_NOWAIT()
            for (int64_t c = 0; c < num_channel_ceil8; c += CVL()) {
                const __fp16 *input_c_base = input_b_base + c * src_h * src_w;
                __fp16 *output_c_base      = output_b_base + c * dst_h * dst_w;
                for (int64_t oh = 0; oh < dst_h; oh++) {
                    const int64_t ih_base      = -pad_h + oh * strd_h;
                    const __fp16 *input_h_base = input_c_base + ih_base * src_w * CVL();
                    const int64_t flt_h_start   = std::max(-ih_base + dltn_h - 1, (int64_t)0) / dltn_h; // inclusive
                    const int64_t flt_h_end     = std::min(flt_h, (src_h - ih_base + dltn_h - 1) / dltn_h); // exclusive

                    for (int64_t ow = 0; ow < dst_w; ow++) {
                        int64_t iw_base            = -pad_w + ow * strd_w;
                        const __fp16 *input_w_base = input_h_base + iw_base * CVL();
                        int64_t flt_w_start         = std::max(-iw_base + dltn_w - 1, (int64_t)0) / dltn_w; // inclusive
                        int64_t flt_w_end           = std::min(flt_w, (src_w - iw_base + dltn_w - 1) / dltn_w); // exclusive

                        float16x8_t vout = vmin;

                        for (int64_t kh = flt_h_start; kh < flt_h_end; kh++) {
                            for (int64_t kw = flt_w_start; kw < flt_w_end; kw++) {
                                float16x8_t vin = vld1q_f16(input_w_base + (kh * dltn_h * src_w + kw * dltn_w) * CVL());
                                vout            = vmaxq_f16(vin, vout);
                            }
                        }

                        vst1q_f16(output_c_base + oh * dst_w * CVL() + ow * CVL(), vout);
                    }
                }
            }
        }
    }
}

static void maxpool2d_n8cx_global_fp16(
    const __fp16 *input,
    __fp16 *output,
    const int64_t src_h,
    const int64_t src_w,
    const int64_t num_channel,
    const int64_t num_batch)
{
    PRAGMA_OMP_PARALLEL()
    {
        const int64_t num_channel_ceil8 = CEIL8(num_channel);
        const float16x8_t vmin          = vdupq_n_f16(-numeric_max<__fp16>());
        for (int64_t n = 0; n < num_batch; n++) {
            const __fp16 *input_b_base = input + n * num_channel_ceil8 * src_h * src_w;
            __fp16 *output_b_base      = output + n * num_channel_ceil8;
            PRAGMA_OMP_FOR_NOWAIT()
            for (int64_t c = 0; c < num_channel_ceil8; c += CVL()) {
                const __fp16 *input_c_base = input_b_base + c * src_h * src_w;

                float16x8_t vout = vmin;
                for (int64_t idx = 0; idx < src_h * src_w; idx++) {
                    float16x8_t vin = vld1q_f16(input_c_base + idx * CVL());
                    vout            = vmaxq_f16(vin, vout);
                }

                vst1q_f16(output_b_base + c, vout);
            }
        }
    }
}

static void maxpool2d_n8cx_f2s2p0_fp16(
    const __fp16 *input,
    __fp16 *output,
    const int64_t src_h,
    const int64_t src_w,
    const int64_t dst_h,
    const int64_t dst_w,
    const int64_t num_channel,
    const int64_t num_batch)
{
    PRAGMA_OMP_PARALLEL()
    {
        const int64_t num_channel_ceil8 = CEIL8(num_channel);
        for (int64_t n = 0; n < num_batch; n++) {
            PRAGMA_OMP_FOR_NOWAIT()
            for (int64_t c = 0; c < num_channel; c += CVL()) {
                const __fp16 *input_c_base = input + (n * num_channel_ceil8 + c) * src_h * src_w;
                __fp16 *output_c_base      = output + (n * num_channel_ceil8 + c) * dst_h * dst_w;
                for (int64_t oh = 0; oh < dst_h; oh++) {
                    const __fp16 *input_h_base = input_c_base + oh * 2 * src_w * CVL();
                    __fp16 *output_h_base      = output_c_base + oh * dst_w * CVL();

                    int64_t ow = 0;
                    for (; ow <= dst_w - 4; ow += 4) {
                        const __fp16 *input_base0 = input_h_base + ow * 2 * CVL();
                        const __fp16 *input_base1 = input_h_base + src_w * CVL() + ow * 2 * CVL();
                        __fp16 *output_base       = output_h_base + ow * CVL();
                        float16x8_t vin0[8];
                        float16x8_t vin1[8];
                        float16x8_t vout[4];

                        vin0[0] = vld1q_f16(input_base0);
                        vin0[1] = vld1q_f16(input_base0 + CVL());
                        vin0[2] = vld1q_f16(input_base0 + CVL() * 2);
                        vin0[3] = vld1q_f16(input_base0 + CVL() * 3);
                        vin0[4] = vld1q_f16(input_base0 + CVL() * 4);
                        vin0[5] = vld1q_f16(input_base0 + CVL() * 5);
                        vin0[6] = vld1q_f16(input_base0 + CVL() * 6);
                        vin0[7] = vld1q_f16(input_base0 + CVL() * 7);

                        vin1[0] = vld1q_f16(input_base1);
                        vin1[1] = vld1q_f16(input_base1 + CVL());
                        vin1[2] = vld1q_f16(input_base1 + CVL() * 2);
                        vin1[3] = vld1q_f16(input_base1 + CVL() * 3);
                        vin1[4] = vld1q_f16(input_base1 + CVL() * 4);
                        vin1[5] = vld1q_f16(input_base1 + CVL() * 5);
                        vin1[6] = vld1q_f16(input_base1 + CVL() * 6);
                        vin1[7] = vld1q_f16(input_base1 + CVL() * 7);

                        vout[0] = vmaxq_f16(vin0[0], vin0[1]);
                        vout[1] = vmaxq_f16(vin0[2], vin0[3]);
                        vout[2] = vmaxq_f16(vin0[4], vin0[5]);
                        vout[3] = vmaxq_f16(vin0[6], vin0[7]);

                        vout[0] = vmaxq_f16(vout[0], vin1[0]);
                        vout[1] = vmaxq_f16(vout[1], vin1[2]);
                        vout[2] = vmaxq_f16(vout[2], vin1[4]);
                        vout[3] = vmaxq_f16(vout[3], vin1[6]);

                        vout[0] = vmaxq_f16(vout[0], vin1[1]);
                        vout[1] = vmaxq_f16(vout[1], vin1[3]);
                        vout[2] = vmaxq_f16(vout[2], vin1[5]);
                        vout[3] = vmaxq_f16(vout[3], vin1[7]);

                        vst1q_f16(output_base, vout[0]);
                        vst1q_f16(output_base + CVL(), vout[1]);
                        vst1q_f16(output_base + CVL() * 2, vout[2]);
                        vst1q_f16(output_base + CVL() * 3, vout[3]);
                    }
                    for (; ow < dst_w; ow++) {
                        const __fp16 *input_base = input_h_base + ow * 2 * CVL();
                        float16x8_t vin[4];
                        float16x8_t vout;

                        vin[0] = vld1q_f16(input_base);
                        vin[1] = vld1q_f16(input_base + CVL());
                        vin[2] = vld1q_f16(input_base + src_w * CVL());
                        vin[3] = vld1q_f16(input_base + src_w * CVL() + CVL());

                        vout = vmaxq_f16(vin[0], vin[1]);
                        vout = vmaxq_f16(vout, vin[2]);
                        vout = vmaxq_f16(vout, vin[3]);

                        vst1q_f16(output_h_base + ow * CVL(), vout);
                    }
                }
            }
        }
    }
}

static void maxpool2d_n8cx_f3s2_fp16(
    const __fp16 *input,
    __fp16 *output,
    const int64_t src_h,
    const int64_t src_w,
    const int64_t dst_h,
    const int64_t dst_w,
    const int64_t num_channel,
    const int64_t num_batch,
    const int64_t pad_h,
    const int64_t pad_w)
{
    PRAGMA_OMP_PARALLEL()
    {
        const int64_t num_channel_pck = CEIL8(num_channel);
        int64_t ow_no_padding_start   = (pad_w + 1) / 2;
        int64_t ow_no_padding_end     = (src_w - 3 + pad_w) / 2 + 1;
        ow_no_padding_end             = std::max(ow_no_padding_end, ow_no_padding_start);
        ow_no_padding_end             = ow_no_padding_start + ((ow_no_padding_end - ow_no_padding_start) & (~7));

        const float16x8_t vmin = vdupq_n_f16(-numeric_max<__fp16>());
        for (int64_t n = 0; n < num_batch; n++) {
            PRAGMA_OMP_FOR_NOWAIT()
            for (int64_t c = 0; c < num_channel; c += CVL()) {
                const __fp16 *input_c_base = input + (n * num_channel_pck + c) * src_h * src_w;
                __fp16 *output_c_base      = output + (n * num_channel_pck + c) * dst_h * dst_w;
                for (int64_t oh = 0; oh < dst_h; oh++) {
                    __fp16 *output_h_base = output_c_base + oh * dst_w * CVL();
                    int64_t ih_start      = oh * 2 - pad_h;
                    int64_t ih_end        = std::min(ih_start + 3, src_h);
                    ih_start              = std::max(ih_start, (int64_t)0);

                    if (ih_end - ih_start != 3) {
                        for (int64_t ow = 0; ow < dst_w; ow++) {
                            int64_t iw_start = ow * 2 - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(iw_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                    } else {
                        const __fp16 *input_h_base = input_c_base + ih_start * src_w * CVL();

                        int64_t ow = 0;
                        for (; ow < ow_no_padding_start; ow++) {
                            int64_t iw_start = ow * 2 - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(iw_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                        for (; ow < ow_no_padding_end; ow += 4) {
                            int64_t iw_start = ow * 2 - pad_w;

                            const __fp16 *input_base0 = input_h_base + iw_start * CVL();

                            const __fp16 *input_base1 = input_base0 + src_w * CVL();
                            const __fp16 *input_base2 = input_base0 + src_w * CVL() * 2;

                            __fp16 *output_base = output_h_base + ow * CVL();

                            float16x8_t vin[27];
                            vin[0] = vld1q_f16(input_base0);
                            vin[1] = vld1q_f16(input_base0 + CVL());
                            vin[2] = vld1q_f16(input_base0 + CVL() * 2);
                            vin[3] = vld1q_f16(input_base0 + CVL() * 3);
                            vin[4] = vld1q_f16(input_base0 + CVL() * 4);
                            vin[5] = vld1q_f16(input_base0 + CVL() * 5);
                            vin[6] = vld1q_f16(input_base0 + CVL() * 6);
                            vin[7] = vld1q_f16(input_base0 + CVL() * 7);
                            vin[8] = vld1q_f16(input_base0 + CVL() * 8);

                            vin[9]  = vld1q_f16(input_base1);
                            vin[10] = vld1q_f16(input_base1 + CVL());
                            vin[11] = vld1q_f16(input_base1 + CVL() * 2);
                            vin[12] = vld1q_f16(input_base1 + CVL() * 3);
                            vin[13] = vld1q_f16(input_base1 + CVL() * 4);
                            vin[14] = vld1q_f16(input_base1 + CVL() * 5);
                            vin[15] = vld1q_f16(input_base1 + CVL() * 6);
                            vin[16] = vld1q_f16(input_base1 + CVL() * 7);
                            vin[17] = vld1q_f16(input_base1 + CVL() * 8);

                            vin[18] = vld1q_f16(input_base2);
                            vin[19] = vld1q_f16(input_base2 + CVL());
                            vin[20] = vld1q_f16(input_base2 + CVL() * 2);
                            vin[21] = vld1q_f16(input_base2 + CVL() * 3);
                            vin[22] = vld1q_f16(input_base2 + CVL() * 4);
                            vin[23] = vld1q_f16(input_base2 + CVL() * 5);
                            vin[24] = vld1q_f16(input_base2 + CVL() * 6);
                            vin[25] = vld1q_f16(input_base2 + CVL() * 7);
                            vin[26] = vld1q_f16(input_base2 + CVL() * 8);

                            float16x8_t vout[4];
                            vout[0] = vmaxq_f16(vin[0], vin[1]);
                            vout[1] = vmaxq_f16(vin[2], vin[3]);
                            vout[2] = vmaxq_f16(vin[4], vin[5]);
                            vout[3] = vmaxq_f16(vin[6], vin[7]);

                            vout[0] = vmaxq_f16(vout[0], vin[2]);
                            vout[1] = vmaxq_f16(vout[1], vin[4]);
                            vout[2] = vmaxq_f16(vout[2], vin[6]);
                            vout[3] = vmaxq_f16(vout[3], vin[8]);

                            vout[0] = vmaxq_f16(vout[0], vin[9]);
                            vout[1] = vmaxq_f16(vout[1], vin[11]);
                            vout[2] = vmaxq_f16(vout[2], vin[13]);
                            vout[3] = vmaxq_f16(vout[3], vin[15]);

                            vout[0] = vmaxq_f16(vout[0], vin[10]);
                            vout[1] = vmaxq_f16(vout[1], vin[12]);
                            vout[2] = vmaxq_f16(vout[2], vin[14]);
                            vout[3] = vmaxq_f16(vout[3], vin[16]);

                            vout[0] = vmaxq_f16(vout[0], vin[11]);
                            vout[1] = vmaxq_f16(vout[1], vin[13]);
                            vout[2] = vmaxq_f16(vout[2], vin[15]);
                            vout[3] = vmaxq_f16(vout[3], vin[17]);

                            vout[0] = vmaxq_f16(vout[0], vin[18]);
                            vout[1] = vmaxq_f16(vout[1], vin[20]);
                            vout[2] = vmaxq_f16(vout[2], vin[22]);
                            vout[3] = vmaxq_f16(vout[3], vin[24]);

                            vout[0] = vmaxq_f16(vout[0], vin[19]);
                            vout[1] = vmaxq_f16(vout[1], vin[21]);
                            vout[2] = vmaxq_f16(vout[2], vin[23]);
                            vout[3] = vmaxq_f16(vout[3], vin[25]);

                            vout[0] = vmaxq_f16(vout[0], vin[20]);
                            vout[1] = vmaxq_f16(vout[1], vin[22]);
                            vout[2] = vmaxq_f16(vout[2], vin[24]);
                            vout[3] = vmaxq_f16(vout[3], vin[26]);

                            vst1q_f16(output_base, vout[0]);
                            vst1q_f16(output_base + CVL(), vout[1]);
                            vst1q_f16(output_base + CVL() * 2, vout[2]);
                            vst1q_f16(output_base + CVL() * 3, vout[3]);
                        }
                        for (; ow < dst_w; ow++) {
                            int64_t iw_start = ow * 2 - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(iw_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                    }
                }
            }
        }
    }
}

static void maxpool2d_n8cx_f3s1_fp16(
    const __fp16 *input,
    __fp16 *output,
    const int64_t src_h,
    const int64_t src_w,
    const int64_t dst_h,
    const int64_t dst_w,
    const int64_t num_channel,
    const int64_t num_batch,
    const int64_t pad_h,
    const int64_t pad_w)
{
    (void)maxpool2d_n8cx_f3s1_fp16;
    PRAGMA_OMP_PARALLEL()
    {
        const int64_t num_channel_ceil8 = CEIL8(num_channel);
        int64_t ow_no_padding_start     = pad_w;
        int64_t ow_no_padding_end       = pad_w + ((src_w - 3) & (~7));

        const float16x8_t vmin = vdupq_n_f16(-numeric_max<__fp16>());
        for (int64_t n = 0; n < num_batch; n++) {
            PRAGMA_OMP_FOR_NOWAIT()
            for (int64_t c = 0; c < num_channel; c += CVL()) {
                const __fp16 *input_c_base = input + (n * num_channel_ceil8 + c) * src_h * src_w;
                __fp16 *output_c_base      = output + (n * num_channel_ceil8 + c) * dst_h * dst_w;
                for (int64_t oh = 0; oh < dst_h; oh++) {
                    __fp16 *output_h_base = output_c_base + oh * dst_w * CVL();
                    int64_t ih_start      = oh - pad_h;
                    int64_t ih_end        = std::min(ih_start + 3, src_h);
                    ih_start              = std::max(ih_start, (int64_t)0);

                    if (ih_end - ih_start != 3) {
                        for (int64_t ow = 0; ow < dst_w; ow++) {
                            int64_t iw_start = ow - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(ih_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                    } else {
                        const __fp16 *input_h_base = input_c_base + ih_start * src_w * CVL();

                        int64_t ow = 0;
                        for (; ow < ow_no_padding_start; ow++) {
                            int64_t iw_start = ow - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(ih_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                        for (; ow < ow_no_padding_end; ow += 4) {
                            int64_t iw_start = ow - pad_h;

                            const __fp16 *input_base0 = input_h_base + iw_start * CVL();

                            const __fp16 *input_base1 = input_base0 + src_w * CVL();
                            const __fp16 *input_base2 = input_base0 + src_w * CVL() * 2;

                            __fp16 *output_base = output_h_base + ow * CVL();

                            float16x8_t vin[18];
                            vin[0] = vld1q_f16(input_base0);
                            vin[1] = vld1q_f16(input_base0 + CVL());
                            vin[2] = vld1q_f16(input_base0 + CVL() * 2);
                            vin[3] = vld1q_f16(input_base0 + CVL() * 3);
                            vin[4] = vld1q_f16(input_base0 + CVL() * 4);
                            vin[5] = vld1q_f16(input_base0 + CVL() * 5);

                            vin[6]  = vld1q_f16(input_base1);
                            vin[7]  = vld1q_f16(input_base1 + CVL());
                            vin[8]  = vld1q_f16(input_base1 + CVL() * 2);
                            vin[9]  = vld1q_f16(input_base1 + CVL() * 3);
                            vin[10] = vld1q_f16(input_base1 + CVL() * 4);
                            vin[11] = vld1q_f16(input_base1 + CVL() * 5);

                            vin[12] = vld1q_f16(input_base2);
                            vin[13] = vld1q_f16(input_base2 + CVL());
                            vin[14] = vld1q_f16(input_base2 + CVL() * 2);
                            vin[15] = vld1q_f16(input_base2 + CVL() * 3);
                            vin[16] = vld1q_f16(input_base2 + CVL() * 4);
                            vin[17] = vld1q_f16(input_base2 + CVL() * 5);

                            float16x8_t vout[4];
                            vout[0] = vmaxq_f16(vin[0], vin[1]);
                            vout[1] = vmaxq_f16(vin[1], vin[2]);
                            vout[2] = vmaxq_f16(vin[2], vin[3]);
                            vout[3] = vmaxq_f16(vin[3], vin[4]);

                            vout[0] = vmaxq_f16(vout[0], vin[2]);
                            vout[1] = vmaxq_f16(vout[1], vin[3]);
                            vout[2] = vmaxq_f16(vout[2], vin[4]);
                            vout[3] = vmaxq_f16(vout[3], vin[5]);

                            vout[0] = vmaxq_f16(vout[0], vin[6]);
                            vout[1] = vmaxq_f16(vout[1], vin[7]);
                            vout[2] = vmaxq_f16(vout[2], vin[8]);
                            vout[3] = vmaxq_f16(vout[3], vin[9]);

                            vout[0] = vmaxq_f16(vout[0], vin[7]);
                            vout[1] = vmaxq_f16(vout[1], vin[8]);
                            vout[2] = vmaxq_f16(vout[2], vin[9]);
                            vout[3] = vmaxq_f16(vout[3], vin[10]);

                            vout[0] = vmaxq_f16(vout[0], vin[8]);
                            vout[1] = vmaxq_f16(vout[1], vin[9]);
                            vout[2] = vmaxq_f16(vout[2], vin[10]);
                            vout[3] = vmaxq_f16(vout[3], vin[11]);

                            vout[0] = vmaxq_f16(vout[0], vin[12]);
                            vout[1] = vmaxq_f16(vout[1], vin[13]);
                            vout[2] = vmaxq_f16(vout[2], vin[14]);
                            vout[3] = vmaxq_f16(vout[3], vin[15]);

                            vout[0] = vmaxq_f16(vout[0], vin[14]);
                            vout[1] = vmaxq_f16(vout[1], vin[15]);
                            vout[2] = vmaxq_f16(vout[2], vin[16]);
                            vout[3] = vmaxq_f16(vout[3], vin[17]);

                            vout[0] = vmaxq_f16(vout[0], vin[15]);
                            vout[1] = vmaxq_f16(vout[1], vin[16]);
                            vout[2] = vmaxq_f16(vout[2], vin[17]);
                            vout[3] = vmaxq_f16(vout[3], vin[18]);

                            vst1q_f16(output_base, vout[0]);
                            vst1q_f16(output_base + CVL(), vout[1]);
                            vst1q_f16(output_base + CVL() * 2, vout[2]);
                            vst1q_f16(output_base + CVL() * 3, vout[3]);
                        }
                        for (; ow < dst_w; ow++) {
                            int64_t iw_start = ow - pad_w;
                            int64_t iw_end   = std::min(iw_start + 3, src_w);
                            iw_start         = std::max(ih_start, (int64_t)0);

                            float16x8_t vout = vmin;
                            for (int ih = ih_start; ih < ih_end; ih++) {
                                const __fp16 *input_h_base = input_c_base + ih * src_w * CVL();
                                for (int iw = iw_start; iw < iw_end; iw++) {
                                    float16x8_t vin = vld1q_f16(input_h_base + iw * CVL());
                                    vout            = vmaxq_f16(vout, vin);
                                }
                            }
                            vst1q_f16(output_h_base + ow * CVL(), vout);
                        }
                    }
                }
            }
        }
    }
}

ppl::common::RetCode maxpool2d_n8cx_fp16(
    const __fp16 *src,
    __fp16 *dst,
    const int32_t src_n,
    const int32_t src_c,
    const int32_t src_h,
    const int32_t src_w,
    const int32_t dst_h,
    const int32_t dst_w,
    const int32_t kernel_h,
    const int32_t kernel_w,
    const int32_t stride_h,
    const int32_t stride_w,
    const int32_t pad_h,
    const int32_t pad_w,
    const int32_t dilation_h,
    const int32_t dilation_w,
    const int32_t global_pooling)
{
    if (global_pooling) {
        maxpool2d_n8cx_global_fp16(src, dst, src_h, src_w, src_c, src_n);
    } else if (kernel_h == 2 && kernel_w == 2 && stride_h == 2 && stride_w == 2 && pad_h == 0 && pad_w == 0 && dilation_h == 1 && dilation_w == 1) {
        maxpool2d_n8cx_f2s2p0_fp16(src, dst, src_h, src_w, dst_h, dst_w, src_c, src_n);
    } else if (kernel_h == 3 && kernel_w == 3 && stride_h == 2 && stride_w == 2 && dilation_h == 1 && dilation_w == 1) {
        maxpool2d_n8cx_f3s2_fp16(src, dst, src_h, src_w, dst_h, dst_w, src_c, src_n, pad_h, pad_w);
    } else {
        maxpool2d_n8cx_general_fp16(src, dst, src_h, src_w, dst_h, dst_w, src_c, src_n, kernel_h, kernel_w, stride_h, stride_w, pad_h, pad_w, dilation_h, dilation_w);
    }
    return ppl::common::RC_SUCCESS;
}

}}}}; // namespace ppl::kernel::arm_server::neon

#endif
