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

#include "plugin/device/cpu/kernel/maximum_grad_grad_cpu_kernel.h"
#include <algorithm>
#include "plugin/device/cpu/hal/device/cpu_device_address.h"

namespace mindspore {
namespace kernel {
namespace {
constexpr size_t kMaximumGradGradInputsNum = 4;
constexpr size_t kMaximumGradGradOutputsNum = 3;
constexpr size_t kInputIndex0 = 0;
constexpr size_t kInputIndex1 = 1;
constexpr size_t kInputIndex2 = 2;
constexpr size_t kInputIndex3 = 3;
constexpr size_t kOutputIndex0 = 0;
constexpr size_t kOutputIndex1 = 1;
constexpr size_t kOutputIndex2 = 2;
}  // namespace

void MaximumGradGradCpuKernelMod::InitKernel(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  kernel_name_ = common::AnfAlgo::GetCNodeName(kernel_node);
  x1_shape_ = AnfAlgo::GetInputDeviceShape(kernel_node, kInputIndex0);
  x2_shape_ = AnfAlgo::GetInputDeviceShape(kernel_node, kInputIndex1);
  grad_y1_shape_ = AnfAlgo::GetInputDeviceShape(kernel_node, kInputIndex2);
  grad_y2_shape_ = AnfAlgo::GetInputDeviceShape(kernel_node, kInputIndex3);
  if (x1_shape_ != grad_y1_shape_) {
    MS_LOG(EXCEPTION) << "For MaximumGradGrad, the shape of x1 should be same with that of grad_y1. But got x1: "
                      << x1_shape_ << " and grad_y1: " << grad_y1_shape_ << ".";
  }
  if (x2_shape_ != grad_y2_shape_) {
    MS_LOG(EXCEPTION) << "For MaximumGradGrad, the shape of x2 should be same with that of grad_y2. But got x2: "
                      << x2_shape_ << " and grad_y2: " << grad_y2_shape_ << ".";
  }
  output_shape_ = CPUKernelUtils::GetBroadcastShape(x1_shape_, x2_shape_);
  for (const uint64_t &d : output_shape_) {
    tensor_size_ *= d;
  }

  auto kernel_attr = GetKernelAttrFromNode(kernel_node);
  auto [is_match, index] = MatchKernelAttr(kernel_attr, GetOpSupport());
  if (!is_match) {
    MS_LOG(EXCEPTION) << "For MaximumGradGrad, kernel data type '" << kernel_attr << "' is not supported.";
  }
  kernel_func_ = func_list_[index].second;
}

template <typename T>
bool MaximumGradGradCpuKernelMod::LaunchKernel(const std::vector<kernel::AddressPtr> &inputs,
                                               const std::vector<kernel::AddressPtr> &outputs) {
  CHECK_KERNEL_INPUTS_NUM(inputs.size(), kMaximumGradGradInputsNum, kernel_name_);
  CHECK_KERNEL_OUTPUTS_NUM(outputs.size(), kMaximumGradGradOutputsNum, kernel_name_);

  auto x1_addr = reinterpret_cast<T *>(inputs[kInputIndex0]->addr);
  auto x2_addr = reinterpret_cast<T *>(inputs[kInputIndex1]->addr);
  auto grad_y1_addr = reinterpret_cast<T *>(inputs[kInputIndex2]->addr);
  auto grad_y2_addr = reinterpret_cast<T *>(inputs[kInputIndex3]->addr);
  auto sopd_x1_addr = reinterpret_cast<T *>(outputs[kOutputIndex0]->addr);
  auto sopd_x2_addr = reinterpret_cast<T *>(outputs[kOutputIndex1]->addr);
  auto sopd_grads_addr = reinterpret_cast<T *>(outputs[kOutputIndex2]->addr);

  auto ret_sopd_x1 = memset_s(sopd_x1_addr, 1, 0, 1);
  if (ret_sopd_x1 != EOK) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', memset output[0] failed. Error no: " << ret_sopd_x1;
  }
  auto ret_sopd_x2 = memset_s(sopd_x2_addr, 1, 0, 1);
  if (ret_sopd_x2 != EOK) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', memset output[1] failed. Error no: " << ret_sopd_x2;
  }
  auto ret_sopd_grads = memset_s(sopd_grads_addr, tensor_size_, 0, tensor_size_);
  if (ret_sopd_grads != EOK) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', memset output[2] failed. Error no: " << ret_sopd_grads;
  }
  if (x1_shape_ == x2_shape_) {
    auto task = [this, &x1_addr, &x2_addr, &grad_y1_addr, &grad_y2_addr, &sopd_grads_addr](size_t start, size_t end) {
      for (uint64_t i = start; i < end; ++i) {
        if (x1_addr[i] >= x2_addr[i]) {
          sopd_grads_addr[i] = grad_y1_addr[i];
        } else {
          sopd_grads_addr[i] = grad_y2_addr[i];
        }
      }
    };
    ParallelLaunchAutoSearch(task, tensor_size_, this, &parallel_search_info_);
  } else {
    BroadcastIterator base_iter(x1_shape_, x2_shape_, output_shape_);
    auto task = [&x1_addr, &x2_addr, &grad_y1_addr, &grad_y2_addr, &sopd_grads_addr, &base_iter](size_t start,
                                                                                                 size_t end) {
      auto iter = base_iter;
      iter.SetPos(start);
      for (uint64_t i = start; i < end; ++i) {
        if (x1_addr[iter.GetInputPosA()] >= x2_addr[iter.GetInputPosB()]) {
          sopd_grads_addr[i] = grad_y1_addr[iter.GetInputPosA()];
        } else {
          sopd_grads_addr[i] = grad_y2_addr[iter.GetInputPosB()];
        }
        iter.GenNextPos();
      }
    };
    output_size_ = 1;
    for (int64_t i = 0; i < static_cast<int64_t>(output_shape_.size()); ++i) {
      output_size_ *= output_shape_[i];
    }
    ParallelLaunchAutoSearch(task, output_size_, this, &parallel_search_info_);
  }
  return true;
}

std::vector<std::pair<KernelAttr, MaximumGradGradCpuKernelMod::MaximumGradGradCPUKernelFunc>>
  MaximumGradGradCpuKernelMod::func_list_ = {{KernelAttr()
                                                .AddInputAttr(kNumberTypeFloat32)
                                                .AddInputAttr(kNumberTypeFloat32)
                                                .AddInputAttr(kNumberTypeFloat32)
                                                .AddInputAttr(kNumberTypeFloat32)
                                                .AddOutputAttr(kNumberTypeFloat32)
                                                .AddOutputAttr(kNumberTypeFloat32)
                                                .AddOutputAttr(kNumberTypeFloat32),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<float>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeInt32)
                                                .AddInputAttr(kNumberTypeInt32)
                                                .AddInputAttr(kNumberTypeInt32)
                                                .AddInputAttr(kNumberTypeInt32)
                                                .AddOutputAttr(kNumberTypeInt32)
                                                .AddOutputAttr(kNumberTypeInt32)
                                                .AddOutputAttr(kNumberTypeInt32),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<int>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeUInt32)
                                                .AddInputAttr(kNumberTypeUInt32)
                                                .AddInputAttr(kNumberTypeUInt32)
                                                .AddInputAttr(kNumberTypeUInt32)
                                                .AddOutputAttr(kNumberTypeUInt32)
                                                .AddOutputAttr(kNumberTypeUInt32)
                                                .AddOutputAttr(kNumberTypeUInt32),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<uint32_t>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeInt64)
                                                .AddInputAttr(kNumberTypeInt64)
                                                .AddInputAttr(kNumberTypeInt64)
                                                .AddInputAttr(kNumberTypeInt64)
                                                .AddOutputAttr(kNumberTypeInt64)
                                                .AddOutputAttr(kNumberTypeInt64)
                                                .AddOutputAttr(kNumberTypeInt64),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<int64_t>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeUInt64)
                                                .AddInputAttr(kNumberTypeUInt64)
                                                .AddInputAttr(kNumberTypeUInt64)
                                                .AddInputAttr(kNumberTypeUInt64)
                                                .AddOutputAttr(kNumberTypeUInt64)
                                                .AddOutputAttr(kNumberTypeUInt64)
                                                .AddOutputAttr(kNumberTypeUInt64),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<uint64_t>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeFloat16)
                                                .AddInputAttr(kNumberTypeFloat16)
                                                .AddInputAttr(kNumberTypeFloat16)
                                                .AddInputAttr(kNumberTypeFloat16)
                                                .AddOutputAttr(kNumberTypeFloat16)
                                                .AddOutputAttr(kNumberTypeFloat16)
                                                .AddOutputAttr(kNumberTypeFloat16),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<float16>},
                                             {KernelAttr()
                                                .AddInputAttr(kNumberTypeFloat64)
                                                .AddInputAttr(kNumberTypeFloat64)
                                                .AddInputAttr(kNumberTypeFloat64)
                                                .AddInputAttr(kNumberTypeFloat64)
                                                .AddOutputAttr(kNumberTypeFloat64)
                                                .AddOutputAttr(kNumberTypeFloat64)
                                                .AddOutputAttr(kNumberTypeFloat64),
                                              &MaximumGradGradCpuKernelMod::LaunchKernel<double>}};

std::vector<KernelAttr> MaximumGradGradCpuKernelMod::GetOpSupport() {
  std::vector<KernelAttr> support_list;
  (void)std::transform(func_list_.begin(), func_list_.end(), std::back_inserter(support_list),
                       [](const std::pair<KernelAttr, MaximumGradGradCPUKernelFunc> &pair) { return pair.first; });
  return support_list;
}

MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, MaximumGradGrad, MaximumGradGradCpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
