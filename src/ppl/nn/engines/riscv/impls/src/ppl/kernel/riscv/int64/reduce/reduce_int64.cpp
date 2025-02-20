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

#include <algorithm>
#include "ppl/kernel/riscv/int64/reduce/reduce_n2cx_int64.h"
#include "ppl/kernel/riscv/int64/reduce/reduce_ndarray_int64.h"

namespace ppl { namespace kernel { namespace riscv {

template <reduce_op_type_t op>
ppl::common::RetCode reduce_int64(
    const int64_t* src,
    int64_t* dst,

    const ppl::nn::TensorShape* src_shape,
    const ppl::nn::TensorShape* dst_shape,
    const int32_t* axes,
    const int32_t num_axes)
{
    if (src_shape->GetElementsExcludingPadding() == dst_shape->GetElementsExcludingPadding()) {
        memcpy(dst, src, src_shape->GetBytesIncludingPadding());
        return ppl::common::RC_SUCCESS;
    }
    if (src_shape->GetDimCount() > PPL_RISCV_TENSOR_MAX_DIMS()) {
        return ppl::common::RC_UNSUPPORTED;
    }

    int32_t real_axes[PPL_RISCV_TENSOR_MAX_DIMS()] = {0};
    // sort axes
    for (int64_t i = 0; i < num_axes; i++) {
        real_axes[i] = axes[i] >= 0 ? axes[i] : axes[i] + src_shape->GetDimCount();
    }
    std::sort(real_axes, real_axes + num_axes);

    if (src_shape->GetDataFormat() == ppl::common::DATAFORMAT_NDARRAY) {
        return reduce_ndarray_int64<op>(src, dst, src_shape, dst_shape, real_axes, num_axes);
    } else if (src_shape->GetDataFormat() == ppl::common::DATAFORMAT_N2CX) {
        return reduce_n2cx_int64<op>(src, dst, src_shape, dst_shape, real_axes, num_axes, 1);
    }

    return ppl::common::RC_UNSUPPORTED;
}

ppl::common::RetCode reduce_max_int64(
    const int64_t* src,
    int64_t* dst,

    const ppl::nn::TensorShape* src_shape,
    const ppl::nn::TensorShape* dst_shape,
    const int32_t* axes,
    const int32_t num_axes)
{
    return reduce_int64<REDUCE_MAX>(src, dst, src_shape, dst_shape, axes, num_axes);
}

ppl::common::RetCode reduce_min_int64(
    const int64_t* src,
    int64_t* dst,

    const ppl::nn::TensorShape* src_shape,
    const ppl::nn::TensorShape* dst_shape,
    const int32_t* axes,
    const int32_t num_axes)
{
    return reduce_int64<REDUCE_MIN>(src, dst, src_shape, dst_shape, axes, num_axes);
}

ppl::common::RetCode reduce_mean_int64(
    const int64_t* src,
    int64_t* dst,

    const ppl::nn::TensorShape* src_shape,
    const ppl::nn::TensorShape* dst_shape,
    const int32_t* axes,
    const int32_t num_axes)
{
    return reduce_int64<REDUCE_MEAN>(src, dst, src_shape, dst_shape, axes, num_axes);
}

ppl::common::RetCode reduce_sum_int64(
    const int64_t* src,
    int64_t* dst,

    const ppl::nn::TensorShape* src_shape,
    const ppl::nn::TensorShape* dst_shape,
    const int32_t* axes,
    const int32_t num_axes)
{
    return reduce_int64<REDUCE_SUM>(src, dst, src_shape, dst_shape, axes, num_axes);
}

}}}; //  namespace ppl::kernel::riscv
