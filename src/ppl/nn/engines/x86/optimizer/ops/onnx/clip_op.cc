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

#include "ppl/nn/engines/x86/optimizer/ops/onnx/clip_op.h"
#include "ppl/nn/engines/x86/kernels/onnx/clip_kernel.h"
#include "ppl/nn/common/logger.h"
using namespace std;
using namespace ppl::common;

namespace ppl { namespace nn { namespace x86 {

RetCode ClipOp::Init(const OptKernelOptions&) {
    infer_dims_func_ = GenericInferDims;
    infer_type_func_ = GenericInferType;
    return RC_SUCCESS;
}

RetCode ClipOp::SelectFormat(const InputOutputInfo& info, vector<dataformat_t>* selected_input_formats,
                             vector<dataformat_t>* selected_output_formats) {
    selected_input_formats->at(0) = info.GetInput<TensorImpl>(0)->GetShape()->GetDataFormat();
    selected_output_formats->at(0) = info.GetInput<TensorImpl>(0)->GetShape()->GetDataFormat();
    return RC_SUCCESS;
}

KernelImpl* ClipOp::CreateKernelImpl() const {
    return CreateKernelImplWithoutParam<ClipKernel>();
}

}}} // namespace ppl::nn::x86
