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

#ifndef MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMPL_SPONGE_BOND_BOND_FORCE_WITH_ATOM_VIRIAL_IMPL_H_
#define MINDSPORE_CCSRC_KERNEL_GPU_CUDA_IMPL_SPONGE_BOND_BOND_FORCE_WITH_ATOM_VIRIAL_IMPL_H_

#include <curand_kernel.h>
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/cuda_common.h"
#include "plugin/device/gpu/kernel/cuda_impl/cuda_ops/cuda_device_info.h"

CUDA_LIB_EXPORT void BondForceWithAtomVirial(int bond_numbers, int atom_numbers, const int *uint_crd_f,
                                             const float *scaler_f, const int *atom_a, const int *atom_b,
                                             const float *bond_k, const float *bond_r0, float *frc_f, float *atom_v,
                                             cudaStream_t stream);
#endif
