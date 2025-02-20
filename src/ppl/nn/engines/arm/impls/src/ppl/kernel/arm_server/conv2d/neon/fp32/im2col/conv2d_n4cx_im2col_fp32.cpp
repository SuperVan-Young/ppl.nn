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

#include <arm_neon.h>
#include <chrono>

#include "ppl/kernel/arm_server/common/internal_include.h"
#include "ppl/kernel/arm_server/conv2d/neon/fp32/im2col/conv2d_n4cx_im2col_fp32.h"
#include "ppl/kernel/arm_server/conv2d/neon/fp32/n4cx_sgemm/n4cx_sgemm.h"
#include "ppl/kernel/arm_server/conv2d/neon/fp32/utils/conv2d_utils_fp32.h"

namespace ppl { namespace kernel { namespace arm_server { namespace neon {

#define CBLK()  4
#define ICBLK() CBLK()
#define OCBLK() CBLK()

#if CBLK() == 4
#define PACK_CHANNEL(c) (((c) + 3) & (~3))
#elif CBLK() == 8
#define PACK_CHANNEL(c) (((c) + 7) & (~7))
#else
#error
#endif

static inline void prefetch_l1(const void *ptr, size_t offset)
{
#if __aarch64__
    asm volatile(
        "prfm pldl1keep, [%0, %1]\n\t"
        :
        : "r"(ptr), "r"(offset)
        : "cc", "memory");
#else
    asm volatile(
        "pld [%0, %1]\n\t"
        :
        : "r"(ptr), "r"(offset)
        : "cc", "memory");
#endif
}

static uint64_t conv_n4cx_tile_im2col_get_tmp_buffer_size(
    const int64_t batch_block3,
    const int64_t group_block3,
    const int64_t hw_block2,
    const int64_t src_hw,
    const int64_t dst_hw,
    const int64_t flt_hw,
    const int64_t ic_g_pck,
    const int64_t oc_g_pck)
{
    const uint64_t input_gbuf_offset  = batch_block3 * group_block3 * ic_g_pck * src_hw;
    const uint64_t output_gbuf_offset = batch_block3 * group_block3 * oc_g_pck * dst_hw;
    const int64_t im2col_gbuf_offset  = hw_block2 * ic_g_pck * flt_hw * PPL_OMP_MAX_THREADS();
    return (input_gbuf_offset + output_gbuf_offset + im2col_gbuf_offset) * sizeof(float) + 128;
}

void conv2d_n4cx_im2col_fp32_runtime_executor::conv_n4cx_tile_im2col_kernel(
    const float *cvt_filter_oc_base,
    const float *bias_oc_base,
    const float *input_g_base,
    const float *fuse_data_row_base,
    float *input_im2col_buffer,
    float *sgemm_output_oc_hw_base,
    const int64_t oc_l2_size,
    const int64_t hw_l2_size,
    const int64_t oc_l2_base,
    const int64_t hw_l2_base,
    const uint32_t fuse_type,
    const bool renew_tile_im2col)
{
    const conv2d_param &cp                           = *conv_param_;
    const conv2d_n4cx_im2col_fp32_schedule_param &sp = sched_param_;
    const conv2d_n4cx_im2col_fp32_kernel_param &kp   = ker_param_;

    const int64_t ic_group = cp.channels / cp.group;
    const int64_t ic_g_pck = PACK_CHANNEL(ic_group);

    const int64_t src_h  = src_shape_->GetDim(2);
    const int64_t src_w  = src_shape_->GetDim(3);
    const int64_t dst_h  = dst_shape_->GetDim(2);
    const int64_t dst_w  = dst_shape_->GetDim(3);
    const int64_t flt_h  = cp.kernel_h;
    const int64_t flt_w  = cp.kernel_w;
    const int64_t pad_h  = cp.pad_h;
    const int64_t pad_w  = cp.pad_w;
    const int64_t strd_h = cp.stride_h;
    const int64_t strd_w = cp.stride_w;
    const int64_t dltn_h = cp.dilation_h;
    const int64_t dltn_w = cp.dilation_w;

    const int64_t m_block1 = sp.sgemm_m_block1;
    const int64_t n_block1 = sp.sgemm_n_block1;
    const int64_t k_block1 = sp.sgemm_k_block1;
    const bool use_im2col  = sp.use_im2col;

    const int64_t src_hw  = src_h * src_w;
    const int64_t dst_hw  = dst_h * dst_w;
    const int64_t flt_hw  = flt_h * flt_w;

    const int64_t k_m8_block0  = kp.k_m8_block0;
    const int64_t k_n10_block0 = kp.k_n10_block0;
    const int64_t k_m4_block0  = kp.k_m4_block0;
    const int64_t k_n12_block0 = kp.k_n12_block0;

    int64_t prv_j2 = -1;

    int64_t input_h_idx[n_block1];
    int64_t input_w_idx[n_block1];
    int64_t src_ofs[n_block1];

    const int64_t m = oc_l2_size;
    const int64_t n = hw_l2_size;
    const int64_t k = ic_g_pck * flt_hw;

    const int64_t lda           = k;
    const int64_t ldc           = dst_hw;
    const int64_t ld_fused_data = ldc;

    const int64_t src_w_cblk = src_w * ICBLK();

    for (int64_t j2 = 0; j2 < n; j2 += n_block1) {
        for (int64_t i2 = 0; i2 < m; i2 += m_block1) {
            const int64_t m_l1 = std::min(m - i2, m_block1);
            const int64_t n_l1 = std::min(n - j2, n_block1);

            float *i2c_local_base = input_im2col_buffer + j2 * ic_g_pck * flt_hw;
            if (renew_tile_im2col && prv_j2 != j2) {
                for (int64_t j = 0; j < n_l1; j++) {
                    input_h_idx[j] = ((hw_l2_base + j2 + j) / dst_w) * strd_h - pad_h;
                    input_w_idx[j] = ((hw_l2_base + j2 + j) % dst_w) * strd_w - pad_w;
                }

                const int kw_stride      = n_block1 * ICBLK();
                const int kh_stride      = flt_w * kw_stride;
                const float32x4_t vzeros = vdupq_n_f32(0.0f);

                for (int64_t kh = 0; kh < flt_h; kh++) {
                    const int64_t kdltn_h  = kh * dltn_h;
                    float *i2c_buf_kh_base = i2c_local_base + kh * kh_stride;
                    for (int64_t kw = 0; kw < flt_w; kw++) {
                        const int64_t kdltn_w  = kw * dltn_w;
                        float *i2c_buf_kw_base = i2c_buf_kh_base + kw * kw_stride;
                        for (int64_t j = 0; j < n_l1; j++) {
                            int in_hid = input_h_idx[j] + kdltn_h;
                            int in_wid = input_w_idx[j] + kdltn_w;
                            if (in_hid < 0 || in_hid >= src_h ||
                                in_wid < 0 || in_wid >= src_w) {
                                src_ofs[j] = -1;
                            } else {
                                src_ofs[j] = in_hid * src_w_cblk + in_wid * ICBLK();
                            }
                        }

                        for (int ic = 0; ic < ic_group; ic += ICBLK()) {
                            const float *in_c_base = input_g_base + ic * src_hw;
                            float *i2c_buf_ptr     = i2c_buf_kw_base + ic * flt_hw * n_block1;
                            for (int j = 0; j < n_l1; j++) {
                                if (src_ofs[j] == -1) {
                                    vst1q_f32(i2c_buf_ptr + j * ICBLK(), vzeros);
                                } else {
                                    vst1q_f32(i2c_buf_ptr + j * ICBLK(), vld1q_f32(in_c_base + src_ofs[j]));
                                }
                                prefetch_l1(in_c_base + src_ofs[j], src_w_cblk * dltn_h);
                            }
                        }
                    }
                }
                prv_j2 = j2;
            }

            const int64_t m_l1_align8 = FLOOR8(CEIL4(m_l1));

            for (int64_t p2 = 0; p2 < k; p2 += k_block1) {
                const bool is_first_k = (p2 == 0);
                const bool is_last_k  = (p2 + k_block1 >= k);
                const int64_t k_l1    = std::min(k - p2, k_block1);

                const float *a_ptr      = cvt_filter_oc_base + i2 * lda + p2 * OCBLK() * 2;
                const float *b_ptr      = (use_im2col) ? (i2c_local_base + p2 * n_block1) : (input_g_base + p2 * src_hw + (hw_l2_base + j2) * CBLK());
                const int64_t ldb_local = (use_im2col) ? n_block1 : src_hw;
                const float *const_ptr  = bias_oc_base + i2;
                const float *fused_ptr  = fuse_data_row_base + i2 * ld_fused_data + j2 * CBLK();
                float *c_ptr            = sgemm_output_oc_hw_base + i2 * ldc + j2 * CBLK();

                uint32_t init_id = (is_first_k) ? ((bias_oc_base) ? 1 : 0) : 2;
                uint32_t fuse_id = (is_last_k) ? fuse_type : 0;

                for (int64_t i = 0; i < m_l1_align8; i += k_m8_block0) {
                    for (int64_t j = 0; j < n_l1; j += k_n10_block0) {
                        const int64_t m_l0 = std::min(m_l1_align8 - i, k_m8_block0);
                        const int64_t n_l0 = std::min(n_l1 - j, k_n10_block0);

                        sgemm_n4cx_kernel_m8nx_fp32_func_table[n_l0 - 1][init_id][fuse_id](
                            a_ptr + i * lda,
                            b_ptr + j * CBLK(),
                            const_ptr + i,
                            fused_ptr + i * ld_fused_data + j * CBLK(),
                            c_ptr + i * ldc + j * CBLK(),
                            m_l0,
                            n_l0,
                            k_l1,
                            lda,
                            ldb_local,
                            ld_fused_data,
                            ldc);
                    }
                }
                if (m_l1_align8 < CEIL4(m_l1)) {
                    a_ptr = cvt_filter_oc_base + i2 * lda + p2 * OCBLK();
                    for (int64_t i = m_l1_align8; i < CEIL4(m_l1); i += k_m4_block0) {
                        for (int64_t j = 0; j < n_l1; j += k_n12_block0) {
                            const int64_t m_l0 = std::min(CEIL4(m_l1) - i, k_m4_block0);
                            const int64_t n_l0 = std::min(n_l1 - j, k_n12_block0);

                            sgemm_n4cx_kernel_m4nx_fp32_func_table[n_l0 - 1][init_id][fuse_id](
                                a_ptr + i * lda,
                                b_ptr + j * CBLK(),
                                const_ptr + i,
                                fused_ptr + i * ld_fused_data + j * CBLK(),
                                c_ptr + i * ldc + j * CBLK(),
                                m_l0,
                                n_l0,
                                k_l1,
                                lda,
                                ldb_local,
                                ld_fused_data,
                                ldc);
                        }
                    }
                }
            }
        }
    }
}

uint64_t conv2d_n4cx_im2col_fp32_runtime_executor::cal_temp_buffer_size()
{
    const conv2d_param &cp                     = *conv_param_;
    conv2d_n4cx_im2col_fp32_schedule_param &sp = sched_param_;

    const int64_t src_h    = src_shape_->GetDim(2);
    const int64_t src_w    = src_shape_->GetDim(3);
    const int64_t dst_h    = dst_shape_->GetDim(2);
    const int64_t dst_w    = dst_shape_->GetDim(3);
    const int64_t ic_g_pck = PACK_CHANNEL(cp.channels / cp.group);
    const int64_t oc_g_pck = PACK_CHANNEL(cp.num_output / cp.group);

    return conv_n4cx_tile_im2col_get_tmp_buffer_size(
        sp.batch_block3, sp.group_block3, sp.hw_block2, src_h * src_w, dst_h * dst_w, cp.kernel_h * cp.kernel_w, ic_g_pck, oc_g_pck);
}

void conv2d_n4cx_im2col_fp32_runtime_executor::adjust_schedule_param()
{
    const conv2d_param &cp                           = *conv_param_;
    const conv2d_n4cx_im2col_fp32_kernel_param &kp   = ker_param_;
    const conv2d_n4cx_im2col_fp32_schedule_param &sp = sched_param_;

    const int64_t num_threads = PPL_OMP_MAX_THREADS();

    const int64_t ic_group = cp.channels / cp.group;
    const int64_t oc_group = cp.num_output / cp.group;
    const int64_t ic_g_pck = PACK_CHANNEL(ic_group);
    const int64_t oc_g_pck = PACK_CHANNEL(oc_group);

    const int64_t num_batch = src_shape_->GetDim(0);
    const int64_t src_hw     = src_shape_->GetDim(2) * src_shape_->GetDim(3);
    const int64_t dst_hw    = dst_shape_->GetDim(2) * dst_shape_->GetDim(3);

    const int64_t k_input_g_stride  = ic_group * src_hw;
    const int64_t k_output_g_stride = oc_group * dst_hw;
    const int64_t kk                = ic_g_pck * cp.kernel_h * cp.kernel_w;

    int64_t l3_cache_size = (kp.target_l3_cache_size * num_threads);
    int64_t bl3           = 1;
    while (bl3 < num_batch && bl3 < num_threads &&
           (bl3 + 1) * (k_input_g_stride + k_output_g_stride) + kk * oc_g_pck < (l3_cache_size * 1.5) / sizeof(float)) {
        ++bl3;
    }
    sched_param_.batch_block3 = bl3;

    int64_t gl3 = 1;
    while (gl3 < cp.group && gl3 * bl3 < num_threads &&
           (gl3 + 1) * bl3 * (k_input_g_stride + k_output_g_stride) + (gl3 + 1) * kk * oc_g_pck < (l3_cache_size * 1.5) / sizeof(float)) {
        ++gl3;
    }
    sched_param_.group_block3 = gl3;
    sched_param_.hw_block2    = sp.sgemm_n_block1;
    sched_param_.oc_block2    = sp.sgemm_m_block1;

    sched_param_.use_im2col = (cp.kernel_h != 1 || cp.kernel_w != 1 ||
                               cp.pad_h != 0 || cp.pad_w != 0 ||
                               cp.stride_h != 1 || cp.stride_w != 1 ||
                               cp.dilation_h != 1 || cp.dilation_w != 1);
    return;
}

ppl::common::RetCode conv2d_n4cx_im2col_fp32_runtime_executor::prepare()
{
    if (!conv_param_ || !src_shape_ || !dst_shape_) {
        return ppl::common::RC_INVALID_VALUE;
    }

    adjust_schedule_param();
    return ppl::common::RC_SUCCESS;
}

ppl::common::RetCode conv2d_n4cx_im2col_fp32_runtime_executor::execute()
{
    PRAGMA_OMP_PARALLEL()
    {
        const conv2d_param &cp                           = *conv_param_;
        const conv2d_n4cx_im2col_fp32_schedule_param &sp = sched_param_;

        const float *converted_filter = (const float *)cvt_filter_;
        const float *bias             = (const float *)cvt_bias_;
        const float *input            = (const float *)src_;
        float *output                 = (float *)dst_;
        float *sum                    = (float *)sum_;
        float *tmp_buffer             = (float *)temp_buffer_;
        const uint32_t fuse_flag      = cp.fuse_flag;

        const int64_t src_h      = src_shape_->GetDim(2);
        const int64_t src_w      = src_shape_->GetDim(3);
        const int64_t channels   = src_shape_->GetDim(1);
        const int64_t num_output = cp.num_output;
        const int64_t dst_h      = dst_shape_->GetDim(2);
        const int64_t dst_w      = dst_shape_->GetDim(3);
        const int64_t flt_h      = cp.kernel_h;
        const int64_t flt_w      = cp.kernel_w;
        const int64_t group      = cp.group;
        const int64_t num_batch  = src_shape_->GetDim(0);

        const int64_t group_block3 = sp.group_block3;
        const int64_t batch_block3 = sp.batch_block3;
        const int64_t hw_block2    = sp.hw_block2;
        const int64_t oc_block2    = sp.oc_block2;

        const int64_t ic_pck = PACK_CHANNEL(channels);
        const int64_t oc_pck = PACK_CHANNEL(num_output);

        const int64_t ic_group = channels / group;
        const int64_t oc_group = num_output / group;
        const int64_t ic_g_pck = PACK_CHANNEL(ic_group);
        const int64_t oc_g_pck = PACK_CHANNEL(oc_group);

        const bool use_in_gbuf  = (group > 1 && ic_group != ic_g_pck);
        const bool use_out_gbuf = (group > 1 && oc_group != oc_g_pck);
        const bool use_im2col   = sp.use_im2col;

        // gemm fuse_type
        uint32_t fuse_type = 0;
        // fuse relu in gemm
        // fuse add + relu in gemm if not groupped
        // fuse add + relu in write-back if groupped
        if (!use_out_gbuf || (fuse_flag & conv_fuse_flag::SUM) == 0) {
            if (fuse_flag & conv_fuse_flag::RELU) {
                fuse_type += 1;
                if (fuse_flag & conv_fuse_flag::RELU6) {
                    fuse_type += 1;
                }
            }
            if (fuse_flag & conv_fuse_flag::SUM) {
                fuse_type += 3;
            }
        }

        const int64_t src_hw  = src_h * src_w;
        const int64_t dst_hw = dst_h * dst_w;

        const int64_t k_input_b_stride  = ic_pck * src_hw;
        const int64_t k_output_b_stride = oc_pck * dst_hw;
        const int64_t k_input_g_stride  = ic_group * src_hw;
        const int64_t k_output_g_stride = oc_group * dst_hw;
        const int64_t kk                = ic_g_pck * flt_h * flt_w;

        const int64_t input_gbuf_offset  = batch_block3 * group_block3 * ic_g_pck * src_hw;
        const int64_t output_gbuf_offset = batch_block3 * group_block3 * oc_g_pck * dst_hw;
        const int64_t im2col_gbuf_offset = hw_block2 * kk;

        float *in_gbuf    = tmp_buffer;
        float *out_gbuf   = in_gbuf + input_gbuf_offset;
        float *im2col_buf = out_gbuf + output_gbuf_offset + PPL_OMP_THREAD_ID() * im2col_gbuf_offset;

        for (int64_t g_l3 = 0; g_l3 < group; g_l3 += group_block3) {
            const int64_t group_l3 = std::min(group_block3, group - g_l3);
            for (int64_t b_l3 = 0; b_l3 < num_batch; b_l3 += batch_block3) {
                const int64_t num_batch_l3   = std::min(batch_block3, num_batch - b_l3);
                int64_t in_b_stride          = k_input_b_stride;
                int64_t in_g_stride          = k_input_g_stride;
                const float *input_bgl3_base = input + b_l3 * k_input_b_stride + g_l3 * k_input_g_stride;
                const float *kernel_input    = input_bgl3_base;
                if (use_in_gbuf) {
                    kernel_input = in_gbuf;
                    in_g_stride  = ic_g_pck * src_hw;
                    in_b_stride  = group_l3 * in_g_stride;

                    for (int64_t g = 0; g < group_l3; g++) {
                        for (int64_t b = 0; b < num_batch_l3; b++) {
                            conv2d_n4cx_load_group_fp32(
                                input + (b + b_l3) * k_input_b_stride,
                                in_gbuf + b * in_b_stride + g * in_g_stride,
                                src_hw,
                                ic_group,
                                g + g_l3,
                                g);
                        }
                    }
                    PRAGMA_OMP_BARRIER();
                }

                int64_t out_b_stride    = k_output_b_stride;
                int64_t out_g_stride    = k_output_g_stride;
                float *kernel_output    = output + b_l3 * k_output_b_stride + g_l3 * k_output_g_stride;
                float *kernel_fuse_data = sum + b_l3 * k_output_b_stride + g_l3 * k_output_g_stride;
                if (use_out_gbuf) {
                    kernel_output    = out_gbuf;
                    kernel_fuse_data = nullptr;
                    out_g_stride     = oc_g_pck * dst_hw;
                    out_b_stride     = group_block3 * out_g_stride;
                }

                int64_t prv_g    = -1;
                int64_t prv_b    = -1;
                int64_t prv_hwl2 = -1;

                PRAGMA_OMP_FOR_COLLAPSE(4)
                for (int64_t g = 0; g < group_l3; g++) {
                    for (int64_t b = 0; b < num_batch_l3; b++) {
                        for (int64_t hw_l2 = 0; hw_l2 < dst_hw; hw_l2 += hw_block2) {
                            for (int64_t oc_l2 = 0; oc_l2 < oc_g_pck; oc_l2 += oc_block2) {
                                const int64_t hw_block2_valid = std::min(hw_block2, dst_hw - hw_l2);
                                const int64_t oc_block2_valid = std::min(oc_block2, oc_g_pck - oc_l2);

                                const int64_t output_offset  = b * out_b_stride + g * out_g_stride + oc_l2 * dst_hw + hw_l2 * OCBLK();
                                const bool renew_tile_im2col = use_im2col && (prv_g != g || prv_b != b || prv_hwl2 != hw_l2);
                                conv_n4cx_tile_im2col_kernel(
                                    converted_filter + (g + g_l3) * oc_g_pck * ic_g_pck * flt_h * flt_w + oc_l2 * ic_g_pck * flt_h * flt_w,
                                    bias + (g + g_l3) * oc_group + oc_l2,
                                    kernel_input + b * in_b_stride + g * in_g_stride,
                                    kernel_fuse_data + output_offset,
                                    im2col_buf,
                                    kernel_output + output_offset,
                                    oc_block2_valid,
                                    hw_block2_valid,
                                    oc_l2,
                                    hw_l2,
                                    fuse_type,
                                    renew_tile_im2col);

                                prv_hwl2 = hw_l2;
                                prv_b    = b;
                                prv_g    = g;
                            }
                        }
                    }
                }

                if (use_out_gbuf) {
                    for (int64_t b = 0; b < num_batch_l3; b++) {
                        for (int64_t g = 0; g < group_l3; g++) {
                            conv2d_n4cx_store_group_fp32(
                                out_gbuf + b * out_b_stride + g * out_g_stride,
                                output + (b + b_l3) * k_output_b_stride,
                                sum + (b + b_l3) * k_output_b_stride,
                                dst_hw,
                                oc_group,
                                g + g_l3,
                                g,
                                fuse_flag);
                        }
                    }
                    PRAGMA_OMP_BARRIER();
                }
            }
        }
    }
    return ppl::common::RC_SUCCESS;
}

size_t conv_n4cx_tile_im2col_get_converted_filter_size(
    const int64_t num_output,
    const int64_t channels,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t group)
{
    const int64_t ic_group       = channels / group;
    const int64_t oc_group       = num_output / group;
    const size_t cvt_filter_size = CEIL128(group * PACK_CHANNEL(oc_group) * PACK_CHANNEL(ic_group) * flt_h * flt_w * sizeof(float));

    return cvt_filter_size;
}

void conv_n4cx_tile_im2col_convert_filter(
    const float *filter,
    float *converted_filter,
    const int64_t num_output,
    const int64_t channels,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t group)
{
    const int64_t ic_group = channels / group;
    const int64_t oc_group = num_output / group;

    const int64_t ic_group_pck = PACK_CHANNEL(ic_group);
    const int64_t oc_group_pck = PACK_CHANNEL(oc_group);

    const int64_t flt_hw = flt_h * flt_w;

    for (int64_t g = 0; g < group; g++) {
        const float *filter_g_base = filter + g * oc_group * ic_group * flt_hw;
        float *cvt_filter_g_base   = converted_filter + g * oc_group_pck * ic_group_pck * flt_hw;

        // NOTE: (num_g * oc_g, ic_g, kh, kw) -> ((num_g * oc_g/8, [ic_g/4, kh, kw, 4ic], 8oc) w/ tailing: (..., 4ic, 4oc)
        for (int64_t oc = 0; oc < oc_group_pck; oc += 2 * OCBLK()) {
            for (int64_t ic = 0; ic < ic_group_pck; ic += ICBLK()) {
                const int64_t ic_valid_blk  = std::min((int64_t)ICBLK(), ic_group - ic);
                const int64_t oc_valid_blk  = std::min((int64_t)2 * OCBLK(), oc_group - oc);
                const int64_t k_ocblk_local = (oc_valid_blk > OCBLK()) ? 2 * OCBLK() : OCBLK();
                float *cvt_filter_c_base    = cvt_filter_g_base + oc * ic_group_pck * flt_hw + ic * flt_hw * k_ocblk_local;

                for (int64_t k = 0; k < flt_hw; k++) {
                    const float *filter_k_base = filter_g_base + oc * ic_group * flt_hw + ic * flt_hw + k;
                    float *cvt_filter_k_base   = cvt_filter_c_base + k * ICBLK() * k_ocblk_local;
                    for (int64_t ic0 = 0; ic0 < ic_valid_blk; ic0++) {
                        for (int64_t oc0 = 0; oc0 < oc_valid_blk; oc0++) {
                            cvt_filter_k_base[ic0 * k_ocblk_local + oc0] = filter_k_base[oc0 * ic_group * flt_hw + ic0 * flt_hw];
                        }
                        for (int64_t oc0 = oc_valid_blk; oc0 < k_ocblk_local; oc0++) {
                            cvt_filter_k_base[ic0 * k_ocblk_local + oc0] = 0.0f;
                        }
                    }
                    for (int64_t ic0 = ic_valid_blk; ic0 < ICBLK(); ic0++) {
                        for (int64_t oc0 = 0; oc0 < k_ocblk_local; oc0++) {
                            cvt_filter_k_base[ic0 * k_ocblk_local + oc0] = 0.0f;
                        }
                    }
                }
            }
        }
    }
}

bool conv2d_n4cx_im2col_fp32_offline_manager::is_supported()
{
    return true;
}

ppl::common::RetCode conv2d_n4cx_im2col_fp32_offline_manager::fast_init_schedule_param()
{
    return ppl::common::RC_SUCCESS;
}

ppl::common::RetCode conv2d_n4cx_im2col_fp32_offline_manager::pick_best_schedule_param(const ppl::nn::TensorShape &src_shape, double &run_time, bool tune_blocksize)
{
    const int64_t num_output = param_.num_output;
    const int64_t channels   = param_.channels;
    const int64_t kernel_h   = param_.kernel_h;
    const int64_t kernel_w   = param_.kernel_w;

    if (src_shape.GetDimCount() < 4) {
        return ppl::common::RC_INVALID_VALUE;
    }
    const int64_t num_batch = src_shape.GetDim(0);
    const int64_t src_h     = src_shape.GetDim(2);
    const int64_t src_w     = src_shape.GetDim(3);
    const int64_t dst_h     = ((src_h + 2 * param_.pad_h - param_.dilation_h * (param_.kernel_h - 1) - 1) / param_.stride_h + 1);
    const int64_t dst_w     = ((src_w + 2 * param_.pad_w - param_.dilation_w * (param_.kernel_w - 1) - 1) / param_.stride_w + 1);
    ppl::nn::TensorShape dst_shape;
    dst_shape.Reshape({num_batch, num_output, dst_h, dst_w});

    uint64_t cvt_filter_size = conv_n4cx_tile_im2col_get_converted_filter_size(
        num_output, channels, kernel_h, kernel_w, param_.group);
    uint64_t cvt_bias_size = CEIL4(num_output) * sizeof(float);
    uint64_t src_size      = num_batch * CEIL4(channels) * src_h * src_w * sizeof(float);
    uint64_t dst_size      = num_batch * CEIL4(num_output) * dst_h * dst_w * sizeof(float);
    float *cvt_filter      = (float *)allocator_->Alloc(cvt_filter_size);
    float *cvt_bias        = (float *)allocator_->Alloc(cvt_bias_size);
    float *src             = (float *)allocator_->Alloc(src_size);
    float *dst             = (float *)allocator_->Alloc(dst_size);

    for (uint64_t idx = 0; idx < cvt_filter_size / sizeof(float); idx++) {
        cvt_filter[idx] = float(rand()) / float((RAND_MAX)) - 0.5;
    }
    for (uint64_t idx = 0; idx < cvt_bias_size / sizeof(float); idx++) {
        cvt_bias[idx] = float(rand()) / float((RAND_MAX)) - 0.5;
    }
    for (uint64_t idx = 0; idx < src_size / sizeof(float); idx++) {
        src[idx] = float(rand()) / float((RAND_MAX)) - 0.5;
    }
    for (uint64_t idx = 0; idx < dst_size / sizeof(float); idx++) {
        dst[idx] = float(rand()) / float((RAND_MAX)) - 0.5;
    }

    std::vector<int64_t> candidate_m_blk_list = {40};
    std::vector<int64_t> candidate_n_blk_list = {68};
    std::vector<int64_t> candidate_k_blk_list = {128};

    if (tune_blocksize) {
        candidate_m_blk_list = {32, 40, 48, 64};
        candidate_n_blk_list = {48, 68, 72, 84, 96};
        candidate_k_blk_list = {64, 128, 192};
    }

    int64_t best_m_blk    = 40;
    int64_t best_n_blk    = 68;
    int64_t best_k_blk    = 128;
    int64_t best_run_time = std::numeric_limits<int64_t>::max();

    const int num_warmup_iter    = 1;
    const int num_benchmark_iter = 5;
    for (auto m_blk : candidate_m_blk_list) {
        for (auto n_blk : candidate_n_blk_list) {
            for (auto k_blk : candidate_k_blk_list) {
                sched_param_.sgemm_m_block1 = m_blk;
                sched_param_.sgemm_n_block1 = n_blk;
                sched_param_.sgemm_k_block1 = k_blk;

                auto conv_exe = gen_executor();
                conv_exe->set_cvt_filter(cvt_filter);
                conv_exe->set_cvt_bias(cvt_bias);
                conv_exe->set_src(src);
                conv_exe->set_src_shape(&src_shape);
                conv_exe->set_dst(dst);
                conv_exe->set_dst_shape(&dst_shape);
                conv_exe->prepare();
                uint64_t tmp_buf_size = conv_exe->cal_temp_buffer_size();
                float *tmp_buffer     = (float *)allocator_->Alloc(tmp_buf_size);
                conv_exe->set_temp_buffer(tmp_buffer);

                for (int i = 0; i < num_warmup_iter; i++) {
                    conv_exe->execute();
                }

                auto begin_ts = std::chrono::system_clock::now();
                for (int i = 0; i < num_benchmark_iter; i++) {
                    conv_exe->execute();
                }
                auto end_ts = std::chrono::system_clock::now();

                int64_t elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(end_ts - begin_ts).count();
                if (elapsed_time < best_run_time) {
                    best_m_blk    = m_blk;
                    best_n_blk    = n_blk;
                    best_k_blk    = k_blk;
                    best_run_time = elapsed_time;
                }

                allocator_->Free(tmp_buffer);
                delete conv_exe;
                if (k_blk >= channels / param_.group) break;
            }

            if (n_blk >= dst_h * dst_w) break;
        }
        if (m_blk >= num_output / param_.group) break;
    }

    cvt_filter_ = nullptr;
    cvt_bias_   = nullptr;
    allocator_->Free(cvt_filter);
    allocator_->Free(cvt_bias);
    allocator_->Free(src);
    allocator_->Free(dst);

    sched_param_.sgemm_m_block1 = best_m_blk;
    sched_param_.sgemm_n_block1 = best_n_blk;
    sched_param_.sgemm_k_block1 = best_k_blk;
#ifdef PPLNN_ENABLE_KERNEL_PROFILING
    LOG(INFO) << "choose sp param m: " << sched_param_.sgemm_m_block1;
    LOG(INFO) << "choose sp param n: " << sched_param_.sgemm_n_block1;
    LOG(INFO) << "choose sp param k: " << sched_param_.sgemm_k_block1;
    LOG(INFO) << "best run time: " << best_run_time / num_benchmark_iter / 1000 << " ms";
#endif
    run_time = (double)best_run_time / (double)num_benchmark_iter;
    return ppl::common::RC_SUCCESS;
}

ppl::common::RetCode conv2d_n4cx_im2col_fp32_offline_manager::gen_cvt_weights(const void *filter, const void *bias)
{
    if (cvt_bias_ != nullptr || cvt_filter_ != nullptr) {
        return ppl::common::RC_PERMISSION_DENIED;
    }

    const int64_t num_output = param_.num_output;
    const int64_t channels   = param_.channels;
    const int64_t kernel_h   = param_.kernel_h;
    const int64_t kernel_w   = param_.kernel_w;
    const int64_t group      = param_.group;

    cvt_bias_size_               = CEIL4(num_output) * sizeof(float);
    cvt_bias_                    = allocator_->Alloc(cvt_bias_size_);
    int64_t padding_offset_bytes = num_output * sizeof(float);
    int64_t padding_bytes        = (CEIL4(num_output) - num_output) * sizeof(float);
    memcpy(cvt_bias_, bias, num_output * sizeof(float));
    memset((uint8_t *)cvt_bias_ + padding_offset_bytes, 0, padding_bytes);

    cvt_filter_size_ = conv_n4cx_tile_im2col_get_converted_filter_size(
        num_output, channels, kernel_h, kernel_w, group);
    cvt_filter_ = allocator_->Alloc(cvt_filter_size_);
    conv_n4cx_tile_im2col_convert_filter(
        (const float *)filter, (float *)cvt_filter_, num_output, channels, kernel_h, kernel_w, group);

    return ppl::common::RC_SUCCESS;
}

conv2d_runtime_executor *conv2d_n4cx_im2col_fp32_offline_manager::gen_executor()
{
    return new conv2d_n4cx_im2col_fp32_runtime_executor(&param_, cvt_filter_, cvt_bias_, sched_param_, ker_param_);
}

#undef CBLK
#undef ICBLK
#undef OCBLK
#undef PACK_CHANNEL

}}}}; // namespace ppl::kernel::arm_server::neon
