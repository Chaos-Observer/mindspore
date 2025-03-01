/**
 * Copyright 2021-2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MINDSPORE_LITE_SRC_EXTENDRT_DELEGATE_TENSORRT_OP_ACTIVATION_TENSORRT_H_
#define MINDSPORE_LITE_SRC_EXTENDRT_DELEGATE_TENSORRT_OP_ACTIVATION_TENSORRT_H_
#include <string>
#include <vector>
#include "src/extendrt/delegate/tensorrt/op/tensorrt_op.h"

namespace mindspore::lite {
class ActivationTensorRT : public TensorRTOp {
 public:
  ActivationTensorRT(const BaseOperatorPtr &base_operator, const std::vector<TensorInfo> &in_tensors,
                     const std::vector<TensorInfo> &out_tensors, std::string name)
      : TensorRTOp(base_operator, in_tensors, out_tensors, name) {}

  ~ActivationTensorRT() override = default;

  int AddInnerOp(TensorRTContext *ctx) override;

  int IsSupport(const BaseOperatorPtr &base_operator, const std::vector<TensorInfo> &in_tensors,
                const std::vector<TensorInfo> &out_tensors) override;

  static nvinfer1::ILayer *AddActivation(
    TensorRTContext *ctx, ActivationType activation_type, float alpha, float min_value, float max_value,
    nvinfer1::ITensor *trt_in_tensor, const std::string &op_name, uint32_t device_id = 0,
    schema::QuantType quant_type = schema::QuantType_QUANT_NONE,
    RuntimePrecisionMode runtime_precision_mode = RuntimePrecisionMode::RuntimePrecisionMode_FP32);

 private:
  static nvinfer1::ILayer *AddHSwishActivation(TensorRTContext *ctx, nvinfer1::ITensor *trt_in_tensor,
                                               const std::string &op_name);
};
}  // namespace mindspore::lite
#endif  // MINDSPORE_LITE_SRC_EXTENDRT_DELEGATE_TENSORRT_OP_ACTIVATION_TENSORRT_H_
