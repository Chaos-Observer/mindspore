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

#include "plugin/device/cpu/kernel/stridedslice_v2_cpu_kernel.h"
#include <utility>
#include <functional>
#include <algorithm>
#include <complex>
#include <unordered_map>
#include <map>
#include "include/common/thread_pool.h"
#include "plugin/device/cpu/hal/device/cpu_device_address.h"
#include "kernel/common_utils.h"
#include "nnacl/errorcode.h"

namespace mindspore {
namespace kernel {
namespace {
constexpr size_t index_one = 1;
constexpr size_t index_two = 2;
constexpr size_t index_three = 3;
constexpr size_t index_four = 4;
constexpr size_t index_five = 5;
constexpr size_t index_six = 6;
constexpr size_t index_seven = 7;
using complex64 = std::complex<float>;
using complex128 = std::complex<double>;
constexpr size_t kStridedSliceV2InputsNum = 1;
constexpr size_t kStridedSliceV2DynamicInputsNum = 4;
constexpr size_t kStridedSliceV2OutputsNum = 1;

void PadStridedSliceV2ComParameterTo8D(StridedSliceParameter *param) {
  int32_t begins[DIMENSION_8D];
  int32_t ends[DIMENSION_8D];
  int32_t strides[DIMENSION_8D];
  int32_t input_shape[DIMENSION_8D];
  int32_t i;
  for (i = 0; i < param->num_axes_; ++i) {
    begins[i] = param->begins_[i];
    ends[i] = MSMIN(param->ends_[i], param->in_shape_[i]);
    strides[i] = param->strides_[i];
    input_shape[i] = param->in_shape_[i];
  }
  for (i = param->num_axes_; i < param->in_shape_length_; ++i) {
    input_shape[i] = param->in_shape_[i];
    begins[i] = 0;
    ends[i] = param->in_shape_[i];
    strides[i] = 1;
  }
  int32_t real_index = param->in_shape_length_ - 1;
  for (i = DIMENSION_8D - 1; i >= 0; --i) {
    if (real_index >= 0) {
      param->begins_[i] = begins[real_index];
      param->ends_[i] = ends[real_index];
      param->strides_[i] = strides[real_index];
      param->in_shape_[i] = input_shape[real_index--];
    } else {
      param->begins_[i] = 0;
      param->ends_[i] = 1;
      param->strides_[i] = 1;
      param->in_shape_[i] = 1;
    }
  }
  param->num_axes_ = DIMENSION_8D;
  param->in_shape_length_ = DIMENSION_8D;
}

bool LoopContinue(int stride, int i, int end) { return stride > 0 ? i < end : i > end; }

template <typename T>
int DoStridedSliceV2Com(const void *in_data, void *out_data, StridedSliceParameter *param) {
  if (in_data == NULL || out_data == NULL || param == NULL) {
    return NNACL_NULL_PTR;
  }
  if (param->num_axes_ > DIMENSION_8D) {
    return NNACL_PARAM_INVALID;
  }
  int *begins = param->begins_;
  int *ends = param->ends_;
  int *strides = param->strides_;
  int *in_shape = param->in_shape_;
  if (param->num_axes_ < DIMENSION_8D) {
    PadStridedSliceV2ComParameterTo8D(param);
  }
  int dim_offset[DIMENSION_8D - 1];
  dim_offset[index_six] = in_shape[index_seven];
  dim_offset[index_five] = in_shape[index_six] * dim_offset[index_six];
  dim_offset[index_four] = in_shape[index_five] * dim_offset[index_five];
  dim_offset[index_three] = in_shape[index_four] * dim_offset[index_four];
  dim_offset[index_two] = in_shape[index_three] * dim_offset[index_three];
  dim_offset[1] = in_shape[index_two] * dim_offset[index_two];
  dim_offset[0] = in_shape[1] * dim_offset[1];
  size_t out_offset = 0;
  int32_t dim0, dim1, dim2, dim3, dim4, dim5, dim6, dim7;
  for (dim0 = begins[0]; LoopContinue(strides[0], dim0, ends[0]); dim0 += strides[0]) {
    for (dim1 = begins[1]; LoopContinue(strides[1], dim1, ends[1]); dim1 += strides[1]) {
      for (dim2 = begins[index_two]; LoopContinue(strides[index_two], dim2, ends[index_two]);
           dim2 += strides[index_two]) {
        for (dim3 = begins[index_three]; LoopContinue(strides[index_three], dim3, ends[index_three]);
             dim3 += strides[index_three]) {
          for (dim4 = begins[index_four]; LoopContinue(strides[index_four], dim4, ends[index_four]);
               dim4 += strides[index_four]) {
            for (dim5 = begins[index_five]; LoopContinue(strides[index_five], dim5, ends[index_five]);
                 dim5 += strides[index_five]) {
              for (dim6 = begins[index_six]; LoopContinue(strides[index_six], dim6, ends[index_six]);
                   dim6 += strides[index_six]) {
                for (dim7 = begins[index_seven]; LoopContinue(strides[index_seven], dim7, ends[index_seven]);
                     dim7 += strides[index_seven]) {
                  int32_t in_offset = dim0 * dim_offset[0] + dim1 * dim_offset[1] + dim2 * dim_offset[index_two] +
                                      dim3 * dim_offset[index_three] + dim4 * dim_offset[index_four] +
                                      dim5 * dim_offset[index_five] + dim6 * dim_offset[index_six] + dim7;
                  auto out_ptr = static_cast<T *>(out_data);
                  auto int_ptr = static_cast<const T *>(in_data);
                  out_ptr[out_offset] = int_ptr[in_offset];
                  out_offset++;
                }
              }
            }
          }
        }
      }
    }
  }
  return NNACL_OK;
}

static std::map<std::string, std::vector<KernelAttr>> support_list_map = {
  {kStridedSliceV2,
   {KernelAttr().AddInputAttr(kNumberTypeBool).AddOutputAttr(kNumberTypeBool),
    KernelAttr().AddInputAttr(kNumberTypeInt16).AddOutputAttr(kNumberTypeInt16),
    KernelAttr().AddInputAttr(kNumberTypeInt32).AddOutputAttr(kNumberTypeInt32),
    KernelAttr().AddInputAttr(kNumberTypeInt64).AddOutputAttr(kNumberTypeInt64),
    KernelAttr().AddInputAttr(kNumberTypeUInt16).AddOutputAttr(kNumberTypeUInt16),
    KernelAttr().AddInputAttr(kNumberTypeUInt32).AddOutputAttr(kNumberTypeUInt32),
    KernelAttr().AddInputAttr(kNumberTypeUInt64).AddOutputAttr(kNumberTypeUInt64),
    KernelAttr().AddInputAttr(kNumberTypeFloat16).AddOutputAttr(kNumberTypeFloat16),
    KernelAttr().AddInputAttr(kNumberTypeFloat32).AddOutputAttr(kNumberTypeFloat32),
    KernelAttr().AddInputAttr(kNumberTypeFloat64).AddOutputAttr(kNumberTypeFloat64),
    KernelAttr().AddInputAttr(kNumberTypeComplex64).AddOutputAttr(kNumberTypeComplex64),
    KernelAttr().AddInputAttr(kNumberTypeComplex128).AddOutputAttr(kNumberTypeComplex128),
    KernelAttr()
      .AddInputAttr(kNumberTypeBool)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeBool),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt8)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeInt8),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt16)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeInt16),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeInt32),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeInt64),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt8)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeUInt8),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt16)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeUInt16),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt32)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeUInt32),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeUInt64),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat16)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeFloat16),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat32)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeFloat32),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeFloat64),
    KernelAttr()
      .AddInputAttr(kNumberTypeComplex64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeComplex64),
    KernelAttr()
      .AddInputAttr(kNumberTypeComplex128)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt64)
      .AddOutputAttr(kNumberTypeComplex128),
    KernelAttr()
      .AddInputAttr(kNumberTypeBool)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeBool),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt8)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeInt8),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt16)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeInt16),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeInt32),
    KernelAttr()
      .AddInputAttr(kNumberTypeInt64)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeInt64),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt8)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeUInt8),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt16)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeUInt16),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeUInt32),
    KernelAttr()
      .AddInputAttr(kNumberTypeUInt64)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeUInt64),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat16)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeFloat16),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeFloat32),
    KernelAttr()
      .AddInputAttr(kNumberTypeFloat64)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeFloat64),
    KernelAttr()
      .AddInputAttr(kNumberTypeComplex64)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeComplex64),
    KernelAttr()
      .AddInputAttr(kNumberTypeComplex128)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddInputAttr(kNumberTypeInt32)
      .AddOutputAttr(kNumberTypeComplex128)}}};

template <typename T>
void ParseStrideSliceMasksST(const CNodePtr &kernel_node, std::vector<T> *begin, std::vector<T> *end,
                             std::vector<T> *stride, const ShapeVector &input_shape, size_t shape_dim_input,
                             size_t slice_len) {
  std::vector<T> &_begin_attr = *begin;
  std::vector<T> &_end_attr = *end;
  std::vector<T> &_stride_attr = *stride;

  auto begin_mask_int = common::AnfAlgo::GetNodeAttr<int64_t>(kernel_node, kAttrBeginMask);
  auto begin_mask = Dec2Bin(begin_mask_int);
  auto end_mask_int = common::AnfAlgo::GetNodeAttr<int64_t>(kernel_node, kAttrEndMask);
  auto end_mask = Dec2Bin(end_mask_int);
  auto ellipsis_mask_int = common::AnfAlgo::GetNodeAttr<int64_t>(kernel_node, kAttrEllipsisMask);
  auto ellipsis_mask = Dec2Bin(ellipsis_mask_int);
  auto new_axis_mask_int = common::AnfAlgo::GetNodeAttr<int64_t>(kernel_node, kAttrNewAxisMask);
  auto new_axis_mask = Dec2Bin(new_axis_mask_int);
  auto shrink_axis_mask_int = common::AnfAlgo::GetNodeAttr<int64_t>(kernel_node, kAttrShrinkAxisMask);
  auto shrink_axis_mask = Dec2Bin(shrink_axis_mask_int);
  size_t i = 0;
  size_t j = 0;
  std::vector<T> begin_new;
  std::vector<T> end_new;
  std::vector<T> stride_new;
  while (i < shape_dim_input || j < slice_len) {
    T begin_j;
    T end_j;
    T stride_j;
    if (j < slice_len) {
      begin_j = _begin_attr[j];
      end_j = _end_attr[j];
      stride_j = _stride_attr[j];
      if (begin_mask[j]) {
        begin_j = _stride_attr[j] < 0 ? static_cast<T>(input_shape[i]) - 1 : 0;
      }
      if (end_mask[j]) {
        end_j = _stride_attr[j] < 0 ? -1 : static_cast<T>(input_shape[i]);
      }
      if (ellipsis_mask[j]) {
        begin_j = 0;
        end_j = static_cast<T>(input_shape[i]);
        stride_j = 1;
      }
      if (new_axis_mask[j]) {
        j++;
        continue;
      }
      if (shrink_axis_mask[j]) {
        end_j = _begin_attr[j] + 1;
        stride_j = 1;
      }
      if (end_j > input_shape[i]) {
        end_j = input_shape[i];
      }
    } else {
      begin_j = 0;
      end_j = static_cast<T>(input_shape[i]);
      stride_j = 1;
    }
    begin_new.push_back(begin_j);
    end_new.push_back(end_j);
    stride_new.push_back(stride_j);
    i++;
    j++;
  }
  _begin_attr.assign(begin_new.begin(), begin_new.end());
  _end_attr.assign(end_new.begin(), end_new.end());
  _stride_attr.assign(stride_new.begin(), stride_new.end());
}

template <typename T>
void FillEmptyDimsST(const CNodePtr &kernel_node, std::vector<T> *begin, std::vector<T> *end, std::vector<T> *stride,
                     ShapeVector *input_shape) {
  std::vector<T> &_begin = *begin;
  std::vector<T> &_end = *end;
  std::vector<T> &_stride = *stride;
  auto &_input_shape = *input_shape;
  if (_begin.size() != _end.size() || _begin.size() != _stride.size() || _begin.size() > _input_shape.size()) {
    MS_LOG(EXCEPTION) << "For '" << common::AnfAlgo::GetCNodeName(kernel_node)
                      << "', the length of 'begin', 'stride' and 'end' should be equal "
                         "and less than or equal to the dimension of 'input_x', but got the length of 'begin': "
                      << _begin.size() << ", the length of 'stride': " << _stride.size()
                      << ", the length of 'end': " << _end.size()
                      << ", the dimension of 'input_x': " << _input_shape.size();
  }
  for (size_t i = 0; i < DIMENSION_8D; i++) {
    if (i >= _input_shape.size()) {
      _input_shape.push_back(1);
    }
    if (i < _begin.size()) {
      T dim = static_cast<T>(_input_shape[i]);
      _begin[i] = std::min(_begin[i] < 0 ? std::max(_begin[i] + dim, static_cast<T>(0)) : _begin[i], dim - 1);
    } else {
      _begin.push_back(0);
    }

    if (i >= _end.size()) {
      _end.push_back(i < _input_shape.size() ? static_cast<T>(_input_shape[i]) : 1);
    }

    if (i >= _stride.size()) {
      _stride.push_back(1);
    }
  }
}
}  // namespace

void StridedSliceV2CpuKernelMod::InitKernel(const CNodePtr &kernel_node) {
  MS_EXCEPTION_IF_NULL(kernel_node);
  kernel_name_ = common::AnfAlgo::GetCNodeName(kernel_node);
  cnode_ptr_ = kernel_node;
  input_shape_ = common::AnfAlgo::GetPrevNodeOutputInferShape(kernel_node, 0);
  output_shape_ = common::AnfAlgo::GetOutputInferShape(kernel_node, 0);
  dtype_ = AnfAlgo::GetInputDeviceDataType(kernel_node, 0);
  size_t input_num = common::AnfAlgo::GetInputTensorNum(kernel_node);
  if (input_num != 1) {
    return;
  }
  // for begin, end, stride are const input
  auto begin = common::AnfAlgo::GetNodeAttr<std::vector<int64_t>>(kernel_node, kAttrBegin);
  auto end = common::AnfAlgo::GetNodeAttr<std::vector<int64_t>>(kernel_node, kAttrEnd);
  auto stride = common::AnfAlgo::GetNodeAttr<std::vector<int64_t>>(kernel_node, kAttrStrides);
  InitSliceParam<int64_t>(kernel_node, &begin, &end, &stride);

  parallel_ = MatchParallelPattern();
  if (parallel_) {
    InitParallelParam();
  }
}

bool StridedSliceV2CpuKernelMod::MatchParallelPattern() {
  // This function is seeking if that the number of only one dimension
  // is different between input and output. If so, we can do some trick.
  // Example 1:
  // input shape info:  [1, 80, 46, 40]
  // output shape info: [1, 80, 20, 40]
  // Example 2:
  // input shape info:  [1, 46, 40]
  // output shape info: [1, 20, 40]
  if (input_shape_.size() == output_shape_.size()) {
    std::vector<int> axis_list;
    for (size_t i = 0; i < input_shape_.size(); ++i) {
      if (input_shape_[i] != output_shape_[i]) {
        (void)axis_list.emplace_back(i);
      }
    }
    if (axis_list.size() == 1) {
      split_axis_ = axis_list.front();
      return true;
    }
  }
  return false;
}

void StridedSliceV2CpuKernelMod::InitParallelParam() {
  outer_ = SizeToInt(
    std::accumulate(input_shape_.begin(), input_shape_.begin() + split_axis_, size_t(1), std::multiplies<size_t>()));
  inner_ = SizeToInt(
    std::accumulate(input_shape_.begin() + split_axis_ + 1, input_shape_.end(), size_t(1), std::multiplies<size_t>()));

  int max_thread_num = SizeToInt(common::ThreadPool::GetInstance().GetSyncRunThreadNum());
  int thread_num = 1;
  if (outer_ == 1) {
    parallel_strategy_ = kOnSplitAxis;
    thread_num = std::min(SizeToInt(output_shape_[split_axis_]), max_thread_num);
    if (thread_num == 0) {
      slice_param_.op_parameter_.thread_num_ = 1;
      return;
    }
    cal_num_per_thread_ = UP_DIV(output_shape_[split_axis_], thread_num);
  } else {
    parallel_strategy_ = kOnOuter;
    thread_num = std::min(outer_, max_thread_num);
    if (thread_num == 0) {
      slice_param_.op_parameter_.thread_num_ = 1;
      return;
    }
    cal_num_per_thread_ = UP_DIV(outer_, thread_num);
  }
  slice_param_.op_parameter_.thread_num_ = thread_num;
}

template <typename T>
void StridedSliceV2CpuKernelMod::InitSliceParam(const CNodePtr &kernel_node, std::vector<T> *begin, std::vector<T> *end,
                                                std::vector<T> *stride) {
  static const std::unordered_map<TypeId, std::pair<TypeIdC, int>> type_convert_map = {
    {kNumberTypeBool, {::kNumberTypeBool, sizeof(bool)}},
    {kNumberTypeInt8, {::kNumberTypeInt8, sizeof(int8_t)}},
    {kNumberTypeInt16, {::kNumberTypeInt16, sizeof(int16_t)}},
    {kNumberTypeInt32, {::kNumberTypeInt32, sizeof(int32_t)}},
    {kNumberTypeInt64, {::kNumberTypeInt64, sizeof(int64_t)}},
    {kNumberTypeUInt8, {::kNumberTypeUInt8, sizeof(uint8_t)}},
    {kNumberTypeUInt16, {::kNumberTypeUInt16, sizeof(uint16_t)}},
    {kNumberTypeUInt32, {::kNumberTypeUInt32, sizeof(uint32_t)}},
    {kNumberTypeUInt64, {::kNumberTypeUInt64, sizeof(uint64_t)}},
    {kNumberTypeFloat16, {::kNumberTypeFloat16, sizeof(float16)}},
    {kNumberTypeFloat32, {::kNumberTypeFloat32, sizeof(float)}},
    {kNumberTypeFloat64, {::kNumberTypeFloat64, sizeof(double)}},
    {kNumberTypeComplex64, {::kNumberTypeComplex64, sizeof(complex64)}},
    {kNumberTypeComplex128, {::kNumberTypeComplex64, sizeof(complex128)}}};

  auto type_pair = type_convert_map.find(dtype_);
  if (type_pair == type_convert_map.end()) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_
                      << "', the dtype of 'input_x' must be bool, int8, int16, int32, int64, float16, float32, "
                         "float64, uint8, uint16, uint32, uint64, complex64 or complex128, but got "
                      << TypeIdToType(dtype_)->ToString();
  }
  data_size_ = type_pair->second.second;
  slice_param_.data_type = type_pair->second.first;
  auto input_shape_pad = input_shape_;
  shape_dim_input = input_shape_.size();
  FillEmptyDimsST<T>(kernel_node, begin, end, stride, &input_shape_pad);
  ParseStrideSliceMasksST<T>(kernel_node, begin, end, stride, input_shape_, shape_dim_input, slice_len);
  FillEmptyDimsST<T>(kernel_node, begin, end, stride, &input_shape_pad);

  std::vector<T> &_begin = *begin;
  std::vector<T> &_end = *end;
  std::vector<T> &_stride = *stride;
  for (size_t i = 0; i < DIMENSION_8D; i++) {
    slice_param_.in_shape_[i] = SizeToInt(input_shape_pad[i]);
    if (dtype_attr == kNumberTypeInt64) {
      slice_param_.begins_[i] = LongToInt(_begin[i]);
      slice_param_.ends_[i] = LongToInt(_end[i]);
      slice_param_.strides_[i] = LongToInt(_stride[i]);
    } else {
      slice_param_.begins_[i] = _begin[i];
      slice_param_.ends_[i] = _end[i];
      slice_param_.strides_[i] = _stride[i];
    }
  }
  slice_param_.in_shape_length_ = DIMENSION_8D;
  slice_param_.num_axes_ = DIMENSION_8D;
}

common::Status StridedSliceV2CpuKernelMod::RunTaskOnOuter(const uint8_t *input_addr, uint8_t *output_addr,
                                                          int start_pos) {
  int begin_index = slice_param_.begins_[split_axis_];
  int inner_size = inner_ * data_size_;
  const uint8_t *cur_in_ptr = input_addr + (start_pos * input_shape_[split_axis_] + begin_index) * inner_size;
  uint8_t *cur_out_ptr = output_addr + start_pos * output_shape_[split_axis_] * inner_size;
  int cur_outer = outer_ - start_pos;
  if (cur_outer <= 0) {
    return common::SUCCESS;
  }
  cur_outer = cur_outer > cal_num_per_thread_ ? cal_num_per_thread_ : cur_outer;
  FastStride(cur_in_ptr, cur_out_ptr, output_shape_[split_axis_], slice_param_.strides_[split_axis_], cur_outer,
             inner_size, input_shape_[split_axis_] * inner_size);
  return common::SUCCESS;
}

common::Status StridedSliceV2CpuKernelMod::RunTaskOnSplitAxis(const uint8_t *input_addr, uint8_t *output_addr,
                                                              int start_pos) {
  int begin_index = slice_param_.begins_[split_axis_];
  int inner_size = inner_ * data_size_;
  const uint8_t *cur_in_ptr = input_addr + (start_pos * slice_param_.strides_[split_axis_] + begin_index) * inner_size;
  uint8_t *cur_out_ptr = output_addr + start_pos * inner_size;
  int cal_axis_num = output_shape_[split_axis_] - start_pos;
  if (cal_axis_num <= 0) {
    return common::SUCCESS;
  }
  cal_axis_num = cal_axis_num > cal_num_per_thread_ ? cal_num_per_thread_ : cal_axis_num;
  FastStride(cur_in_ptr, cur_out_ptr, cal_axis_num, slice_param_.strides_[split_axis_], 1, inner_size, 0);
  return common::SUCCESS;
}

void StridedSliceV2CpuKernelMod::ParallelRun(const uint8_t *input_addr, uint8_t *output_addr, int thread_num) {
  int thread_index = 0;
  std::vector<common::Task> tasks;
  std::function<common::Status(StridedSliceV2CpuKernelMod *, const uint8_t *, uint8_t *, int)> execute_func;
  if (parallel_strategy_ == kOnOuter) {
    execute_func = &StridedSliceV2CpuKernelMod::RunTaskOnOuter;
  } else if (parallel_strategy_ == kOnSplitAxis) {
    execute_func = &StridedSliceV2CpuKernelMod::RunTaskOnSplitAxis;
  } else {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', not supports parallel execute strategy.";
  }

  while (thread_index < thread_num) {
    (void)tasks.emplace_back(
      std::bind(execute_func, this, input_addr, output_addr, thread_index * cal_num_per_thread_));
    thread_index++;
  }
  ParallelLaunch(tasks);
}

template <typename T>
bool StridedSliceV2CpuKernelMod::StridedSliceV2LaunchDynamicType(const std::vector<kernel::AddressPtr> &inputs,
                                                                 const std::vector<kernel::AddressPtr> &outputs) {
  auto cnode = cnode_ptr_.lock();
  auto begin_shape = common::AnfAlgo::GetPrevNodeOutputInferShape(cnode, 1);
  auto end_shape = common::AnfAlgo::GetPrevNodeOutputInferShape(cnode, 2);
  auto stride_shape = common::AnfAlgo::GetPrevNodeOutputInferShape(cnode, 3);
  if (begin_shape.size() != 1 || end_shape.size() != 1 || stride_shape.size() != 1) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_
                      << "', the dimension of 'begin', 'end', 'strides' should be equal "
                         "to 1, but got the dimension of 'begin': "
                      << begin_shape.size() << ", the dimension of 'end': " << end_shape.size()
                      << ", and the dimension of 'strides': " << stride_shape.size();
  }
  auto begin_ptr = static_cast<T *>(inputs[1]->addr);
  auto end_ptr = static_cast<T *>(inputs[2]->addr);
  auto strides_ptr = static_cast<T *>(inputs[3]->addr);
  std::vector<T> begin{begin_ptr, begin_ptr + begin_shape[0]};
  std::vector<T> end{end_ptr, end_ptr + end_shape[0]};
  std::vector<T> stride{strides_ptr, strides_ptr + stride_shape[0]};
  slice_len = begin.size();
  InitSliceParam<T>(cnode, &begin, &end, &stride);
  return true;
}

bool StridedSliceV2CpuKernelMod::StridedSliceV2LaunchCal(const std::vector<kernel::AddressPtr> &inputs,
                                                         const std::vector<kernel::AddressPtr> &outputs) {
  if (inputs.size() != kStridedSliceV2InputsNum && inputs.size() != kStridedSliceV2DynamicInputsNum) {
    MS_LOG(EXCEPTION) << "For '" << kernel_name_ << "', the number of inputs should be " << kStridedSliceV2InputsNum
                      << " or " << kStridedSliceV2DynamicInputsNum << ", but got " << inputs.size();
  }
  CHECK_KERNEL_OUTPUTS_NUM(outputs.size(), kStridedSliceV2OutputsNum, kernel_name_);

  auto cnode = cnode_ptr_.lock();
  size_t input_num = common::AnfAlgo::GetInputTensorNum(cnode);
  if (input_num == kStridedSliceV2DynamicInputsNum) {
    // for begin, end, stride are not const input
    dtype_attr = AnfAlgo::GetInputDeviceDataType(cnode, 1);
    if (dtype_attr == kNumberTypeInt32) {
      StridedSliceV2LaunchDynamicType<int32_t>(inputs, outputs);
    } else {
      StridedSliceV2LaunchDynamicType<int64_t>(inputs, outputs);
    }

    parallel_ = MatchParallelPattern();
    if (parallel_) {
      InitParallelParam();
    }
  }
  return true;
}

bool StridedSliceV2CpuKernelMod::Launch(const std::vector<kernel::AddressPtr> &inputs,
                                        const std::vector<kernel::AddressPtr> & /* workspace */,
                                        const std::vector<kernel::AddressPtr> &outputs) {
  bool ret = StridedSliceV2LaunchCal(inputs, outputs);
  if (ret != true) {
    MS_LOG(EXCEPTION) << "For StridedSliceV2 LaunchCal failed.";
  }
  auto input_addr = static_cast<uint8_t *>(inputs[0]->addr);
  auto output_addr = static_cast<uint8_t *>(outputs[0]->addr);

  int thread_std = 2;
  int thread_num = slice_param_.op_parameter_.thread_num_;
  if (parallel_ && thread_num >= thread_std) {
    ParallelRun(input_addr, output_addr, thread_num);
  } else if (dtype_ == kNumberTypeComplex128) {
    (void)DoStridedSliceV2Com<complex128>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeComplex64) {
    (void)DoStridedSliceV2Com<complex64>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeFloat16) {
    (void)DoStridedSliceV2Com<float16>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeFloat32) {
    (void)DoStridedSliceV2Com<float>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeFloat64) {
    (void)DoStridedSliceV2Com<double>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeInt8) {
    (void)DoStridedSliceV2Com<int8_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeInt16) {
    (void)DoStridedSliceV2Com<int16_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeInt32) {
    (void)DoStridedSliceV2Com<int32_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeInt64) {
    (void)DoStridedSliceV2Com<int64_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeUInt8) {
    (void)DoStridedSliceV2Com<uint8_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeUInt16) {
    (void)DoStridedSliceV2Com<uint16_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeUInt32) {
    (void)DoStridedSliceV2Com<uint32_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeUInt64) {
    (void)DoStridedSliceV2Com<uint64_t>(input_addr, output_addr, &slice_param_);
  } else if (dtype_ == kNumberTypeBool) {
    (void)DoStridedSliceV2Com<bool>(input_addr, output_addr, &slice_param_);
  }
  return true;
}

std::vector<KernelAttr> StridedSliceV2CpuKernelMod::GetOpSupport() {
  auto iter = support_list_map.find(kStridedSliceV2);
  return iter->second;
}

MS_KERNEL_FACTORY_REG(NativeCpuKernelMod, StridedSliceV2, StridedSliceV2CpuKernelMod);
}  // namespace kernel
}  // namespace mindspore
