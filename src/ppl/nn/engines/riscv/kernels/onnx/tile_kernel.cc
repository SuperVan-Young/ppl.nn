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

#include "ppl/nn/engines/riscv/kernels/onnx/tile_kernel.h"
#include "ppl/nn/engines/riscv/utils/macros.h"
#include "ppl/nn/common/logger.h"

#include "ppl/kernel/riscv/fp16/tile.h"
#include "ppl/kernel/riscv/fp32/tile.h"
#include "ppl/kernel/riscv/int64/tile.h"

namespace ppl { namespace nn { namespace riscv {

ppl::common::RetCode TileKernel::DoExecute(KernelExecContext* ctx) {
    PPLNN_RISCV_REQUIRED_INPUT(A, 0);
    PPLNN_RISCV_REQUIRED_INPUT(B, 1);
    PPLNN_RISCV_REQUIRED_OUTPUT(C, 0);

    PPLNN_RISCV_DEBUG_TRACE("Op: %s\n", GetName().c_str());
    PPLNN_RISCV_DEBUG_TRACE("Input [A]:\n");
    PPL_RISCV_TENSOR_PRINT_DEBUG_MSG(A);
    PPLNN_RISCV_DEBUG_TRACE("Input [B]:\n");
    PPL_RISCV_TENSOR_PRINT_DEBUG_MSG(B);

    PPLNN_RISCV_REALLOC_TENSOR_BUFFER(C);
    PPLNN_RISCV_DEBUG_TRACE("Output [C]:\n");
    PPL_RISCV_TENSOR_PRINT_DEBUG_MSG(C);

    const auto data_type = ctx->GetInput<TensorImpl>(0)->GetShape()->GetDataType();
    const auto data_format = ctx->GetInput<TensorImpl>(0)->GetShape()->GetDataFormat();
    if (data_format == ppl::common::DATAFORMAT_NDARRAY) {
        if (data_type == ppl::common::DATATYPE_FLOAT16) {
            return ppl::kernel::riscv::tile_ndarray_fp16(A->GetShape(), C->GetShape(), A->GetBufferPtr<const __fp16>(),
                                                         B->GetBufferPtr<const int64_t>(), C->GetBufferPtr<__fp16>());
        }
        if (data_type == ppl::common::DATATYPE_FLOAT32) {
            return ppl::kernel::riscv::tile_ndarray_fp32(A->GetShape(), C->GetShape(), A->GetBufferPtr<const float>(),
                                                         B->GetBufferPtr<const int64_t>(), C->GetBufferPtr<float>());
        } else if (data_type == ppl::common::DATATYPE_INT64) {
            return ppl::kernel::riscv::tile_ndarray_int64(A->GetShape(), C->GetShape(),
                                                          A->GetBufferPtr<const int64_t>(),
                                                          B->GetBufferPtr<const int64_t>(), C->GetBufferPtr<int64_t>());
        } else {
            LOG(ERROR) << "unsupported data type " << ppl::common::GetDataTypeStr(data_type) << ".";
        }
    } else {
        LOG(ERROR) << "unsupported data format " << ppl::common::GetDataFormatStr(data_format) << ".";
    }

    return ppl::common::RC_UNSUPPORTED;
}

}}} // namespace ppl::nn::riscv