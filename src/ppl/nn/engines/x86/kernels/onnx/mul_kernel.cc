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

#include "ppl/nn/engines/x86/kernels/onnx/mul_kernel.h"
#include "ppl/nn/common/logger.h"
#include "ppl/nn/engines/x86/utils.h"

#include "ppl/kernel/x86/fp32/arithmetic.h"
#include "ppl/kernel/x86/int64/arithmetic.h"
#include "ppl/kernel/x86/int32/arithmetic.h"

namespace ppl { namespace nn { namespace x86 {

ppl::common::RetCode MulKernel::DoExecute(KernelExecContext* ctx) {
    PPLNN_X86_REQUIRED_INPUT(A, 0);
    PPLNN_X86_REQUIRED_INPUT(B, 1);
    PPLNN_X86_REQUIRED_OUTPUT(C, 0);

    PPLNN_X86_DEBUG_TRACE("Op: %s\n", GetName().c_str());

    PPLNN_X86_DEBUG_TRACE("Input [A]:\n");
    PPL_X86_TENSOR_PRINT_DEBUG_MSG(A);
    PPLNN_X86_DEBUG_TRACE("Input [B]:\n");
    PPL_X86_TENSOR_PRINT_DEBUG_MSG(B);

    PPLNN_X86_DEBUG_TRACE("isa: %u\n", GetISA());

    const common::datatype_t data_type = ctx->GetOutput<TensorImpl>(0)->GetShape()->GetDataType();

    if (fuse_relu_ && data_type != common::DATATYPE_FLOAT32) {
        LOG(ERROR) << "fuse relu support DATATYPE_FLOAT32 only.";
        return ppl::common::RC_UNSUPPORTED;
    }

    auto lA = A;
    auto lB = B;
    if (A->GetEdge()->CalcConsumerCount() == 1 && A->GetType() == TENSORTYPE_NORMAL && TensorShapeEqual(*A->GetShape(), *C->GetShape())) {
        C->TransferBufferFrom(A);
        lA = C;
    } else if (B->GetEdge()->CalcConsumerCount() == 1 && B->GetType() == TENSORTYPE_NORMAL && TensorShapeEqual(*B->GetShape(), *C->GetShape())) {
        C->TransferBufferFrom(B);
        lB = C;
    } else {
        PPLNN_X86_REALLOC_TENSOR_BUFFER(C);
    }

    PPLNN_X86_DEBUG_TRACE("Output [C]:\n");
    PPL_X86_TENSOR_PRINT_DEBUG_MSG(C);

    if (data_type == common::DATATYPE_FLOAT32) {
        if (MayUseISA(ppl::common::ISA_X86_AVX)) {
            return kernel::x86::mul_fp32_avx(A->GetShape(), B->GetShape(), C->GetShape(),
                                             lA->GetBufferPtr<const float>(), lB->GetBufferPtr<const float>(), fuse_relu_,
                                             C->GetBufferPtr<float>());
        } else if (MayUseISA(ppl::common::ISA_X86_SSE)) {
            return kernel::x86::mul_fp32_sse(A->GetShape(), B->GetShape(), C->GetShape(),
                                             lA->GetBufferPtr<const float>(), lB->GetBufferPtr<const float>(), fuse_relu_,
                                             C->GetBufferPtr<float>());
        } else {
            LOG(ERROR) << "get unsupported isa " << GetISA();
            return ppl::common::RC_UNSUPPORTED;
        }
    } else if (data_type == common::DATATYPE_INT64) {
        return kernel::x86::mul_int64(A->GetShape(), B->GetShape(), C->GetShape(), lA->GetBufferPtr<const int64_t>(),
                                      lB->GetBufferPtr<const int64_t>(), C->GetBufferPtr<int64_t>());
    } else if (data_type == common::DATATYPE_INT32) {
        return kernel::x86::mul_int32(A->GetShape(), B->GetShape(), C->GetShape(), lA->GetBufferPtr<const int32_t>(),
                                      lB->GetBufferPtr<const int32_t>(), C->GetBufferPtr<int32_t>());
    } else {
        LOG(ERROR) << "unsupported datatype " << common::GetDataTypeStr(data_type);
        return ppl::common::RC_UNSUPPORTED;
    }

    return ppl::common::RC_UNSUPPORTED;
}

}}} // namespace ppl::nn::x86
