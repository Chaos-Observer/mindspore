/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_SPARSE_GATHERV2_GPU_KERNEL_H_
#define MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_SPARSE_GATHERV2_GPU_KERNEL_H_

#include <vector>
#include <algorithm>
#include "plugin/device/gpu/kernel/gpu_kernel.h"
#include "plugin/device/gpu/kernel/gpu_kernel_factory.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/gatherv2.cuh"
#include "backend/common/session/anf_runtime_algorithm.h"
#include "include/common/utils/anfalgo.h"

namespace mindspore {
namespace kernel {
template <typename T, typename S, typename G>
class SparseGatherV2FwdGpuKernelMod : public DeprecatedNativeGpuKernelMod {
 public:
  SparseGatherV2FwdGpuKernelMod() { ResetResource(); }
  ~SparseGatherV2FwdGpuKernelMod() = default;

  bool Launch(const std::vector<AddressPtr> &inputs, const std::vector<AddressPtr> &workspace,
              const std::vector<AddressPtr> &outputs, void *stream_ptr) override {
    if (is_null_input_) {
      return true;
    }
    VARIABLE_NOT_USED(workspace);
    T *input_addr = GetDeviceAddress<T>(inputs, 0);
    S *indices_addr = GetDeviceAddress<S>(inputs, 1);
    T *output_addr = GetDeviceAddress<T>(outputs, 0);
    if (is_dynamic_shape_) {
      G *axis_device_address = GetDeviceAddress<G>(inputs, 2);  // only get this if in dynamic mode
      CHECK_CUDA_RET_WITH_EXCEPT(kernel_node_,
                                 cudaMemcpyAsync(&axis_, axis_device_address, sizeof(G), cudaMemcpyDeviceToHost,
                                                 reinterpret_cast<cudaStream_t>(stream_ptr)),
                                 "cudaMemcpyAsync axis_ failed");
      CHECK_CUDA_RET_WITH_EXCEPT(kernel_node_, cudaDeviceSynchronize(),
                                 "cudaDeviceSyncFailed - GatherV2 - in dynamic mode");
      Reshape();
    }
    auto input_dim1 = input_shapes_[axis_];

    MS_EXCEPTION_IF_NULL(input_addr);
    MS_EXCEPTION_IF_NULL(indices_addr);
    GatherV2(input_addr, indices_addr, output_addr, dims_[0], dims_[1], dims_[2], LongToSize(input_dim1),
             reinterpret_cast<cudaStream_t>(stream_ptr));
    return true;
  }
  bool Init(const CNodePtr &kernel_node) override {
    auto kernel_name = common::AnfAlgo::GetCNodeName(kernel_node);
    kernel_node_ = kernel_node;
    InitResource();
    size_t input_num = common::AnfAlgo::GetInputTensorNum(kernel_node);
    if (input_num == kSizeThree) {
      is_dynamic_shape_ = true;
      MS_LOG(INFO) << " GatherGpuV2FwdKernel running in Dynamic Mode.";
    } else if (input_num == kSizeTwo) {
      MS_LOG(INFO) << " GatherGpuV2FwdKernel running in Normal Mode.";
    } else {
      MS_LOG(EXCEPTION) << "For '" << kernel_name << "', the number of inputs must be 2 or 3, but got " << input_num;
    }
    input_shapes_ = AnfAlgo::GetInputDeviceShapeAdaptively(kernel_node, 0);
    indices_shapes_ = AnfAlgo::GetInputDeviceShapeAdaptively(kernel_node, 1);
    output_shapes_ = AnfAlgo::GetOutputDeviceShapeAdaptively(kernel_node, 0);
    is_null_input_ = CHECK_SHAPE_NULL(input_shapes_, kernel_name, "input") ||
                     CHECK_SHAPE_NULL(indices_shapes_, kernel_name, "indices") ||
                     CHECK_SHAPE_NULL(output_shapes_, kernel_name, "output");
    if (is_null_input_) {
      InitSizeLists();
      return true;
    }
    if (!is_dynamic_shape_) {
      int dims = SizeToInt(input_shapes_.size());
      axis_ = static_cast<G>(GetAttr<int64_t>(kernel_node, "axis"));
      if (axis_ < -dims || axis_ >= dims) {
        MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', the 'axis' must be in the range [-" << dims << "," << dims
                          << "), but got " << axis_;
      }
      Reshape();
    }
    InitSizeLists();
    return true;
  }
  void ResetResource() noexcept override {
    is_dynamic_shape_ = false;
    input_shapes_.clear();
    indices_shapes_.clear();
    output_shapes_.clear();
    std::fill(dims_, dims_ + kSizeThree, 0);
    axis_ = 0;
    is_null_input_ = false;
    input_size_list_.clear();
    output_size_list_.clear();
    workspace_size_list_.clear();
  }

 protected:
  void InitSizeLists() override {
    size_t size = common::AnfAlgo::TensorSizeInByte<T>(input_shapes_);
    input_size_list_.push_back(size);
    size = common::AnfAlgo::TensorSizeInByte<T>(indices_shapes_);
    input_size_list_.push_back(size);
    if (is_dynamic_shape_) {
      input_size_list_.push_back(sizeof(G));
    }
    size = common::AnfAlgo::TensorSizeInByte<T>(output_shapes_);
    output_size_list_.push_back(size);
  }

 private:
  void Reshape() {
    if (axis_ < 0) {
      axis_ = axis_ + SizeToInt(input_shapes_.size());
    }
    int64_t dim_before_axis = 1;
    for (size_t i = 0; i < std::min(IntToSize(axis_), output_shapes_.size()); i++) {
      dim_before_axis *= output_shapes_[i];
    }
    int64_t dim_of_indices = 1;
    for (size_t i = 0; i < indices_shapes_.size(); i++) {
      dim_of_indices *= indices_shapes_[i];
    }
    int64_t dim_after_indices = 1;
    for (size_t i = IntToSize(axis_) + indices_shapes_.size(); i < output_shapes_.size(); i++) {
      dim_after_indices *= output_shapes_[i];
    }
    dims_[kIndex0] = dim_before_axis;
    dims_[kIndex1] = dim_of_indices;
    dims_[kIndex2] = dim_after_indices;
    return;
  }

  std::vector<int64_t> input_shapes_;
  std::vector<int64_t> indices_shapes_;
  std::vector<int64_t> output_shapes_;
  int64_t dims_[kIndex3] = {};
  G axis_;
  bool is_dynamic_shape_;
  bool is_null_input_;
};
}  // namespace kernel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_BACKEND_KERNEL_COMPILER_GPU_ARRAYS_SPARSE_GATHERV2_GPU_KERNEL_H_
