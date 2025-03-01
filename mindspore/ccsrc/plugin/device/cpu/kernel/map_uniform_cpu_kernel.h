/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_MAP_UNIFORM_CPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_MAP_UNIFORM_CPU_KERNEL_H_

#include <math.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "plugin/device/cpu/kernel/cpu_kernel.h"
#include "plugin/factory/ms_factory.h"

namespace mindspore {
namespace kernel {
class MapUniformCpuKernelMod : public DeprecatedNativeCpuKernelMod {
 public:
  MapUniformCpuKernelMod() = default;
  ~MapUniformCpuKernelMod() override = default;

  void InitKernel(const CNodePtr &kernel_node) override;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs) override;

  std::vector<KernelAttr> GetOpSupport() override {
    static const std::vector<KernelAttr> support_list = {KernelAttr()
                                                           .AddInputAttr(kNumberTypeInt32)
                                                           .AddInputAttr(kNumberTypeInt32)
                                                           .AddInputAttr(kNumberTypeInt32)
                                                           .AddOutputAttr(kNumberTypeInt32),
                                                         KernelAttr()
                                                           .AddInputAttr(kNumberTypeInt64)
                                                           .AddInputAttr(kNumberTypeInt64)
                                                           .AddInputAttr(kNumberTypeInt64)
                                                           .AddOutputAttr(kNumberTypeInt64)};
    return support_list;
  }

 private:
  template <typename T>
  void LaunchKernel(const std::vector<AddressPtr> &inputs, const std::vector<kernel::AddressPtr> &outputs);

  size_t batch_size_{1};
  TypeId dtype_{kTypeUnknown};
  CNodeWeakPtr node_wpt_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_CPU_MAP_UNIFORM_CPU_KERNEL_H_
