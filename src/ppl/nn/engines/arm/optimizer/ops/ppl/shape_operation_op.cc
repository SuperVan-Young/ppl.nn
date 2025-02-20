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

#include "ppl/nn/engines/arm/optimizer/ops/ppl/shape_operation_op.h"

#include "ppl/nn/common/logger.h"
#include "ppl/nn/engines/common/ppl/shape_operation_kernel.h"

using namespace std;
using namespace ppl::common;

namespace ppl { namespace nn { namespace arm {

RetCode PPLShapeOperationOp::Init(const OptKernelOptions& options) {
    auto status = GenericLoadParam<ppl::nn::common::PPLShapeOperationParam>(options, &param_);
    if (status != RC_SUCCESS) {
        LOG(ERROR) << "load param failed: " << GetRetCodeStr(status);
        return status;
    }
    infer_type_func_ = GenericInferType;
    infer_dims_func_ = GenericInferDims;
    return RC_SUCCESS;
}

KernelImpl* PPLShapeOperationOp::CreateKernelImpl() const {
    auto kernel = op_.CreateKernelImpl();
    ((ppl::nn::common::PPLShapeOperationKernel*)kernel)->SetParam(param_.get());
    return kernel;
}

}}} // namespace ppl::nn::arm
