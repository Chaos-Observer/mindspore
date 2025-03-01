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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_UNIQUE_WITH_PAD_GPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_UNIQUE_WITH_PAD_GPU_KERNEL_H_

#include <vector>
#include <memory>
#include <map>
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/factory/ms_factory.h"
#include "plugin/device/gpu/kernel/arrays/unique_gpu_kernel.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_class/unique_with_pad_helper.h"

namespace mindspore {
namespace kernel {
class UniqueWithPadGpuKernelMod : public UniqueGpuKernelMod {
 public:
  UniqueWithPadGpuKernelMod() {
    KernelMod::kernel_name_ = "UniqueWithPad";
    ResetResource();
  }
  ~UniqueWithPadGpuKernelMod() override = default;

  bool Init(const BaseOperatorPtr &base_operator, const std::vector<KernelTensorPtr> &inputs,
            const std::vector<KernelTensorPtr> &outputs) override;

 protected:
  void SyncData() override{};
  std::vector<KernelAttr> GetOpSupport() override;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_UNIQUE_WITH_PAD_GPU_KERNEL_H_
