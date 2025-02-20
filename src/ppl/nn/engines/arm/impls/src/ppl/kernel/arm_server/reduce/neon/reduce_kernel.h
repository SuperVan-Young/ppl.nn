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

#ifndef __ST_PPL_KERNEL_ARM_SERVER_REDUCE_NEON_REDUCE_KERNEL_H_
#define __ST_PPL_KERNEL_ARM_SERVER_REDUCE_NEON_REDUCE_KERNEL_H_

#include <arm_neon.h>

#include "ppl/kernel/arm_server/reduce/neon/reduce_common.h"

namespace ppl { namespace kernel { namespace arm_server { namespace neon {

template <>
inline float reduce_scalar_kernel<float, REDUCE_MAX>(const float val, const float reduced)
{
    return max(val, reduced);
}

template <>
inline float reduce_scalar_kernel<float, REDUCE_MIN>(const float val, const float reduced)
{
    return min(val, reduced);
}

template <>
inline float reduce_scalar_kernel<float, REDUCE_SUM>(const float val, const float reduced)
{
    return val + reduced;
}

template <>
inline float reduce_scalar_kernel<float, REDUCE_MEAN>(const float val, const float reduced)
{
    return val + reduced;
}

template <>
inline float reduce_scalar_kernel<float, REDUCE_PROD>(const float val, const float reduced)
{
    return val * reduced;
}

template <>
inline float32x4_t reduce_vector_kernel<float32x4_t, REDUCE_MAX>(const float32x4_t val, const float32x4_t reduced)
{
    return vmax(val, reduced);
}

template <>
inline float32x4_t reduce_vector_kernel<float32x4_t, REDUCE_MIN>(const float32x4_t val, const float32x4_t reduced)
{
    return vmin(val, reduced);
}

template <>
inline float32x4_t reduce_vector_kernel<float32x4_t, REDUCE_SUM>(const float32x4_t val, const float32x4_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline float32x4_t reduce_vector_kernel<float32x4_t, REDUCE_MEAN>(const float32x4_t val, const float32x4_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline float32x4_t reduce_vector_kernel<float32x4_t, REDUCE_PROD>(const float32x4_t val, const float32x4_t reduced)
{
    return vmul(val, reduced);
}

template <>
inline float reduce_vector_to_scalar_kernel<float, float32x4_t, REDUCE_MAX>(const float32x4_t val, const float reduced)
{
    return max(vmaxvq_f32(val), reduced);
}

template <>
inline float reduce_vector_to_scalar_kernel<float, float32x4_t, REDUCE_MIN>(const float32x4_t val, const float reduced)
{
    return min(vminvq_f32(val), reduced);
}

template <>
inline float reduce_vector_to_scalar_kernel<float, float32x4_t, REDUCE_SUM>(const float32x4_t val, const float reduced)
{
    return vaddvq_f32(val) + reduced;
}

template <>
inline float reduce_vector_to_scalar_kernel<float, float32x4_t, REDUCE_MEAN>(const float32x4_t val, const float reduced)
{
    return vaddvq_f32(val) + reduced;
}

template <>
inline float reduce_vector_to_scalar_kernel<float, float32x4_t, REDUCE_PROD>(const float32x4_t val, const float reduced)
{
    return reduced * vgetq_lane_f32(val, 0) * vgetq_lane_f32(val, 1) * vgetq_lane_f32(val, 2) * vgetq_lane_f32(val, 3);
}

template <>
inline int64_t reduce_scalar_kernel<int64_t, REDUCE_MAX>(const int64_t val, const int64_t reduced)
{
    return max(val, reduced);
}

template <>
inline int64_t reduce_scalar_kernel<int64_t, REDUCE_MIN>(const int64_t val, const int64_t reduced)
{
    return min(val, reduced);
}

template <>
inline int64_t reduce_scalar_kernel<int64_t, REDUCE_SUM>(const int64_t val, const int64_t reduced)
{
    return val + reduced;
}

template <>
inline int64_t reduce_scalar_kernel<int64_t, REDUCE_MEAN>(const int64_t val, const int64_t reduced)
{
    return val + reduced;
}

template <>
inline int64_t reduce_scalar_kernel<int64_t, REDUCE_PROD>(const int64_t val, const int64_t reduced)
{
    return val * reduced;
}

template <>
inline int64x2_t reduce_vector_kernel<int64x2_t, REDUCE_MAX>(const int64x2_t val, const int64x2_t reduced)
{
    return vmax(val, reduced);
}

template <>
inline int64x2_t reduce_vector_kernel<int64x2_t, REDUCE_MIN>(const int64x2_t val, const int64x2_t reduced)
{
    return vmin(val, reduced);
}

template <>
inline int64x2_t reduce_vector_kernel<int64x2_t, REDUCE_SUM>(const int64x2_t val, const int64x2_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline int64x2_t reduce_vector_kernel<int64x2_t, REDUCE_MEAN>(const int64x2_t val, const int64x2_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline int64x2_t reduce_vector_kernel<int64x2_t, REDUCE_PROD>(const int64x2_t val, const int64x2_t reduced)
{
    return vmul(val, reduced);
}

template <>
inline int64_t reduce_vector_to_scalar_kernel<int64_t, int64x2_t, REDUCE_MAX>(const int64x2_t val, const int64_t reduced)
{
    return max(max(vgetq_lane_s64(val, 0), vgetq_lane_s64(val, 1)), reduced);
}

template <>
inline int64_t reduce_vector_to_scalar_kernel<int64_t, int64x2_t, REDUCE_MIN>(const int64x2_t val, const int64_t reduced)
{
    return min(min(vgetq_lane_s64(val, 0), vgetq_lane_s64(val, 1)), reduced);
}

template <>
inline int64_t reduce_vector_to_scalar_kernel<int64_t, int64x2_t, REDUCE_SUM>(const int64x2_t val, const int64_t reduced)
{
    return reduced + vaddvq_s64(val);
}

template <>
inline int64_t reduce_vector_to_scalar_kernel<int64_t, int64x2_t, REDUCE_MEAN>(const int64x2_t val, const int64_t reduced)
{
    return reduced + vaddvq_s64(val);
}

template <>
inline int64_t reduce_vector_to_scalar_kernel<int64_t, int64x2_t, REDUCE_PROD>(const int64x2_t val, const int64_t reduced)
{
    return reduced * vgetq_lane_s64(val, 0) * vgetq_lane_s64(val, 1);
}

#ifdef PPLNN_USE_ARMV8_2_FP16
template <>
inline __fp16 reduce_scalar_kernel<__fp16, REDUCE_MAX>(const __fp16 val, const __fp16 reduced)
{
    return max(val, reduced);
}

template <>
inline __fp16 reduce_scalar_kernel<__fp16, REDUCE_MIN>(const __fp16 val, const __fp16 reduced)
{
    return min(val, reduced);
}

template <>
inline __fp16 reduce_scalar_kernel<__fp16, REDUCE_SUM>(const __fp16 val, const __fp16 reduced)
{
    return val + reduced;
}

template <>
inline __fp16 reduce_scalar_kernel<__fp16, REDUCE_MEAN>(const __fp16 val, const __fp16 reduced)
{
    return val + reduced;
}

template <>
inline __fp16 reduce_scalar_kernel<__fp16, REDUCE_PROD>(const __fp16 val, const __fp16 reduced)
{
    return val * reduced;
}

template <>
inline float16x8_t reduce_vector_kernel<float16x8_t, REDUCE_MAX>(const float16x8_t val, const float16x8_t reduced)
{
    return vmax(val, reduced);
}

template <>
inline float16x8_t reduce_vector_kernel<float16x8_t, REDUCE_MIN>(const float16x8_t val, const float16x8_t reduced)
{
    return vmin(val, reduced);
}

template <>
inline float16x8_t reduce_vector_kernel<float16x8_t, REDUCE_SUM>(const float16x8_t val, const float16x8_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline float16x8_t reduce_vector_kernel<float16x8_t, REDUCE_MEAN>(const float16x8_t val, const float16x8_t reduced)
{
    return vadd(val, reduced);
}

template <>
inline float16x8_t reduce_vector_kernel<float16x8_t, REDUCE_PROD>(const float16x8_t val, const float16x8_t reduced)
{
    return vmul(val, reduced);
}

template <>
inline __fp16 reduce_vector_to_scalar_kernel<__fp16, float16x8_t, REDUCE_MAX>(const float16x8_t val, const __fp16 reduced)
{
    float16x4_t v_half_f16 = vmax_f16(vget_low_f16(val), vget_high_f16(val));
    float32x4_t v_half_f32 = vcvt_f32_f16(v_half_f16);
    return max(reduced, (__fp16)vmaxvq_f32(v_half_f32));
}

template <>
inline __fp16 reduce_vector_to_scalar_kernel<__fp16, float16x8_t, REDUCE_MIN>(const float16x8_t val, const __fp16 reduced)
{
    float16x4_t v_half_f16 = vmin_f16(vget_low_f16(val), vget_high_f16(val));
    float32x4_t v_half_f32 = vcvt_f32_f16(v_half_f16);
    return min(reduced, (__fp16)vminvq_f32(v_half_f32));
}

template <>
inline __fp16 reduce_vector_to_scalar_kernel<__fp16, float16x8_t, REDUCE_SUM>(const float16x8_t val, const __fp16 reduced)
{
    float16x4_t v_half_f16 = vadd_f16(vget_low_f16(val), vget_high_f16(val));
    float32x4_t v_half_f32 = vcvt_f32_f16(v_half_f16);
    return reduced + (__fp16)vaddvq_f32(v_half_f32);
}

template <>
inline __fp16 reduce_vector_to_scalar_kernel<__fp16, float16x8_t, REDUCE_MEAN>(const float16x8_t val, const __fp16 reduced)
{
    float16x4_t v_half_f16 = vadd_f16(vget_low_f16(val), vget_high_f16(val));
    float32x4_t v_half_f32 = vcvt_f32_f16(v_half_f16);
    return reduced + (__fp16)vaddvq_f32(v_half_f32);
}

template <>
inline __fp16 reduce_vector_to_scalar_kernel<__fp16, float16x8_t, REDUCE_PROD>(const float16x8_t val, const __fp16 reduced)
{
    float16x4_t v_half_f16 = vmul_f16(vget_low_f16(val), vget_high_f16(val));
    return reduced * vget_lane_f16(v_half_f16, 0) * vget_lane_f16(v_half_f16, 1) * vget_lane_f16(v_half_f16, 2) * vget_lane_f16(v_half_f16, 3);
}
#endif

}}}}; // namespace ppl::kernel::arm_server::neon

#endif // __ST_PPL_KERNEL_ARM_SERVER_REDUCE_NEON_REDUCE_KERNEL_H_