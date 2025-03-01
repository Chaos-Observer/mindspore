/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_NON_MAX_SUPPRESSION_WITH_OVERLAPS_GPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_NON_MAX_SUPPRESSION_WITH_OVERLAPS_GPU_KERNEL_H_

#include <iostream>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "mindspore/core/abstract/utils.h"
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/device/gpu/kernel/kernel_constants.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
class NMSWithOverlapsFwdGpuKernelMod : public NativeGpuKernelMod {
 public:
  NMSWithOverlapsFwdGpuKernelMod() {
    KernelMod::kernel_name_ = "NonMaxSuppressionWithOverlaps";
    num_input_ = 0;
    num_output_ = 0;
    is_null_input_ = false;
    ceil_power_2 = 0;
    data_unit_size_ = 0;
    stream_ptr_ = nullptr;
  }
  ~NMSWithOverlapsFwdGpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  int Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
             const std::vector<KernelTensorPtr> &outputs,
             const std::map<uint32_t, tensor::TensorPtr> &inputsOnHost) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs, void *stream_ptr) override {
    return kernel_func_(this, inputs, workspace, outputs, stream_ptr);
  }

 protected:
  std::vector<KernelAttr> GetOpSupport() override;
  template <typename T>
  bool LaunchKernel(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
                    const std::vector<AddressPtr> &outputs, void *stream_ptr);
  using NMSWithOverlapsFunc =
    std::function<bool(NMSWithOverlapsFwdGpuKernelMod *, const std::vector<kernel::AddressPtr> &,
                       const std::vector<kernel::AddressPtr> &, const std::vector<kernel::AddressPtr> &, void *)>;
  NMSWithOverlapsFunc kernel_func_;
  static std::vector<std::pair<KernelAttr, NMSWithOverlapsFunc>> func_list_;
  void SyncData() override;
  std::vector<KernelTensorPtr> GetOutputs() override { return outputs_; }

 private:
  void ResetResource();
  void InitSizeLists();
  void *stream_ptr_;
  int num_input_;
  int num_output_;
  bool is_null_input_;
  // default values
  size_t ceil_power_2;
  size_t data_unit_size_; /* size of T */
  std::vector<KernelTensorPtr> outputs_ = {};
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_MATH_NON_MAX_SUPPRESSION_WITH_OVERLAPS_GPU_KERNEL_H_
