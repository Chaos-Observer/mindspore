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

#include "plugin/device/ascend/hal/hardware/ascend_device_context.h"
#include <memory>
#include "plugin/device/ascend/hal/common/ascend_utils.h"
#ifdef ENABLE_DEBUGGER
#include "debug/tensor_load.h"
#include "debug/debugger/proto_exporter.h"
#else
#include "debug/debugger/proto_exporter_stub.h"
#endif
#ifndef ENABLE_SECURITY
#include "plugin/device/ascend/hal/profiler/ascend_profiling.h"
#include "plugin/device/ascend/hal/device/profiling/profiling_manager.h"

using mindspore::profiler::ascend::AscendProfiler;
#endif

namespace mindspore {
namespace device {
namespace ascend {
void AscendDeviceContext::Initialize() {
  MS_LOG(INFO) << "Start Initialize...";
  if (initialized_) {
    MS_EXCEPTION_IF_NULL(runtime_instance_);
    runtime_instance_->SetContext();
    return;
  } else {
#ifndef ENABLE_SECURITY
    AscendProfiler::GetInstance()->MsprofInitProfiler();
#endif
  }
  MS_EXCEPTION_IF_NULL(device_res_manager_);
  device_res_manager_->Initialize();
  auto ascend_res_manager = dynamic_cast<AscendDeviceResManager *>(device_res_manager_.get());
  MS_EXCEPTION_IF_NULL(ascend_res_manager);
  runtime_instance_ = ascend_res_manager->runtime_instance_;
  MS_EXCEPTION_IF_NULL(runtime_instance_);
  auto ascend_kernel_executor = dynamic_cast<AscendKernelExecutor *>(kernel_executor_.get());
  MS_EXCEPTION_IF_NULL(ascend_kernel_executor);
  ascend_kernel_executor->Initialize();
  auto ascend_graph_executor = dynamic_cast<AscendGraphExecutor *>(graph_executor_.get());
  MS_EXCEPTION_IF_NULL(ascend_graph_executor);
  ascend_graph_executor->Initialize();
  initialized_ = true;
  MS_LOG(INFO) << "Initialize success.";
}

void AscendDeviceContext::Destroy() {
#ifdef ENABLE_DEBUGGER
  auto debugger = Debugger::GetInstance();
  if (debugger && debugger->debugger_enabled()) {
    debugger->SetTrainingDone(true);
    bool ret = debugger->SendMetadata(false);
    if (!ret) {
      MS_LOG(ERROR) << "Failed to SendMetadata when finalize";
    }
  }
#endif
  MS_LOG(INFO) << "Enter Destroy...";
  if (!initialized_) {
    if (deprecated_interface_ != nullptr) {
      (void)deprecated_interface_->CloseTsd(MsContext::GetInstance(), true);
    }
    return;
  }

  MS_LOG(INFO) << "Start Destroy ";
  auto ascend_graph_executor = dynamic_cast<AscendGraphExecutor *>(graph_executor_.get());
  ascend_graph_executor->Destroy();
  auto ascend_kernel_executor = dynamic_cast<AscendKernelExecutor *>(kernel_executor_.get());
  ascend_kernel_executor->Destroy();
  device_res_manager_->Destroy();
  if (runtime_instance_) {
    runtime_instance_ = nullptr;
  }
  if (deprecated_interface_ != nullptr) {
    (void)deprecated_interface_->CloseTsd(MsContext::GetInstance(), true);
  }
  initialized_ = false;
  MS_LOG(INFO) << "Destroy success.";
}

// @todo move SetRunMode to here when old runtime is delete
bool AscendDeviceContext::PartitionGraph(const FuncGraphPtr &func_graph) const {
  auto context_ptr = MsContext::GetInstance();
  MS_EXCEPTION_IF_NULL(context_ptr);
  return context_ptr->get_param<bool>(MS_CTX_IS_MULTI_GRAPH_SINK);
}

RunMode AscendDeviceContext::GetRunMode(const FuncGraphPtr &func_graph) const {
  auto ms_context = MsContext::GetInstance();
  MS_EXCEPTION_IF_NULL(ms_context);
  if (ms_context->get_param<bool>(MS_CTX_ENABLE_TASK_SINK) && !IsDynamicShapeGraph(func_graph)) {
    return RunMode::kGraphMode;
  } else {
    return RunMode::kKernelMode;
  }
}

DeprecatedInterface *AscendDeviceContext::GetDeprecatedInterface() {
  // need lock when multi-threads
  if (deprecated_interface_ == nullptr) {
    deprecated_interface_ = std::make_unique<AscendDeprecatedInterface>(nullptr);
  }
  return deprecated_interface_.get();
}

MS_REGISTER_DEVICE(kAscendDevice, AscendDeviceContext);
}  // namespace ascend
}  // namespace device
}  // namespace mindspore
