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

#ifndef _ST_HPC_PPL_NN_ENGINES_X86_OPTIMIZER_OPS_ONNX_CONV_OP_H_
#define _ST_HPC_PPL_NN_ENGINES_X86_OPTIMIZER_OPS_ONNX_CONV_OP_H_

#include "ppl/nn/params/onnx/conv_param.h"
#include "ppl/nn/engines/x86/params/conv_param.h"
#include "ppl/nn/engines/x86/optimizer/opt_kernel.h"

namespace ppl { namespace nn { namespace x86 {

class PostDepthwiseConvOp;
class ConvOp final : public X86OptKernel {
public:
    ConvOp(const ir::Node* node) : X86OptKernel(node), conv2d_param_(nullptr) {}

    ~ConvOp();
    ppl::common::RetCode Init(const OptKernelOptions& options) override;
    KernelImpl* CreateKernelImpl() const override;
    ppl::common::RetCode SelectFormat(const InputOutputInfo& info,
                                      std::vector<ppl::common::dataformat_t>* selected_input_formats,
                                      std::vector<ppl::common::dataformat_t>* selected_output_formats) override;
    ppl::common::RetCode SelectAlgorithm(const InputOutputInfo& info, const OptKernelOptions& options) override;
    ppl::common::RetCode OmitConstantsData(std::map<edgeid_t, int64_t> *constants_data_refcount) override;
    bool GetBiasTerm() { return bias_term_; };
    bool TryFuseReLU();
    bool TryFuseReLU6();
    bool TryFuseSum();

private:
    int32_t bias_term_ = 0;
    Conv2dParam* conv2d_param_;
    std::shared_ptr<ppl::nn::common::ConvParam> param_;

    friend PostDepthwiseConvOp;
};

}}} // namespace ppl::nn::x86

#endif
