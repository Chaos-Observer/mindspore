/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#ifndef MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMPL_SPONGE_COMMON_ATOMCRDTOCV_IMPL_H_
#define MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMPL_SPONGE_COMMON_ATOMCRDTOCV_IMPL_H_

#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/cuda_common.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/cuda_device_info.h"

CUDA_LIB_EXPORT void AtomCrdToCV(int atom_numbers, int start_serial, int end_serial, int number, const float *crd_f,
                                 const float *old_crd, float *nowarp_crd, int *box_map_times, float *box,
                                 float *g_radial, float *g_angular, cudaStream_t stream);

#endif  // MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMPL_SPONGE_COMMON_ATOMCRDTOCV_IMPL_H_
