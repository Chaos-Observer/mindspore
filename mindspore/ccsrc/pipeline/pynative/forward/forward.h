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

#ifndef MINDSPORE_MINDSPORE_CCSRC_PIPELINE_PYNATIVE_GRAD_FORWARD_H_
#define MINDSPORE_MINDSPORE_CCSRC_PIPELINE_PYNATIVE_GRAD_FORWARD_H_

#include <memory>
#include <string>
#include <map>
#include <utility>
#include <stack>
#include "pipeline/pynative/forward/do_cast.h"
#include "pipeline/pynative/forward/do_infer.h"
#include "pipeline/pynative/dynamic_shape.h"
#include "backend/common/session/session_factory.h"
#include "backend/common/session/session_basic.h"
#include "backend/graph_compiler/backend.h"
#include "ir/cell.h"

namespace mindspore {
namespace pynative {
class GradExecutor;
using GradExecutorPtr = std::shared_ptr<GradExecutor>;
using GradExecutorWeakPtr = std::weak_ptr<GradExecutor>;

using SessionBackendMap = std::map<std::string, std::shared_ptr<session::SessionBasic>>;
using MindrtBackendMap = std::map<std::string, std::shared_ptr<compile::MindRTBackend>>;

class ForwardExecutor {
 public:
  ForwardExecutor()
      : cast_operation_(std::make_shared<CastOperation>()),
        infer_operation_(std::make_shared<InferOperation>()),
        dynamic_shape_(std::make_shared<DynamicShape>()) {}
  ~ForwardExecutor() = default;

  std::function<void(py::object *, const FrontendOpRunInfoPtr &)> RunOpS = [this](auto &&PH1, auto &&PH2) {
    RunOpInner(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
  };

  void Init();
  void RunOpInner(py::object *ret, const FrontendOpRunInfoPtr &op_run_info);
  ValuePtr RunOpForward(const FrontendOpRunInfoPtr &op_run_info);
  FrontendOpRunInfoPtr GenerateOpRunInfo(const py::args &args) const;
  void set_grad_executor(const GradExecutorPtr &grad_executor) { grad_executor_ = GradExecutorWeakPtr(grad_executor); }
  void ClearNodeAbsMap() const;
  void EraseFromNodeAbsMap(const std::string &id) const;
  void SetNodeAbsMapByValue(const std::string &op_name, const ValuePtr &value,
                            const abstract::AbstractBasePtr &abs) const;
  void SetNodeAbsMapById(const std::string &id, const abstract::AbstractBasePtr &abs) const;
  const NodeAbsCache &NodeAbsMap() const;
  void ClearRes();
  void set_lazy_build(bool lazy_build) { lazy_build_ = lazy_build; }
  const MindrtBackendMap &mindrt_backend() const { return mindrt_backends_; }
  inline bool IsFirstCell() const { return forward_cell_stack_.empty(); }
  void PushForwardCell(const py::object &cell) { forward_cell_stack_.push(cell.cast<CellPtr>()); }
  void PopForwardCell() { forward_cell_stack_.pop(); }
  void ExecuteLazyTask();
  void Sync();
  void PrintPyObjInfo(const py::object &cell, const std::string &str) const;
  void ProcessBeforeNewGraph(const py::object &cell, const py::args &args);
  void ProcessBeforeEndGraph(const py::object &cell);
  void ProcessAfterEndGraph(const py::object &cell) const;
  bool CellNotSetMixedPrecision(const FrontendOpRunInfoPtr &op_run_info);
  void set_is_ms_function_compiling(bool is_compiling) { is_ms_function_compiling_ = is_compiling; }
  inline DynamicShapePtr dynamic_shape() const {
    MS_EXCEPTION_IF_NULL(dynamic_shape_);
    return dynamic_shape_;
  }
  inline InferOperationPtr infer_operation() const {
    MS_EXCEPTION_IF_NULL(infer_operation_);
    return infer_operation_;
  }

 private:
  GradExecutorPtr grad() const;
  std::string GetCurrentDeviceTarget(const PrimitivePtr &op_prim);
  session::SessionPtr GetCurrentSession(const std::string &device_target);
  compile::MindRTBackendPtr GetMindRtBackend(const std::string &device_target);
  inline CastOperationPtr cast_operation() const {
    MS_EXCEPTION_IF_NULL(cast_operation_);
    return cast_operation_;
  }
  ValuePtr RunOpInVM(const FrontendOpRunInfoPtr &op_run_info) const;
  ValuePtr RunOpInMs(const FrontendOpRunInfoPtr &op_run_info);
  ValuePtr RunOpWithBackendPolicy(const FrontendOpRunInfoPtr &op_run_info);
  ValuePtr GetOutput(const FrontendOpRunInfoPtr &op_run_info);
  // Mix precision and Implicit transform
  void SetCastForInputs(const FrontendOpRunInfoPtr &op_run_info) const;
  // Infer output abstract
  ValuePtr InferOutputAbstract(const FrontendOpRunInfoPtr &op_run_info) const;
  // Check sync condition in heterogeneous
  void CheckIfNeedSyncForHeterogeneous(const std::string &cur_target);

 private:
  bool init_{false};
  bool lazy_build_{false};
  bool enable_mind_rt_{false};
  uint32_t device_id_;
  std::string last_target_{"Unknown"};
  std::string device_target_;
  std::stack<CellPtr> forward_cell_stack_;
  GradExecutorWeakPtr grad_executor_;
  CastOperationPtr cast_operation_;
  InferOperationPtr infer_operation_;
  DynamicShapePtr dynamic_shape_;
  SessionBackendMap session_backends_;
  MindrtBackendMap mindrt_backends_;
  bool is_ms_function_compiling_{false};
};
}  // namespace pynative
}  // namespace mindspore

#endif  // MINDSPORE_MINDSPORE_CCSRC_PIPELINE_PYNATIVE_GRAD_FORWARD_H_
