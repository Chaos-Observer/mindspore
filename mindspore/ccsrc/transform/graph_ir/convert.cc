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

#include "transform/graph_ir/convert.h"

#include <cinttypes>
#include <algorithm>
#include <queue>
#include <stack>
#include "include/common/utils/utils.h"
#include "mindspore/core/ops/core_ops.h"
#include "utils/anf_utils.h"
#include "utils/log_adapter.h"
#include "ir/graph_utils.h"
#include "utils/symbolic.h"
#include "include/common/utils/config_manager.h"
#include "include/transform/graph_ir/utils.h"
#include "utils/ms_context.h"
#include "utils/check_convert_utils.h"
#include "transform/graph_ir/op_adapter_map.h"
#include "ops/state_ops.h"
#include "ops/array_ops.h"
#include "ops/elewise_calculation_ops.h"
#include "ops/math_ops.h"
#include "ops/save_ops.h"
#include "ops/data_flow_ops.h"
#include "transform/graph_ir/op_adapter.h"
#include "transform/graph_ir/op_adapter_desc.h"

namespace mindspore {
namespace transform {
using ::ge::Operator;
using mindspore::kAnyValue;
using std::make_shared;
using std::shared_ptr;
using std::string;
using std::vector;
using Variable = ::ge::op::Variable;
using Constant = ::ge::op::Constant;
using Assign = ::ge::op::Assign;
using Data = ::ge::op::Data;
using std::endl;

constexpr size_t kInputOffset = 2;
constexpr size_t kSwitchInputSize = 4;
constexpr size_t kSwitchBodyIndex = 2;
constexpr size_t kSwitchAfterIndex = 3;
constexpr size_t kAfterIndexInCache = 2;
constexpr size_t kDataInputIndex = 1;
constexpr size_t kReturnInputSize = 2;
constexpr size_t kMergeInputSize = 2;
constexpr auto kTypeNoOp = "NoOp";
constexpr auto kTypeIdentity = "Identity";
constexpr auto kTypeIdentityN = "IdentityN";
constexpr auto kTypeMerge = "Merge";
constexpr auto kTypeIf = "If";

namespace {
std::vector<AnfNodePtr> GetOrderedCNodes(const FuncGraphPtr fg, const AnfNodePtr node = nullptr) {
  MS_EXCEPTION_IF_NULL(fg);
  auto BelongSameGraph = std::bind(IncludeBelongGraph, fg, std::placeholders::_1);
  auto succ_include_fv = [&fg](const AnfNodePtr &node) -> std::vector<AnfNodePtr> {
    std::vector<AnfNodePtr> vecs;
    if (node == nullptr) {
      return vecs;
    }
    if (node->isa<CNode>()) {
      auto cnode = node->cast<CNodePtr>();
      auto &inputs = cnode->inputs();
      // Check if free variables used.
      for (const auto &input : inputs) {
        auto input_fg = GetValueNode<FuncGraphPtr>(input);
        if (input_fg) {
          for (auto &fv : input_fg->free_variables_nodes()) {
            if (fv->func_graph() == fg && fg->nodes().contains(fv)) {
              vecs.push_back(fv);
            }
          }
        }
      }
      (void)vecs.insert(vecs.end(), inputs.begin(), inputs.end());
    }
    return vecs;
  };

  return (node == nullptr) ? TopoSort(fg->get_return(), succ_include_fv, BelongSameGraph)
                           : TopoSort(node, succ_include_fv, BelongSameGraph);
}
}  // namespace

// ---------------implement of DfGraphConvertor-------------

bool IsDynamicShapeNode(const AnfNodePtr node) {
  auto shape = node->Shape();
  if (shape == nullptr) {
    return false;
  }
  if (!shape->isa<abstract::Shape>()) {  // do not accept tuple shape as call node input
    return false;
  }
  if (AnfUtils::IsShapeDynamic(shape->cast<abstract::ShapePtr>())) {
    return true;
  }
  return false;
}

void DfGraphConvertor::InitLoopVar(std::vector<::ge::Operator> *init_input) {
  MS_EXCEPTION_IF_NULL(init_input);
  if (this->training_) {
    GeTensorDesc desc(GeShape(), ::ge::FORMAT_NCHW, ::ge::DT_INT64);
    auto var_iter_num = std::make_shared<Variable>("npu_runconfig/iterations_per_loop");
    auto var_loop_cond = std::make_shared<Variable>("npu_runconfig/loop_cond");
    auto var_one = std::make_shared<Variable>("npu_runconfig/one");
    auto var_zero = std::make_shared<Variable>("npu_runconfig/zero");
    (void)var_iter_num->update_output_desc_y(desc);
    (void)var_loop_cond->update_output_desc_y(desc);
    (void)var_one->update_output_desc_y(desc);
    (void)var_zero->update_output_desc_y(desc);
    vars_["npu_runconfig/iterations_per_loop"] = var_iter_num;
    vars_["npu_runconfig/loop_cond"] = var_loop_cond;
    vars_["npu_runconfig/one"] = var_one;
    vars_["npu_runconfig/zero"] = var_zero;

    int64_t value = 0;
    auto const_iter_num = std::make_shared<Constant>("const/npu_runconfig/iterations_per_loop");
    if (ConfigManager::GetInstance().dataset_mode() == DS_SINK_MODE) {
      value = ConfigManager::GetInstance().iter_num();
    } else {
      MS_LOG(INFO) << "Run with normal(non-sink) mode, the iterator number will always be 1";
      ConfigManager::GetInstance().ResetIterNum();
    }
    value -= 1;  // iteration start from 0, the max iteration number for n loop should be n-1
    (void)const_iter_num->set_attr_value(GeTensor(desc, reinterpret_cast<uint8_t *>(&value), sizeof(int64_t)));

    auto const_loop_cond = std::make_shared<Constant>("const/npu_runconfig/loop_cond");
    value = 0;
    (void)const_loop_cond->set_attr_value(GeTensor(desc, reinterpret_cast<uint8_t *>(&value), sizeof(int64_t)));

    auto const_one = std::make_shared<Constant>("const/npu_runconfig/one");
    value = 1;
    (void)const_one->set_attr_value(GeTensor(desc, reinterpret_cast<uint8_t *>(&value), sizeof(int64_t)));

    auto const_zero = std::make_shared<Constant>("const/npu_runconfig/zero");
    value = 0;
    (void)const_zero->set_attr_value(GeTensor(desc, reinterpret_cast<uint8_t *>(&value), sizeof(int64_t)));

    (void)const_iter_num->update_output_desc_y(desc);
    (void)const_loop_cond->update_output_desc_y(desc);
    (void)const_one->update_output_desc_y(desc);
    (void)const_zero->update_output_desc_y(desc);

    auto assign_iter_num = std::make_shared<Assign>("assign/npu_runconfig/iterations_per_loop");
    (void)assign_iter_num->set_input_ref(*var_iter_num).set_input_value(*const_iter_num);
    auto assign_loop_cond = std::make_shared<Assign>("assign/npu_runconfig/loop_cond");
    (void)assign_loop_cond->set_input_ref(*var_loop_cond).set_input_value(*const_loop_cond);
    auto assign_one = std::make_shared<Assign>("assign/npu_runconfig/one");
    (void)assign_one->set_input_ref(*var_one).set_input_value(*const_one);
    auto assign_zero = std::make_shared<Assign>("assign/npu_runconfig/zero");
    (void)assign_zero->set_input_ref(*var_zero).set_input_value(*const_zero);

    init_input->push_back(*var_iter_num);
    init_input->push_back(*var_loop_cond);
    init_input->push_back(*var_one);
    init_input->push_back(*var_zero);
    init_ops_.push_back(var_iter_num);
    init_ops_.push_back(var_loop_cond);
    init_ops_.push_back(var_one);
    init_ops_.push_back(var_zero);
    init_ops_.push_back(const_iter_num);
    init_ops_.push_back(const_loop_cond);
    init_ops_.push_back(const_one);
    init_ops_.push_back(const_zero);
    init_ops_.push_back(assign_iter_num);
    init_ops_.push_back(assign_loop_cond);
    init_ops_.push_back(assign_one);
    init_ops_.push_back(assign_zero);
  }
}

void DfGraphConvertor::DrawParamInitSubGraph(const std::string &name, const AnfNodePtr &it) {
  // draw init subgraph
  init_sout_ << "op_assign" << it.get() << "[label=<";
  init_sout_ << "<table border='1' cellborder='1'>" << endl;
  init_sout_ << "<tr>";
  init_sout_ << "<td port='1'>resource</td>";
  init_sout_ << "<td port='2'>value</td>";
  init_sout_ << "</tr>" << endl;
  init_sout_ << "<tr><td colspan=\"2\">"
             << "\"assign_" << name << "\"</td></tr>" << endl;
  init_sout_ << "</table>> shape=plaintext]" << endl;
  init_sout_ << "param" << it.get() << "[shape=octagon, label=\"" << name << "\"]" << endl;
  init_sout_ << "const" << it.get() << "[label= \"" << name << "_const"
             << "\" shape=ellipse]" << endl;
  init_sout_ << "param" << it.get() << "->"
             << "op_assign" << it.get() << ":1" << endl;
  init_sout_ << "const" << it.get() << "->"
             << "op_assign" << it.get() << ":2" << endl;
}

void DfGraphConvertor::SetupParamInitSubGraph(const TensorOrderMap &tensors,
                                              const std::vector<::ge::Operator> *const init_input) {
  DfGraphPtr init_graph = std::make_shared<DfGraph>("init");
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph_);

  for (auto &it : nodes) {
    MS_EXCEPTION_IF_NULL(it);
    if (it->isa<ValueNode>()) {
      if (IsValueNode<SymbolicKeyInstance>(it)) {
        auto symbolic = GetValueNode<SymbolicKeyInstancePtr>(it);
        auto name = std::static_pointer_cast<Parameter>(symbolic->node())->name();
        auto iter = vars_.find(name);  // get corresponding variable op
        if (iter != vars_.end()) {
          op_cache_[it.get()] = iter->second;
          // #ifdef DRAW_GE_GRAPH
          compute_sout_ << op_draw_name_[params_[name].get()] << " -> " << op_draw_name_[it.get()]
                        << "[style=\"dotted\"]" << endl;
          // #endif
        }
      } else if (IsValueNode<RefKey>(it)) {
        auto refkey = GetValueNode<StringImmPtr>(it);
        MS_EXCEPTION_IF_NULL(refkey);
        auto name = refkey->value();
        auto iter = vars_.find(name);  // get corresponding variable op
        if (iter != vars_.end()) {
          op_cache_[it.get()] = iter->second;
          compute_sout_ << op_draw_name_[params_[name].get()] << " -> " << op_draw_name_[it.get()]
                        << "[style=\"dotted\"]" << endl;
        }
      }
    }
  }

  for (auto &it : tensors) {
    if (vars_.find(it.first) == vars_.end()) {
      MS_LOG(WARNING) << "Init parameter " << it.first << " didn't appear in graph.";
      vars_[it.first] = nullptr;
    }
  }

  // set up init sub graph
  if (init_input->size() != 0) {
    // init sub graph needs no input
    MS_LOG(INFO) << "Build data init subgraph.";
    (void)init_graph->SetInputs(*init_input);
    this->init_graph_ = init_graph;
  } else {
    this->init_graph_ = nullptr;
  }
}

void DfGraphConvertor::MakeDatasetHandler(const std::string &name, const size_t &input_idx, const AnfNodePtr &it) {
  MS_LOG(INFO) << "The " << name << " is the " << input_idx << "(st/nd/th) input";
  if (ConfigManager::GetInstance().dataset_mode() == DS_SINK_MODE) {
    auto getnext_idx = static_cast<int64_t>(input_idx);
    DatasetGraphParam param = ConfigManager::GetInstance().dataset_param();
    if (!param.input_indexes().empty() && input_idx <= param.input_indexes().size()) {
      getnext_idx = param.input_indexes()[input_idx] - 1;  // input_idx start from 0.
      MS_LOG(INFO) << "remap input_index:" << input_idx << " to getnext_index:" << getnext_idx << ".";
    }
    // use iterator_getnext op with output_name instead of data op in BuildGraph.
    if (dataset_iter_getnext_ != nullptr) {
      out_handle_cache_[it.get()] = OutHandler(dataset_iter_getnext_, "y" + std::to_string(getnext_idx));
    }
    // heterogeneous getnextfromqueue op with output_name instead of data op in BuildGraph.
    if (get_next_from_queue_ != nullptr) {
      out_handle_cache_[it.get()] = OutHandler(get_next_from_queue_, "y" + std::to_string(getnext_idx));
    }
  }
}

void DfGraphConvertor::SetupBroadcast(const std::shared_ptr<HcomBroadcast> &broadcast,
                                      const std::vector<GeTensorDesc> &broadcast_desc,
                                      const DfGraphPtr &broadcast_graph, std::vector<::ge::Operator> broadcast_input) {
  MS_LOG(INFO) << "build broadcast subgraph";
  if (broadcast_desc.size() != broadcast_input.size()) {
    MS_LOG(EXCEPTION) << "Desc number of BroadCast is not equal to number of Input";
  }
  (void)broadcast->create_dynamic_input_x(static_cast<unsigned int>(broadcast_input.size()));
  (void)broadcast->create_dynamic_output_y(static_cast<unsigned int>(broadcast_desc.size()));
  for (unsigned int i = 0; i < broadcast_input.size(); i++) {
    (void)broadcast->set_dynamic_input_x(i, broadcast_input[i]);
    (void)broadcast->update_dynamic_output_desc_y(i, broadcast_desc[i]);
  }
  (void)broadcast_graph->SetInputs(broadcast_input);
  this->broadcast_graph_ = broadcast_graph;
}

void DfGraphConvertor::InitParamWithData(const TensorOrderMap &tensors) {
  int index = 0;
  std::vector<Operator> init_input;
  for (auto it : tensors) {
    std::string name = it.first;
    auto node_itor = params_.find(name);
    // if name not in params_, create a node in graph
    if (node_itor == params_.end()) {
      MS_LOG(WARNING) << name << " is not in params, and create a new node.";
      ParameterPtr param = std::make_shared<Parameter>(nullptr);
      name = name + "_temp";
      param->set_name(name);
      (void)ConvertParameter(param);
      node_itor = params_.find(name);
    }
    auto node = node_itor->second;
    auto op_itor = op_cache_.find(node.get());
    if (op_itor == op_cache_.end()) {
      MS_LOG(EXCEPTION) << "Can not find op for node " << node->ToString() << ".";
    }
    auto adpt = FindAdapter(kNameParam, training_);
    if (adpt == nullptr) {
      continue;
    }
    auto param_op = adpt->generate(name + "_data");
    MS_LOG(INFO) << "Add parameter " << name << " as input, index " << index << ".";

    if (!training_) {
      auto adpt_const = FindAdapter(kNameConst, training_);
      if (adpt_const == nullptr) {
        continue;
      }
      auto const_op = adpt_const->generate(name + "_const");
      (void)adpt_const->setAttr(const_op, "value", it.second);

      auto const_op_desc = TransformUtil::GetGeTensorDesc(it.second->shape_c(), it.second->data_type(), kOpFormat_NCHW);
      if (const_op_desc == nullptr) {
        MS_LOG(WARNING) << "Create variable " << name << " output descriptor failed!";
        continue;
      }
      (void)std::static_pointer_cast<Constant>(const_op)->update_output_desc_y(*const_op_desc);
      const_op_to_value_[const_op] = it.second;
      vars_[name] = const_op;
      op_itor->second = const_op;
      continue;
    }

    // create tensor descriptor for output descriptor
    auto desc = TransformUtil::GetGeTensorDesc(it.second->shape_c(), it.second->data_type(), kOpFormat_NCHW);
    if (desc == nullptr) {
      MS_LOG(ERROR) << "Create variable " << name << " output descriptor failed!";
      continue;
    }

    // we need three variable ops for each graph with same name
    // build init subgraph
    if (it.second->is_init() == 0) {
      (void)std::static_pointer_cast<Data>(param_op)->set_attr_index(index++);
      auto init_var = std::make_shared<Variable>(name);
      auto assign_op = std::make_shared<Assign>("assign_" + name);
      (void)init_var->update_output_desc_y(*desc);
      (void)assign_op->set_input_ref(*init_var).set_input_value(*param_op);
      init_input.push_back(*init_var);
      init_ops_.push_back(param_op);
      init_ops_.push_back(assign_op);
      init_ops_.push_back(init_var);
    }

    auto variable = std::make_shared<Variable>(name);
    (void)variable->update_output_desc_y(*desc);
    // do not use read variable while variable sink
    MS_LOG(DEBUG) << "InitParam, op_name = " << name << ", var = " << variable->GetName() << ".";
    op_itor->second = variable;  // replace parameter with variable
    vars_[name] = variable;      // prevent the variable operator from being freed
    DrawParamInitSubGraph(name, node);
  }
  InitLoopVar(&init_input);
  SetupParamInitSubGraph(tensors, &init_input);
}

// convert all parameter need initialize to variable
DfGraphConvertor &DfGraphConvertor::InitParam(const TensorOrderMap &tensors) {
  size_t input_idx = 0;
  if (error_ != SUCCESS) {
    return *this;
  }
  if (anf_graph_ == nullptr || anf_graph_->output() == nullptr) {
    error_ = INVALID_ARGUMENT;
    MS_LOG(ERROR) << "Invalid AnfGraph in InitParam.";
    return *this;
  }

  // Processing input with MakeDatasetHandler
  for (auto &it : anf_graph_->parameters()) {
    auto op_itor = op_cache_.find(it.get());  // converted node
    if (it->isa<Parameter>() && op_itor != op_cache_.end()) {
      string name = std::static_pointer_cast<Parameter>(it)->name();
      auto tensor_itor = tensors.find(name);  // in init value map
      if (tensor_itor == tensors.end()) {
        DfGraphConvertor::MakeDatasetHandler(name, input_idx, it);
        input_idx++;
      }
    }
  }
  InitParamWithData(tensors);
  init_sout_ << "}" << endl;
  return *this;
}

#if (defined ENABLE_D)
void DfGraphConvertor::BuildSaveCheckpointGraph() {
  std::vector<Operator> graph_inputs;
  ::ge::op::Save save_op("save_parms");
  int save_op_is_active = 0;
  size_t index = 0;
  string name;

  auto count_size = std::count_if(vars_.begin(), vars_.end(), [](const auto &it) {
    return LongToUlong(it.second == nullptr || it.first.find("/") != std::string::npos);
  });

  (void)save_op.create_dynamic_input_tensors(static_cast<uint32_t>(vars_.size() - static_cast<size_t>(count_size)));

  // for each "parameter" in anf graph excluding "input"
  for (const auto &it : vars_) {
    name = it.first;
    if (it.second == nullptr || name.find("/") != std::string::npos) {
      continue;
    }
    Variable variable(name);
    (void)variable.update_output_desc_y(it.second->GetOutputDesc(0));
    (void)save_op.set_dynamic_input_tensors(static_cast<uint32_t>(index++), variable);

    graph_inputs.push_back(variable);

    if (save_op_is_active == 0) {
      checkpoint_sout_ << "op_save" << &save_op << "[label=<";
      checkpoint_sout_ << "<table border='1' cellborder='1'>" << endl;
      checkpoint_sout_ << "<tr><td port='1'>tensor</td></tr>" << endl;
      checkpoint_sout_ << "<tr><td colspan=\"1\">"
                       << "\"saveop"
                       << "\"</td></tr>" << endl;
      checkpoint_sout_ << "</table>> shape=plaintext]" << endl;
    }

    checkpoint_sout_ << "param" << it.second << "[shape=octagon, label=\"" << name << "\"]" << endl;

    checkpoint_sout_ << "param" << it.second << "->"
                     << "op_save" << &save_op << ":1" << endl;
    save_op_is_active = 1;
  }
  if (save_op_is_active != 0) {
    std::vector<Operator> graph_output;
    (void)graph_output.emplace_back(save_op);
    DfGraphPtr checkpoint_graph = std::make_shared<DfGraph>("checkpoint");
    (void)checkpoint_graph->SetInputs(graph_inputs);
    (void)checkpoint_graph->SetOutputs(graph_output);
    this->save_ckp_graph_ = checkpoint_graph;
  } else {
    this->save_ckp_graph_ = nullptr;
  }

  checkpoint_sout_ << "}" << endl;
  return;
}
#endif

DfGraphConvertor &DfGraphConvertor::GenerateBroadcastGraph(const TensorOrderMap &tensors) {
  if (error_ != SUCCESS) {
    return *this;
  }
  if (anf_graph_ == nullptr || anf_graph_->output() == nullptr) {
    error_ = INVALID_ARGUMENT;
    MS_LOG(ERROR) << "Invalid AnfGraph in generate broadcast graph";
    return *this;
  }

  DfGraphPtr broadcast_graph = std::make_shared<DfGraph>("broadcast");
  // collect the operators create for broadcast sub graph, in order to avoid auto release
  std::vector<Operator> broadcast_input;
  std::vector<GeTensorDesc> broadcast_desc;
  auto broadcast = std::make_shared<HcomBroadcast>("broadcast_parameter");
  (void)broadcast->set_attr_root_rank(0);
  (void)broadcast->set_attr_group("hccl_world_group");
  broadcast_ops_.push_back(broadcast);

  // find every parameter, build broadcast subgraph (or initialize the parameter with constant)
  for (auto &it : anf_graph_->parameters()) {
    auto op_itor = op_cache_.find(it.get());  // converted node
    if (it->isa<Parameter>() && op_itor != op_cache_.end()) {
      string name = std::static_pointer_cast<Parameter>(it)->name();
      auto tensor_itor = tensors.find(name);  // in init tensor map
      if (tensor_itor != tensors.end()) {
        auto tensor = tensor_itor->second;
        auto shape_ge = tensor->shape_c();

        // create tensor descriptor for output descriptor
        auto desc = TransformUtil::GetGeTensorDesc(shape_ge, tensor->data_type(), kOpFormat_NCHW);
        if (desc == nullptr) {
          MS_LOG(ERROR) << "Create variable " << name << " output descriptor failed!";
          continue;
        }

        // build broadcast subgraph
        if (distribute_) {
          auto broadcast_var = std::make_shared<Variable>(name);
          (void)broadcast_var->update_output_desc_y(*desc);
          broadcast_input.push_back(*broadcast_var);
          broadcast_desc.push_back(*desc);
          broadcast_ops_.push_back(broadcast_var);
        }
      }
    }
  }

  // set up broadcast sub graph
  if (!broadcast_input.empty()) {
    DfGraphConvertor::SetupBroadcast(broadcast, broadcast_desc, broadcast_graph, broadcast_input);
  } else {
    this->broadcast_graph_ = nullptr;
  }
  return *this;
}

DfGraphConvertor &DfGraphConvertor::GenerateCheckpointGraph() {
  if (error_ != SUCCESS) {
    MS_LOG(ERROR) << "Generate checkpoint graph failed, found error code " << error_ << ".";
    return *this;
  }
  if (anf_graph_ == nullptr || anf_graph_->output() == nullptr) {
    error_ = INVALID_ARGUMENT;
    MS_LOG(ERROR) << "Invalid AnfGraph in GenerateCheckpointGraph";
    return *this;
  }
#ifdef ENABLE_D
  auto ms_context = MsContext::GetInstance();
  MS_EXCEPTION_IF_NULL(ms_context);
  if (ms_context->backend_policy() == "ge") {
    BuildSaveCheckpointGraph();
    // Restoring from checkpoint file is done by pyfront, not in graph now.
  }
#endif
  return *this;
}

DfGraphConvertor &DfGraphConvertor::ConvertAllNode() {
  if (error_ != SUCCESS) {
    return *this;
  }
  if (anf_graph_ == nullptr || anf_graph_->output() == nullptr) {
    MS_LOG(ERROR) << "Invalid AnfGraph";
    error_ = FAILED;
    return *this;
  }

  compute_sout_.clear();
  compute_sout_ << "digraph {" << endl;
  init_sout_.clear();
  init_sout_ << "digraph {" << endl;
#ifdef ENABLE_D
  auto ms_context = MsContext::GetInstance();
  MS_EXCEPTION_IF_NULL(ms_context);
  if (ms_context->backend_policy() == "ge") {
    checkpoint_sout_.clear();
    checkpoint_sout_ << "digraph {" << endl;
  }
#endif
  restore_checkpoint_sout_.clear();
  restore_checkpoint_sout_ << "digraph {" << endl;
  // Convert ResizeBilinear attr size to input
  ConvertResizeBilinear(anf_graph_);
  // Convert SpaceBatch attr to input
  ConvertSpaceBatchNd(anf_graph_);
  // Convert Tile input1 to int32
  ConvertTile(anf_graph_);
  // Convert all anf node to Operator
  MS_LOG(INFO) << "convert all node, graph: " << anf_graph_->ToString();
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph_, while_cond_node_);
  for (auto &it : nodes) {
    MS_LOG(DEBUG) << "Convert node: " << it->fullname_with_scope();
    if (IsSubGraph() && it->isa<Parameter>()) {
      continue;
    }
    if (IsSubGraph() && (IsPartialSuccNode(it) || IsPartialCNode(it))) {
      continue;
    }
    (void)Convert(it);
    if (this->error_ != SUCCESS) {
      MS_LOG(ERROR) << "failed to convert node: " << it->DebugString() << ".";
    }
  }

#ifdef ENABLE_D
  // Create dataset iterator and iterator_getnext node
  if (ConfigManager::GetInstance().dataset_mode() == DS_SINK_MODE) {
    DatasetGraphParam param = ConfigManager::GetInstance().dataset_param();
    MS_LOG(INFO) << "Dataset param is " << param.ToString() << ".";
    std::vector<enum ::ge::DataType> getnext_types;
    const auto &origin_ge_types = param.ge_types();
    (void)std::transform(
      origin_ge_types.begin(), origin_ge_types.end(), std::back_inserter(getnext_types),
      [](int64_t t_num) -> enum ::ge::DataType { return static_cast<enum ::ge::DataType>(t_num); });
    if (ms_context->get_param<bool>(MS_CTX_ENABLE_GE_HETEROGENOUS)) {
      // QueueData
      auto queue_data = make_shared<::ge::op::QueueData>("queue_data");
      (void)queue_data->set_attr_output_types(getnext_types);
      (void)queue_data->set_attr_output_shapes(param.shapes());
      (void)queue_data->set_attr_queue_name(param.queue_name());
      (void)queue_data->set_attr_index(0);
      queue_data_ = queue_data;

      // get next from queue
      auto get_next_from_queue = make_shared<::ge::op::GetNextFromQueue>("get_next_from_queue");
      (void)get_next_from_queue->set_attr_output_types(getnext_types);
      (void)get_next_from_queue->set_attr_output_shapes(param.shapes());
      get_next_from_queue_ = get_next_from_queue;
      get_next_from_queue_->SetInput("x", *queue_data_);
    } else {
      // GetNext
      auto iter_getnext_op = make_shared<::ge::op::GetNext>("get_next_tmp");
      (void)iter_getnext_op->set_attr_output_types(getnext_types);
      (void)iter_getnext_op->set_attr_output_shapes(param.shapes());
      (void)iter_getnext_op->set_attr_channel_name(param.queue_name());

      // save iter_getnext_op for later use
      dataset_iter_getnext_ = iter_getnext_op;
    }
  }
#endif
  // return the data flow graph
  return *this;
}

void DfGraphConvertor::SetupDatasetIterGetNextNode() {
  if (ConfigManager::GetInstance().dataset_mode() == DS_SINK_MODE) {
    DatasetGraphParam param = ConfigManager::GetInstance().dataset_param();
    size_t output_num = param.ge_types().size();
    MS_LOG(INFO) << "Set iterator_getnext op's output num = " << output_num << ".";

    auto ms_context = MsContext::GetInstance();
    MS_EXCEPTION_IF_NULL(ms_context);
    if (ms_context->get_param<bool>(MS_CTX_ENABLE_GE_HETEROGENOUS)) {
      MS_EXCEPTION_IF_NULL(get_next_from_queue_);
      // set iterator_getnext op's output num
      MS_LOG(INFO) << "SetupDatasetQueueGetNextNode " << get_next_from_queue_->GetName();
      shared_ptr<::ge::op::GetNextFromQueue> iter_getnext =
        std::static_pointer_cast<::ge::op::GetNextFromQueue>(get_next_from_queue_);
      (void)iter_getnext->create_dynamic_output_y(static_cast<unsigned int>(output_num));
      for (uint32_t i = 0; i < output_num; i++) {
        ::ge::TensorDesc desc(GeShape(param.shapes()[i]), ::ge::FORMAT_NCHW,
                              static_cast<enum ::ge::DataType>(param.ge_types()[i]));
        // we don't SetRealDimCnt here since GE do not use this output's real-dim
        (void)iter_getnext->update_dynamic_output_desc_y((i), desc);
      }
    } else {
      MS_EXCEPTION_IF_NULL(dataset_iter_getnext_);
      // set iterator_getnext op's output num
      shared_ptr<::ge::op::GetNext> iter_getnext = std::static_pointer_cast<::ge::op::GetNext>(dataset_iter_getnext_);
      (void)iter_getnext->create_dynamic_output_y(static_cast<unsigned int>(output_num));
      for (uint32_t i = 0; i < output_num; i++) {
        ::ge::TensorDesc desc(GeShape(param.shapes()[i]), ::ge::FORMAT_NCHW, (::ge::DataType)param.ge_types()[i]);
        // we don't SetRealDimCnt here since GE do not use this output's real-dim
        (void)iter_getnext->update_dynamic_output_desc_y((i), desc);
      }
    }
  }
  return;
}

void DfGraphConvertor::CacheWhileGraph(const CNodePtr &cnode) {
  if (while_graph_cache_.find(cnode) != while_graph_cache_.end()) {
    return;
  }
  ValueNodePtr graph_node = nullptr;
  if (cnode->input(0)->isa<ValueNode>()) {
    graph_node = cnode->input(0)->cast<ValueNodePtr>();
  } else {
    auto partial_node = cnode->input(0);
    graph_node = partial_node->cast<CNodePtr>()->input(1)->cast<ValueNodePtr>();
  }
  MS_EXCEPTION_IF_NULL(graph_node);
  FuncGraphPtr cond_graph = graph_node->value()->cast<FuncGraphPtr>();
  MS_EXCEPTION_IF_NULL(cond_graph);
  const auto &cond_set = cond_graph->nodes();
  for (auto beg = cond_set.begin(); beg != cond_set.end(); ++beg) {
    if (!((*beg)->isa<CNode>())) {
      continue;
    }
    auto c_beg = (*beg)->cast<CNodePtr>();
    if (IsPartialSuccNode(c_beg) && c_beg->inputs().size() == kSwitchInputSize &&
        IsPartialCNode(c_beg->input(kSwitchBodyIndex)) && IsPartialCNode(c_beg->input(kSwitchAfterIndex)) &&
        GetCNodeFuncName(c_beg) == prim::kPrimSwitch->name()) {
      while_graph_cache_[cnode] = {c_beg->input(1), c_beg->input(kSwitchBodyIndex), c_beg->input(kSwitchAfterIndex)};
    }
  }
}

std::vector<Operator> DfGraphConvertor::GetWhileBodyOutputs() {
  std::vector<Operator> outputs;

  const auto &node = anf_graph_->get_return()->input(1);
  AnfNodePtr real_ret = node;
  while (real_ret->isa<CNode>() && GetCNodeTargetFuncName(real_ret->cast<CNodePtr>()) == prim::kPrimDepend->name()) {
    real_ret = real_ret->cast<CNodePtr>()->input(1);
  }

  // skip input of UMonad, IOMonad
  if (HasAbstractUMonad(real_ret) || HasAbstractIOMonad(real_ret)) {
    return outputs;
  }

  // skip input of the None, UpdateState
  if (IsValueNode<None>(real_ret) || IsPrimitiveCNode(real_ret, prim::kPrimUpdateState)) {
    return outputs;
  }

  if (IsPrimitiveCNode(real_ret, prim::kPrimLoad)) {
    real_ret = ParseLoadInput(real_ret->cast<CNodePtr>());
  }

  if (!real_ret->isa<CNode>()) {
    return outputs;
  }

  auto c_node = real_ret->cast<CNodePtr>();
  std::vector<AnfNodePtr> inputs;
  if (c_node->input(0)->isa<CNode>()) {
    auto in0 = c_node->input(0)->cast<CNodePtr>();
    (void)std::copy(in0->inputs().begin() + kInputOffset, in0->inputs().end(), std::back_inserter(inputs));
  }
  size_t partial_input_size = inputs.size();

  (void)std::copy(c_node->inputs().begin() + 1, c_node->inputs().end(), std::back_inserter(inputs));

  for (size_t i = 0; i < inputs.size(); i++) {
    auto j = inputs[i];
    CNodePtr cur = nullptr;
    if (i < partial_input_size) {
      cur = c_node->input(0)->cast<CNodePtr>();
    } else {
      cur = c_node;
    }
    j = GetRealInputNode(cur, j);
    if (j == nullptr) {
      continue;
    }

    if (j->isa<Parameter>()) {
      int64_t idx = find(inputs_.begin(), inputs_.end(), j) - inputs_.begin();
      auto idx_cond = body_cond_map_[idx];
      if (while_used_input_index_.find(idx_cond) == while_used_input_index_.end() ||
          while_const_input_index_.find(idx_cond) != while_const_input_index_.end()) {
        continue;
      }
      outputs.push_back(*(subgraph_input_cache_[idx]));
    } else {
      outputs.push_back(*Convert(j));
    }
  }
  MS_LOG(DEBUG) << "get while body outputs size: " << outputs.size();
  return outputs;
}

std::shared_ptr<std::vector<Operator>> DfGraphConvertor::GetWhileSubGraphInput() {
  std::shared_ptr<std::vector<Operator>> graph_in = std::make_shared<std::vector<Operator>>();
  subgraph_input_cache_.clear();
  size_t i = 0;
  OperatorPtr op = nullptr;
  ParamIndexMap cond_body;
  std::string name_app = "_in_cond";
  if (graph_type_ == GraphType::kBody) {
    name_app = "_in_body";
    for (auto &p : body_cond_map_) {
      cond_body[p.second] = p.first;
    }
  }
  for (auto &idx : while_used_input_index_) {
    if (while_const_input_index_.find(idx) == while_const_input_index_.end()) {
      op = std::make_shared<Data>();
      (void)std::static_pointer_cast<Data>(op)->set_attr_index(i);
      i++;
    } else {
      auto temp = while_const_input_index_[idx].op;
      auto name = temp->GetName();
      auto value = const_op_to_value_[temp];
      MS_EXCEPTION_IF_NULL(value);
      auto adpt_const = FindAdapter(kNameConst, training_);
      if (adpt_const == nullptr) {
        continue;
      }
      name += name_app;
      auto const_op = adpt_const->generate(name);
      (void)adpt_const->setAttr(const_op, "value", value);
      auto const_op_desc = TransformUtil::GetGeTensorDesc(value->shape_c(), value->data_type(), kOpFormat_NCHW);
      if (const_op_desc == nullptr) {
        MS_LOG(WARNING) << "Create variable " << name << " output descriptor failed!";
        continue;
      }
      (void)std::static_pointer_cast<Constant>(const_op)->update_output_desc_y(*const_op_desc);
      op = const_op;
    }
    graph_in->push_back(*op);
    if (graph_type_ == GraphType::kCond) {
      subgraph_input_cache_[idx] = op;
    } else if (graph_type_ == GraphType::kBody) {
      subgraph_input_cache_[cond_body[idx]] = op;
    }
  }
  MS_LOG(DEBUG) << "created " << subgraph_input_cache_.size() << " data node "
                << " in graph: " << anf_graph_->ToString();
  return graph_in;
}

void DfGraphConvertor::BuildWhileSubGraph() {
  // set up dependencies
  MS_LOG(DEBUG) << "begin to build graph: " << anf_graph_->ToString();
  MS_LOG(DEBUG) << "set up dependencies";

  std::vector<Operator> graph_in = *GetWhileSubGraphInput();
  auto nodes = GetOrderedCNodes(anf_graph_, while_cond_node_);

  auto iter = std::find_if(nodes.begin(), nodes.end(), [](const AnfNodePtr &it) {
    if (IsPartialSuccNode(it) && IsWhileNode(it)) {
      return true;
    }
    return false;
  });
  if (iter != nodes.end()) {
    call_node_in_while_body_ = *iter;
  }
  AnfNodePtr real_ret = anf_graph_->get_return()->input(1);
  while (real_ret->isa<CNode>() && GetCNodeTargetFuncName(real_ret->cast<CNodePtr>()) == prim::kPrimDepend->name()) {
    real_ret = real_ret->cast<CNodePtr>()->input(1);
  }
  for (auto &it : nodes) {
    if (it->isa<CNode>() && IsCaseNode(it->cast<CNodePtr>())) {
      auto node = it->cast<CNodePtr>();
      MS_EXCEPTION_IF_NULL(node);
      auto input = node->input(0);
      MS_EXCEPTION_IF_NULL(input);
      auto cinput = input->cast<CNodePtr>();
      GetCaseNodeInput(node, cinput);
    }
  }

  for (auto &it : nodes) {
    if (it == real_ret || HasAbstractMonad(it)) {
      continue;
    }
    SetNodeInput(it);
    SetSubgraph(it);
    UpdateOpDesc(it);
  }
  MS_LOG(DEBUG) << "trace output";
  std::vector<Operator> graph_out;
  auto graph_name = TransformUtil::NormOpName(cur_while_node_->fullname_with_scope());
  if (graph_type_ == GraphType::kCond) {
    if (op_cache_.find(while_cond_node_.get()) == op_cache_.end()) {
      return;
    }
    graph_name += "_cond_graph";
    graph_out.push_back(*(op_cache_[while_cond_node_.get()]));
  } else {
    graph_name += "_body_graph";
    graph_out = GetWhileBodyOutputs();
  }
  if (error_ == SUCCESS) {
    if (df_graph_->GetName() != graph_name) {
      MS_LOG(DEBUG) << "convert anf graph name : " << df_graph_->GetName() << " to df graph name: " << graph_name;
    }
    df_graph_ = make_shared<DfGraph>(graph_name);
  } else {
    return;
  }
  MS_LOG(DEBUG) << "set while sub graph input num: " << graph_in.size();
  MS_LOG(DEBUG) << "set while sub graph output num: " << graph_out.size();

  compute_sout_ << "}" << endl;
  (void)df_graph_->SetInputs(graph_in).SetOutputs(graph_out);
  IdentityOptimization();
  MS_LOG(DEBUG) << "build graph: " << anf_graph_->ToString() << " end";
}

void DfGraphConvertor::BuildWhileAfterSubGraph() {
  size_t i = 0;
  prev_cond_to_while_out_index_.clear();
  for (auto n : prev_while_used_input_index_) {
    if (prev_while_const_input_index_.find(n) == prev_while_const_input_index_.end()) {
      prev_cond_to_while_out_index_[n] = i;
      i++;
    }
  }
  GetCallNodeInputs(cur_while_node_);
  // set up dependencies
  MS_LOG(INFO) << "set up dependencies";
  auto nodes = GetOrderedCNodes(anf_graph_);
  for (auto &it : nodes) {
    SetNodeInput(it);
    SetSubgraph(it);
    UpdateOpDesc(it);
  }
  MS_LOG(INFO) << "trace output";
  if (graph_outputs_.empty()) {
    SetGraphOutputs();
  }
  compute_sout_ << "}" << endl;
  return;
}

void DfGraphConvertor::ConvertWhileBody(const AnfNodePtr &node) {
  if (!node->isa<CNode>() || GetCNodeFuncName(node->cast<CNodePtr>()) != prim::kPrimPartial->name()) {
    return;
  }
  MS_LOG(DEBUG) << "begin to convert while node body graph";
  auto graph_node = node->cast<CNodePtr>()->input(1)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(graph_node);
  FuncGraphPtr anf_graph = graph_node->value()->cast<FuncGraphPtr>();
  DfGraphConvertor converter(anf_graph);
  converter.use_inputs_ = true;

  const auto &params = anf_graph->parameters();
  converter.inputs_ = params;

  converter.graph_type_ = GraphType::kBody;
  converter.cur_while_node_ = cur_while_node_;
  converter.body_cond_map_ = body_cond_map_;
  converter.while_const_input_index_ = while_const_input_index_;
  converter.while_used_input_index_ = while_used_input_index_;
  converter.const_op_to_value_ = const_op_to_value_;
  converter.ConvertAllNode().BuildWhileSubGraph();
  while_dfgraph_cache_[cur_while_node_]->push_back(*(converter.df_graph_));
  std::string name = graph_node->ToString() + "_ge_graph.dot";
  if (MsContext::GetInstance()->get_param<bool>(MS_CTX_SAVE_GRAPHS_FLAG)) {
    converter.DrawComputeGraph(name);
  }
  MS_LOG(DEBUG) << "convert while node body graph end";
  return;
}

void DfGraphConvertor::GetWhileUsedInputIndex(const std::vector<AnfNodePtr> &graphs) {
  if (!while_used_input_index_.empty()) {
    return;
  }

  auto cond_graph_node = graphs.at(0);
  auto graph = cond_graph_node->func_graph();
  const auto &cond_params = graph->parameters();
  auto nodes = GetOrderedCNodes(graph, cond_graph_node);

  std::set<size_t> used_params_index;
  for (auto &n : nodes) {
    if (!n->isa<CNode>()) {
      continue;
    }
    auto c = n->cast<CNodePtr>();
    auto inputs = c->inputs();
    for (size_t idx = 1; idx < inputs.size(); idx++) {
      auto &i = inputs[idx];
      if (!i->isa<Parameter>() || HasAbstractMonad(i) || IsDynamicShapeNode(i)) {
        continue;
      }
      auto idx_cond = std::find(cond_params.begin(), cond_params.end(), i) - cond_params.begin();
      (void)used_params_index.insert(idx_cond);
    }
  }

  auto body_graph_node_in_cond = graphs.at(1)->cast<CNodePtr>();
  auto body_graph_node = body_graph_node_in_cond->input(1)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(body_graph_node);
  graph = body_graph_node->value()->cast<FuncGraphPtr>();
  const auto &body_params = graph->parameters();

  auto real_ret = graph->get_return()->input(1);
  while (real_ret->isa<CNode>() && GetCNodeTargetFuncName(real_ret->cast<CNodePtr>()) == prim::kPrimDepend->name()) {
    real_ret = real_ret->cast<CNodePtr>()->input(1);
  }

  nodes = GetOrderedCNodes(graph);
  for (auto &n : nodes) {
    if (!n->isa<CNode>()) {
      continue;
    }
    auto c = n->cast<CNodePtr>();
    if (c == real_ret || c == real_ret->cast<CNodePtr>()->input(0)) {
      continue;
    }
    auto inputs = c->inputs();
    for (size_t idx = 1; idx < inputs.size(); idx++) {
      auto &i = inputs[idx];
      if (!i->isa<Parameter>() || HasAbstractMonad(i) || IsDynamicShapeNode(i)) {
        continue;
      }
      auto idx_body = std::find(body_params.begin(), body_params.end(), i) - body_params.begin();
      auto p = body_graph_node_in_cond->input(idx_body + kInputOffset);
      auto idx_cond = std::find(cond_params.begin(), cond_params.end(), p) - cond_params.begin();
      (void)used_params_index.insert(idx_cond);
    }
  }
  while_used_input_index_ = used_params_index;
}

void DfGraphConvertor::SetParamIndexMap(const std::vector<AnfNodePtr> &graphs) {
  auto cond_graph_node = graphs.at(0);
  auto cond_graph = cond_graph_node->func_graph();
  const auto &cond_params = cond_graph->parameters();

  auto body_graph_node = graphs.at(1);
  if (!body_graph_node->isa<CNode>()) {
    return;
  }
  auto body_graph_node_inputs = body_graph_node->cast<CNodePtr>()->inputs();
  std::vector<AnfNodePtr> body_params;
  for (auto it = body_graph_node_inputs.begin() + kInputOffset; it != body_graph_node_inputs.end(); ++it) {
    body_params.push_back(*it);
  }

  for (size_t i = 0; i < body_params.size(); i++) {
    auto p = body_params[i];
    int64_t idx = find(cond_params.begin(), cond_params.end(), p) - cond_params.begin();
    body_cond_map_[i] = static_cast<size_t>(idx);
    MS_LOG(DEBUG) << "body_cond_map_'s key: " << i << " value: " << idx;
  }

  auto after_graph_node = graphs.at(kSwitchBodyIndex);
  if (!after_graph_node->isa<CNode>()) {
    return;
  }
  auto after_graph_node_inputs = after_graph_node->cast<CNodePtr>()->inputs();
  std::vector<AnfNodePtr> after_params;
  for (auto it = after_graph_node_inputs.begin() + 2; it != after_graph_node_inputs.end(); ++it) {
    after_params.push_back(*it);
  }

  for (size_t i = 0; i < after_params.size(); i++) {
    auto p = after_params[i];
    int64_t idx = find(cond_params.begin(), cond_params.end(), p) - cond_params.begin();
    after_cond_map_[i] = static_cast<size_t>(idx);
    MS_LOG(DEBUG) << "after_cond_map_'s key: " << i << " value: " << idx;
  }
  return;
}

void DfGraphConvertor::ConvertWhileCond(const AnfNodePtr &node) {
  MS_LOG(DEBUG) << "begin to convert while node cond graph";
  auto func_graph = node->func_graph();

  DfGraphConvertor converter(func_graph);
  converter.use_inputs_ = true;

  converter.inputs_ = func_graph->parameters();

  converter.graph_type_ = GraphType::kCond;
  converter.cur_while_node_ = cur_while_node_;
  converter.while_cond_node_ = node;
  converter.while_const_input_index_ = while_const_input_index_;
  converter.while_used_input_index_ = while_used_input_index_;
  converter.const_op_to_value_ = const_op_to_value_;
  converter.ConvertAllNode().BuildWhileSubGraph();
  while_dfgraph_cache_[cur_while_node_]->push_back(*(converter.df_graph_));
  std::string name = func_graph->ToString() + "_ge_graph.dot";
  if (MsContext::GetInstance()->get_param<bool>(MS_CTX_SAVE_GRAPHS_FLAG)) {
    converter.DrawComputeGraph(name);
  }

  MS_LOG(DEBUG) << "convert while node cond graph end";
}

void DfGraphConvertor::SetWhileOutputHandle(const OperatorPtr &prev_while_op) {
  if (while_output_handle_cache_.find(prev_while_node_) != while_output_handle_cache_.end()) {
    return;
  }
  auto out_handler = std::make_shared<std::vector<OutHandler>>();
  string str = "output";
  for (size_t i = 0; i < prev_while_node_out_size_; i++) {
    (void)out_handler->emplace_back(prev_while_op, str + std::to_string(i), prev_while_node_);
  }
  while_output_handle_cache_[prev_while_node_] = out_handler;
  return;
}

void DfGraphConvertor::ConvertWhileAfter(const AnfNodePtr &node) {
  if (!node->isa<CNode>() || GetCNodeFuncName(node->cast<CNodePtr>()) != prim::kPrimPartial->name()) {
    return;
  }
  MS_LOG(DEBUG) << "begin to convert while node after graph";
  auto graph_node = node->cast<CNodePtr>()->input(1)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(graph_node);
  FuncGraphPtr anf_graph = graph_node->value()->cast<FuncGraphPtr>();
  DfGraphConvertor converter(anf_graph);
  converter.use_inputs_ = true;

  const auto &params = anf_graph->parameters();
  converter.inputs_ = params;

  converter.graph_type_ = GraphType::kAfter;
  converter.prev_after_cond_map_ = after_cond_map_;
  converter.prev_while_node_ = cur_while_node_;
  converter.prev_while_node_out_size_ = cur_while_node_out_size_;
  converter.bypass_node_prev_handle_cache_ = bypass_node_handle_cache_;
  converter.prev_while_used_input_index_ = while_used_input_index_;
  converter.prev_while_const_input_index_ = while_const_input_index_;
  converter.const_op_to_value_ = const_op_to_value_;

  auto while_op = Convert(converter.prev_while_node_);
  converter.SetWhileOutputHandle(while_op);
  converter.ConvertAllNode().BuildWhileAfterSubGraph();
  std::string name = graph_node->ToString() + "_ge_graph.dot";
  if (MsContext::GetInstance()->get_param<bool>(MS_CTX_SAVE_GRAPHS_FLAG)) {
    converter.DrawComputeGraph(name);
  }
  MS_LOG(DEBUG) << "add while after graph " << converter.graph_const_inputs_.size()
                << " const inputs to main graph const inputs";
  (void)std::transform(converter.graph_const_inputs_.begin(), converter.graph_const_inputs_.end(),
                       std::back_inserter(graph_const_inputs_), [](OperatorPtr x) { return x; });

  graph_outputs_ = converter.graph_outputs_;
  MS_LOG(DEBUG) << "convert while node after graph end";
  return;
}

void DfGraphConvertor::ConvertWhileNode(const CNodePtr &node) {
  if (IsSubGraph()) {
    return;
  }

  auto while_graph = while_graph_cache_[node];
  cur_while_node_ = node;

  auto &while_inputs = *(call_input_handle_cache_[node]);
  cur_while_node_out_size_ = while_inputs.size();
  while_dfgraph_cache_[node] = std::make_shared<std::vector<DfGraph>>();
  // convert cond graph
  MS_LOG(DEBUG) << "convert while cond begin...";
  auto cond_graph_node = while_graph[0];
  ConvertWhileCond(cond_graph_node);
  MS_LOG(DEBUG) << "convert while cond end...";

  // convert body graph
  MS_LOG(DEBUG) << "convert while body begin...";
  auto body_graph_node = while_graph[1];
  ConvertWhileBody(body_graph_node);
  MS_LOG(DEBUG) << "convert while body end...";

  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    MS_LOG(DEBUG) << "Not found adapter";
    return;
  }

  OperatorPtr op = Convert(node);
  auto graphs = while_dfgraph_cache_[node];
  adpt->setSubgraph(op, graphs);

  // convert after graph
  MS_LOG(DEBUG) << "convert while after begin...";
  auto after_graph_node = while_graph[kAfterIndexInCache];
  ConvertWhileAfter(after_graph_node);
  MS_LOG(DEBUG) << "convert while after end...";
  return;
}

void DfGraphConvertor::SetSubgraph(const AnfNodePtr &node) {
  if (!node->isa<CNode>()) {
    return;
  }
  MS_LOG(DEBUG) << "set sub graph begin";
  auto cnode = node->cast<CNodePtr>();
  if (IsWhileNode(cnode)) {
    CacheWhileGraph(cnode);
    ConvertWhileNode(cnode);
    MS_LOG(DEBUG) << "set sub graph end....";
    return;
  }

  if (!IsCaseNode(cnode)) {
    MS_LOG(DEBUG) << "set sub graph end....";
    return;
  }
  std::vector<AnfNodePtr> case_inputs;
  std::shared_ptr<std::vector<DfGraph>> df_branches = std::make_shared<std::vector<DfGraph>>();
  case_call_input_size_ = 0;
  if (IsNormalGraph()) {
    for (size_t i = 1; i < cnode->inputs().size(); i++) {
      (void)case_inputs.emplace_back(cnode->input(i));
      case_call_input_size_++;
    }
    auto bnode = cnode->input(0)->cast<CNodePtr>()->input(2)->cast<CNodePtr>();

    for (size_t i = 1; i < bnode->inputs().size(); i++) {
      if (!bnode->input(i)->isa<CNode>()) {
        continue;
      }
      auto branch_node = bnode->input(i)->cast<CNodePtr>();
      for (size_t j = kInputOffset; j < branch_node->inputs().size(); j++) {
        if (std::find(case_inputs.begin(), case_inputs.end(), branch_node->input(j)) == case_inputs.end()) {
          (void)case_inputs.emplace_back(branch_node->input(j));
        }
      }
    }
    for (size_t i = 1; i < bnode->inputs().size(); i++) {
      ProcessSubgraph(bnode->input(i), case_inputs);
    }
    for (size_t i = 1; i < bnode->inputs().size(); i++) {
      (void)df_branches->emplace_back(branches_map_[bnode->input(i).get()]);
    }
  } else {
    std::vector<AnfNodePtr> inputs;
    for (size_t i = 1; i < cnode->inputs().size(); i++) {
      auto ele_node = cnode->input(i);
      if (HasAbstractMonad(ele_node)) {
        continue;
      }
      inputs.push_back(ele_node);
      case_call_input_size_++;
    }
    auto bnode = cnode->input(0)->cast<CNodePtr>()->input(kInputOffset);
    auto cbnode = bnode->cast<CNodePtr>();

    for (size_t i = 1; i < cbnode->inputs().size(); i++) {
      auto br = cbnode->input(i);
      if (!cbnode->input(i)->isa<CNode>()) {
        ProcessSubgraph(br, inputs);
        (void)df_branches->emplace_back(branches_map_[br.get()]);
        continue;
      }

      auto branch_node = cbnode->input(i)->cast<CNodePtr>();
      auto branch_input = inputs;
      for (size_t j = kInputOffset; j < branch_node->inputs().size(); j++) {
        branch_input.push_back(branch_node->input(j));
      }
      ProcessSubgraph(cbnode->input(i), branch_input);
      (void)df_branches->emplace_back(branches_map_[br.get()]);
    }
  }
  if (op_cache_.find(node.get()) == op_cache_.end()) {
    return;
  }

  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    MS_LOG(DEBUG) << "Not found adapter";
    return;
  }

  OperatorPtr op = Convert(node);

  adpt->setSubgraph(op, 0, df_branches);
  MS_LOG(DEBUG) << "set sub graph end....";
  return;
}

void DfGraphConvertor::GetCaseNodeInput(const CNodePtr node, const CNodePtr input_node) {
  if (case_input_handle_cache_.find(node.get()) != case_input_handle_cache_.end()) {
    return;
  }

  std::vector<AnfNodePtr> case_inputs;
  const size_t case_index = 1;
  const size_t make_tuple_index = 2;

  AnfNodePtr case_index_iter = input_node->input(case_index);
  AnfNodePtr make_tuple_iter = input_node->input(make_tuple_index);
  AnfNodePtr make_tuple_node = make_tuple_iter;

  std::shared_ptr<std::vector<OutHandler>> tuple_items = std::make_shared<std::vector<OutHandler>>();

  if (IsNormalGraph()) {
    make_tuple_node = make_tuple_iter->cast<CNodePtr>();
    for (size_t i = 1; i < node->inputs().size(); i++) {
      (void)case_inputs.emplace_back(node->input(i));
    }

    auto bnode = input_node->input(2)->cast<CNodePtr>();
    MS_EXCEPTION_IF_NULL(bnode);
    for (size_t i = 1; i < bnode->inputs().size(); i++) {
      if (!bnode->input(i)->isa<CNode>()) {
        continue;
      }
      auto branch_node = bnode->input(i)->cast<CNodePtr>();
      for (size_t j = 2; j < branch_node->inputs().size(); j++) {
        if (std::find(case_inputs.begin(), case_inputs.end(), branch_node->input(j)) == case_inputs.end()) {
          (void)case_inputs.emplace_back(branch_node->input(j));
        }
      }
    }
    for (size_t i = 0; i < case_inputs.size(); i++) {
      auto item = case_inputs[i];
      tuple_items->push_back(GetHandler(item));
    }
  } else if (IsSubGraph()) {
    auto &inputs = node->inputs();
    for (size_t i = 1; i < inputs.size(); i++) {
      auto input = inputs[i];
      if (HasAbstractUMonad(input) || HasAbstractIOMonad(input)) {
        continue;
      }
      if (input->isa<Parameter>()) {
        auto idx = std::find(inputs_.begin(), inputs_.end(), input) - inputs_.begin();
        tuple_items->push_back(OutHandler(subgraph_input_cache_[idx], "", input));
      } else {
        tuple_items->push_back(GetHandler(input));
      }
    }
    MS_LOG(DEBUG) << "tuple input size of case in sub graph is " << tuple_items->size();
  } else {
    MS_LOG(ERROR) << "case in after graph is not supported.";
    return;
  }

  tuple_out_handle_cache_[make_tuple_node.get()] = tuple_items;

  std::shared_ptr<std::vector<AnfNodePtr>> case_input_items = std::make_shared<std::vector<AnfNodePtr>>();
  (void)case_input_items->emplace_back(case_index_iter);
  (void)case_input_items->emplace_back(make_tuple_iter);
  case_input_handle_cache_[node.get()] = case_input_items;
}

void DfGraphConvertor::GetCallNodeInputs(const CNodePtr &node) {
  if (node == nullptr) {
    return;
  }
  if (call_input_handle_cache_.find(node) != call_input_handle_cache_.end()) {
    return;
  }
  MS_LOG(DEBUG) << "begin to get call node inputs.";

  auto call_input_items = std::make_shared<std::vector<OutHandler>>();
  std::vector<AnfNodePtr> inputs;
  if (node->input(0)->isa<CNode>()) {
    auto in0 = node->input(0)->cast<CNodePtr>();
    (void)std::copy(in0->inputs().begin() + kInputOffset, in0->inputs().end(), std::back_inserter(inputs));
  }
  (void)std::copy(node->inputs().begin() + 1, node->inputs().end(), std::back_inserter(inputs));

  auto &params = anf_graph_->parameters();
  auto while_op = Convert(node);

  while_const_input_index_.clear();
  std::set<size_t> while_input_node_index;
  for (auto iter = while_used_input_index_.begin(); iter != while_used_input_index_.end(); ++iter) {
    auto n = inputs[*iter];
    OutHandler out_handler;
    if (IsAfterGraph() && n->isa<Parameter>()) {
      auto idx = std::find(params.begin(), params.end(), n) - params.begin();
      auto idx_cond = prev_after_cond_map_[idx];
      if (bypass_node_prev_handle_cache_.find(idx_cond) != bypass_node_prev_handle_cache_.end()) {
        out_handler = bypass_node_prev_handle_cache_[idx_cond];
      } else {
        auto idx_out = prev_cond_to_while_out_index_[idx_cond];
        out_handler = while_output_handle_cache_[prev_while_node_]->at(idx_out);
      }
    } else {
      out_handler = GetHandler(inputs[*iter]);
    }
    if ((out_handler.op->GetOpType() == "Const" || out_handler.op->GetOpType() == "Constant") &&
        const_op_to_value_.find(out_handler.op) != const_op_to_value_.end()) {
      while_const_input_index_[*iter] = out_handler;
    } else {
      (void)while_input_node_index.insert(*iter);
      call_input_items->push_back(out_handler);
    }
  }
  cur_while_node_out_size_ = call_input_items->size();
  bypass_node_handle_cache_.clear();

  for (size_t i = 0; i < inputs.size(); i++) {
    if (while_input_node_index.find(i) == while_input_node_index.end()) {
      auto n = inputs[i];
      if (HasAbstractMonad(n)) {
        continue;
      }
      if (IsAfterGraph() && n->isa<Parameter>()) {
        auto idx = std::find(params.begin(), params.end(), n) - params.begin();
        auto idx_cond = prev_after_cond_map_[idx];
        if (bypass_node_prev_handle_cache_.find(idx_cond) != bypass_node_prev_handle_cache_.end()) {
          bypass_node_handle_cache_[i] = bypass_node_prev_handle_cache_[idx_cond];
        } else {
          auto idx_out = prev_cond_to_while_out_index_[idx_cond];
          bypass_node_handle_cache_[i] = while_output_handle_cache_[prev_while_node_]->at(idx_out);
        }
      } else {
        bypass_node_handle_cache_[i] = GetHandler(n);
      }
    }
  }

  MS_LOG(DEBUG) << "while node out size: " << cur_while_node_out_size_
                << ", while const input size: " << while_const_input_index_.size()
                << ", bypass node size: " << bypass_node_handle_cache_.size();
  auto op = Convert(node);
  auto adpt = FindAdapter(node, training_);
  adpt->setDynamicOutputNum(op, cur_while_node_out_size_);

  call_input_handle_cache_[node] = call_input_items;
  MS_LOG(DEBUG) << "get call node inputs end.";
  return;
}

void DfGraphConvertor::SetGraphInputs(std::vector<Operator> *inputs) {
  if (ConfigManager::GetInstance().dataset_mode() == DS_SINK_MODE) {
    auto ms_context = MsContext::GetInstance();
    MS_EXCEPTION_IF_NULL(ms_context);
    if (ms_context->get_param<bool>(MS_CTX_ENABLE_GE_HETEROGENOUS)) {
      inputs->push_back(*queue_data_);
    } else {
      inputs->push_back(*dataset_iter_getnext_);
    }
  } else {
    auto params = anf_graph_->parameters();
    if (use_inputs_) {
      params = inputs_;
      auto anf_params = anf_graph_->parameters();
      for (size_t i = 0; i < params.size(); i++) {
        for (size_t j = 0; j < anf_params.size(); j++) {
          if (TransformUtil::NormOpName(params[i]->ToString()) ==
              TransformUtil::NormOpName(anf_params[j]->ToString())) {
            params[i] = anf_params[j];
          }
        }
      }
    }

    int index = 0;
    for (auto &it : params) {
      auto name = std::static_pointer_cast<Parameter>(it)->name();
      //  the parameters which has not been converted to var
      if (vars_.find(name) == vars_.end()) {
        if (HasAbstractMonad(it)) {
          MS_LOG(INFO) << it->DebugString() << " is a monad parameter, skip.";
          continue;
        }
        auto op = Convert(it);
        MS_EXCEPTION_IF_NULL(op);
        MS_LOG(INFO) << "add not var input " << it->ToString() << ", index " << index;
        if (op == nullptr) {
          MS_LOG(ERROR) << "Convert graph failed!";
          return;
        }
        UpdateDataOpDesc(it, op);

        MS_LOG(INFO) << "add input " << it->ToString() << ", index " << index;
        (void)std::static_pointer_cast<Data>(op)->set_attr_index(index++);
        inputs->push_back(*op);
      } else if (vars_[name] != nullptr) {
        MS_LOG(INFO) << "add var input " << it->ToString();
        auto op = Convert(it);
        MS_EXCEPTION_IF_NULL(op);
        UpdateConstOpDesc(it, vars_[name]);
        inputs->push_back(*op);
      }
    }
  }
}

DfGraphConvertor &DfGraphConvertor::BuildGraph() {
  MS_LOG(INFO) << "Start BuildGraph, graph: " << anf_graph_->ToString();
  SetupDatasetIterGetNextNode();

  if (error_ != SUCCESS) {
    return *this;
  }

  GetCallNodeInputs(cur_while_node_);
  // Case node set input.
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph_);
  for (auto &it : nodes) {
    if (it != nullptr && it->isa<CNode>() && IsCaseNode(it->cast<CNodePtr>())) {
      auto node = it->cast<CNodePtr>();
      auto input_node = node->input(0)->cast<CNodePtr>();
      GetCaseNodeInput(node, input_node);
    }
  }

  // set up dependencies
  MS_LOG(DEBUG) << "set up dependencies";
  nodes = GetOrderedCNodes(anf_graph_);
  for (auto &it : nodes) {
    SetNodeInput(it);
    SetSubgraph(it);
    UpdateOpDesc(it);
  }

  if (error_ == SUCCESS) {
    df_graph_ = make_shared<DfGraph>(anf_graph_->ToString());
  } else {
    return *this;
  }

  // set graph input according to the order from anf graph
  std::vector<Operator> inputs;
  SetGraphInputs(&inputs);

  // Add const nodes as graph input for some operator work with constant
  MS_LOG(INFO) << "graph const input size: " << graph_const_inputs_.size();
  (void)std::transform(graph_const_inputs_.begin(), graph_const_inputs_.end(), std::back_inserter(inputs),
                       [](const OperatorPtr &x) { return *x; });

  MS_LOG(INFO) << "set graph input num: " << inputs.size();
  (void)df_graph_->SetInputs(inputs);

  SetGraphOutputs();
  (void)df_graph_->SetOutputs(graph_outputs_);

  IdentityOptimization();

  compute_sout_ << "}" << endl;
  // For the graph(e.g. eval_subgraph) whose IterNum is 1, donot set NeedIteration flag.
  if (ConfigManager::GetInstance().iter_num() > 1) {
    df_graph_->SetNeedIteration(true);
  }
  MS_LOG(INFO) << "End BuildGraph, graph: " << anf_graph_->ToString();
  return *this;
}

void DfGraphConvertor::SetGraphOutputs() {
  if (cur_while_node_ == nullptr) {
    graph_outputs_.clear();
    auto ret_node = anf_graph_->get_return();
    auto adpt = FindAdapter(ret_node, training_);
    MS_EXCEPTION_IF_NULL(adpt);
    auto handles = adpt->getOutputs(Convert(ret_node));
    for (const auto &handle : handles) {
      (void)graph_outputs_.emplace_back(std::make_pair(*handle.op, handle.out));
    }
  }

  MS_LOG(INFO) << "Set graph " << anf_graph_->ToString() << " output, num: " << graph_outputs_.size();
  for (size_t i = 0; i < graph_outputs_.size(); i++) {
    MS_LOG(INFO) << "Graph output " << i << ": node: " << graph_outputs_[i].first.GetName()
                 << ", out: " << graph_outputs_[i].second;
  }
}

void DfGraphConvertor::UpdateConstOpDesc(const AnfNodePtr &it, const OperatorPtr &op) const {
  if (!it->isa<Parameter>()) {
    MS_LOG(DEBUG) << "It is not parameter, name: " << it->DebugString();
    return;
  }
  auto para = it->cast<ParameterPtr>();
  MS_EXCEPTION_IF_NULL(para);
  std::string format = kOpFormat_NCHW;
  std::string param_debug_info = para->DebugString();
  auto param_format = param_format_.find(param_debug_info);
  if (param_format != param_format_.end()) {
    format = param_format->second;
    MS_LOG(DEBUG) << "Parameter debug info: " << param_debug_info << ", format is " << format;
  }
  if (format == kOpFormat_NCHW) {
    MS_LOG(DEBUG) << "Format is not changed, no need to update op desc, name: " << param_debug_info;
    return;
  }
  if (!para->has_default()) {
    MS_LOG(DEBUG) << "Parameter has no default, no need to update op desc, name: " << param_debug_info;
    return;
  }
  auto value = para->default_param();
  MS_EXCEPTION_IF_NULL(value);
  auto tensor = value->cast<std::shared_ptr<tensor::Tensor>>();
  MS_EXCEPTION_IF_NULL(tensor);
  auto const_op_desc = TransformUtil::GetGeTensorDesc(tensor->shape_c(), tensor->data_type(), format);
  if (const_op_desc == nullptr) {
    MS_LOG(WARNING) << "Create parameter " << para->name() << " output descriptor failed!";
    return;
  }
  (void)std::static_pointer_cast<Constant>(op)->update_output_desc_y(*const_op_desc);
}

void DfGraphConvertor::UpdateDataOpDesc(const AnfNodePtr &it, const OperatorPtr &op) const {
  auto node = std::static_pointer_cast<AnfNode>(it);
  if (node == nullptr) {
    MS_LOG(ERROR) << "Update data op descriptor failed! Invalid node.";
    return;
  }
  std::vector<int64_t> shape;
  if (auto normal_shape_ptr = dyn_cast<abstract::Shape>(node->Shape()); normal_shape_ptr != nullptr) {
    shape = normal_shape_ptr->shape();
  } else if (auto no_shape_ptr = dyn_cast<abstract::NoShape>(node->Shape()); no_shape_ptr != nullptr) {
    shape = {};
  } else {
    MS_LOG(INFO) << "Invalid shape to update data op descriptor.";
    return;
  }
  if (node->Type() == nullptr) {
    MS_LOG(INFO) << "Invalid type to update data op descriptor.";
    return;
  }
  TypeId me_type = node->Type()->type_id();
  if (kObjectTypeTensorType == me_type) {
    me_type = dyn_cast<TensorType>(node->Type())->element()->type_id();
  }
  std::ostringstream buf;
  buf << "[" << shape << "]";
  MS_LOG(INFO) << "input shape is " << buf.str() << ", type is " << me_type;
  std::string format = "NCHW";
  if (it->isa<Parameter>()) {
    auto param = it->cast<ParameterPtr>();
    std::string param_name = param->DebugString();
    auto param_format = param_format_.find(param_name);
    if (param_format != param_format_.end()) {
      format = param_format->second;
      MS_LOG(DEBUG) << "parameter: " << param_name << ", format is " << format;
    }
  }
  auto desc = TransformUtil::GetGeTensorDesc(shape, me_type, format);
  if (desc == nullptr) {
    MS_LOG(ERROR) << "Update data op descriptor failed! TensorDesc is null.";
  } else {
    (void)std::static_pointer_cast<Data>(op)->update_input_desc_x(*desc);
    (void)std::static_pointer_cast<Data>(op)->update_output_desc_y(*desc);
  }
}

DfGraphPtr DfGraphConvertor::GetComputeGraph() { return df_graph_; }

DfGraphPtr DfGraphConvertor::GetInitGraph() { return init_graph_; }

DfGraphPtr DfGraphConvertor::GetSaveCheckpointGraph() { return save_ckp_graph_; }

DfGraphPtr DfGraphConvertor::GetBroadcastGraph() { return broadcast_graph_; }

const std::vector<std::string> trans_var_list = {string(kNameAssign), string(kNameAssignAdd), string(kNameAssignSub)};

AnfNodePtr DfGraphConvertor::ParseLoadInput(const CNodePtr &cnode) const {
  if (cnode->inputs().size() < 3) {
    MS_LOG(EXCEPTION) << "input size error, " << cnode->ToString();
  }
  const size_t para_index = 1;
  return cnode->input(para_index);
}

void DfGraphConvertor::TransformConstOp(const CNodePtr &node, const AnfNodePtr &pred) {
  // transform "Const" op to "Variable" op when the next node is "Assign" op.
  std::string c_name = GetCNodeTargetFuncName(node);
  auto pos = std::find(trans_var_list.begin(), trans_var_list.end(), c_name);
  if (!training_ && !IsSubGraph() && pos != trans_var_list.end() && pred->isa<Parameter>()) {
    std::string name = std::static_pointer_cast<Parameter>(pred)->name();
    auto op_itor = op_cache_.find(pred.get());
    if (op_itor == op_cache_.end()) {
      MS_LOG(EXCEPTION) << "Can not find op for node " << pred->ToString() << ".";
    }
    if (op_itor->second != nullptr &&
        (op_itor->second->GetOpType() == "Constant" || op_itor->second->GetOpType() == "Const") &&
        vars_.find(name) != vars_.end()) {
      auto variable = std::make_shared<Variable>(name);
      auto desc = vars_[name]->GetOutputDesc("y");
      (void)variable->update_output_desc_y(desc);
      MS_LOG(DEBUG) << "Trans to variable, var = " << variable->GetName() << ".";
      op_itor->second = variable;  // replace parameter with variable
      vars_[name] = variable;
    }
  }
}

AnfNodePtr DfGraphConvertor::GetRealInputNode(const CNodePtr &node, const AnfNodePtr &input) {
  if (input == nullptr || node == nullptr) {
    return nullptr;
  }
  AnfNodePtr pred = input;
  while (pred->isa<CNode>() && GetCNodeTargetFuncName(pred->cast<CNodePtr>()) == prim::kPrimDepend->name()) {
    pred = pred->cast<CNodePtr>()->input(1);
  }

  // skip input of UMonad, IOMonad
  if (IsValueNode<UMonad>(pred) || IsValueNode<IOMonad>(pred)) {
    return nullptr;
  }
  if (HasAbstractMonad(pred)) {
    return nullptr;
  }

  // skip input of the None, UpdateState
  if (IsValueNode<None>(pred) || IsPrimitiveCNode(pred, prim::kPrimUpdateState)) {
    return nullptr;
  }

  if (IsPrimitiveCNode(pred, prim::kPrimLoad)) {
    pred = ParseLoadInput(pred->cast<CNodePtr>());
    // for scenario like: Depend->Load->TensorMove
    if (IsPrimitiveCNode(pred, prim::kPrimDepend)) {
      return GetRealInputNode(node, pred);
    }
  }
  TransformConstOp(node, pred);
  return pred;
}

bool DfGraphConvertor::IsDataInput(const AnfNodePtr &node, const AnfNodePtr &input, size_t input_index) const {
  if (node == nullptr || input == nullptr) {
    MS_LOG(ERROR) << "Node or input is null.";
    return false;
  }
  // UpdateState has no data input
  if (IsPrimitiveCNode(node, prim::kPrimUpdateState)) {
    return false;
  }

  // skip input of UMonad, IOMonad
  if (IsValueNode<UMonad>(input) || IsValueNode<IOMonad>(input)) {
    return false;
  }
  if (HasAbstractMonad(input)) {
    return false;
  }

  // skip input of the None, UpdateState
  if (IsValueNode<None>(input) || IsPrimitiveCNode(input, prim::kPrimUpdateState)) {
    return false;
  }

  const PrimitiveSet has_control_node = {prim::kPrimLoad, prim::kPrimDepend, prim::kPrimTupleGetItem};
  if (input_index != kDataInputIndex && IsOneOfPrimitiveCNode(node, has_control_node)) {
    return false;
  }

  return true;
}

OutHandler DfGraphConvertor::GetNormalOpInput(const AnfNodePtr &node, const AnfNodePtr &pred) {
  OutHandler out_handler;
  if (IsSubGraph() && pred->isa<Parameter>()) {
    auto idx = std::find(inputs_.begin(), inputs_.end(), pred) - inputs_.begin();
    OperatorPtr op = subgraph_input_cache_[idx];
    out_handler.op = op;
    return out_handler;
  }

  if (IsAfterGraph() && pred->isa<Parameter>()) {
    auto idx = std::find(inputs_.begin(), inputs_.end(), pred) - inputs_.begin();
    auto idx_cond = prev_after_cond_map_[idx];
    if (bypass_node_prev_handle_cache_.find(idx_cond) != bypass_node_prev_handle_cache_.end()) {
      out_handler = bypass_node_prev_handle_cache_[idx_cond];
    } else {
      auto idx_out = prev_cond_to_while_out_index_[idx_cond];
      out_handler = while_output_handle_cache_[prev_while_node_]->at(idx_out);
    }
    return out_handler;
  }

  if (IsPrimitiveCNode(node, prim::kPrimTupleGetItem)) {
    uint64_t output_index = 0;
    TraceTupleGetItem(node->cast<CNodePtr>(), &output_index);
    auto adpt = FindAdapter(pred, training_);
    MS_EXCEPTION_IF_NULL(adpt);
    out_handler = adpt->getOutput(Convert(pred), static_cast<int32_t>(output_index));
    return out_handler;
  }

  if (out_handle_cache_.find(pred.get()) != out_handle_cache_.end()) {
    return out_handle_cache_[pred.get()];
  }
  auto op = Convert(pred);
  if (op == nullptr) {
    MS_LOG(WARNING) << "Convert input node failed, input node: " << pred->fullname_with_scope()
                    << ", node: " << node->fullname_with_scope() << ", graph: " << anf_graph_->ToString()
                    << ". Please check whether the node is Partial node or successor node of Partial in sub-graph.";
  }
  out_handler.op = op;
  return out_handler;
}

void DfGraphConvertor::DrawOpInput(const AnfNodePtr &node, const AnfNodePtr &pred, size_t i) {
  if (pred->isa<CNode>() && GetCNodeTargetFuncName(pred->cast<CNodePtr>()) == prim::kTupleGetItem) {
    compute_sout_ << op_draw_name_[pred->cast<CNodePtr>()->input(1).get()] << " -> " << op_draw_name_[node.get()] << ":"
                  << i << endl;
  } else if (pred->isa<Parameter>()) {
    compute_sout_ << op_draw_name_[pred.get()] << " -> " << op_draw_name_[node.get()] << ":" << i << endl;
  } else {
    // don't draw anything.
    MS_LOG(INFO) << "DRAW_GE_GRAPH: Shouldn't have this case.";
  }
  return;
}

std::vector<OutHandler> DfGraphConvertor::GetInputHandles(const AnfNodePtr &node, const AnfNodePtr &input) {
  MS_EXCEPTION_IF_NULL(node);
  MS_EXCEPTION_IF_NULL(input);
  std::vector<OutHandler> handles;
  auto cache_ret = tuple_out_handle_cache_.find(input.get());
  if (cache_ret != tuple_out_handle_cache_.end()) {
    handles = *(cache_ret->second);
  } else if (IsWhileNode(input)) {
    // While node in subgraph does not convert.
    // Output handle of While node is inconsistent with MS.
    MS_LOG(WARNING) << "Input node is while node, input node: " << input->fullname_with_scope()
                    << ", node: " << node->fullname_with_scope() << ", graph: " << anf_graph_->ToString();
    std::transform(graph_outputs_.begin(), graph_outputs_.end(), std::back_inserter(handles), [](const auto output) {
      return OutHandler(std::make_shared<::ge::Operator>(output.first), output.second);
    });
  } else {
    auto pred_adpt = FindAdapter(input, training_);
    MS_EXCEPTION_IF_NULL(pred_adpt);
    if (pred_adpt->IsDyOutputOp(0)) {
      MS_EXCEPTION_IF_NULL(Convert(input));
      handles = pred_adpt->getOutputs(Convert(input));
    } else {
      auto handle = GetNormalOpInput(node, input);
      if (handle.op != nullptr) {
        handles.emplace_back(handle);
      }
    }
  }
  return handles;
}

std::vector<OutHandler> DfGraphConvertor::GetMakeTupleAllInputHandle(const CNodePtr &node) {
  MS_EXCEPTION_IF_NULL(node);
  auto inputs = node->inputs();
  std::vector<OutHandler> handles;
  for (size_t i = 1; i < inputs.size(); i++) {
    auto input = inputs[i];
    TransformConstOp(node, input);
    auto input_handles = GetInputHandles(node, input);
    handles.insert(handles.end(), input_handles.begin(), input_handles.end());
    if (input_handles.empty()) {
      MS_LOG(INFO) << "input handles is empty, node: " << node->fullname_with_scope()
                   << ", input node: " << input->fullname_with_scope();
      continue;
    }
    AddGraphConstInput(input_handles[0].op);
    DrawOpInput(node, input, i);
  }
  return handles;
}

bool DfGraphConvertor::IsMergeInput(const CNodePtr &node) const {
  auto manager = anf_graph_->manager();
  if (manager == nullptr) {
    auto new_manager = MakeManager({anf_graph_});
    MS_EXCEPTION_IF_NULL(new_manager);
    new_manager->AddFuncGraph(anf_graph_);
    anf_graph_->set_manager(new_manager);
    manager = new_manager;
  }
  auto node_users = manager->node_users()[node];

  return (node_users.size() == 1 && std::find_if(node_users.begin(), node_users.end(), [](const auto &node_user) {
                                      return IsPrimitiveCNode(node_user.first, prim::kPrimMerge);
                                    }) != node_users.end());
}

void DfGraphConvertor::SetMakeTupleInput(const OpAdapterPtr &adpt, const CNodePtr &make_tuple_node) {
  MS_EXCEPTION_IF_NULL(adpt);
  MS_EXCEPTION_IF_NULL(make_tuple_node);
  MS_LOG(DEBUG) << "Set MakeTuple input handle: " << make_tuple_node->fullname_with_scope();
  // Skip MakeTuple make_tuple_node before Merge. Two branches(true/false) should not be merged before Merge, which will
  // lead to assign stream error in GE.
  if (IsMergeInput(make_tuple_node)) {
    MS_LOG(INFO) << "Skip make_tuple_node " << make_tuple_node->fullname_with_scope() << ", not set input handle.";
    return;
  }

  auto handles = GetMakeTupleAllInputHandle(make_tuple_node);
  auto ret = adpt->setInput(Convert(make_tuple_node), 1, std::make_shared<std::vector<OutHandler>>(handles));
  if (ret != SUCCESS) {
    MS_LOG(EXCEPTION) << "Set make_tuple_node input handle failed, make_tuple_node:"
                      << make_tuple_node->fullname_with_scope();
  }
}

void DfGraphConvertor::SetMergeInput(const OpAdapterPtr &adpt, const CNodePtr &merge_node) {
  MS_EXCEPTION_IF_NULL(adpt);
  MS_EXCEPTION_IF_NULL(merge_node);
  auto inputs = merge_node->inputs();
  if (inputs.size() != kMergeInputSize) {
    MS_LOG(EXCEPTION) << "Merge input size should be " << kMergeInputSize << ", but is " << inputs.size()
                      << ", node: " << merge_node->fullname_with_scope();
  }
  auto make_tuple = inputs[1];
  MS_EXCEPTION_IF_NULL(make_tuple);
  if (!IsPrimitiveCNode(make_tuple, prim::kPrimMakeTuple)) {
    MS_LOG(EXCEPTION) << "Merge input is not MakeTuple, but is " << make_tuple->fullname_with_scope()
                      << ", node: " << merge_node->fullname_with_scope();
  }
  auto handles = GetMakeTupleAllInputHandle(make_tuple->cast<CNodePtr>());
  auto ret = adpt->setInput(Convert(merge_node), 1, std::make_shared<std::vector<OutHandler>>(handles));
  if (ret != SUCCESS) {
    MS_LOG(EXCEPTION) << "Set node input handle failed, node:" << merge_node->fullname_with_scope();
  }
}

void DfGraphConvertor::SetNodeControlInput(const AnfNodePtr &node, const AnfNodePtr &input) {
  MS_EXCEPTION_IF_NULL(node);
  MS_EXCEPTION_IF_NULL(input);
  if (IsPrimitiveCNode(node, prim::kPrimTupleGetItem) && input->isa<ValueNode>()) {
    return;
  }
  if (input->isa<Parameter>() && HasAbstractMonad(input)) {
    MS_LOG(DEBUG) << "Node input is monad node, do not add control edge. node:" << node->fullname_with_scope()
                  << ", input: " << input->ToString();
    return;
  }
  auto dst = Convert(node);
  MS_EXCEPTION_IF_NULL(dst);
  auto src = Convert(input);
  if (src != nullptr) {
    dst->AddControlInput(*src);
  }
}

void DfGraphConvertor::SetOpInput(const OpAdapterPtr &adpt, const CNodePtr &node) {
  OperatorPtr src = Convert(node);
  int case_flag = 0;
  auto &inputs = node->inputs();
  size_t input_size = inputs.size();
  if (case_input_handle_cache_.find(node.get()) != case_input_handle_cache_.end()) {
    case_flag = 1;
    input_size = case_input_handle_cache_[node.get()]->size() + 1;
  } else if (!IsSubGraph() && call_input_handle_cache_.find(node) != call_input_handle_cache_.end()) {
    auto &handles = call_input_handle_cache_[node];
    MS_LOG(DEBUG) << "call node input size: " << handles->size();
    adpt->setInput(src, 1, handles);
    return;
  }
  MS_LOG(DEBUG) << "op:  " << src->GetName() << "'s input size is " << input_size - 1;

  if (IsPrimitiveCNode(node, prim::kPrimMakeTuple)) {
    SetMakeTupleInput(adpt, node);
    return;
  }

  if (IsPrimitiveCNode(node, prim::kPrimMerge)) {
    SetMergeInput(adpt, node);
    return;
  }

  for (size_t i = 1; i < input_size; i++) {
    AnfNodePtr pred = (case_flag != 0) ? case_input_handle_cache_[node.get()]->at(i - 1) : inputs[i];
    MS_EXCEPTION_IF_NULL(pred);
    if (!IsDataInput(node, pred, i)) {
      SetNodeControlInput(node, pred);
      continue;
    }
    TransformConstOp(node, pred);
    auto handles = GetInputHandles(node, pred);
    if (handles.empty()) {
      MS_LOG(INFO) << "Input handles is empty, input node: " << pred->fullname_with_scope()
                   << ", node: " << node->fullname_with_scope() << ", index: " << i;
      continue;
    }

    int ret;
    if (adpt->IsDynInputOp(i)) {
      ret = adpt->setInput(Convert(node), i, std::make_shared<std::vector<OutHandler>>(handles));
    } else {
      if (handles.size() != 1) {
        MS_LOG(EXCEPTION) << "Input handles size " << handles.size() << " is not equal to 1, "
                          << node->fullname_with_scope() << ", input node: " << pred->fullname_with_scope()
                          << ", index: " << i;
      }
      ret = adpt->setInput(src, SizeToInt(i), handles[0]);
    }
    if (ret != SUCCESS) {
      MS_LOG(DEBUG) << "Set node input handle failed, node:" << node->fullname_with_scope()
                    << ", input node: " << pred->fullname_with_scope() << ", index: " << i;
    } else {
      DrawOpInput(node, pred, i);
      AddGraphConstInput(handles[0].op);
    }
  }
}

void DfGraphConvertor::AddGraphConstInput(const OperatorPtr &op) {
  if (op == nullptr) {
    return;
  }
  if (IsSubGraph()) {
    return;
  }

  if (op->GetOpType() == "Constant" || op->GetOpType() == "Const") {
    graph_const_inputs_.push_back(op);
  }
}

void DfGraphConvertor::SetNodeInput(const AnfNodePtr node) {
  if (!node->isa<CNode>()) {
    return;
  }
  if (op_cache_.find(node.get()) == op_cache_.end()) {
    return;
  }
  auto cnode = node->cast<CNodePtr>();
  MS_EXCEPTION_IF_NULL(cnode);
  OpAdapterPtr adpt = FindAdapter(cnode, training_);
  if (adpt == nullptr) {
    error_ = NOT_FOUND;
    return;
  }

  // get Operator from op_cache_, use adapter to set Inputs
  DfGraphConvertor::SetOpInput(adpt, cnode);
}

std::string DfGraphConvertor::GetGNodeName(const ::ge::GNode &node) const {
  ::ge::AscendString name;
  auto ret = node.GetName(name);
  if (ret == ::ge::GRAPH_SUCCESS) {
    return std::string(name.GetString());
  } else {
    MS_LOG(WARNING) << "Get GNode name failed, ret: " << ret;
    return std::string();
  }
}

std::string DfGraphConvertor::GetGNodeType(const ::ge::GNode &node) const {
  ::ge::AscendString node_type;
  auto ret = node.GetType(node_type);
  if (ret == ::ge::GRAPH_SUCCESS) {
    return std::string(node_type.GetString());
  } else {
    MS_LOG(WARNING) << "Get GNode type failed, ret: " << ret;
    return std::string();
  }
}

// 1) Identity or IdentityN is the input of Merge, not delete
// 2) Identity or IdentityN is the subgraph(If) input, not delete
// 3) Identity or IdentityN it the output, not delete
// 4) Identity or IdentityN has multiple users, not delete
bool DfGraphConvertor::IsIdentityRedundant(const ::ge::GNode &node) const {
  auto node_type = GetGNodeType(node);
  if (node_type != kTypeIdentityN && node_type != kTypeIdentity) {
    MS_LOG(DEBUG) << "Node is not Identity or IdentityN, but is " << node_type << ", node name: " << GetGNodeName(node);
    return false;
  }

  auto node_name = GetGNodeName(node);
  auto ret = std::find_if(graph_outputs_.begin(), graph_outputs_.end(),
                          [&node_name](const auto &output) { return output.first.GetName() == node_name; });
  if (ret != graph_outputs_.end()) {
    return false;
  }

  for (size_t output_index = 0; output_index < node.GetOutputsSize(); output_index++) {
    auto output_nodes = node.GetOutDataNodesAndPortIndexs(output_index);
    if (output_nodes.size() != 1) {
      return false;
    }

    auto output_node_type = GetGNodeType(*(output_nodes.begin()->first));
    if (output_node_type == kTypeMerge || output_node_type == kTypeIf) {
      return false;
    }
  }
  return true;
}

void DfGraphConvertor::RemoveIdentity(::ge::GNode identity_node) {
  MS_LOG(INFO) << "Start Remove Identity or IdentityN, identity_node: " << GetGNodeName(identity_node);
  auto node_type = GetGNodeType(identity_node);
  if (node_type != kTypeIdentity && node_type != kTypeIdentityN) {
    MS_LOG(EXCEPTION) << "Node is not Identity or IdentityN, but is " << node_type
                      << ", identity_node name: " << GetGNodeName(identity_node);
    return;
  }
  if (identity_node.GetInputsSize() != identity_node.GetOutputsSize()) {
    MS_LOG(EXCEPTION) << "Node output size " << identity_node.GetOutputsSize() << " is not equal to input size "
                      << identity_node.GetInputsSize() << ", identity_node: " << GetGNodeName(identity_node);
    return;
  }

  ::ge::graphStatus ret;
  for (size_t output_index = 0; output_index < identity_node.GetOutputsSize(); output_index++) {
    auto output_nodes = identity_node.GetOutDataNodesAndPortIndexs(output_index);
    if (output_nodes.size() != 1) {
      return;
    }

    // 1. Set identity_node data edge
    auto node_output = *(output_nodes.begin());
    auto input_index = output_index;
    auto node_input = identity_node.GetInDataNodesAndPortIndexs(input_index);
    ret = df_graph_->RemoveEdge(identity_node, output_index, *node_output.first, node_output.second);
    if (ret != ::ge::GRAPH_SUCCESS) {
      MS_LOG(EXCEPTION) << "Remove edge failed, src identity_node: " << GetGNodeName(identity_node)
                        << ", index: " << output_index << ", dst identity_node: " << GetGNodeName(*node_output.first)
                        << ", index: " << node_output.second << ", ret: " << ret;
      return;
    }
    ret = df_graph_->AddDataEdge(*node_input.first, node_input.second, *node_output.first, node_output.second);
    if (ret != ::ge::GRAPH_SUCCESS) {
      MS_LOG(EXCEPTION) << "Add data edge failed, src identity_node: " << GetGNodeName(*node_input.first) << ", index: "
                        << ", dst identity_node: " << GetGNodeName(*node_output.first)
                        << ", index: " << node_output.second << ", ret: " << ret;
      return;
    }

    // 2. Set identity_node control edge
    auto node_control = identity_node.GetInControlNodes();
    for (const auto &item : node_control) {
      ret = df_graph_->AddControlEdge(*item, *node_output.first);
      if (ret != ::ge::GRAPH_SUCCESS) {
        MS_LOG(EXCEPTION) << "Add control edge failed, src identity_node: " << GetGNodeName(*item)
                          << ", dst identity_node: " << GetGNodeName(*node_output.first) << ", ret: " << ret;
        return;
      }
    }
  }

  // 3. Remove identity
  ret = df_graph_->RemoveNode(identity_node);
  if (ret != ::ge::GRAPH_SUCCESS) {
    MS_LOG(EXCEPTION) << "Remove identity_node failed, identity_node: " << GetGNodeName(identity_node)
                      << ", ret: " << ret;
    return;
  }
}

void DfGraphConvertor::IdentityOptimization() {
  MS_LOG(INFO) << "Start IdentityOptimization, graph: " << anf_graph_->ToString();
  MS_EXCEPTION_IF_NULL(df_graph_);
  auto all_nodes = df_graph_->GetAllNodes();
  for (const auto &node : all_nodes) {
    if (IsIdentityRedundant(node)) {
      RemoveIdentity(node);
    }
  }
  MS_LOG(INFO) << "End IdentityOptimization, graph: " << anf_graph_->ToString();
}

void DfGraphConvertor::ProcessSubgraph(const AnfNodePtr &node, const std::vector<AnfNodePtr> &inputs) {
  ValueNodePtr graph_node = nullptr;
  if (node->isa<CNode>()) {
    graph_node = node->cast<CNodePtr>()->input(1)->cast<ValueNodePtr>();
  } else if (node->isa<ValueNode>()) {
    graph_node = node->cast<ValueNodePtr>();
  } else {
    return;
  }

  MS_EXCEPTION_IF_NULL(graph_node);
  FuncGraphPtr anf_graph = graph_node->value()->cast<FuncGraphPtr>();
  DfGraphConvertor converter(anf_graph);
  converter.use_inputs_ = true;
  converter.inputs_ = inputs;
  if (IsSubGraph()) {
    converter.case_call_input_size_ = case_call_input_size_;
    auto &params = anf_graph->parameters();
    for (size_t i = case_call_input_size_; i < inputs.size(); i++) {
      auto p = inputs[i];
      if (HasAbstractMonad(p)) {
        continue;
      }
      if (p->isa<Parameter>()) {
        auto idx = std::find(inputs_.begin(), inputs_.end(), p) - inputs_.begin();
        auto idx_cond = body_cond_map_[idx];
        if (while_const_input_index_.find(idx_cond) != while_const_input_index_.end()) {
          auto temp = while_const_input_index_[idx_cond].op;
          auto name = temp->GetName();
          auto value = const_op_to_value_[temp];
          MS_EXCEPTION_IF_NULL(value);
          auto adpt_const = FindAdapter(kNameConst, training_);
          if (adpt_const == nullptr) {
            continue;
          }
          name += "_case";
          auto const_op = adpt_const->generate(name);
          (void)adpt_const->setAttr(const_op, "value", value);
          auto const_op_desc = TransformUtil::GetGeTensorDesc(value->shape_c(), value->data_type(), kOpFormat_NCHW);
          if (const_op_desc == nullptr) {
            MS_LOG(WARNING) << "Create variable " << name << " output descriptor failed!";
            continue;
          }
          (void)std::static_pointer_cast<Constant>(const_op)->update_output_desc_y(*const_op_desc);
          auto n = params.at(i - case_call_input_size_);
          converter.op_cache_[n.get()] = const_op;
          MS_LOG(DEBUG) << "node :" << n->ToString() << " and op: " << const_op->GetName()
                        << " has cached in graph: " << anf_graph->ToString();
        }
      }
    }
  }

  (void)converter.ConvertAllNode().BuildGraph();
#ifdef ENABLE_DUMP_IR
  std::string name = graph_node->ToString() + "_ge_graph.dot";
  if (MsContext::GetInstance()->get_param<bool>(MS_CTX_SAVE_GRAPHS_FLAG)) {
    converter.DrawComputeGraph(name);
  }
#endif
  branches_map_[node.get()] = *(converter.df_graph_);
  return;
}

// Update GE op's shape and type info
void DfGraphConvertor::UpdateOpDesc(const AnfNodePtr node) {
  if (node == nullptr || !node->isa<CNode>()) {
    return;
  }

  if (op_cache_.find(node.get()) == op_cache_.end()) {
    return;
  }

  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    error_ = NOT_FOUND;
    return;
  }

  // get Operator from op_cache_
  OperatorPtr op = Convert(node);
  std::string name = op->GetOpType();
  // When IdentityN's input is Function or IdentityN, it can not find GEType mapping to MSType. There are ERROR logs
  // that do not affect the result. So it no need to set OutputDesc of IdentityN, It can be inferred by GE. eg:
  // MakeTuple-->MakeTuple
  if (name == kTypeIdentityN) {
    MS_LOG(DEBUG) << "No need set IdentityN and NoOp OpDesc, node: " << node->fullname_with_scope();
    return;
  }
  // NoOp has not output, so it no need to set OutputDesc.
  if (name == kTypeNoOp) {
    return;
  }

  adpt->updateOutputDesc(op, node->Shape(), node->Type(), node);

  if (name == prim::kPrimNonZeroWithValueShape->name()) {
    MS_EXCEPTION_IF_NULL(op);
    auto op_desc = ::ge::OpDescUtils::GetOpDescFromOperator(*op);
    if (op_desc == nullptr) {
      return;
    }
    const auto output_desc0 = op_desc->MutableOutputDesc("out_value");
    ::ge::TensorUtils::SetReuseInput(*output_desc0, true);
    ::ge::TensorUtils::SetReuseInputIndex(*output_desc0, 0);

    const auto output_desc1 = op_desc->MutableOutputDesc("out_index");
    ::ge::TensorUtils::SetReuseInput(*output_desc1, true);
    ::ge::TensorUtils::SetReuseInputIndex(*output_desc1, 1);
  }
}

OperatorPtr DfGraphConvertor::Convert(const AnfNodePtr node) {
  if (node == nullptr) {
    MS_LOG(ERROR) << "node is nullptr";
    error_ = NOT_FOUND;
    return nullptr;
  }
  // find in cache
  if (op_cache_.count(node.get()) != 0) {
    return op_cache_[node.get()];
  }
  if (IsSubGraph()) {
    node->set_user_data<bool>("subgraph_node", make_shared<bool>(true));
  }

  // do not convert primitive node
  if (IsValueNode<Primitive>(node)) {
    return nullptr;
  }
  // convert a new one
  if (node->isa<CNode>()) {
    auto cnode = node->cast<CNodePtr>();
    if (IsSubGraph() && IsWhileNode(cnode)) {
      return nullptr;
    }
    if (!IsSubGraph() && IsWhileNode(cnode)) {
      CacheWhileGraph(cnode);
      auto &graphs = while_graph_cache_[cnode];
      GetWhileUsedInputIndex(graphs);
      SetParamIndexMap(graphs);
      cur_while_node_ = cnode;
    }
    return ConvertCNode(cnode);
  }

  if (node->isa<Parameter>() && IsSubGraph()) {
    return nullptr;
  }

  if (node->isa<Parameter>()) {
    return ConvertParameter(node);
  }
  if (node->isa<ValueNode>()) {
    if (IsValueNode<Monad>(node)) {
      return nullptr;
    }
    return ConvertValueNode(node->cast<ValueNodePtr>());
  }

  MS_LOG(ERROR) << "Invalid AnfNode";
  error_ = INVALID_ARGUMENT;
  return nullptr;
}

void DfGraphConvertor::ConvertTopK(const CNodePtr node) {
  MS_EXCEPTION_IF_NULL(node);
  MS_LOG(INFO) << "Convert TopK second input's type from int64 to int32.";
  auto value_ptr = node->input(2)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_ptr);
  std::ostringstream ss;
  ss << "op" << value_ptr.get();
  op_draw_name_[value_ptr.get()] = ss.str();
  compute_sout_ << ss.str() << "[label= \"" << value_ptr->value()->ToString() << "\" shape=ellipse]" << endl;
  auto input_value = value_ptr->value();
  auto int64_value = GetValue<int64_t>(input_value);
  OpAdapterPtr adpt = FindAdapter(value_ptr, training_);
  auto op = adpt->generate(value_ptr);
  (void)adpt->setAttr(op, "value", static_cast<int32_t>(int64_value));
  op_cache_[value_ptr.get()] = op;
}

void DfGraphConvertor::ConvertResizeBilinear(const FuncGraphPtr anf_graph) const {
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph);
  for (auto &it : nodes) {
    if (it->isa<CNode>()) {
      auto node = it->cast<CNodePtr>();
      MS_EXCEPTION_IF_NULL(node);
      std::string name = GetCNodeTargetFuncName(node);
      if (name == prim::kPrimResizeBilinear->name()) {
        AnfNodePtr op = node->input(0);
        if (IsValueNode<Primitive>(op)) {
          auto prim = GetValueNode<PrimitivePtr>(op);
          ValuePtr size_value = prim->GetAttr("size");
          auto int64_value = GetValue<std::vector<int64_t>>(size_value);
          std::vector<int32_t> int32_value;
          (void)std::transform(int64_value.begin(), int64_value.end(), std::back_inserter(int32_value), LongToInt);
          auto valuend = NewValueNode(int32_value);
          valuend->set_abstract(size_value->ToAbstract());
          node->add_input(valuend);
        }
      }
    }
  }
}

void DfGraphConvertor::ConvertSpaceBatchNd(const FuncGraphPtr anf_graph) const {
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph);
  for (auto &it : nodes) {
    if (it->isa<CNode>()) {
      auto node = it->cast<CNodePtr>();
      MS_EXCEPTION_IF_NULL(node);
      std::string name = GetCNodeTargetFuncName(node);
      if (name == prim::kPrimSpaceToBatchND->name() || name == prim::kPrimBatchToSpaceND->name()) {
        AnfNodePtr op = node->input(0);
        if (IsValueNode<Primitive>(op)) {
          auto prim = GetValueNode<PrimitivePtr>(op);
          MS_EXCEPTION_IF_NULL(prim);
          ValuePtr block_shape = prim->GetAttr("block_shape");
          auto int64_value = GetValue<std::vector<int64_t>>(block_shape);
          std::vector<int32_t> int32_value;
          (void)std::transform(int64_value.begin(), int64_value.end(), std::back_inserter(int32_value), LongToInt);
          auto new_value = NewValueNode(int32_value);
          new_value->set_abstract(block_shape->ToAbstract());
          node->add_input(new_value);
          ValuePtr attr_value = nullptr;
          if (name == prim::kPrimSpaceToBatchND->name()) {
            attr_value = prim->GetAttr("paddings");
          } else {
            attr_value = prim->GetAttr("crops");
          }
          std::vector<int64_t> attr_list;
          if (attr_value->isa<ValueList>()) {
            const ValueListPtr &value = dyn_cast<ValueList>(attr_value);
            for (const auto &item : value->value()) {
              if (item->isa<ValueList>()) {
                auto value_list = GetValue<std::vector<int64_t>>(item);
                (void)std::copy(value_list.begin(), value_list.end(), std::back_inserter(attr_list));
              }
            }
          }
          auto new_value_attr = NewValueNode(attr_list);
          new_value_attr->set_abstract(attr_value->ToAbstract());
          node->add_input(new_value_attr);
        }
      }
    }
  }
}

AnfNodePtr DfGraphConvertor::CreateCast(const AnfNodePtr &input, const TypePtr &dst_type) const {
  auto func_graph = input->func_graph();
  MS_EXCEPTION_IF_NULL(func_graph);
  AnfNodePtrList inputs = {NewValueNode(prim::kPrimCast), input, NewValueNode(dst_type)};
  auto cnode = func_graph->NewCNode(inputs);
  MS_EXCEPTION_IF_NULL(cnode);
  auto abs_tensor = std::make_shared<abstract::AbstractTensor>(dst_type, input->Shape());
  cnode->set_abstract(abs_tensor);
  return cnode;
}

void DfGraphConvertor::ConvertTile(const FuncGraphPtr anf_graph) const {
  std::vector<AnfNodePtr> nodes = GetOrderedCNodes(anf_graph);
  for (auto &it : nodes) {
    if (it->isa<CNode>()) {
      auto node = it->cast<CNodePtr>();
      MS_EXCEPTION_IF_NULL(node);
      std::string name = GetCNodeTargetFuncName(node);
      if (name == prim::kPrimTile->name()) {
        auto type_ptr = node->input(1)->Type();
        MS_EXCEPTION_IF_NULL(type_ptr);
        auto tensor_type = type_ptr->cast<TensorTypePtr>();
        MS_EXCEPTION_IF_NULL(tensor_type);
        if (tensor_type->element()->number_type() == kNumberTypeInt64) {
          auto new_cast = CreateCast(node->input(1), kInt32);
          node->set_input(1, new_cast);
        }
      }
    }
  }
}

std::vector<int64_t> DfGraphConvertor::CastToInt(const ValuePtr &value) const {
  if (value == nullptr) {
    return {};
  }
  std::vector<int64_t> cur_value = {};
  if (utils::isa<ValueSequencePtr>(value)) {
    auto val_seq_ptr = value->cast<ValueSequencePtr>();
    MS_EXCEPTION_IF_NULL(val_seq_ptr);
    if (!val_seq_ptr->value().empty()) {
      auto first_val = val_seq_ptr->value().front();
      MS_EXCEPTION_IF_NULL(first_val);
      MS_EXCEPTION_IF_NULL(first_val->type());
      if (first_val->type()->number_type() == kNumberTypeInt64) {
        cur_value = GetValue<std::vector<int64_t>>(value);
      } else {
        auto origin_value = GetValue<std::vector<int>>(value);
        (void)std::transform(origin_value.begin(), origin_value.end(), std::back_inserter(cur_value),
                             [](int index) { return static_cast<int64_t>(index); });
      }
    }
  } else {
    MS_EXCEPTION_IF_NULL(value->type());
    if (value->type()->number_type() == kNumberTypeInt64) {
      cur_value.push_back(GetValue<int64_t>(value));
    } else {
      cur_value.push_back(static_cast<int64_t>(GetValue<int>(value)));
    }
  }
  return cur_value;
}

void DfGraphConvertor::ConvertReshape(const CNodePtr node) {
  MS_LOG(INFO) << "Convert the second input of reshape to op attr.";
  const auto kInputNum = 3;
  if (node->size() < kInputNum) {
    MS_LOG(WARNING) << "Reshape must have two inputs.";
    return;
  }
  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    return;
  }
  auto op = adpt->generate(node);
  MS_EXCEPTION_IF_NULL(op);
  // get shape form attr
  auto value_node = node->input(0)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  MS_EXCEPTION_IF_NULL(value_node->value());
  auto primitive = value_node->value()->cast<PrimitivePtr>();
  MS_EXCEPTION_IF_NULL(primitive);
  auto value = primitive->GetAttr("shape");
  std::vector<int64_t> list;
  list = CastToInt(value);

  (void)op->SetAttr("shape", list);
  op_cache_[node.get()] = op;
}

void DfGraphConvertor::ConvertAllReduce(const CNodePtr node) {
  MS_LOG(INFO) << "Add AllReduce fusion_id";
  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    return;
  }
  auto op = adpt->generate(node);
  MS_EXCEPTION_IF_NULL(op);
  // get shape form attr
  auto value_node = node->input(0)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  MS_EXCEPTION_IF_NULL(value_node->value());
  auto primitive = value_node->value()->cast<PrimitivePtr>();
  MS_EXCEPTION_IF_NULL(primitive);
  auto fusion_value = primitive->GetAttr("fusion");
  auto fusion = GetValue<int64_t>(fusion_value);
  int64_t fusion_id = -1;

  if (fusion != 0 && fusion != 1) {
    fusion_id = fusion;
  }

  (void)op->SetAttr("fusion_id", fusion_id);
  op_cache_[node.get()] = op;
}

void DfGraphConvertor::ConvertConv2D(const CNodePtr node) {
  MS_EXCEPTION_IF_NULL(node);
  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    return;
  }
  auto op = adpt->generate(node);
  MS_EXCEPTION_IF_NULL(op);
  auto value_node = node->input(0)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  MS_EXCEPTION_IF_NULL(value_node->value());
  auto primitive = value_node->value()->cast<PrimitivePtr>();
  MS_EXCEPTION_IF_NULL(primitive);
  auto value = primitive->GetAttr("padding");
  if (value != nullptr) {
    std::string pad_mode = GetValue<std::string>(value);
    (void)op->SetAttr("padding", pad_mode);
  }
  op_cache_[node.get()] = op;
}

void DfGraphConvertor::ConvertOCRRecPreHandle(const CNodePtr node) {
  MS_LOG(INFO) << "Add OCRRecognitionPreHandle _op_max_shape attr";
  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    return;
  }
  auto op = adpt->generate(node);
  MS_EXCEPTION_IF_NULL(op);
  // get shape form attr
  auto value_node = node->input(0)->cast<ValueNodePtr>();
  MS_EXCEPTION_IF_NULL(value_node);
  MS_EXCEPTION_IF_NULL(value_node->value());
  auto primitive = value_node->value()->cast<PrimitivePtr>();
  MS_EXCEPTION_IF_NULL(primitive);
  auto value = primitive->GetAttr("_op_max_shape");
  if (value == nullptr) {
    return;
  }
  auto op_max_shape = GetValue<std::string>(value);
  (void)op->SetAttr("_op_max_shape", op_max_shape);
  op_cache_[node.get()] = op;
}

AnfNodePtr DfGraphConvertor::TraceTupleGetItem(const CNodePtr &node, uint64_t *index) {
  MS_EXCEPTION_IF_NULL(node);
  const int TUPLE_GET_ITEM_INDEX = 2;
  if (node->inputs().size() < 3) {  // "tuple_getitem" primitive must have 3 inputs
    MS_LOG(EXCEPTION) << "length of inputs of TupleGetItem is less than 3";
  }
  auto index_node = node->inputs()[TUPLE_GET_ITEM_INDEX];
  if (!index_node->isa<ValueNode>()) {
    error_ = INVALID_ARGUMENT;
    MS_LOG(EXCEPTION) << "can't convert get item with non-constant index";
  }
  auto index_vec = CastToInt(GetValueNode(index_node));
  if (index_vec.empty()) {
    MS_LOG(EXCEPTION) << "Get index failed from index node of tuple get item.";
  }
  *index = LongToUlong(index_vec[0]);
  return node->inputs()[1];
}

OutHandler DfGraphConvertor::GetHandler(const AnfNodePtr &node) {
  if (node == nullptr) {
    MS_LOG(ERROR) << "Get nullptr while getting handler from node";
    return OutHandler(nullptr, "");
  }
  auto op = Convert(node);
  if (op != nullptr) {
    auto name = op->GetName();
    if ((vars_.count(name) != 0) && vars_[name] != nullptr) {
      op = vars_[name];
      MS_LOG(DEBUG) << "update tuple_out_handle_cache_ " << name;
    }
    return OutHandler(op, "", node);
  } else if (out_handle_cache_.find(node.get()) != out_handle_cache_.end()) {
    return out_handle_cache_[node.get()];
  } else {
    MS_LOG(DEBUG) << "Add an empty out handler: " << node->ToString();
    return OutHandler();
  }
}

bool DfGraphConvertor::CheckCNode(const std::string &name, const CNodePtr node) {
  // ignore apply node of return
  if (name == "" || name == prim::kPrimSwitch->name() || name == prim::kPrimSwitchLayer->name() ||
      name == prim::kPrimPartial->name()) {
    return false;
  }

  // Convert TopK second input from int64 to int32.
  if (name == prim::kPrimTopK->name()) {
    ConvertTopK(node);
    return true;
  }

  // Convert Reshape add const input to attr(shape)
  if (name == prim::kPrimReshape->name()) {
    ConvertReshape(node);
    return true;
  }

  if (name == prim::kPrimOCRRecognitionPreHandle->name()) {
    ConvertOCRRecPreHandle(node);
    return true;
  }

  // Add attr pad mode to Conv2D
  if (name == prim::kPrimConv2D->name() || name == prim::kPrimDepthwiseConv2dNative->name() ||
      name == kNameConv2DBackpropInputV2) {
    ConvertConv2D(node);
    return true;
  }

  if (name == prim::kPrimAllReduce->name()) {
    ConvertAllReduce(node);
    return true;
  }

  return true;
}

void CheckAndAddScopeAttrInt(const OperatorPtr op, const PrimitivePtr primitive, const std::string &attr_name) {
  auto attr_value = primitive->GetAttr(attr_name);
  if (attr_value != nullptr) {
    auto value = GetValue<int64_t>(attr_value);
    (void)op->SetAttr(attr_name, value);
  }
}

void CheckAndAddScopeAttrString(const OperatorPtr op, const PrimitivePtr primitive, const std::string &attr_name) {
  auto attr_value = primitive->GetAttr(attr_name);
  if (attr_value != nullptr) {
    auto value = GetValue<std::string>(attr_value);
    (void)op->SetAttr(attr_name, value);
  }
}

// If node does not have abstract, it will fail when the node is generated to operator.
void DfGraphConvertor::SetNodeAbstract(const CNodePtr &node) const {
  MS_EXCEPTION_IF_NULL(node);
  if (node->abstract() != nullptr) {
    return;
  }
  if (IsPrimitiveCNode(node, prim::kPrimMakeTuple)) {
    auto inputs = node->inputs();
    AbstractBasePtrList elem;
    std::transform(inputs.begin() + 1, inputs.end(), std::back_inserter(elem),
                   [](const AnfNodePtr &node) { return node->abstract(); });
    node->set_abstract(std::make_shared<abstract::AbstractTuple>(elem));
    return;
  }
  if (IsPrimitiveCNode(node, prim::kPrimReturn)) {
    auto inputs = node->inputs();
    if (inputs.size() < kReturnInputSize) {
      MS_LOG(EXCEPTION) << "Return node input size " << inputs.size()
                        << " less than 2, node: " << node->fullname_with_scope();
    }
    auto input = inputs[1];
    MS_EXCEPTION_IF_NULL(input);
    node->set_abstract(input->abstract());
    return;
  }
  MS_LOG(ERROR) << "Node has not abstract:" << node->fullname_with_scope() << ", DebugString: " << node->ToString();
}

OperatorPtr DfGraphConvertor::ConvertCNode(const CNodePtr node) {
  SaveParamFormat(node);
  std::string name = GetCNodeTargetFuncName(node);
  if (!CheckCNode(name, node)) {
    return nullptr;
  }

  // get corresponding OpAdapter
  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    error_ = NOT_FOUND;
    return nullptr;
  }
  SetNodeAbstract(node);
  // get operator
  OperatorPtr op = nullptr;
  auto it_op = op_cache_.find(node.get());
  if (it_op != op_cache_.end()) {
    op = it_op->second;
  } else {
    if (cur_while_node_ == node) {
      op = adpt->generateDynOutputOp(node);
    } else {
      op = adpt->generate(node);
    }
  }

  // set attribute for primitive
  (void)adpt->setAttr(op, node);
  MS_LOG(INFO) << "begin to set attr for subgraph multi dims";
  auto value_node = node->input(0)->cast<ValueNodePtr>();
  if (value_node != nullptr && value_node->value()->cast<PrimitivePtr>() != nullptr) {
    auto primitive = value_node->value()->cast<PrimitivePtr>();
    CheckAndAddScopeAttrInt(op, primitive, "_subgraph_multi_dims_index");
    CheckAndAddScopeAttrString(op, primitive, "_subgraph_multi_dims_input_dims");
    CheckAndAddScopeAttrString(op, primitive, "_subgraph_multi_dims_input_shape");
  }
  MS_LOG(INFO) << "set attr for subgraph multi dims end";

  // add into cache
  (void)op_cache_.emplace(node.get(), op);

  DrawCNode(node, adpt);

  return op_cache_[node.get()];
}

OperatorPtr DfGraphConvertor::ConvertParameter(const AnfNodePtr node) {
  // convert Parameter in ANF to variable in DataFlow
  auto adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    MS_LOG(EXCEPTION) << "Can not find adapter for Parameter";
  }
  auto op = adpt->generate(node);
  op_cache_[node.get()] = op;

  // build index for parameter using name
  std::string name = std::static_pointer_cast<Parameter>(node)->name();
  params_[name] = node;
  std::ostringstream ss;
  ss << "op" << node.get();
  op_draw_name_[node.get()] = ss.str();
  compute_sout_ << ss.str() << "[shape=octagon, label=\"" << name << "\"]" << endl;
  return op_cache_[node.get()];
}

void DfGraphConvertor::SaveParamFormat(const CNodePtr node) {
  AnfNodePtr op = node->input(0);
  if (IsValueNode<Primitive>(op)) {
    auto prim = GetValueNode<PrimitivePtr>(op);
    for (auto attr : prim->attrs()) {
      if (attr.first == "format") {
        std::string format;
        if (attr.second->isa<Int64Imm>()) {
          bool converted = CheckAndConvertUtils::ConvertAttrValueToString(prim->name(), "format", &attr.second);
          if (converted) {
            format = attr.second->ToString();
          } else {
            CheckAndConvertUtils::GetFormatStringVal(prim, &format);
          }
        } else if (attr.second->isa<StringImm>()) {
          format = attr.second->ToString();
        }
        if (format != "NCDHW" && format != "NHWC") {
          break;
        }
        for (size_t i = 1; i < node->size(); i++) {
          auto input = node->input(i);
          if (input->isa<Parameter>()) {
            param_format_[input->DebugString()] = format;
            MS_LOG(DEBUG) << "Save Param " << input->DebugString() << " format: " << format;
          }
        }
      }
    }
  }
}

Status DfGraphConvertor::TryConvertValueNodeToMultiConst(const ValueNodePtr node) {
  MS_EXCEPTION_IF_NULL(node);
  ValuePtr value = node->value();
  MS_EXCEPTION_IF_NULL(value);
  if (!value->isa<ValueList>() && !value->isa<ValueTuple>()) {
    return FAILED;
  }

  auto vec = value->isa<ValueTuple>() ? value->cast<ValueTuplePtr>()->value() : value->cast<ValueListPtr>()->value();
  if (vec.empty()) {
    return FAILED;
  }

  std::shared_ptr<std::vector<OutHandler>> tuple_items = std::make_shared<std::vector<OutHandler>>();
  for (size_t i = 0; i < vec.size(); i++) {
    MS_EXCEPTION_IF_NULL(vec[i]);
    if (vec[i]->isa<MeTensor>()) {
      GeTensorPtr ge_tensor = transform::TransformUtil::ConvertTensor(vec[i]->cast<MeTensorPtr>(), kOpFormat_NCHW);
      auto const_op = std::make_shared<Constant>(node->fullname_with_scope() + "/const/inputs/" + std::to_string(i));
      AddGraphConstInput(const_op);
      (void)const_op->set_attr_value(*ge_tensor);
      (void)const_op->update_output_desc_y(ge_tensor->GetTensorDesc());
      (void)tuple_items->emplace_back(OutHandler(const_op, ""));
    } else {
      return FAILED;
    }
  }
  if (tuple_items->empty()) {
    return FAILED;
  }

  tuple_out_handle_cache_[node.get()] = tuple_items;
  return SUCCESS;
}

OperatorPtr DfGraphConvertor::ConvertValueNode(const ValueNodePtr node) {
  // convert valuenode in ANF to Const in DataFlow
  // find paramerte referenced by SymbolicKeyInstance of valuenode
  std::ostringstream ss;
  ss << "op" << node.get();
  op_draw_name_[node.get()] = ss.str();
  compute_sout_ << ss.str() << "[label= \"" << node->value()->ToString() << "\" shape=ellipse]" << endl;

  if (TryConvertValueNodeToMultiConst(node) == SUCCESS) {
    MS_LOG(INFO) << "Convert value node to multi Constant OP success";
    return nullptr;
  }

  OpAdapterPtr adpt = FindAdapter(node, training_);
  if (adpt == nullptr) {
    error_ = NOT_FOUND;
    return nullptr;
  }
  auto op = adpt->generate(node);
  // set const's attrs
  if (adpt->setAttr(op, "value", node->value()) != 0) {
    MS_LOG(WARNING) << "set attr value for const failed";
  }

  auto const_op = std::static_pointer_cast<Constant>(op);
  if (const_op == nullptr) {
    MS_LOG(ERROR) << "Get Constant operator failed";
    return nullptr;
  }
  auto ge_tensor = const_op->get_attr_value();
  auto ge_desc = ge_tensor.GetTensorDesc();
  (void)const_op->update_output_desc_y(ge_desc);

  op_cache_[node.get()] = op;
  return op_cache_[node.get()];
}

void DfGraphConvertor::DrawCNode(const CNodePtr node, const OpAdapterPtr adpt) {
  if (adpt == nullptr || node == nullptr) {
    MS_LOG(ERROR) << "Failed to draw apply node as adpt or node is nullptr!";
    return;
  }
  std::ostringstream ss;
  ss << "op" << node.get();
  op_draw_name_[node.get()] = ss.str();

  compute_sout_ << ss.str() << "[label=<";
  compute_sout_ << "<table border='1' cellborder='1'>" << endl;

  auto input_map = adpt->getInputMap();
  auto dyn_input_map = adpt->getDynInputMap();
  if (input_map.size() + dyn_input_map.size() > 0) {
    compute_sout_ << "<tr>";
    for (auto &it : input_map) {
      compute_sout_ << "<td port='" << it.first << "'>" << it.second.name << "</td>";
    }
    for (auto &it : dyn_input_map) {
      compute_sout_ << "<td port='" << it.first << "'>" << it.second.name << "</td>";
    }
    compute_sout_ << "</tr>" << endl;
  }

  compute_sout_ << "<tr><td colspan=\"" << (input_map.size() + dyn_input_map.size()) << "\">\"" << node->ToString()
                << ":" << GetCNodeTargetFuncName(node) << "\"</td></tr>" << endl;

  // print attrs' values
  auto atts = adpt->GetAttrsFromDrawGraph();
  for (auto &it : atts) {
    compute_sout_ << "<tr><td colspan=\"" << (input_map.size() + dyn_input_map.size()) << "\">\"" << it
                  << "\"</td></tr>";
  }

  adpt->clearAttrVect();

  compute_sout_ << "</table>> shape=plaintext]" << endl;
}
void DfGraphConvertor::RegisterAdapter(const std::string &name, OpAdapterPtr adpt) {
  OpAdapterMap::get()[name] = std::make_shared<OpAdapterDesc>(adpt);
}
void DfGraphConvertor::RegisterAdapter(const std::string &name, OpAdapterPtr train_adpt, OpAdapterPtr infer_adpt) {
  OpAdapterMap::get()[name] = std::make_shared<OpAdapterDesc>(train_adpt, infer_adpt);
}

std::map<std::string, ValuePtr> GeOpConvertor::GetAttrAndValue(const AnfNodePtr &node, const bool training = true) {
  MS_EXCEPTION_IF_NULL(node);
  std::map<std::string, ValuePtr> attr_list;
  if (!node->isa<CNode>()) {
    MS_LOG(INFO) << "Current node isn't a cnode! node info:" << node->DebugString();
    return attr_list;
  }

  OpAdapterPtr adpt = FindAdapter(node, training);
  if (adpt == nullptr) {
    MS_LOG(INFO) << "Current node can't find adpt! node info:" << node->DebugString();
    return attr_list;
  }

  auto cnode = node->cast<CNodePtr>();
  MS_EXCEPTION_IF_NULL(cnode);
  AnfNodePtr primitive = cnode->input(0);
  if (IsValueNode<Primitive>(primitive)) {
    auto prim = GetValueNode<PrimitivePtr>(primitive);
    attr_list = adpt->GetNormalOpAttrList(prim);
  }
  return attr_list;
}

std::string GeOpConvertor::GetOpType(const AnfNodePtr &node, const bool training = true) {
  MS_EXCEPTION_IF_NULL(node);
  OpAdapterPtr adpt = FindAdapter(node, training);
  if (adpt == nullptr) {
    MS_LOG(INFO) << "Current node can't find adpt! node info:" << node->DebugString();
    return "";
  }
  return adpt->getOpType();
}

std::shared_ptr<GeTensorDesc> GeOpConvertor::GetTensorDesc(const ShapeVector &dev_shape, const TypeId &dev_type,
                                                           const std::string &dev_format, const ShapeVector &ori_shape,
                                                           const std::string &ori_format) {
  auto tensor_desc = transform::TransformUtil::GetGeTensorDesc(dev_shape, dev_type, dev_format, ori_shape, ori_format);
  MS_EXCEPTION_IF_NULL(tensor_desc);
  return tensor_desc;
}
}  // namespace transform
}  // namespace mindspore
