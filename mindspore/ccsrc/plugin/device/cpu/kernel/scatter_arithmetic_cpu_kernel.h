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

#ifndef MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_SCATTER_ARITHMETIC_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_SCATTER_ARITHMETIC_CPU_KERNEL_H_

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <utility>
#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/factory/ms_factory.h"
#include "kernel/common_utils.h"
namespace mindspore {
namespace kernel {
class ScatterArithmeticCpuKernelMod : public NativeCpuKernelMod,
                                      public MatchKernelHelper<ScatterArithmeticCpuKernelMod> {
 public:
  ScatterArithmeticCpuKernelMod() = default;
  ~ScatterArithmeticCpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

  int Resize(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
             const std::vector<KernelTensorPtr> &outputs, const std::map<uint32_t, tensor::TensorPtr> &) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs) override {
    return kernel_func_(this, inputs, workspace, outputs);
  }

  const std::vector<std::pair<KernelAttr, KernelRunFunc>> &GetFuncList() const override;

  std::vector<KernelAttr> GetOpSupport() override { return OpSupport(); }

 private:
  template <typename T>
  bool LaunchKernel(const std::vector<kernel::AddressPtr> &inputs, const std::vector<kernel::AddressPtr> &workspace,
                    const std::vector<kernel::AddressPtr> &outputs);

  using ScatterSupportListType = std::vector<std::pair<KernelAttr, ScatterArithmeticCpuKernelMod::KernelRunFunc>>;
  size_t input_size_{0};
  size_t inner_size_{0};
  size_t indices_size_{0};
  int first_dim_size_{0};
  bool use_embedding_cache_;
  int64_t parameter_key_{-1};
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PLUGIN_DEVICE_CPU_SCATTER_ARITHMETIC_CPU_KERNEL_H_
