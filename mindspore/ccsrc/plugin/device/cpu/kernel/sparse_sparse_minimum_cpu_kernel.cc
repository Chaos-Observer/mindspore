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

#include "plugin/device/cpu/kernel/sparse_sparse_minimum_cpu_kernel.h"
#include "plugin/device/cpu/hal/device/cpu_device_address.h"
#include "Eigen/Core"

namespace mindspore {
namespace kernel {
namespace {
constexpr int64_t kSparseSparseMinimumInputsNum = 6;
constexpr int64_t kSparseSparseMinimumOutputsNum = 2;
constexpr char kKernelName[] = "SparseSparseMinimum";
}  // namespace

void SparseSparseMinimumCpuKernelMod::CheckParam(const CNodePtr &kernel_node) const {
  int64_t input_num = static_cast<int64_t>(common::AnfAlgo::GetInputTensorNum(kernel_node));
  CHECK_KERNEL_INPUTS_NUM(input_num, kSparseSparseMinimumInputsNum, kKernelName);
  int64_t output_num = static_cast<int64_t>(common::AnfAlgo::GetOutputTensorNum(kernel_node));
  CHECK_KERNEL_OUTPUTS_NUM(output_num, kSparseSparseMinimumOutputsNum, kKernelName);
}

bool SparseSparseMinimumCpuKernelMod::Launch(const std::vector<kernel::AddressPtr> &inputs,
                                             const std::vector<kernel::AddressPtr> &,
                                             const std::vector<kernel::AddressPtr> &outputs) {
  if (dtype_ == kNumberTypeUInt8) {
    LaunchKernel<uint8_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeUInt16) {
    LaunchKernel<uint16_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeInt8) {
    LaunchKernel<int8_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeInt16) {
    LaunchKernel<int16_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeInt32) {
    LaunchKernel<int32_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeInt64) {
    LaunchKernel<int64_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeFloat16) {
    LaunchKernel<Eigen::half>(inputs, outputs);
  } else if (dtype_ == kNumberTypeFloat32) {
    LaunchKernel<float_t>(inputs, outputs);
  } else if (dtype_ == kNumberTypeFloat64) {
    LaunchKernel<double>(inputs, outputs);
  } else {
    MS_LOG(EXCEPTION) << "For SparseSparseMinimum, data type is " << TypeIdLabel(dtype_) << " which is not supported.";
  }
  auto node_ = node_wpt_.lock();
  if (!node_) {
    MS_LOG(EXCEPTION) << "For SparseSparseMinimum, node_wpt_ is expired. Error no: " << node_;
  }
  int64_t output_nm = static_cast<int64_t>(common::AnfAlgo::GetOutputTensorNum(node_));
  std::vector<TypeId> dtypes(output_nm);
  for (size_t i = 0; i < static_cast<size_t>(output_nm); i++) {
    dtypes[i] = static_cast<TypeId>(AnfAlgo::GetOutputDeviceDataType(node_, i));
  }
  std::vector<int64_t> dims;
  (void)dims.emplace_back(y_nnz_);
  (void)dims.emplace_back(num_dims_);
  std::vector<int64_t> dim;
  (void)dim.emplace_back(y_nnz_);
  common::AnfAlgo::SetOutputInferTypeAndShape(dtypes, {dims, dim}, node_.get());

  return true;
}

void SparseSparseMinimumCpuKernelMod::InitKernel(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  CheckParam(kernel_node);
  constexpr size_t kzero = 0;
  constexpr size_t kone = 1;
  constexpr size_t kthree = 3;
  node_wpt_ = kernel_node;
  dtype_ = AnfAlgo::GetInputDeviceDataType(kernel_node, kone);
  auto x1_indices_shape = common::AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, kzero);
  auto x2_indices_shape = common::AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, kthree);
  x1_nnz_ = x1_indices_shape[0];
  x2_nnz_ = x2_indices_shape[0];
  num_dims_ = x1_indices_shape[1];
}

template <typename T>
void SparseSparseMinimumCpuKernelMod::LaunchKernel(const std::vector<kernel::AddressPtr> &inputs,
                                                   const std::vector<kernel::AddressPtr> &outputs) {
  auto x1_indices_addr = static_cast<int64_t *>(inputs[0]->addr);
  auto x1_values_addr = static_cast<T *>(inputs[1]->addr);
  auto x1_shape_addr = static_cast<int64_t *>(inputs[2]->addr);
  auto x2_indices_addr = static_cast<int64_t *>(inputs[3]->addr);
  auto x2_values_addr = static_cast<T *>(inputs[4]->addr);
  auto x2_shape_addr = static_cast<int64_t *>(inputs[5]->addr);
  auto y_indices_addr = static_cast<int64_t *>(outputs[0]->addr);
  auto y_values_addr = static_cast<T *>(outputs[1]->addr);

  for (size_t n = 0; n < static_cast<size_t>(num_dims_); n++) {
    if (x1_shape_addr[n] != x2_shape_addr[n]) {
      MS_EXCEPTION(ValueError) << "For SparseSparseMinimum, operands' shapes do not match.";
    }
  }

  std::vector<std::pair<bool, int64_t>> entries_to_copy;
  (void)entries_to_copy.reserve(static_cast<size_t>(x1_nnz_ + x2_nnz_));
  std::vector<T> out_values;
  size_t i = 0, j = 0;
  T s;
  while (i < static_cast<size_t>(x1_nnz_) && j < static_cast<size_t>(x2_nnz_)) {
    int64_t index_cmp = 0;
    for (size_t n = 0; n < static_cast<size_t>(num_dims_); n++) {
      if (x1_indices_addr[i * static_cast<size_t>(num_dims_) + n] <
          x2_indices_addr[j * static_cast<size_t>(num_dims_) + n]) {
        index_cmp = -1;
        break;
      }
      if (x1_indices_addr[i * static_cast<size_t>(num_dims_) + n] >
          x2_indices_addr[j * static_cast<size_t>(num_dims_) + n]) {
        index_cmp = 1;
        break;
      }
    }
    switch (index_cmp) {
      case -1:
        s = std::min(x1_values_addr[i], static_cast<T>(0));
        (void)entries_to_copy.emplace_back(true, i);
        out_values.push_back(s);
        ++i;
        break;
      case 0:
        s = std::min(x1_values_addr[i], x2_values_addr[j]);
        (void)entries_to_copy.emplace_back(true, i);
        (void)out_values.push_back(s);
        ++i;
        ++j;
        break;
      case 1:
        s = std::min(static_cast<T>(0), x2_values_addr[j]);
        (void)entries_to_copy.emplace_back(false, j);
        (void)out_values.push_back(s);
        ++j;
        break;
      default:
        MS_EXCEPTION(ValueError) << "For SparseSparseMinimum, some inner errors happen in the computation.";
    }
  }

#define HANDLE_LEFTOVERS(X1_OR_X2, IDX, IS_A)                       \
  do {                                                              \
    while (IDX < static_cast<size_t>(X1_OR_X2##_nnz_)) {            \
      s = std::min(X1_OR_X2##_values_addr[IDX], static_cast<T>(0)); \
      (void)entries_to_copy.emplace_back(IS_A, IDX);                \
      (void)out_values.push_back(s);                                \
      ++IDX;                                                        \
    }                                                               \
  } while (0)
  HANDLE_LEFTOVERS(x1, i, true);
  HANDLE_LEFTOVERS(x2, j, false);
#undef HANDLE_LEFTOVERS

  y_nnz_ = static_cast<int64_t>(out_values.size());
  for (size_t k = 0; k < static_cast<size_t>(y_nnz_); ++k) {
    const bool from_x1 = entries_to_copy[k].first;
    const int64_t idx = entries_to_copy[k].second;
    if (from_x1) {
      for (size_t n = 0; n < static_cast<size_t>(num_dims_); n++) {
        y_indices_addr[k * static_cast<size_t>(num_dims_) + n] =
          x1_indices_addr[static_cast<size_t>(idx * num_dims_) + n];
      }
    } else {
      for (size_t n = 0; n < static_cast<size_t>(num_dims_); n++) {
        y_indices_addr[k * static_cast<size_t>(num_dims_) + n] =
          x2_indices_addr[static_cast<size_t>(idx * num_dims_) + n];
      }
    }
  }

  for (size_t n = 0; n < static_cast<size_t>(y_nnz_); n++) {
    y_values_addr[n] = static_cast<T>(out_values[n]);
  }
}  // namespace kernel
#define ADD_KERNEL(t1, t2, t3, t4, t5, t6, t7, t8) \
  KernelAttr()                                     \
    .AddInputAttr(kNumberType##t1)                 \
    .AddInputAttr(kNumberType##t2)                 \
    .AddInputAttr(kNumberType##t3)                 \
    .AddInputAttr(kNumberType##t4)                 \
    .AddInputAttr(kNumberType##t5)                 \
    .AddInputAttr(kNumberType##t6)                 \
    .AddOutputAttr(kNumberType##t7)                \
    .AddOutputAttr(kNumberType##t8)

std::vector<KernelAttr> SparseSparseMinimumCpuKernelMod::GetOpSupport() {
  static std::vector<KernelAttr> kernel_attr_list = {
    ADD_KERNEL(Int64, UInt8, Int64, Int64, UInt8, Int64, Int64, UInt8),
    ADD_KERNEL(Int64, UInt16, Int64, Int64, UInt16, Int64, Int64, UInt16),
    ADD_KERNEL(Int64, Int8, Int64, Int64, Int8, Int64, Int64, Int8),
    ADD_KERNEL(Int64, Int16, Int64, Int64, Int16, Int64, Int64, Int16),
    ADD_KERNEL(Int64, Int32, Int64, Int64, Int32, Int64, Int64, Int32),
    ADD_KERNEL(Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64),
    ADD_KERNEL(Int64, Float16, Int64, Int64, Float16, Int64, Int64, Float16),
    ADD_KERNEL(Int64, Float32, Int64, Int64, Float32, Int64, Int64, Float32),
    ADD_KERNEL(Int64, Float64, Int64, Int64, Float64, Int64, Int64, Float64)};

  return kernel_attr_list;
}
MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, SparseSparseMinimum, SparseSparseMinimumCpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
