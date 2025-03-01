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
#include "backend/common/session/kernel_graph_mgr.h"

#include <algorithm>
#include <set>
#include <queue>
#include <utility>
#include <functional>
#include <unordered_map>

#include "utils/hash_map.h"
#include "ir/manager.h"
#include "abstract/utils.h"
#include "runtime/device/kernel_runtime_manager.h"
#include "utils/ms_utils.h"
#include "ir/anf.h"
#include "ir/func_graph_cloner.h"
#ifndef ENABLE_SECURITY
#include "debug/data_dump/dump_json_parser.h"
#include "debug/data_dump/e2e_dump.h"
#endif

namespace mindspore {
namespace session {
namespace {
constexpr size_t max_depth = 128;

bool RecursiveCheck(const FuncGraphManagerPtr &manager, const std::pair<AnfNodePtr, int64_t> &kernel, size_t *idx) {
  auto node = kernel.first;
  MS_EXCEPTION_IF_NULL(manager);
  MS_EXCEPTION_IF_NULL(node);
  if (kernel.second > 1 && (common::AnfAlgo::CheckPrimitiveType(node, prim::kPrimDepend) ||
                            common::AnfAlgo::CheckPrimitiveType(node, prim::kPrimLoad))) {
    return false;
  }
  if (AnfUtils::IsRealKernel(node) && !common::AnfAlgo::CheckPrimitiveType(node, prim::kPrimPartial)) {
    return true;
  }
  (*idx) += 1;
  // max recursion depth
  if (*idx <= max_depth) {
    auto users = manager->node_users()[node];
    if (std::any_of(users.begin(), users.end(), [&](const std::pair<AnfNodePtr, int64_t> &kernel) {
          return RecursiveCheck(manager, kernel, idx);
        })) {
      return true;
    }
  }
  return false;
}

bool IsUsedByRealKernel(const FuncGraphManagerPtr &manager, const AnfNodePtr &node, const uint32_t graph_id) {
  MS_EXCEPTION_IF_NULL(manager);
  MS_EXCEPTION_IF_NULL(node);
  auto node_users = manager->node_users()[node];
  // filter nodes not in current graph
  for (auto iter = node_users.begin(); iter != node_users.end();) {
    auto func_graph = iter->first->func_graph();
    auto kernel_graph = func_graph->cast<KernelGraphPtr>();
    if (kernel_graph == nullptr) {
      MS_LOG(EXCEPTION) << "func graph cast kernel graph failed, related node is: " << iter->first->DebugString();
    }
    if (kernel_graph->graph_id() != graph_id) {
      iter = node_users.erase(iter);
    } else {
      iter++;
    }
  }

  size_t idx = 0;
  if (std::any_of(node_users.begin(), node_users.end(), [&](const std::pair<AnfNodePtr, int64_t> &kernel) {
        return RecursiveCheck(manager, kernel, &idx);
      })) {
    return true;
  }
  return false;
}

bool ExistGraphCaller(const AnfNodePtr &partial_node) {
  MS_EXCEPTION_IF_NULL(partial_node);
  auto partial_cnode = partial_node->cast<CNodePtr>();
  MS_EXCEPTION_IF_NULL(partial_cnode);
  auto partial_graph = GetValueNode<FuncGraphPtr>(partial_cnode->input(kFirstDataInputIndex));
  // If graph is nullptr, it means that the funcgraph in the partial node is a deadnode, and the processing is skipped.
  if (partial_graph == nullptr) {
    return false;
  }
  auto graph_nodes = TopoSort(partial_graph->get_return());
  return std::any_of(graph_nodes.begin(), graph_nodes.end(), IsValueNode<FuncGraph>);
}
}  // namespace

ParamInfoPtr GetParamDefaultValue(const AnfNodePtr &node) {
  if (node == nullptr) {
    return nullptr;
  }
  auto parameter = node->cast<ParameterPtr>();
  if (parameter == nullptr || !parameter->has_default()) {
    return nullptr;
  }
  return parameter->param_info();
}

#ifndef ENABLE_SECURITY
bool ExistSummaryNode(const KernelGraph *graph) {
  MS_EXCEPTION_IF_NULL(graph);
  auto ret = graph->get_return();
  MS_EXCEPTION_IF_NULL(ret);
  auto all_nodes = DeepLinkedGraphSearch(ret);
  for (auto &n : all_nodes) {
    if (IsPrimitiveCNode(n, prim::kPrimScalarSummary) || IsPrimitiveCNode(n, prim::kPrimTensorSummary) ||
        IsPrimitiveCNode(n, prim::kPrimImageSummary) || IsPrimitiveCNode(n, prim::kPrimHistogramSummary)) {
      return true;
    }
  }
  return false;
}
#endif

GraphId KernelGraphMgr::graph_sum_ = 0;

ValueNodePtr KernelGraphMgr::CreateNewValueNode(const AnfNodePtr &anf, KernelGraph *graph) const {
  MS_EXCEPTION_IF_NULL(anf);
  MS_EXCEPTION_IF_NULL(graph);
  auto value_node = anf->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  auto value = value_node->value();
  MS_EXCEPTION_IF_NULL(value);
  if (value->isa<None>()) {
    return nullptr;
  }
  auto new_value_node = graph->NewValueNode(value_node);
  graph->FrontBackendMapAdd(anf, new_value_node);
  graph->AddValueNodeToGraph(new_value_node);
  return new_value_node;
}

GraphId KernelGraphMgr::GetGraphIdByNode(const AnfNodePtr &front_anf) const {
  for (const auto &graph_item : graphs_) {
    auto graph = graph_item.second;
    MS_EXCEPTION_IF_NULL(graph);
    // if front_anf is a parameter,the backend parameter may have two
    if (graph->GetBackendAnfByFrontAnf(front_anf) != nullptr) {
      return graph_item.first;
    }
  }
  MS_EXCEPTION_IF_NULL(front_anf);
  MS_LOG(DEBUG) << "Front_anf " << front_anf->DebugString() << " is not exist in any graph";
  return kInvalidGraphId;
}

KernelGraphPtr KernelGraphMgr::GetGraph(mindspore::GraphId graph_id) const {
  auto it = graphs_.find(graph_id);
  if (it == graphs_.end()) {
    MS_LOG(INFO) << "Can't find graph " << graph_id;
    return nullptr;
  }
  return it->second;
}

void KernelGraphMgr::ClearGraph() {
  auto graph_iter = graphs_.begin();
  while (graph_iter != graphs_.end()) {
    graph_iter->second.reset();
    graph_iter = graphs_.erase(graph_iter);
  }
  graph_sum_ = 0;
}

void KernelGraphMgr::InitInternalOutputParameter(const AnfNodePtr &out_node, const AnfNodePtr &parameter) const {
  auto graph_id = GetGraphIdByNode(out_node);
  if (graph_id == kInvalidGraphId) {
    return;
  }
  auto node_graph = GetGraph(graph_id);
  if (node_graph == nullptr) {
    return;
  }
  MS_LOG(INFO) << "Init parameter with pre graph output node: " << out_node->DebugString();
  auto ref_node_with_index = node_graph->GetInternalOutputByFrontNode(out_node);
  auto ref_node = ref_node_with_index.first;
  if (ref_node == nullptr) {
    MS_LOG(INFO) << "No corresponding internal output for output node";
    return;
  }
  size_t output_idx = ref_node_with_index.second;
  if (common::AnfAlgo::CheckPrimitiveType(out_node, prim::kPrimTupleGetItem)) {
    output_idx = common::AnfAlgo::GetTupleGetItemOutIndex(out_node->cast<CNodePtr>());
  }
  auto real_kernel = common::AnfAlgo::VisitKernel(ref_node, output_idx);
  auto ref_real_node = real_kernel.first;
  auto ref_real_node_index = real_kernel.second;
  if (ref_real_node->isa<CNode>() && node_graph->IsUniqueTargetInternalOutput(ref_real_node, ref_real_node_index)) {
    auto kernel_info = ref_real_node->kernel_info();
    if (kernel_info == nullptr || !kernel_info->has_build_info()) {
      MS_LOG(INFO) << "No kernel info";
      return;
    }
    if (!common::AnfAlgo::IsNopNode(ref_real_node) && !AnfAlgo::OutputAddrExist(ref_real_node, ref_real_node_index)) {
      MS_LOG(INFO) << "No kernel address";
      return;
    }
    if (!AnfAlgo::OutputAddrExist(ref_real_node, ref_real_node_index, true)) {
      return;
    }
    auto address = AnfAlgo::GetMutableOutputAddr(ref_real_node, ref_real_node_index);
    auto format = AnfAlgo::GetOutputFormat(ref_real_node, ref_real_node_index);
    auto type = AnfAlgo::GetOutputDeviceDataType(ref_real_node, ref_real_node_index);
    auto d_kernel_info = std::make_shared<device::KernelInfo>();
    MS_EXCEPTION_IF_NULL(d_kernel_info);
    parameter->set_kernel_info(d_kernel_info);
    kernel::KernelBuildInfo::KernelBuildInfoBuilder builder;
    builder.SetOutputsDeviceType({type});
    builder.SetOutputsFormat({format});
    d_kernel_info->set_select_kernel_build_info(builder.Build());
    AnfAlgo::SetOutputAddr(address, 0, parameter.get());
    auto abstract = std::make_shared<abstract::AbstractTensor>(TypeIdToType(type),
                                                               parameter->Shape()->cast<abstract::BaseShapePtr>());
    parameter->set_abstract(abstract);
  }
}

AnfNodePtr KernelGraphMgr::CreateParameterFromTuple(const AnfNodePtr &node, KernelGraph *graph) const {
  MS_EXCEPTION_IF_NULL(node);
  MS_EXCEPTION_IF_NULL(graph);
  auto new_parameter = graph->TransTupleToMakeTuple(graph->NewParameter(node->abstract()));
  auto parameters = common::AnfAlgo::GetAllOutput(new_parameter);
  std::vector<AnfNodePtr> pre_graph_out = {node};
  // If a cnode is a call, it's input0 is a cnode too, so it doesn't have primitive
  if (!pre_graph_out.empty() && !AnfUtils::IsRealKernel(node)) {
    pre_graph_out = common::AnfAlgo::GetAllOutput(node, {prim::kPrimTupleGetItem, prim::kPrimUpdateState});
  }

  for (size_t i = 0; i < parameters.size(); ++i) {
    const auto &parameter = parameters[i];
    auto context_ptr = MsContext::GetInstance();
    MS_EXCEPTION_IF_NULL(context_ptr);
    if (context_ptr->get_param<bool>(MS_CTX_ENABLE_MINDRT) == true) {
      // In control flow, if the input of the cnode is a call node, it will be processed as a make_tuple input,
      // which needs to be linked when processing the internal node.
      graph->CacheInternalParameterToFrontNode(parameter, {node, i});
    }
    auto valid_inputs = graph->MutableValidInputs();
    MS_EXCEPTION_IF_NULL(valid_inputs);
    auto graph_inputs = graph->MutableInputs();
    MS_EXCEPTION_IF_NULL(graph_inputs);
    valid_inputs->push_back(true);
    graph_inputs->push_back(parameter);
  }
  size_t param_index = 0;
  for (const auto &out_node : pre_graph_out) {
    size_t output_size = common::AnfAlgo::GetOutputTensorNum(out_node);
    for (size_t i = 0; i < output_size; i++) {
      if (param_index >= parameters.size()) {
        MS_LOG(EXCEPTION) << "Parameters size:" << parameters.size() << "out of range.Node:" << node->DebugString()
                          << ",out_node:" << out_node->DebugString();
      }
      InitInternalOutputParameter(out_node, parameters[param_index++]);
    }
  }
  return new_parameter;
}

ParameterPtr KernelGraphMgr::CreateNewParameterFromParameter(const AnfNodePtr &anf, KernelGraph *graph) {
  MS_EXCEPTION_IF_NULL(anf);
  if (!anf->isa<Parameter>()) {
    MS_LOG(EXCEPTION) << "Anf[" << anf->DebugString() << "] is not a parameter";
  }
  MS_EXCEPTION_IF_NULL(graph);
  auto param_value = GetParamDefaultValue(anf);
  auto valid_inputs = graph->MutableValidInputs();
  MS_EXCEPTION_IF_NULL(valid_inputs);
  auto graph_inputs = graph->MutableInputs();
  MS_EXCEPTION_IF_NULL(graph_inputs);
  ParameterPtr new_parameter = nullptr;
  auto func_graph = anf->func_graph();
  MS_EXCEPTION_IF_NULL(func_graph);
  if (func_graph->manager() != nullptr && func_graph->exist_multi_target() &&
      graph->device_target() == device::DeviceType::kCPU) {
    auto iter = default_param_map_.find(anf);
    if (iter != default_param_map_.end()) {
      new_parameter = iter->second;
    }
    if (new_parameter != nullptr) {
      return new_parameter;
    }
    TraceGuard trace_guard(std::make_shared<TraceCopy>(anf->debug_info()));
    new_parameter = graph->NewParameter(anf->cast<ParameterPtr>());
    graph_inputs->push_back(new_parameter);
    valid_inputs->push_back(true);
    default_param_map_[anf] = new_parameter;
    return new_parameter;
  }
  // if parameter's python parameter has been exist a backend parameter, reuse the exist parameter
  if (param_value != nullptr) {
    new_parameter = param_value->parameter();
  }
  if (new_parameter == nullptr) {
    TraceGuard trace_guard(std::make_shared<TraceCopy>(anf->debug_info()));
    new_parameter = graph->NewParameter(anf->cast<ParameterPtr>());

    auto input_node_iter = partial_parameters_map_.find(anf);
    if (input_node_iter != partial_parameters_map_.end()) {
      InitInternalOutputParameter(input_node_iter->second, new_parameter);
    }

    if (param_value != nullptr) {
      param_value->set_parameter(new_parameter);
    }
  }
  new_parameter->IncreaseUsedGraphCount();
  graph_inputs->push_back(new_parameter);
  valid_inputs->push_back(true);
  return new_parameter;
}

AnfNodePtr KernelGraphMgr::CreateNewParameterFromCNode(const AnfNodePtr &anf, KernelGraph *graph) {
  MS_EXCEPTION_IF_NULL(anf);
  MS_EXCEPTION_IF_NULL(graph);
  MS_LOG(INFO) << "Create a new parameter from cnode[" << anf->DebugString() << "]";
  if (IsPrimitiveCNode(anf, prim::kPrimLoad)) {
    auto input = common::AnfAlgo::GetInputNode(anf->cast<CNodePtr>(), 0);
    MS_EXCEPTION_IF_NULL(input);
    if (input->isa<Parameter>()) {
      auto new_param = CreateNewParameterFromParameter(input, graph);
      auto context_ptr = MsContext::GetInstance();
      MS_EXCEPTION_IF_NULL(context_ptr);
      if (context_ptr->get_param<bool>(MS_CTX_ENABLE_MINDRT) == true) {
        graph->CacheInternalParameterToFrontNode(new_param, {anf, 0});
      }
      return new_param;
    }
  }
  return CreateParameterFromTuple(anf, graph);
}

void KernelGraphMgr::GetCNodeInfo(const CNodePtr &cnode, std::vector<AnfNodePtr> *cnode_inputs) const {
  MS_EXCEPTION_IF_NULL(cnode);
  MS_EXCEPTION_IF_NULL(cnode_inputs);
  auto prim = common::AnfAlgo::GetCNodePrimitive(cnode);
  if (prim != nullptr) {
    // push attr to inputs[0] of new cnode
    cnode_inputs->push_back(std::make_shared<ValueNode>(std::make_shared<Primitive>(*prim)));
  } else {
    auto fg = common::AnfAlgo::GetCNodeFuncGraphPtr(cnode);
    MS_EXCEPTION_IF_NULL(fg);
    auto new_fg = BasicClone(fg);
    cnode_inputs->push_back(std::make_shared<ValueNode>(new_fg));
  }
}

void KernelGraphMgr::GetNewCNodeInputs(const CNodePtr &cnode, KernelGraph *graph, std::vector<AnfNodePtr> *cnode_inputs,
                                       mindspore::HashMap<AnfNodePtr, AnfNodePtr> *other_graph_cnode) {
  MS_EXCEPTION_IF_NULL(cnode);
  MS_EXCEPTION_IF_NULL(graph);
  MS_EXCEPTION_IF_NULL(other_graph_cnode);
  MS_EXCEPTION_IF_NULL(cnode_inputs);
  auto origin_inputs = cnode->inputs();
  const bool is_depend = IsPrimitiveCNode(cnode, prim::kPrimDepend);
  // if has multiple depends,only select first depend as parameter
  for (size_t input_idx = 1; input_idx < origin_inputs.size(); input_idx++) {
    auto anf = origin_inputs[input_idx];
    MS_EXCEPTION_IF_NULL(anf);
    // anf has been created before
    if (graph->GetBackendAnfByFrontAnf(anf) != nullptr) {
      (void)cnode_inputs->emplace_back(graph->GetBackendAnfByFrontAnf(anf));
      continue;
    } else if ((is_depend && input_idx > kRealInputIndexInDepend)) {
      cnode_inputs->push_back(NewValueNode(MakeValue(SizeToInt(input_idx))));
      continue;
    } else if (other_graph_cnode->find(anf) != other_graph_cnode->end()) {
      cnode_inputs->push_back((*other_graph_cnode)[anf]);
      continue;
    } else if (anf->isa<ValueNode>() && !IsValueNode<FuncGraph>(anf)) {
      // if input is a value node,
      auto new_value_node = CreateNewValueNode(anf, graph);
      if (new_value_node != nullptr) {
        (void)cnode_inputs->emplace_back(new_value_node);
      }
      continue;
    } else if (anf->isa<Parameter>()) {
      auto new_parameter = CreateNewParameterFromParameter(anf, graph);
      cnode_inputs->push_back(new_parameter);
      graph->FrontBackendMapAdd(anf, new_parameter);
      continue;
    } else {
      // the input node is a cnode from other graph
      auto parameter_from_cnode = CreateNewParameterFromCNode(anf, graph);
      if (parameter_from_cnode == nullptr) {
        parameter_from_cnode = NewValueNode(MakeValue(SizeToLong(input_idx)));
      }
      if (parameter_from_cnode->isa<Parameter>() && IsPrimitiveCNode(anf, prim::kPrimLoad)) {
        auto para = parameter_from_cnode->cast<ParameterPtr>();
        auto load_cnode = anf->cast<CNodePtr>();
        para->set_name(load_cnode->input(kFirstDataInputIndex)->fullname_with_scope());
      }
      cnode_inputs->push_back(parameter_from_cnode);
      (*other_graph_cnode)[anf] = parameter_from_cnode;
    }
  }
}

CNodePtr KernelGraphMgr::CreateNewCNode(const CNodePtr &cnode, KernelGraph *graph,
                                        mindspore::HashMap<AnfNodePtr, AnfNodePtr> *other_graph_cnode) {
  MS_EXCEPTION_IF_NULL(cnode);
  MS_EXCEPTION_IF_NULL(graph);
  MS_EXCEPTION_IF_NULL(other_graph_cnode);
  // get primitive of old node
  std::vector<AnfNodePtr> cnode_inputs;
  GetCNodeInfo(cnode, &cnode_inputs);
  GetNewCNodeInputs(cnode, graph, &cnode_inputs, other_graph_cnode);
  TraceGuard trace_guard(std::make_shared<TraceCopy>(cnode->debug_info()));
  auto new_cnode = graph->NewCNodeWithInfos(cnode_inputs, cnode);
  return new_cnode;
}

KernelGraphPtr KernelGraphMgr::ConstructKernelGraph(const AnfNodePtrList &lst, const AnfNodePtrList &outputs,
                                                    DeviceType device_target, bool common_opt) {
  mindspore::HashMap<AnfNodePtr, AnfNodePtr> other_graph_cnode;
  auto graph = NewKernelGraph();
  MS_EXCEPTION_IF_NULL(graph);
  MS_LOG(INFO) << "Create graph: " << graph->graph_id();
  for (const auto &node : lst) {
    MS_EXCEPTION_IF_NULL(node);
    MS_LOG(DEBUG) << "Start create new cnode, node = " << node->DebugString();
    if (!node->isa<CNode>()) {
      MS_LOG(EXCEPTION) << "Node " << node->DebugString() << " is not CNode";
    }
    auto cnode = node->cast<CNodePtr>();
    MS_EXCEPTION_IF_NULL(cnode);
    graph->set_device_target(device_target);
    // create a new cnode object
    auto new_cnode = CreateNewCNode(cnode, graph.get(), &other_graph_cnode);
    MS_EXCEPTION_IF_NULL(new_cnode);
    new_cnode->set_abstract(cnode->abstract());
    new_cnode->set_scope(cnode->scope());
    new_cnode->set_attrs(cnode->attrs());
    // record map relations between anf from ME and new anf node used in backend
    graph->FrontBackendMapAdd(node, new_cnode);
  }
  // add a make_tuple at the end of graph as output
  graph->set_output(ConstructOutput(outputs, graph));
  FuncGraphManagerPtr manager = MakeManager({graph});
  if (manager) {
    manager->AddFuncGraph(graph);
    graph->set_manager(manager);
  }
  graph->SetExecOrderByDefault();

#ifndef ENABLE_SECURITY
  if (ExistSummaryNode(graph.get())) {
    graph->set_summary_node_exist(true);
  }
#endif

  MS_EXCEPTION_IF_NULL(MsContext::GetInstance());
  if (!MsContext::GetInstance()->get_param<bool>(MS_CTX_ENABLE_MINDRT)) {
    UnifyMindIR(graph);
    graph->UpdateGraphAquireGilAttr();
    if (common_opt) {
      opt::BackendCommonOptimization(graph);
    }
    graph->SetInputNodes();
    SetInputNodeUsage(graph, manager);
    graph->SetOptimizerFlag();
  }
  return graph;
}

void KernelGraphMgr::SetInputNodeUsage(const KernelGraphPtr &graph, const FuncGraphManagerPtr &manager) const {
  MS_EXCEPTION_IF_NULL(graph);
  MS_EXCEPTION_IF_NULL(manager);
  auto input_nodes = graph->input_nodes();
  for (auto &input_node : input_nodes) {
    MS_EXCEPTION_IF_NULL(input_node);
    if (input_node->isa<Parameter>()) {
      auto node_ptr = input_node->cast<ParameterPtr>();
      MS_EXCEPTION_IF_NULL(node_ptr);
      if (!IsUsedByRealKernel(manager, input_node, graph->graph_id())) {
        node_ptr->SetNotUsedByRealKernelInGraph(graph->graph_id());
      }
      auto shape = node_ptr->Shape();
      if (AnfUtils::IsShapeDynamic(shape->cast<abstract::ShapePtr>())) {
        node_ptr->set_has_dynamic_shape(true);
      }
    }
  }
}

namespace {
bool CNodeFirstInputIsPrimitive(const AnfNodePtr &node) {
  if (node == nullptr) {
    return false;
  }
  auto cnode = node->cast<CNodePtr>();
  if (cnode == nullptr) {
    return false;
  }
  auto prim = cnode->input(kAnfPrimitiveIndex);
  if (prim == nullptr || !IsValueNode<Primitive>(prim)) {
    return false;
  }
  return true;
}

std::vector<AnfNodePtr> ExtendNodeUsers(const FuncGraphManagerPtr &front_func_graph_manager,
                                        const AnfNodePtr &front_node) {
  MS_EXCEPTION_IF_NULL(front_func_graph_manager);
  auto &users = front_func_graph_manager->node_users()[front_node];
  std::vector<AnfNodePtr> result;
  for (auto &user : users) {
    if (common::AnfAlgo::CheckPrimitiveType(user.first, prim::kPrimDepend) ||
        common::AnfAlgo::CheckPrimitiveType(user.first, prim::kPrimLoad)) {
      auto depend_cnode = user.first->cast<CNodePtr>();
      if (depend_cnode == nullptr) {
        continue;
      }
      if (front_node != depend_cnode->input(1)) {
        continue;
      }
      auto res = ExtendNodeUsers(front_func_graph_manager, user.first);
      (void)result.insert(result.cend(), res.cbegin(), res.cend());
    } else if (common::AnfAlgo::CheckPrimitiveType(user.first, prim::kPrimMakeTuple)) {
      auto res = ExtendNodeUsers(front_func_graph_manager, user.first);
      (void)result.insert(result.cend(), res.cbegin(), res.cend());
    } else {
      (void)result.emplace_back(user.first);
    }
  }
  return result;
}

AnfNodePtr GetSupportedInternalNode(const AnfNodePtr &front_node) {
  MS_EXCEPTION_IF_NULL(front_node);
  if (!front_node->isa<CNode>()) {
    return nullptr;
  }
  if (AnfUtils::IsRealKernel(front_node)) {
    return front_node;
  }
  if (common::AnfAlgo::CheckPrimitiveType(front_node, prim::kPrimTupleGetItem)) {
    return front_node;
  }
  if (common::AnfAlgo::CheckPrimitiveType(front_node, prim::kPrimMakeTuple)) {
    auto cnode = front_node->cast<CNodePtr>();
    MS_EXCEPTION_IF_NULL(cnode);
    auto &inputs = cnode->inputs();
    if (inputs.size() > 1) {
      return GetSupportedInternalNode(inputs[1]);
    }
  }
  if (common::AnfAlgo::CheckPrimitiveType(front_node, prim::kPrimDepend)) {
    auto cnode = front_node->cast<CNodePtr>();
    MS_EXCEPTION_IF_NULL(cnode);
    auto &inputs = cnode->inputs();
    if (inputs.size() >= kDependInputSize) {
      return GetSupportedInternalNode(inputs[kRealInputIndexInDepend]);
    }
  }
  return nullptr;
}

bool IsUnusedInternlOutput(const AnfNodePtr &user) {
  if (!CNodeFirstInputIsPrimitive(user)) {
    return true;
  }
  if (IsPrimitiveCNode(user, prim::kPrimSwitch) || IsPrimitiveCNode(user, prim::kPrimSwitchLayer)) {
    return true;
  }
  if (!AnfUtils::IsRealKernel(user)) {
    return true;
  }
  return false;
}
}  // namespace

constexpr auto kMixTarget = "MixTarget";
constexpr auto kNoTarget = "NoTarget";
std::string KernelGraphMgr::AddPartialParametersMap(const AnfNodePtr &partial_node) {
  MS_EXCEPTION_IF_NULL(partial_node);
  auto iter = partial_target_map_.find(partial_node);
  if (iter != partial_target_map_.end()) {
    return iter->second;
  }
  auto partial_cnode = partial_node->cast<CNodePtr>();
  MS_EXCEPTION_IF_NULL(partial_cnode);
  auto partial_graph = GetValueNode<FuncGraphPtr>(partial_cnode->input(kFirstDataInputIndex));
  // If graph is nullptr, it means that the funcgraph in the partial node is a deadnode, and the processing is skipped.
  if (partial_graph == nullptr) {
    return kNoTarget;
  }
  auto parameters = partial_graph->parameters();
  auto partial_inputs = partial_cnode->inputs();
  const size_t kNonParameterNum = 2;
  if (parameters.size() + kNonParameterNum != partial_inputs.size()) {
    return kMixTarget;
  }
  for (size_t i = 0; i < parameters.size(); ++i) {
    partial_parameters_map_[parameters[i]] = partial_inputs[kNonParameterNum + i];
  }
  auto graph_nodes = TopoSort(partial_graph->get_return());
  std::string graph_target = kNoTarget;
  for (auto &node : graph_nodes) {
    if (!node->isa<CNode>()) {
      continue;
    }
    if (!AnfUtils::IsRealKernel(node)) {
      continue;
    }
    std::string cur_target = GetCNodeTarget(node);
    if (graph_target == kNoTarget) {
      graph_target = cur_target;
    }
    if (graph_target != cur_target) {
      graph_target = kMixTarget;
      break;
    }
  }
  (void)partial_target_map_.emplace(std::pair<AnfNodePtr, std::string>(partial_node, graph_target));
  return graph_target;
}

void KernelGraphMgr::HandleInternalOutput(const AnfNodePtr &input_front_node, const AnfNodePtr &backend_node,
                                          const FuncGraphManagerPtr &front_func_graph_manager,
                                          const std::shared_ptr<KernelGraph> &backend_graph) {
  if (device::KernelRuntime::UseMemScheduler()) {
    return;
  }
  auto front_node = GetSupportedInternalNode(input_front_node);
  if (front_node == nullptr) {
    return;
  }
  auto front_real_kernel_pair = common::AnfAlgo::VisitKernel(front_node, 0);
  auto backend_real_kernel_pair = common::AnfAlgo::VisitKernel(backend_node, 0);
  auto backend_real_kernel = backend_real_kernel_pair.first;
  if (backend_real_kernel == nullptr || !backend_real_kernel->isa<CNode>()) {
    return;
  }
  auto front_real_kernel = front_real_kernel_pair.first;
  std::string kernel_target = GetCNodeTarget(front_real_kernel);
  bool internal_output = CNodeFirstInputIsPrimitive(front_real_kernel);
  bool unique_target = true;
  if (internal_output && common::AnfAlgo::IsNopNode(front_real_kernel)) {
    auto pre_node_pair = common::AnfAlgo::GetPrevNodeOutput(front_real_kernel, 0);
    auto pre_node_target = GetCNodeTarget(pre_node_pair.first);
    if (pre_node_target != kernel_target) {
      unique_target = false;
    }
  }
  if (internal_output) {
    auto users = ExtendNodeUsers(front_func_graph_manager, front_node);
    for (auto &user : users) {
      if (common::AnfAlgo::CheckPrimitiveType(user, prim::kPrimPartial) && kernel_target != kGPUDevice &&
          !ExistGraphCaller(user)) {
        auto partial_target = AddPartialParametersMap(user);
        if (partial_target != kNoTarget && partial_target != kernel_target) {
          unique_target = false;
        }
        continue;
      }
      if (common::AnfAlgo::CheckPrimitiveType(user, prim::kPrimUpdateState)) {
        continue;
      }
      if (IsUnusedInternlOutput(user)) {
        internal_output = false;
        break;
      }
      if (kernel_target != GetCNodeTarget(user)) {
        unique_target = false;
      }
    }
  }
  if (internal_output) {
    MS_LOG(INFO) << "AddInternalOutput: " << front_node->DebugString() << " To " << backend_real_kernel->DebugString()
                 << ", unique_target: " << unique_target;
    backend_graph->AddInternalOutput(front_node, backend_real_kernel, backend_real_kernel_pair.second, unique_target);
  }
}

CNodePtr KernelGraphMgr::ConstructOutput(const AnfNodePtrList &outputs, const std::shared_ptr<KernelGraph> &graph) {
  MS_EXCEPTION_IF_NULL(graph);
  std::vector<AnfNodePtr> output_args;
  for (const auto &output : outputs) {
    MS_EXCEPTION_IF_NULL(output);
    MS_LOG(INFO) << "Output:" << output->DebugString();
  }
  auto FindEqu = [graph, outputs, this](const AnfNodePtr &out) -> AnfNodePtr {
    auto backend_anf = graph->GetBackendAnfByFrontAnf(out);
    if (backend_anf != nullptr) {
      auto context_ptr = MsContext::GetInstance();
      MS_EXCEPTION_IF_NULL(context_ptr);
      if (context_ptr->get_param<int>(MS_CTX_EXECUTION_MODE) == kPynativeMode) {
        return backend_anf;
      }

      MS_EXCEPTION_IF_NULL(out);
      auto out_func_graph = out->func_graph();
      MS_EXCEPTION_IF_NULL(out_func_graph);
      auto out_func_graph_manager = out_func_graph->manager();
      if (out_func_graph_manager == nullptr) {
        return backend_anf;
      }
      HandleInternalOutput(out, backend_anf, out_func_graph_manager, graph);
      return backend_anf;
    }
    MS_LOG(EXCEPTION) << "Can't find the node in the equiv map!";
  };
  output_args.push_back(mindspore::NewValueNode(std::make_shared<Primitive>(prim::kPrimMakeTuple->name())));
  (void)std::transform(outputs.begin(), outputs.end(), std::back_inserter(output_args),
                       [&](const AnfNodePtr &out) -> AnfNodePtr { return FindEqu(out); });
  auto output_node = graph->NewCNode(output_args);

  // Create abstract for output maketuple node.
  AbstractBasePtrList output_abs_list;
  const auto &inputs = output_node->inputs();
  (void)std::transform(
    inputs.begin() + 1, inputs.end(), std::back_inserter(output_abs_list), [](const AnfNodePtr &input) {
      return input->abstract() == nullptr ? std::make_shared<abstract::AbstractNone>() : input->abstract();
    });
  auto abstract_tuple = std::make_shared<abstract::AbstractTuple>(output_abs_list);
  MS_EXCEPTION_IF_NULL(abstract_tuple);
  output_node->set_abstract(abstract_tuple);
  return output_node;
}

KernelGraphPtr KernelGraphMgr::NewKernelGraph() {
  auto graph = std::make_shared<KernelGraph>();
  graph->set_graph_id(graph_sum_);
  graphs_[graph_sum_++] = graph;
  return graph;
}

void KernelGraphMgr::UnifyMindIR(const KernelGraphPtr &graph) { opt::CommonUnifyMindIR(graph); }
}  // namespace session
}  // namespace mindspore
