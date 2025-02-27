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

#include "ops/apply_proximal_gradient_descent.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include "abstract/ops/primitive_infer_map.h"
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "utils/tensor_construct_utils.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
namespace {
abstract::ShapePtr ApplyProximalGradientDescentInferShape(const PrimitivePtr &primitive,
                                                          const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  auto var_shape = input_args[kInputIndex0]->BuildShape();
  auto alpha_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex1]->BuildShape())[kShape];
  auto l1_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex2]->BuildShape())[kShape];
  auto l2_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex3]->BuildShape())[kShape];
  auto delta_shape = input_args[kInputIndex4]->BuildShape();

  size_t batch_rank = 0;
  if (primitive->HasAttr(kBatchRank)) {
    auto value_ptr = primitive->GetAttr(kBatchRank);
    batch_rank = GetValue<int64_t>(value_ptr);
  }

  (void)CheckAndConvertUtils::CheckInteger("alpha_shape size", SizeToLong(alpha_shape.size()), kLessEqual, batch_rank,
                                           prim_name);
  (void)CheckAndConvertUtils::CheckInteger("l1_shape size", SizeToLong(l1_shape.size()), kLessEqual, batch_rank,
                                           prim_name);
  (void)CheckAndConvertUtils::CheckInteger("l2_shape size", SizeToLong(l2_shape.size()), kLessEqual, batch_rank,
                                           prim_name);

  // var and delta must have the same shape
  auto var_shape_ptr = var_shape->cast<abstract::ShapePtr>();
  auto delta_shape_ptr = delta_shape->cast<abstract::ShapePtr>();
  if (!var_shape_ptr->IsDynamic() && !delta_shape_ptr->IsDynamic()) {
    if (*var_shape != *delta_shape) {
      MS_EXCEPTION(ValueError) << "For '" << primitive->name()
                               << "', evaluator arg 'delta' must have the same shape as 'var'. But got 'delta' shape: "
                               << delta_shape->ToString() << ", 'var' shape: " << var_shape->ToString() << ".";
    }
  }
  auto shape_element = var_shape->cast<abstract::ShapePtr>();
  MS_EXCEPTION_IF_NULL(shape_element);
  return shape_element;
}

TypePtr ApplyProximalGradientDescentInferType(const PrimitivePtr &prim,
                                              const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(prim);
  auto prim_name = prim->name();
  auto var_type = input_args[kInputIndex0]->BuildType();
  auto alpha_type = input_args[kInputIndex1]->BuildType();
  auto l1_type = input_args[kInputIndex2]->BuildType();
  auto l2_type = input_args[kInputIndex3]->BuildType();
  auto delta_type = input_args[kInputIndex4]->BuildType();
  const std::set<TypePtr> valid_types = {kFloat16, kFloat32, kFloat64};
  // var, delta must have the same type as var
  std::map<std::string, TypePtr> args;
  (void)args.insert(std::make_pair("var_type", var_type));
  (void)args.insert(std::make_pair("delta_type", delta_type));
  (void)CheckAndConvertUtils::CheckTensorTypeSame(args, valid_types, prim_name);
  // alpha、l1、l2 must be a scalar type
  std::map<std::string, TypePtr> args_alpha;
  std::map<std::string, TypePtr> args_l1;
  std::map<std::string, TypePtr> args_l2;
  (void)args_alpha.insert(std::make_pair("alpha_type", alpha_type));
  (void)CheckAndConvertUtils::CheckScalarOrTensorTypesSame(args_alpha, valid_types, prim_name);
  (void)args_l1.insert(std::make_pair("l1_type", l1_type));
  (void)CheckAndConvertUtils::CheckScalarOrTensorTypesSame(args_l1, valid_types, prim_name);
  (void)args_l2.insert(std::make_pair("l2_type", l2_type));
  (void)CheckAndConvertUtils::CheckScalarOrTensorTypesSame(args_l2, valid_types, prim_name);
  return var_type;
}
}  // namespace

MIND_API_OPERATOR_IMPL(ApplyProximalGradientDescent, BaseOperator);
AbstractBasePtr ApplyProximalGradientDescentInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                                  const std::vector<AbstractBasePtr> &input_args) {
  const int64_t input_num = 5;
  CheckAndConvertUtils::CheckInputArgs(input_args, kGreaterEqual, input_num, primitive->name());
  auto infer_type = ApplyProximalGradientDescentInferType(primitive, input_args);
  auto infer_shape = ApplyProximalGradientDescentInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}

REGISTER_PRIMITIVE_EVAL_IMPL(ApplyProximalGradientDescent, prim::kPrimApplyProximalGradientDescent,
                             ApplyProximalGradientDescentInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
