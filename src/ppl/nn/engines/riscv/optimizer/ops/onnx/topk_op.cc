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

#include "ppl/nn/engines/riscv/optimizer/ops/onnx/topk_op.h"
#include "ppl/nn/engines/riscv/kernels/onnx/topk_kernel.h"
#include "ppl/nn/oputils/onnx/reshape_topk.h"
#include "ppl/nn/common/logger.h"
using namespace std;
using namespace ppl::common;

namespace ppl { namespace nn { namespace riscv {

RetCode TopKOp::Init(const OptKernelOptions& options) {
    auto status = GenericLoadParam(options, &param_);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "load param failed: " << GetRetCodeStr(status);
        return status;
    }

    infer_dims_func_ = [this](InputOutputInfo* info) -> RetCode {
        return oputils::ReshapeTopK(info, param_.get());
    };

    infer_type_func_ = [](InputOutputInfo* info) -> void {
        GenericInferType(info);
        auto out_shape = info->GetOutput<TensorImpl>(1)->GetShape();
        out_shape->SetDataType(DATATYPE_INT64);
    };

    return RC_SUCCESS;
}

RetCode TopKOp::SelectFormat(const InputOutputInfo& info, vector<dataformat_t>* selected_input_formats,
                             vector<dataformat_t>* selected_output_formats) {
    selected_input_formats->at(0) = DATAFORMAT_NDARRAY;
    selected_input_formats->at(1) = DATAFORMAT_NDARRAY;
    selected_output_formats->at(0) = DATAFORMAT_NDARRAY;
    selected_output_formats->at(1) = DATAFORMAT_NDARRAY;

    return RC_SUCCESS;
}

RetCode TopKOp::SelectDataType(const InputOutputInfo& info, ppl::common::datatype_t forward_precision,
                               std::vector<datatype_t>* selected_input_data_types,
                               std::vector<datatype_t>* selected_output_data_types) {
    selected_input_data_types->at(1) = DATATYPE_INT64;
    selected_output_data_types->at(1) = DATATYPE_INT64;

    selected_input_data_types->at(0) = forward_precision;
    selected_output_data_types->at(0) = forward_precision;

    return RC_SUCCESS;
}

KernelImpl* TopKOp::CreateKernelImpl() const {
    return CreateKernelImplWithParam<TopKKernel>(param_.get());
}

}}} // namespace ppl::nn::riscv