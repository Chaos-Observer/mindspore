/**
 * Copyright 2020-2022 Huawei Technologies Co., Ltd
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

#include "frontend/optimizer/irpass.h"
#include "frontend/optimizer/irpass/arithmetic_simplify.h"
#include "frontend/optimizer/irpass/branch_culling.h"
#include "frontend/optimizer/irpass/cast_eliminate.h"
#include "frontend/optimizer/irpass/convert.h"
#include "frontend/optimizer/irpass/environ_eliminate.h"
#include "frontend/optimizer/irpass/meta_fg_var_prepare.h"
#include "frontend/optimizer/irpass/taylor_eliminate.h"
#include "frontend/optimizer/irpass/inline.h"
#include "frontend/optimizer/irpass/updatestate_eliminate.h"
#include "frontend/optimizer/irpass/load_eliminate.h"
#include "frontend/optimizer/irpass/stopgrad_eliminate.h"
#include "frontend/optimizer/irpass/incorporate_call.h"
#include "frontend/optimizer/irpass/item_tuple_or_list_eliminate.h"
#include "frontend/optimizer/irpass/merge_addn.h"
#include "frontend/optimizer/irpass/accumulaten_eliminate.h"
#include "frontend/optimizer/irpass/less_batch_normalization.h"
#include "frontend/optimizer/irpass/minmax_grad.h"
#include "frontend/optimizer/irpass/param_replace.h"
#include "frontend/optimizer/irpass/partial_eliminate.h"
#include "frontend/optimizer/irpass/reduce_eliminate.h"
#include "frontend/optimizer/irpass/reshape_eliminate.h"
#include "frontend/optimizer/irpass/special_op_eliminate.h"
#include "frontend/optimizer/irpass/specialize_transform.h"
#include "frontend/optimizer/irpass/symbol_resolver.h"
#include "frontend/optimizer/irpass/tile_eliminate.h"
#include "frontend/optimizer/irpass/transpose_eliminate.h"
#include "frontend/optimizer/irpass/value_based_eliminate.h"
#include "frontend/optimizer/irpass/pynative_no_grad_eliminate.h"
#include "frontend/optimizer/opt.h"
#include "frontend/optimizer/irpass/row_tensor_eliminate.h"
#include "frontend/optimizer/irpass/sparse_tensor_eliminate.h"
#include "frontend/optimizer/irpass/stack_unstack_eliminate.h"
#include "frontend/optimizer/irpass/switch_or_switch_layer_defer_inline.h"
#include "frontend/optimizer/irpass/call_graph_tuple_transform.h"
#include "frontend/optimizer/irpass/recompute_prepare.h"
#include "frontend/optimizer/irpass/real_op_eliminate.h"
#include "frontend/optimizer/irpass/ge/ge_tensor_array.h"
#include "frontend/optimizer/irpass/ge/sparse_softmax_cross_entropy_with_logits_split.h"
#include "frontend/optimizer/irpass/ge/avg_pool_grad_for_ge.h"
#include "frontend/optimizer/irpass/ge/dropout_for_ge.h"
#include "frontend/optimizer/irpass/ge/lamb_split.h"
#include "frontend/optimizer/irpass/ge/clip_by_norm_split.h"
namespace mindspore {
namespace opt {
namespace irpass {
OptimizeIRPassLib::OptimizeIRPassLib() {
  arithmetic_simplify_ = MakeSubstitution(std::make_shared<ArithmeticSimplify>(), "arithmetic_simplify",
                                          {prim::kPrimScalarAdd, prim::kPrimScalarMul, prim::kPrimAdd,
                                           prim::kPrimIdentity, prim::kPrimMomentum, prim::kPrimMul, prim::kPrimPow});
  arithmetic_simplify2_ =
    MakeSubstitution(std::make_shared<ArithmeticSimplify2>(), "arithmetic_simplify2", {prim::kPrimMul});
  special_op_eliminate_ =
    MakeSubstitution(std::make_shared<SpecialOpEliminater>(), "special_op_eliminate",
                     {prim::kPrimInsertGradientOf, prim::kPrimHookBackward, prim::kPrimCellBackwardHook,
                      prim::kPrimPrintShapeType, prim::kPrimMutable});
  ad_related_special_op_eliminate_ =
    MakeSubstitution(std::make_shared<SpecialOpEliminater>(), "ad_related_special_op_eliminate",
                     {prim::kPrimMirror, prim::kPrimVirtualDiv, prim::kPrimStopGradient});
  pynative_eliminate_ = MakeSubstitution(std::make_shared<PynativeEliminater>(), "pynative_eliminate", IsCNodeDup);
  pynative_no_grad_eliminate_ =
    MakeSubstitution(std::make_shared<PynativeNoGradEliminater>(), "pynative_no_grad_eliminate", prim::kPrimMakeTuple);
  zero_like_fill_zero_ =
    MakeSubstitution(std::make_shared<ZeroLikeFillZero>(), "zero_like_fill_zero", prim::kPrimZerosLike);
  adjust_all_reduce_mul_add_ =
    MakeSubstitution(std::make_shared<AdjustAllReduceMulAdd>(), "adjust_all_reduce_mul_add", prim::kPrimAddN);
  float_depend_g_call_ = MakeSubstitution(std::make_shared<FloatDependGCall>(), "float_depend_g_call", IsCNodeDup);

  // ops eliminate
  tuple_list_get_item_eliminator_ =
    MakeSubstitution(std::make_shared<TupleListGetitemEliminator>(), "tuple_list_get_item_eliminator",
                     {prim::kPrimTupleGetItem, prim::kPrimListGetItem});
  tuple_list_get_item_const_eliminator_ =
    MakeSubstitution(std::make_shared<TupleListGetitemConstEliminator>(), "tuple_list_get_item_const_eliminator",
                     {prim::kPrimTupleGetItem, prim::kPrimListGetItem});
  tuple_list_set_item_eliminator_ =
    MakeSubstitution(std::make_shared<TupleListSetitemEliminator>(), "tuple_list_set_item_eliminator",
                     {prim::kPrimTupleSetItem, prim::kPrimListSetItem});
  tuple_list_get_set_item_eliminator_ =
    MakeSubstitution(std::make_shared<TupleListGetSetitemEliminator>(), "tuple_list_get_set_item_eliminator",
                     {prim::kPrimTupleGetItem, prim::kPrimListGetItem});
  tuple_list_get_item_depend_reorder_ =
    MakeSubstitution(std::make_shared<TupleListGetitemDependReorder>(), "tuple_list_get_item_depend_reorder",
                     {prim::kPrimTupleGetItem, prim::kPrimListGetItem});
  tuple_list_convert_item_index_to_positive_ = MakeSubstitution(
    std::make_shared<TupleListConvertItemIndexToPositive>(), "tuple_list_convert_item_index_to_positive",
    {prim::kPrimTupleGetItem, prim::kPrimTupleSetItem, prim::kPrimListGetItem, prim::kPrimListSetItem});
  make_slice_get_slice_eliminator_ = MakeSubstitution(std::make_shared<MakeSliceSliceGetItemEliminator>(),
                                                      "make_slice_get_slice_eliminator", {prim::kPrimSliceGetItem});
  stack_unstack_eliminate_ =
    MakeSubstitution(std::make_shared<StackUnstackEliminator>(), "stack_unstack_eliminate", prim::kPrimUnstack);
  tile_eliminate_ = MakeSubstitution(std::make_shared<TileEliminater>(), "tile_eliminate", prim::kPrimTile);
  cast_eliminate_ = MakeSubstitution(std::make_shared<CastEliminater>(), "cast_eliminate", prim::kPrimCast);
  reshape_eliminate_ = MakeSubstitution(std::make_shared<ReshapeEliminater>(), "reshape_eliminate", prim::kPrimReshape);
  transpose_eliminate_ =
    MakeSubstitution(std::make_shared<TransposeSameIOEliminater>(), "transpose_eliminate", prim::kPrimTranspose);
  reduce_eliminate_ = MakeSubstitution(
    std::make_shared<ReduceOneEliminater>(), "reduce_eliminate",
    {prim::kPrimReduceMean, prim::kPrimReduceAll, prim::kPrimReduceSum, prim::kPrimReduceMax, prim::kPrimReduceMin});
  partial_eliminate_ = MakeSubstitution(std::make_shared<PartialEliminater>(), "partial_eliminate", IsCNodeDup);
  same_eliminate_ = MakeSubstitution(std::make_shared<SameEliminater>(), "same_eliminate", prim::kPrimSameTypeShape);
  mini_step_allgather_replace_ = MakeSubstitution(std::make_shared<MiniStepAllGatherPass>(),
                                                  "mini_step_allgather_replace", prim::kPrimMiniStepAllGather);
  micro_step_allgather_replace_ = MakeSubstitution(std::make_shared<MicroStepAllGatherPass>(),
                                                   "micro_step_allgather_replace", prim::kPrimMicroStepAllGather);
  check_bprop_eliminate_ =
    MakeSubstitution(std::make_shared<CheckBpropEliminater>(), "check_bprop_eliminate", prim::kPrimCheckBprop);
  reset_defer_inline_ =
    MakeSubstitution(std::make_shared<ResetDeferInline>(), "reset_defer_inline", IsValueNode<FuncGraph>);
  depend_value_elim_ = MakeSubstitution(std::make_shared<DependValueElim>(), "depend_value_elim", prim::kPrimDepend);
  all_reduce_const_elim_ =
    MakeSubstitution(std::make_shared<AllReduceConstElim>(), "reduce_all_const_elim", prim::kPrimAllReduce);
  real_op_eliminate_ = MakeSubstitution(std::make_shared<RealOpEliminate>(), "real_op_eliminate", prim::kPrimRealInner);

  // Environ Item Eliminate
  environ_get_eliminate_ =
    MakeSubstitution(std::make_shared<EnvironGetEliminater>(), "environ_get_eliminate", prim::kPrimEnvironGet);
  environ_get_add_eliminate_ =
    MakeSubstitution(std::make_shared<EnvironGetAddEliminater>(), "environ_get_add_eliminate", prim::kPrimEnvironGet);
  environ_get_set_eliminate_ =
    MakeSubstitution(std::make_shared<EnvironGetSetEliminater>(), "environ_get_set_eliminate", prim::kPrimEnvironGet);
  environ_get_depend_swap_ =
    MakeSubstitution(std::make_shared<EnvironGetDependSwap>(), "environ_get_depend_swap", prim::kPrimEnvironGet);
  environ_add_const_eliminate_ = MakeSubstitution(std::make_shared<EnvironAddConstEliminater>(),
                                                  "environ_add_const_eliminate_", prim::kPrimEnvironAdd);
  split_environ_get_set_with_tuple_value_ =
    MakeSubstitution(std::make_shared<SplitEnvironGetSetWithTupleValue>(), "split_environ_get_set_with_tuple_value",
                     {prim::kPrimEnvironGet, prim::kPrimEnvironSet});

  // Ref eliminate
  replace_old_param_ = MakeSubstitution(std::make_shared<ReplaceOldParam>(), "replace_old_param", IsParam);
  minmaximum_grad_ = MakeSubstitution(std::make_shared<MinMaximumGrad>(), "minmaximum_grad", prim::kPrimTupleGetItem);

  // branch culling
  switch_simplify_ = MakeSubstitution(std::make_shared<SwitchSimplify>(), "switch_simplify", prim::kPrimSwitch);
  compare_switch_simplify_ =
    MakeSubstitution(std::make_shared<CompareSwitchSimplify>(), "compare_switch_simplify", prim::kPrimSwitch);
  float_tuple_getitem_switch_ = MakeSubstitution(std::make_shared<FloatTupleGetItemSwitch>(),
                                                 "float_tuple_getitem_switch", prim::kPrimTupleGetItem);
  float_environ_get_switch_ =
    MakeSubstitution(std::make_shared<FloatEnvironGetSwitch>(), "float_environ_get_switch", prim::kPrimEnvironGet);
  exchange_switch_depend_value_ =
    MakeSubstitution(std::make_shared<ExchangeSwitchDependValue>(), "exchange_switch_depend_value", prim::kPrimSwitch);

  switch_partial_eliminater_ =
    MakeSubstitution(std::make_shared<SwitchPartialEliminater>(), "eliminate_switch_partial_", IsCNodeDup);
  switch_layer_partial_eliminater_ =
    MakeSubstitution(std::make_shared<SwitchLayerPartialEliminater>(), "eliminate_switch_layer_partial_", IsCNodeDup);

  // Addn
  merge_addn_ = MakeSubstitution(std::make_shared<MergeAddN>(), "merge_addn", prim::kPrimAddN);
  addn_zero_filter_ = MakeSubstitution(std::make_shared<AddNZeroFilter>(), "addn_zero_filter", prim::kPrimAddN);
  addn_check_dump_ = MakeSubstitution(std::make_shared<AddNCheckDump>(), "addn_check_dump", prim::kPrimAddN);

  // AccumulateNV2
  accumulaten_eliminater_ =
    MakeSubstitution(std::make_shared<AccumulateNV2Eliminater>(), "accumulaten_eliminater", prim::kPrimAccumulateNV2);

  // Accelerated Algorithm
  less_batch_normalization_ =
    MakeSubstitution(std::make_shared<LessBatchNormalization>(), "less_batch_normalization",
                     {prim::kPrimAdd, prim::kPrimRelu6, prim::kPrimMatMul, prim::kPrimMakeTuple, prim::kPrimMaxPool});

  // inline
  inline_ = MakeSubstitution(std::make_shared<Inliner>(), "inline", IsCNodeGraph);
  inline_without_move_ = MakeSubstitution(std::make_shared<DirectInliner>(false), "inline", IsCNodeGraph);
  replace_applicator_ =
    MakeSubstitution(std::make_shared<ReplaceApplicator>(), "replace_applicator", IsValueNode<FuncGraph>);
  specialize_transform_ =
    MakeSubstitution(std::make_shared<SpecializeOnGraphArguments>(), "specialize_transform", IsCNodeGraph);

  // UpdateState eliminate
  updatestate_useless_node_eliminater_ =
    MakeSubstitution(std::make_shared<UpdatestateUselessNodeEliminater>(), "updatestate_useless_node_eliminater",
                     prim::kPrimUpdateState);
  updatestate_pure_node_eliminater_ = MakeSubstitution(std::make_shared<UpdatestatePureNodeEliminater>(),
                                                       "updatestate_pure_node_eliminater", prim::kPrimUpdateState);
  switch_call_monad_eliminater_ = MakeSubstitution(std::make_shared<SwitchCallMonadParameterEliminater>(),
                                                   "switch_call_monad_eliminater", IsCNodeDup);

  // Load eliminate
  load_eliminater_ = MakeSubstitution(std::make_shared<LoadEliminater>(), "load_eliminater", prim::kPrimLoad);

  // StopGradient eliminate
  stopgrad_eliminater_ =
    MakeSubstitution(std::make_shared<StopGradientEliminater>(), "stopgrad_eliminater", prim::kPrimStopGradient);

  // Incorporation
  incorporate_call_ = MakeSubstitution(std::make_shared<IncorporateCall>(), "incorporate_call", IsCNodeDup);
  incorporate_call_switch_ =
    MakeSubstitution(std::make_shared<IncorporateCallSwitch>(), "incorporate_call_switch", IsCNodeDup);

  // Virtual Dataset
  virtual_dataset_eliminate_ = MakeSubstitution(std::make_shared<VirtualDatasetEliminater>(),
                                                "virtual_dataset_eliminate", prim::kPrimVirtualDataset);

  // Virtual Output
  virtual_output_eliminate_ =
    MakeSubstitution(std::make_shared<VirtualOutputEliminater>(), "virtual_output_eliminate", prim::kPrimVirtualOutput);

  // PipelineSplit
  parallel_virtual_node_ =
    MakeSubstitution(std::make_shared<ParallelVirtualNodeEliminater>(), "parallel_virtual_node",
                     {prim::kPrimVirtualAssignAdd, prim::kPrimVirtualPipelineEnd, prim::kPrimVirtualAccuGrad,
                      prim::kPrimMirrorMicroStep, prim::kPrimVirtualAdd, prim::kPrimMirrorMiniStep});

  // Convert
  print_tuple_wrapper_ =
    MakeSubstitution(std::make_shared<PrintTupleWrapper>(), "print_tuple_wrapper", prim::kPrimPrint);

  // tuple parameter graph transform
  call_graph_tuple_transform_ =
    MakeSubstitution(std::make_shared<CallGraphTupleTransform>(), "graph_param_transform", IsNode);

  // RowTensor Eliminate
  row_tensor_eliminate_ = MakeSubstitution(
    std::make_shared<RowTensorEliminater>(), "row_tensor_eliminate",
    {prim::kPrimRowTensorGetIndices, prim::kPrimRowTensorGetValues, prim::kPrimRowTensorGetDenseShape});

  // RowTensorAddZerosLike Eliminate
  row_tensor_add_zeros_like_ =
    MakeSubstitution(std::make_shared<RowTensorAddZerosLike>(), "row_tensor_add_zeros_like", prim::kPrimRowTensorAdd);

  // SparseTensor Eliminate
  sparse_tensor_eliminate_ = MakeSubstitution(
    std::make_shared<SparseTensorEliminater>(), "sparse_tensor_eliminate",
    {prim::kPrimCOOTensorGetIndices, prim::kPrimCOOTensorGetValues, prim::kPrimCOOTensorGetDenseShape});

  // Value_Based Eliminate
  value_based_eliminate_ = MakeSubstitution(std::make_shared<ValueBasedEliminate>(), "value_based_eliminate",
                                            {prim::kPrimSelect, prim::kPrimMinimum, prim::kPrimMaximum});

  // switch defer inline
  switch_defer_inline_ =
    MakeSubstitution(std::make_shared<SwitchDeferInline>(), "switch_defer_inline", prim::kPrimSwitch);

  // switch_layer defer inline
  switch_layer_defer_inline_ =
    MakeSubstitution(std::make_shared<SwitchLayerDeferInline>(), "switch_layer_defer_inline", prim::kPrimSwitchLayer);

  // recompute
  set_cell_output_no_recompute_ = MakeSubstitution(std::make_shared<SetCellOutputNoRecompute>(),
                                                   "set_cell_output_no_recompute", IsValueNode<FuncGraph>);

  // Workaround
  stop_gradient_special_op_ =
    MakeSubstitution(std::make_shared<StopGradientSpecialOp>(), "stop_gradient_special_op", prim::kPrimBiasAddGrad);

  // ge_tensor_array_link_flow
  ge_tensor_array_add_flow_ = MakeSubstitution(std::make_shared<GeTensorArrayAddFlow>(), "ge_tensor_array_add_flow",
                                               {prim::kPrimTensorArrayWrite, prim::kPrimTensorArrayGather});
  // ge_tensor_array_cast_index
  ge_tensor_array_cast_index_ = MakeSubstitution(std::make_shared<GeTensorArrayCastIndex>(),
                                                 "ge_tensor_array_cast_index", prim::kPrimTensorArrayWrite);
  // sparse_softmax_cross_entropy_with_logits split for ge
  sparse_softmax_cross_entropy_with_logits_ = MakeSubstitution(
    std::make_shared<SparseSoftmaxCrossEntropyWithLogitsSplit>(), "sparse_softmax_cross_entropy_with_logits_", IsCNode);
  // process AvgPoolGrad for ge
  avg_pool_grad_for_ge_ =
    MakeSubstitution(std::make_shared<AvgPoolGradForGE>(), "avg_pool_grad_for_ge", prim::kPrimAvgPoolGrad);

  // process Dropout and DropoutGrad for ge
  dropout_for_ge_ = MakeSubstitution(std::make_shared<DropoutForGE>(), "dropout_for_ge", prim::kPrimDropout);
  dropout_grad_for_ge_ =
    MakeSubstitution(std::make_shared<DropoutGradForGE>(), "dropout_grad_for_ge", prim::kPrimDropoutGrad);
  lamb_for_ge_ = MakeSubstitution(std::make_shared<LambForGE>(), "lamb_for_ge", prim::kPrimLamb);
  clip_by_norm_for_ge_ =
    MakeSubstitution(std::make_shared<ClipByNormForGE>(), "clip_by_norm_for_ge", prim::kPrimClipByNorm);
}

ResolveIRPassLib::ResolveIRPassLib() {
  // In resolver_, some patterns have priority over others.
  resolver_ = MakeSubstitution(std::make_shared<Resolver>(), "getattr_resolve",
                               {prim::kPrimGetAttr, prim::kPrimResolve}, opt::CHECK_RENORM, true);
}

MetaUnpackPrepareLib::MetaUnpackPrepareLib() {
  meta_unpack_prepare_ = MakeSubstitution(std::make_shared<MetaFgVarPrepare>(), "meta_unpack_prepare", IsCNode);
}
}  // namespace irpass
}  // namespace opt
}  // namespace mindspore
