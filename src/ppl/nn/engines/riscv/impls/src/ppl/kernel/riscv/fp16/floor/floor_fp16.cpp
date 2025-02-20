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

#include <math.h>
#include "ppl/kernel/riscv/common/internal_include.h"

namespace ppl { namespace kernel { namespace riscv {

ppl::common::RetCode floor_fp16(
    const ppl::nn::TensorShape* src_shape,
    const __fp16* src,
    __fp16* dst)
{
    const int64_t n_elem      = src_shape->GetElementsIncludingPadding();
    const int64_t unroll_n    = 16;
    const int64_t unroll_body = round(n_elem, unroll_n);

    for (int64_t i = 0; i < unroll_body; i += unroll_n) {
        dst[i + 0]  = floor((float)src[i + 0]);
        dst[i + 1]  = floor((float)src[i + 1]);
        dst[i + 2]  = floor((float)src[i + 2]);
        dst[i + 3]  = floor((float)src[i + 3]);
        dst[i + 4]  = floor((float)src[i + 4]);
        dst[i + 5]  = floor((float)src[i + 5]);
        dst[i + 6]  = floor((float)src[i + 6]);
        dst[i + 7]  = floor((float)src[i + 7]);
        dst[i + 8]  = floor((float)src[i + 8]);
        dst[i + 9]  = floor((float)src[i + 9]);
        dst[i + 10] = floor((float)src[i + 10]);
        dst[i + 11] = floor((float)src[i + 11]);
        dst[i + 12] = floor((float)src[i + 12]);
        dst[i + 13] = floor((float)src[i + 13]);
        dst[i + 14] = floor((float)src[i + 14]);
        dst[i + 15] = floor((float)src[i + 15]);
    }
    for (int64_t i = unroll_body; i < n_elem; i++) {
        dst[i] = floor((float)src[i]);
    }

    return ppl::common::RC_SUCCESS;
}

}}}; // namespace ppl::kernel::riscv