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

#include "ops/conjugate_transpose.h"
#include <vector>
#include <memory>
#include <set>
#include <algorithm>
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
namespace {
abstract::ShapePtr ConjugateTransposeInferShape(const PrimitivePtr &primitive,
                                                const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto op_name = primitive->name();
  auto x_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape())[kShape];
  ShapeVector p_value;
  ShapeVector p_value_raw;
  auto perm_value = input_args[1]->BuildValue();
  MS_EXCEPTION_IF_NULL(perm_value);
  if (perm_value->isa<ValueTuple>()) {
    p_value_raw = CheckAndConvertUtils::CheckTupleInt("input[perm]", perm_value, op_name);
  } else {
    MS_EXCEPTION(TypeError) << "For '" << op_name << "', the type of perm must be Tuple, but got "
                            << input_args[1]->BuildType()->ToString() << " .";
  }
  for (auto p : p_value_raw) {
    p = (p >= 0) ? p : (p_value_raw.size() + p);
    p_value.emplace_back(p);
  }
  if (x_shape.size() != p_value.size()) {
    MS_EXCEPTION(ValueError) << "For '" << op_name << "', the dimension of x " << x_shape.size() << " and perm "
                             << p_value.size() << " must be equal, but got the dimension of x " << x_shape.size()
                             << " and perm " << p_value.size() << " .";
  }
  constexpr int64_t dim_7 = 7;
  if (p_value.size() > dim_7) {
    MS_EXCEPTION(ValueError) << "For '" << op_name << "', the dimension of perm must be less than 8, but get "
                             << p_value.size() << " .";
  }
  for (auto i : p_value) {
    (void)CheckAndConvertUtils::CheckInteger("perm element", i, kLessThan, SizeToLong(p_value.size()), op_name);
  }
  std::vector<int64_t> tmp(p_value);
  for (auto it = tmp.begin(); it != tmp.end();) {
    auto dim = *it;
    if (!tmp.empty()) {
      it = tmp.erase(it);
    }
    if (std::find(tmp.begin(), tmp.end(), dim) != tmp.end()) {
      MS_EXCEPTION(ValueError) << "For '" << op_name << "', the value of perm must be different.";
    }
  }
  std::vector<int64_t> in_shape(p_value);
  (void)std::transform(in_shape.begin(), in_shape.end(), in_shape.begin(), [x_shape](size_t i) { return x_shape[i]; });
  return std::make_shared<abstract::Shape>(in_shape);
}

TypePtr ConjugateTransposeInferType(const PrimitivePtr &prim, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(prim);
  const std::set<TypePtr> all_types_with_complex = {kBool,    kInt,     kInt8,    kInt16,     kInt32,     kInt64,
                                                    kUInt,    kUInt8,   kUInt16,  kUInt32,    kUInt64,    kFloat,
                                                    kFloat16, kFloat32, kFloat64, kComplex64, kComplex128};
  (void)CheckAndConvertUtils::CheckTensorTypeValid("x", input_args[0]->BuildType(), all_types_with_complex,
                                                   prim->name());
  return input_args[0]->BuildType();
}
}  // namespace

MIND_API_OPERATOR_IMPL(ConjugateTranspose, BaseOperator);
AbstractBasePtr ConjugateTransposeInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                        const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t kInputsNum = 2;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, kInputsNum, primitive->name());
  auto type = ConjugateTransposeInferType(primitive, input_args);
  auto shape = ConjugateTransposeInferShape(primitive, input_args);
  return abstract::MakeAbstract(shape, type);
}
REGISTER_PRIMITIVE_EVAL_IMPL(ConjugateTranspose, prim::kPrimConjugateTranspose, ConjugateTransposeInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
