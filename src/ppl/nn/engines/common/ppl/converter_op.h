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

#ifndef _ST_HPC_PPL_NN_ENGINES_COMMON_PPL_CONVERTER_OP_H_
#define _ST_HPC_PPL_NN_ENGINES_COMMON_PPL_CONVERTER_OP_H_

#include "ppl/nn/runtime/opt_kernel.h"
#include "ppl/nn/engines/common/ppl/converter_kernel.h"

namespace ppl { namespace nn { namespace common {

class ConverterOp final : public OptKernel {
public:
    ConverterOp(const ir::Node* node) : OptKernel(node) {}
    KernelImpl* CreateKernelImpl() const override {
        return new ConverterKernel(GetNode());
    }
#ifdef PPLNN_ENABLE_PMX_MODEL
    ppl::common::RetCode SerializeData(const pmx::SerializationContext&, utils::DataStream*) const override {
        return ppl::common::RC_UNSUPPORTED;
    }
    ppl::common::RetCode DeserializeData(const pmx::DeserializationContext&, const void*, uint64_t) override {
        return ppl::common::RC_UNSUPPORTED;
    }
#endif
};

}}} // namespace ppl::nn::common

#endif
