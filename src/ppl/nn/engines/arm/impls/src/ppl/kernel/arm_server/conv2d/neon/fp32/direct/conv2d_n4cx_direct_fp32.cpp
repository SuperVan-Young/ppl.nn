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

#include "ppl/kernel/arm_server/conv2d/neon/fp32/direct/conv2d_n4cx_direct_fp32.h"

#include <arm_neon.h>
#include <chrono>
#include <new>
#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "ppl/common/arm/sysinfo.h"
#include "ppl/kernel/arm_server/common/internal_include.h"
#include "ppl/kernel/arm_server/conv2d/neon/fp32/utils/conv2d_utils_fp32.h"

namespace ppl { namespace kernel { namespace arm_server { namespace neon {

#define CBLK()  4
#define ICBLK() CBLK()
#define OCBLK() CBLK()

template <const int64_t ocblk, const int64_t dst_tile_w>
void ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func(
    const float *input_base,
    const float *filter_base,
    const float *bias_base,
    float *output_base,
    float *sum_base,
    int64_t ic_section_pck,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t flt_h_offset_byte,
    const int64_t flt_hw_offset_byte,
    const int64_t dst_ocblk_offset_byte,
    const int64_t src_icblk_offset_byte,
    const int64_t src_filter_row_offset_byte,
    const int64_t src_filter_elem_offset_byte,
    const int64_t src_out_elem_offset_byte,
    const uint32_t fuse_flag);

template <const uint64_t ocblk, const int64_t dst_tile_w>
void ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func(
    const float *input_base,
    const float *filter_base,
    const float *bias_base,
    float *output_base,
    float *sum_base,
    int64_t ic_block_ceil8,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t flt_h_offset_byte,
    const int64_t flt_hw_offset_byte,
    const int64_t dst_ocblk_offset_byte,
    const int64_t src_icblk_offset_byte,
    const int64_t src_filter_row_offset_byte,
    const int64_t src_filter_elem_offset_byte,
    const int64_t src_out_elem_offset_byte,
    const uint32_t fuse_flag);

#define DST_TILE_W() 10
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 9
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 8
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 7
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 6
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 5
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 4
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 3
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 2
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 1
#include "conv2d_direct_fxsx_kernel.inc"
#undef DST_TILE_W

#define DST_TILE_W() 10
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 9
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 8
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 7
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 6
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 5
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 4
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 3
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 2
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W
#define DST_TILE_W() 1
#include "conv2d_direct_f1s1_kernel.inc"
#undef DST_TILE_W

typedef void (*ppl_arm_server_kernel_fp32_conv_direct_kernel_t)(
    const float *input_base,
    const float *filter_base,
    const float *bias_base,
    float *output_base,
    float *sum_base,
    int64_t ic_block_ceil4,
    const int64_t flt_h,
    const int64_t flt_w,
    const int64_t flt_diff_w_x_icv_x_ocs_bytes,
    const int64_t flt_diff_h_x_flt_w_x_icv_x_ocs_bytes,
    const int64_t dst_h_x_dst_w_x_ocv_bytes,
    const int64_t src_h_x_src_w_x_icv_bytes,
    const int64_t dltn_h_x_src_w_x_icv_bytes,
    const int64_t dltn_w_x_icv_bytes,
    const int64_t strd_w_x_icv_bytes,
    const uint32_t fuse_flag);

#define OW_CASE() 10
static ppl_arm_server_kernel_fp32_conv_direct_kernel_t ppl_arm_server_kernel_fp32_conv_direct_oc8_kernel_func_table[OW_CASE() + 1] =
    {
        nullptr,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 1>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 2>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 3>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 4>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 5>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 6>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 7>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 8>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 9>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<8, 10>,
};

static ppl_arm_server_kernel_fp32_conv_direct_kernel_t ppl_arm_server_kernel_fp32_conv_direct_oc4_kernel_func_table[OW_CASE() + 1] =
    {
        nullptr,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 1>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 2>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 3>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 4>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 5>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 6>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 7>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 8>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 9>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_func<4, 10>,
};

static ppl_arm_server_kernel_fp32_conv_direct_kernel_t ppl_arm_server_kernel_fp32_conv_direct_oc8_f1s1_kernel_func_table[OW_CASE() + 1] =
    {
        nullptr,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 1>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 2>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 3>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 4>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 5>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 6>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 7>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 8>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 9>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<8, 10>,
};

static ppl_arm_server_kernel_fp32_conv_direct_kernel_t ppl_arm_server_kernel_fp32_conv_direct_oc4_f1s1_kernel_func_table[OW_CASE() + 1] =
    {
        nullptr,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 1>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 2>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 3>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 4>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 5>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 6>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 7>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 8>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 9>,
        ppl_arm_server_kernel_fp32_conv_direct_n4cx_h1wx_f1s1_func<4, 10>,
};
#undef OW_CASE

uint64_t conv2d_n4cx_direct_fp32_runtime_executor::cal_temp_buffer_size()
{
    const conv2d_param &cp = *conv_param_;
    const int64_t ic_g_pck = CEIL4(cp.channels / cp.group);
    const int64_t oc_g_pck = CEIL4(cp.num_output / cp.group);

    const int64_t src_h     = src_shape_->GetDim(2);
    const int64_t src_w     = src_shape_->GetDim(3);
    const int64_t dst_h     = dst_shape_->GetDim(2);
    const int64_t dst_w     = dst_shape_->GetDim(3);
    const int64_t num_batch = src_shape_->GetDim(0);

    uint64_t input_gbuf_size     = num_batch * ic_g_pck * src_h * src_w * sizeof(float);
    uint64_t output_gbuf_size    = num_batch * oc_g_pck * dst_h * dst_w * sizeof(float);
    uint64_t packing_buffer_size = PPL_OMP_MAX_THREADS() * dst_h * dst_w * sched_param_.ic_tile * sizeof(float);

    return input_gbuf_size + output_gbuf_size + packing_buffer_size;
}

void conv2d_n4cx_direct_fp32_runtime_executor::adjust_schedule_param()
{
    const int64_t dst_h                         = dst_shape_->GetDim(2);
    const int64_t dst_w                         = dst_shape_->GetDim(3);
    sched_param_.packing_buffer_size_per_thread = dst_h * dst_w * sched_param_.ic_tile * sizeof(float);
    return;
}

ppl::common::RetCode conv2d_n4cx_direct_fp32_runtime_executor::prepare()
{
    if (!conv_param_ || !src_shape_ || !dst_shape_) {
        return ppl::common::RC_INVALID_VALUE;
    }

    adjust_schedule_param();
    return ppl::common::RC_SUCCESS;
}

ppl::common::RetCode conv2d_n4cx_direct_fp32_runtime_executor::execute()
{
    const conv2d_param &cp                           = *conv_param_;
    const conv2d_n4cx_direct_fp32_schedule_param &sp = sched_param_;
    const conv2d_n4cx_direct_fp32_kernel_param &kp   = ker_param_;

    const float *input                          = (const float *)src_;
    const float *cvt_filter                     = (const float *)cvt_filter_;
    const float *bias                           = (const float *)cvt_bias_;
    float *output                               = (float *)dst_;
    float *sum                                  = (float *)sum_;
    float *tmp_buffer                           = (float *)temp_buffer_;
    const int64_t src_h                         = src_shape_->GetDim(2);
    const int64_t src_w                         = src_shape_->GetDim(3);
    const int64_t channels                      = src_shape_->GetDim(1);
    const int64_t num_output                    = cp.num_output;
    const int64_t dst_h                         = dst_shape_->GetDim(2);
    const int64_t dst_w                         = dst_shape_->GetDim(3);
    const int64_t flt_h                         = cp.kernel_h;
    const int64_t flt_w                         = cp.kernel_w;
    const int64_t pad_h                         = cp.pad_h;
    const int64_t pad_w                         = cp.pad_w;
    const int64_t strd_h                        = cp.stride_h;
    const int64_t strd_w                        = cp.stride_w;
    const int64_t dltn_h                        = cp.dilation_h;
    const int64_t dltn_w                        = cp.dilation_w;
    const int64_t single_core_tmp_buffer_offset = sp.packing_buffer_size_per_thread / sizeof(float);
    const int64_t num_batch                     = src_shape_->GetDim(0);

    PRAGMA_OMP_PARALLEL()
    {
        const int64_t thread_id = PPL_OMP_THREAD_ID();

        const int64_t ic_pck  = CEIL4(channels);
        const int64_t num_output_pck = CEIL4(num_output);

        const int64_t ic_group = channels / cp.group;
        const int64_t oc_group = num_output / cp.group;
        const int64_t ic_g_pck = CEIL4(ic_group);
        const int64_t oc_g_pck = CEIL4(oc_group);

        const int64_t dst_tile_h = kp.dst_tile_h;
        const int64_t dst_tile_w = kp.dst_tile_w;

        const int64_t ocblk2  = kp.ocblk2;
        const int64_t ic_tile = sp.ic_tile;

        const int64_t icblk_bytes = 16;
        const int64_t ocblk_bytes = 16;

        const int64_t src_icblk_offset_byte    = src_h * src_w * icblk_bytes;
        const int64_t dst_ocblk_offset_byte    = dst_h * dst_w * ocblk_bytes;
        const int64_t src_filter_w_offset_byte = dltn_w * icblk_bytes;
        const int64_t src_output_w_offset_byte = strd_w * icblk_bytes;
        const int64_t src_filter_h_offset_byte = src_w * dltn_h * icblk_bytes;
        const int64_t flt_w_offset_byte        = ocblk2 * icblk_bytes;
        const int64_t flt_h_offset_byte        = flt_w * ocblk2 * icblk_bytes;

        const int64_t single_batch_input_size  = ic_pck * src_h * src_w;
        const int64_t single_batch_output_size = num_output_pck * dst_h * dst_w;

        const bool use_in_gbuf                   = (cp.group > 1 && ic_g_pck != ic_group);
        const bool use_out_gbuf                  = (cp.group > 1 && oc_g_pck != oc_group);
        const int64_t input_group_buffer_offset  = num_batch * ic_g_pck * src_h * src_w;
        const int64_t output_group_buffer_offset = num_batch * oc_g_pck * dst_h * dst_w;
        float *input_gbuf                        = tmp_buffer;
        float *output_gbuf                       = input_gbuf + input_group_buffer_offset;
        float *input_aux_buffer                  = output_gbuf + output_group_buffer_offset + thread_id * single_core_tmp_buffer_offset;

        const bool use_pack    = (flt_h == 1 && flt_w == 1 && (strd_h > 1 || strd_w > 1));
        const bool use_f1s1    = (flt_h == 1 && flt_w == 1 && strd_h == 1 && strd_w == 1);
        int64_t packed_batch   = -1;
        int64_t packed_ic      = -1;

        int64_t ow_inner_start = std::max((int64_t)0, DIV_CEIL((pad_w - 0 * dltn_w), strd_w)); // inclusive
        int64_t ow_inner_end   = std::min((int64_t)dst_w, DIV_CEIL((src_w + pad_w - (flt_w - 1) * dltn_w), strd_w)); // exclusive
        ow_inner_start         = std::min(ow_inner_start, dst_w);
        ow_inner_end           = std::max(ow_inner_end, ow_inner_start);

        uint32_t kernel_fuse_type = cp.fuse_flag;
        if (use_out_gbuf && (cp.fuse_flag & conv_fuse_flag::SUM)) {
            kernel_fuse_type = conv_fuse_flag::NONE;
        }

        for (int64_t g = 0; g < cp.group; g++) {
            int64_t in_b_stride  = single_batch_input_size;
            int64_t out_b_stride = single_batch_output_size;

            const float *cvt_filter_g_base = cvt_filter + g * CEIL(oc_group, ocblk2) * ic_g_pck * flt_h * flt_w;
            const float *bias_g_base       = bias + g * oc_group;

            const float *kernel_input = input + g * ic_group * src_h * src_w;
            float *kernel_output      = output + g * oc_group * dst_h * dst_w;
            if (use_in_gbuf) {
                in_b_stride  = ic_g_pck * src_h * src_w;
                kernel_input = input_gbuf;
                for (int64_t b = 0; b < num_batch; b++) {
                    conv2d_n4cx_load_group_fp32(
                        input + b * single_batch_input_size,
                        input_gbuf + b * in_b_stride,
                        src_h * src_w,
                        ic_group,
                        g,
                        0);
                }
                PRAGMA_OMP_BARRIER()
            }
            if (use_out_gbuf) {
                out_b_stride  = oc_g_pck * dst_h * dst_w;
                kernel_output = output_gbuf;
            }

#if not defined PPL_USE_ARM_SERVER_OMP
            for (int64_t batch_id = 0; batch_id < num_batch; batch_id++) {
                const float *input_batch_base_ptr = kernel_input + batch_id * in_b_stride;
                float *output_batch_base_ptr      = kernel_output + batch_id * out_b_stride;
                float *sum_bg_base_ptr            = sum + batch_id * single_batch_output_size + g * oc_group * dst_h * dst_w;
                for (int64_t ic_l1 = 0; ic_l1 < ic_g_pck; ic_l1 += ic_tile) {
                    const int64_t ic_remain  = std::min(ic_tile, ic_g_pck - ic_l1);
                    const uint32_t fuse_flag = (ic_l1 + ic_tile >= ic_g_pck) ? kernel_fuse_type : (const uint32_t)conv_fuse_flag::NONE;
                    for (int64_t oc_l1 = 0; oc_l1 < oc_g_pck; oc_l1 += ocblk2) {
                        const float *const bias_ptr = (ic_l1 == 0) ? (bias_g_base + oc_l1) : nullptr;
                        const int64_t oc_remains    = std::min(ocblk2, oc_g_pck - oc_l1);
                        const ppl_arm_server_kernel_fp32_conv_direct_kernel_t *conv_direct_kernel_func_table =
                            (oc_remains > OCBLK()) ? ppl_arm_server_kernel_fp32_conv_direct_oc8_kernel_func_table : ppl_arm_server_kernel_fp32_conv_direct_oc4_kernel_func_table;
                        if (use_pack || use_f1s1) {
                            conv_direct_kernel_func_table =
                                (oc_remains > OCBLK()) ? ppl_arm_server_kernel_fp32_conv_direct_oc8_f1s1_kernel_func_table : ppl_arm_server_kernel_fp32_conv_direct_oc4_f1s1_kernel_func_table;
                        }
                        for (int64_t oh = 0; oh < dst_h; oh += dst_tile_h) {
#else
            for (int64_t ic_l1 = 0; ic_l1 < ic_g_pck; ic_l1 += ic_tile) {
                const uint32_t fuse_flag = (ic_l1 + ic_tile >= ic_g_pck) ? kernel_fuse_type : (const uint32_t)conv_fuse_flag::NONE;
                PRAGMA_OMP_FOR_COLLAPSE(3)
                for (int64_t batch_id = 0; batch_id < num_batch; batch_id++) {
                    for (int64_t oc_l1 = 0; oc_l1 < oc_g_pck; oc_l1 += ocblk2) {
                        for (int64_t oh = 0; oh < dst_h; oh += dst_tile_h) {
                            const float *input_batch_base_ptr = kernel_input + batch_id * in_b_stride;
                            float *output_batch_base_ptr      = kernel_output + batch_id * out_b_stride;
                            float *sum_bg_base_ptr            = sum + batch_id * single_batch_output_size + g * oc_group * dst_h * dst_w;
                            const float *const bias_ptr       = (ic_l1 == 0) ? (bias_g_base + oc_l1) : nullptr;
                            const int64_t ic_remain           = std::min(ic_tile, ic_g_pck - ic_l1);
                            const int64_t oc_remains          = std::min(ocblk2, oc_g_pck - oc_l1);
                            const ppl_arm_server_kernel_fp32_conv_direct_kernel_t *conv_direct_kernel_func_table =
                                (oc_remains > OCBLK()) ? ppl_arm_server_kernel_fp32_conv_direct_oc8_kernel_func_table : ppl_arm_server_kernel_fp32_conv_direct_oc4_kernel_func_table;
                            if (use_pack || use_f1s1) {
                                conv_direct_kernel_func_table =
                                    (oc_remains > OCBLK()) ? ppl_arm_server_kernel_fp32_conv_direct_oc8_f1s1_kernel_func_table : ppl_arm_server_kernel_fp32_conv_direct_oc4_f1s1_kernel_func_table;
                            }
#endif
                            const int64_t ih            = -pad_h + oh * strd_h;
                            int64_t flt_h_start         = DIV_CEIL(std::max((int64_t)0, -ih), dltn_h);
                            int64_t flt_h_end           = std::min(flt_h, DIV_CEIL((src_h - ih), dltn_h));
                            flt_h_end                   = std::max(flt_h_end, flt_h_start);
                            const int64_t flt_h_skipped = flt_h - (flt_h_end - flt_h_start);
                            if (flt_h_skipped == flt_h) continue;

                            if (use_pack && (ic_l1 != packed_ic || batch_id != packed_batch)) {
                                for (int ic = 0; ic < ic_remain; ic += ICBLK()) {
                                    for (int oh_iter = 0; oh_iter < dst_h; oh_iter++) {
                                        const int64_t ih = -pad_h + oh_iter * strd_h;
                                        for (int ow = 0; ow < dst_w; ow++) {
                                            const int64_t iw = -pad_w + ow * strd_w;
                                            vst1q_f32(input_aux_buffer + ic * dst_h * dst_w + oh_iter * dst_w * OCBLK() + ow * OCBLK(),
                                                      vld1q_f32(input_batch_base_ptr + (ic_l1 + ic) * src_h * src_w + ih * src_w * ICBLK() + iw * ICBLK()));
                                        }
                                    }
                                }
                                packed_batch = batch_id;
                                packed_ic    = ic_l1;
                            }

                            const float *input_h_base               = input_batch_base_ptr + ic_l1 * src_w * src_h + (ih + flt_h_start * dltn_h) * src_w * CBLK();
                            int64_t local_strd_w                    = strd_w;
                            int64_t input_cblk_stride_bytes         = src_icblk_offset_byte;
                            int64_t input_filtered_row_stride_bytes = src_filter_h_offset_byte;
                            if (use_pack) {
                                local_strd_w                    = 1;
                                input_h_base                    = input_aux_buffer + oh * dst_w * CBLK();
                                input_cblk_stride_bytes         = dst_h * dst_w * icblk_bytes;
                                input_filtered_row_stride_bytes = dst_w * icblk_bytes;
                            }
                            if (0 < ow_inner_start) {
                                int64_t prv_ow          = 0;
                                int64_t ow              = 0;
                                int64_t prv_flt_w_start = -1;
                                int64_t prv_flt_w_end   = -1;
                                for (; ow < ow_inner_start + 1; ow++) {
                                    const int64_t iw               = -pad_w + ow * strd_w;
                                    int64_t flt_w_start             = DIV_CEIL(std::max((int64_t)0, -iw), dltn_w);
                                    int64_t flt_w_end               = std::min(flt_w, DIV_CEIL((src_w - iw), dltn_w));
                                    flt_w_end                       = std::max(flt_w_end, flt_w_start);
                                    const int64_t prv_flt_w_skipped = flt_w - (prv_flt_w_end - prv_flt_w_start);
                                    if (prv_flt_w_start != flt_w_start || prv_flt_w_end != flt_w_end || ow - prv_ow == dst_tile_w || ow == ow_inner_start) {
                                        if (prv_flt_w_skipped < flt_w && ow > prv_ow) {
                                            const int64_t iw_iter = -pad_w + prv_ow * local_strd_w + prv_flt_w_start * dltn_w;
                                            conv_direct_kernel_func_table[ow - prv_ow](
                                                input_h_base + iw_iter * CBLK(),
                                                cvt_filter_g_base + oc_l1 * ic_g_pck * flt_h * flt_w + ic_l1 * flt_h * flt_w * ocblk2 + flt_h_start * flt_w * CBLK() * ocblk2 + prv_flt_w_start * CBLK() * ocblk2,
                                                bias_ptr,
                                                output_batch_base_ptr + oc_l1 * dst_h * dst_w + oh * dst_w * CBLK() + prv_ow * CBLK(),
                                                sum_bg_base_ptr + oc_l1 * dst_h * dst_w + oh * dst_w * CBLK() + prv_ow * CBLK(),
                                                ic_remain,
                                                flt_h_end - flt_h_start,
                                                prv_flt_w_end - prv_flt_w_start,
                                                prv_flt_w_skipped * flt_w_offset_byte,
                                                flt_h_skipped * flt_h_offset_byte,
                                                dst_ocblk_offset_byte,
                                                input_cblk_stride_bytes,
                                                input_filtered_row_stride_bytes,
                                                src_filter_w_offset_byte,
                                                use_pack ? icblk_bytes : src_output_w_offset_byte,
                                                fuse_flag);
                                        }
                                        prv_ow         = ow;
                                        prv_flt_w_start = flt_w_start;
                                        prv_flt_w_end   = flt_w_end;
                                    }
                                }
                            }
                            for (int64_t ow = ow_inner_start; ow < ow_inner_end; ow += dst_tile_w) {
                                const int64_t ow_len = std::min(dst_tile_w, ow_inner_end - ow);
                                const int64_t iw     = -pad_w + ow * local_strd_w;

                                conv_direct_kernel_func_table[ow_len](
                                    input_h_base + iw * CBLK(),
                                    cvt_filter_g_base + oc_l1 * ic_g_pck * flt_h * flt_w + ic_l1 * flt_h * flt_w * ocblk2 + flt_h_start * flt_w * CBLK() * ocblk2,
                                    bias_ptr,
                                    output_batch_base_ptr + oc_l1 * dst_w * dst_h + oh * dst_w * CBLK() + ow * CBLK(),
                                    sum_bg_base_ptr + oc_l1 * dst_w * dst_h + oh * dst_w * CBLK() + ow * CBLK(),
                                    ic_remain,
                                    flt_h_end - flt_h_start,
                                    flt_w,
                                    0,
                                    flt_h_skipped * flt_h_offset_byte,
                                    dst_ocblk_offset_byte,
                                    input_cblk_stride_bytes,
                                    input_filtered_row_stride_bytes,
                                    src_filter_w_offset_byte,
                                    use_pack ? icblk_bytes : src_output_w_offset_byte,
                                    fuse_flag);
                            }
                            if (ow_inner_end < dst_w) {
                                int64_t prv_ow         = ow_inner_end;
                                int64_t ow             = ow_inner_end;
                                int64_t prv_flt_w_start = -1;
                                int64_t prv_flt_w_end   = -1;
                                for (; ow < dst_w + 1; ow++) {
                                    const int64_t iw   = -pad_w + ow * strd_w;
                                    int64_t flt_w_start = DIV_CEIL(std::max((int64_t)0, -iw), dltn_w);
                                    int64_t flt_w_end   = std::min(flt_w, DIV_CEIL((src_w - iw), dltn_w));
                                    flt_w_end           = std::max(flt_w_end, flt_w_start);
                                    if (prv_flt_w_start != flt_w_start || prv_flt_w_end != flt_w_end || ow - prv_ow == dst_tile_w || ow == dst_w) {
                                        const int64_t prv_flt_w_skipped = flt_w - (prv_flt_w_end - prv_flt_w_start);
                                        if (prv_flt_w_skipped < flt_w && ow > prv_ow) {
                                            const int64_t iw_iter = -pad_w + prv_ow * local_strd_w + prv_flt_w_start * dltn_w;
                                            conv_direct_kernel_func_table[ow - prv_ow](
                                                input_h_base + iw_iter * CBLK(),
                                                cvt_filter_g_base + oc_l1 * ic_g_pck * flt_h * flt_w + ic_l1 * flt_h * flt_w * ocblk2 + flt_h_start * flt_w * CBLK() * ocblk2 + prv_flt_w_start * CBLK() * ocblk2, //TODO
                                                bias_ptr,
                                                output_batch_base_ptr + oc_l1 * dst_h * dst_w + oh * dst_w * CBLK() + prv_ow * CBLK(),
                                                sum_bg_base_ptr + oc_l1 * dst_h * dst_w + oh * dst_w * CBLK() + prv_ow * CBLK(),
                                                ic_remain,
                                                flt_h_end - flt_h_start,
                                                prv_flt_w_end - prv_flt_w_start,
                                                prv_flt_w_skipped * flt_w_offset_byte,
                                                flt_h_skipped * flt_h_offset_byte,
                                                dst_ocblk_offset_byte,
                                                input_cblk_stride_bytes,
                                                input_filtered_row_stride_bytes,
                                                src_filter_w_offset_byte,
                                                use_pack ? icblk_bytes : src_output_w_offset_byte,
                                                fuse_flag);
                                        }
                                        prv_ow          = ow;
                                        prv_flt_w_start = flt_w_start;
                                        prv_flt_w_end   = flt_w_end;
                                    }
                                }
                            }
                        } // close loop over oh
                    } // close loop over ic tile
                } // close loop over oc block x 2
            } // close loop over batch

            if (use_out_gbuf) {
                for (int64_t b = 0; b < num_batch; b++) {
                    conv2d_n4cx_store_group_fp32(
                        output_gbuf + b * out_b_stride,
                        output + b * single_batch_output_size,
                        sum + b * single_batch_output_size,
                        dst_h * dst_w,
                        oc_group,
                        g,
                        0,
                        cp.fuse_flag);
                }
                PRAGMA_OMP_BARRIER()
            }
        } // close loop over group
    }
    return ppl::common::RC_SUCCESS;
}

bool conv2d_n4cx_direct_fp32_offline_manager::is_supported()
{
    return true;
}

ppl::common::RetCode conv2d_n4cx_direct_fp32_offline_manager::fast_init_schedule_param()
{
    sched_param_.ic_tile = 64;
    if (sched_param_.ic_tile != 64) {
        return ppl::common::RC_INVALID_VALUE;
    }
    return ppl::common::RC_SUCCESS;
}

// NOTE: (oc, ic, kh, kw) -> (oc/8, ic/4, kh, kw, 4ic, 8oc)
static inline int64_t ppl_arm_server_kernel_fp32_conv_direct_n4cx_get_converted_filter_size(
    const int64_t group,
    const int64_t in_c,
    const int64_t out_c,
    const int64_t ker_h,
    const int64_t ker_w)
{
    const int64_t ic_group  = in_c / group;
    const int64_t oc_group  = out_c / group;
    const int64_t ic_g_pck  = CEIL4(ic_group);
    const int64_t oc_g_pck2 = CEIL8(oc_group);
    return CEIL128(group * oc_g_pck2 * ic_g_pck * ker_h * ker_w * sizeof(float)) + ker_w * 2 * OCBLK() * ICBLK() * sizeof(float)  + 128;
}

ppl::common::RetCode conv2d_n4cx_direct_fp32_offline_manager::pick_best_schedule_param(const ppl::nn::TensorShape &src_shape, double &run_time, bool tune_blocksize)
{
    const int64_t num_output = param_.num_output;
    const int64_t channels   = param_.channels;
    const int64_t kernel_h   = param_.kernel_h;
    const int64_t kernel_w   = param_.kernel_w;
    const int64_t group      = param_.group;

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

    uint64_t cvt_filter_size = ppl_arm_server_kernel_fp32_conv_direct_n4cx_get_converted_filter_size(
        group, channels, num_output, kernel_h, kernel_w);
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

    std::vector<int64_t> candidate_ic_tile_list = {64};

    if (tune_blocksize) {
        candidate_ic_tile_list = {32, 48, 64, 96, 112, 128};
    }

    int64_t best_ic_tile  = 64;
    int64_t best_run_time = std::numeric_limits<int64_t>::max();

    const int num_warmup_iter    = 1;
    const int num_benchmark_iter = 5;
    for (auto ic_tile : candidate_ic_tile_list) {
        sched_param_.ic_tile = ic_tile;

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
            best_ic_tile   = ic_tile;
            best_run_time = elapsed_time;
        }

        allocator_->Free(tmp_buffer);
        delete conv_exe;

        if (ic_tile >= channels / group) break;
    }

    cvt_filter_ = nullptr;
    cvt_bias_   = nullptr;
    allocator_->Free(cvt_filter);
    allocator_->Free(cvt_bias);
    allocator_->Free(src);
    allocator_->Free(dst);

    sched_param_.ic_tile = best_ic_tile;
#ifdef PPLNN_ENABLE_KERNEL_PROFILING
    LOG(DEBUG) << "choose sp param ic: " << sched_param_.ic_tile;
    LOG(DEBUG) << "best run time: " << best_run_time / num_benchmark_iter / 1000 << " ms";
#endif
    run_time = (double)best_run_time / (double)num_benchmark_iter;
    return ppl::common::RC_SUCCESS;
}

// NOTE: (oc, ic, kh, kw) -> (oc/8, ic/4, kh, kw, 4ic, 8oc)
static void ppl_arm_server_kernel_fp32_conv_direct_n4cx_convert_filter(
    const float *filter,
    float *converted_filter,
    const int64_t group,
    const int64_t in_c,
    const int64_t out_c,
    const int64_t flt_h,
    const int64_t flt_w)
{
    const int64_t ocblk2    = OCBLK() * 2;
    const int64_t ic_group  = in_c / group;
    const int64_t oc_group  = out_c / group;
    const int64_t ic_g_pck  = CEIL4(ic_group);
    const int64_t oc_g_pck2 = CEIL8(oc_group);

    for (int64_t g = 0; g < group; g++) {
        const float *filter_g_base = filter + g * oc_group * ic_group * flt_h * flt_w;
        float *cvt_filter_g_base   = converted_filter + g * oc_g_pck2 * ic_g_pck * flt_h * flt_w;
        for (int64_t oc = 0; oc < oc_group; oc++) {
            for (int64_t ic = 0; ic < ic_group; ic++) {
                for (int64_t kh = 0; kh < flt_h; kh++) {
                    for (int64_t kw = 0; kw < flt_w; kw++) {
                        const int64_t cvt_index = (oc / ocblk2) * DIV_CEIL(ic_group, ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  (ic / ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  kh * flt_w * ICBLK() * ocblk2 +
                                                  kw * ICBLK() * ocblk2 +
                                                  (ic % ICBLK()) * ocblk2 +
                                                  oc % ocblk2;
                        cvt_filter_g_base[cvt_index] = filter_g_base[oc * ic_group * flt_h * flt_w + ic * flt_h * flt_w + kh * flt_w + kw];
                    }
                }
            }
            for (int64_t ic = ic_group; ic < ic_g_pck; ic++) {
                for (int64_t kh = 0; kh < flt_h; kh++) {
                    for (int64_t kw = 0; kw < flt_w; kw++) {
                        const int64_t cvt_index = (oc / ocblk2) * DIV_CEIL(ic_group, ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  (ic / ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  kh * flt_w * ICBLK() * ocblk2 +
                                                  kw * ICBLK() * ocblk2 +
                                                  (ic % ICBLK()) * ocblk2 +
                                                  oc % ocblk2;
                        cvt_filter_g_base[cvt_index] = 0.0f;
                    }
                }
            }
        }

        for (int64_t oc = oc_group; oc < oc_g_pck2; oc++) {
            for (int64_t ic = 0; ic < ic_g_pck; ic++) {
                for (int64_t kh = 0; kh < flt_h; kh++) {
                    for (int64_t kw = 0; kw < flt_w; kw++) {
                        const int64_t cvt_index = (oc / ocblk2) * DIV_CEIL(ic_group, ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  (ic / ICBLK()) * flt_h * flt_w * ICBLK() * ocblk2 +
                                                  kh * flt_w * ICBLK() * ocblk2 +
                                                  kw * ICBLK() * ocblk2 +
                                                  (ic % ICBLK()) * ocblk2 +
                                                  oc % ocblk2;
                        cvt_filter_g_base[cvt_index] = 0.0f;
                    }
                }
            }
        }
    }
}

ppl::common::RetCode conv2d_n4cx_direct_fp32_offline_manager::gen_cvt_weights(const void *filter, const void *bias)
{
    if (cvt_bias_ != nullptr || cvt_filter_ != nullptr) {
        return ppl::common::RC_PERMISSION_DENIED;
    }

    const int64_t group  = param_.group;
    const int64_t num_output = param_.num_output;
    const int64_t channels   = param_.channels;
    const int64_t kernel_h   = param_.kernel_h;
    const int64_t kernel_w   = param_.kernel_w;

    cvt_bias_size_               = CEIL4(num_output) * sizeof(float);
    cvt_bias_                    = allocator_->Alloc(cvt_bias_size_);
    int64_t padding_offset_bytes = num_output * sizeof(float);
    int64_t padding_bytes        = (CEIL4(num_output) - num_output) * sizeof(float);
    memcpy(cvt_bias_, bias, num_output * sizeof(float));
    memset((uint8_t *)cvt_bias_ + padding_offset_bytes, 0, padding_bytes);

    cvt_filter_size_ = ppl_arm_server_kernel_fp32_conv_direct_n4cx_get_converted_filter_size(
        group, channels, num_output, kernel_h, kernel_w);
    cvt_filter_ = allocator_->Alloc(cvt_filter_size_);
    ppl_arm_server_kernel_fp32_conv_direct_n4cx_convert_filter(
        (const float *)filter,
        (float *)cvt_filter_,
        group,
        channels,
        num_output,
        kernel_h,
        kernel_w);
    return ppl::common::RC_SUCCESS;
}

conv2d_runtime_executor *conv2d_n4cx_direct_fp32_offline_manager::gen_executor()
{
    return new conv2d_n4cx_direct_fp32_runtime_executor(&param_, cvt_filter_, cvt_bias_, sched_param_);
}

#undef CBLK
#undef ICBLK
#undef OCBLK

}}}}; // namespace ppl::kernel::arm_server::neon
