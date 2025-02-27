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

#include "ops/sparse_reorder.h"
#include <set>
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"
#include "utils/tensor_construct_utils.h"

namespace mindspore {
namespace ops {
namespace {
abstract::TupleShapePtr SparseReorderInferShape(const PrimitivePtr &primitive,
                                                const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  auto indices_shape_ptr = input_args[0]->BuildShape();
  auto values_shape_ptr = input_args[1]->BuildShape();
  auto indices_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(indices_shape_ptr)[kShape];
  auto values_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(values_shape_ptr)[kShape];
  auto shape_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[2]->BuildShape())[kShape];
  // Indices  must be 2D
  const int64_t indices_dims = 2;
  (void)CheckAndConvertUtils::CheckInteger("indices dim", SizeToLong(indices_shape.size()), kEqual, indices_dims,
                                           prim_name);
  // Args shape and values must be 1D
  (void)CheckAndConvertUtils::CheckInteger("values dim", SizeToLong(values_shape.size()), kEqual, 1, prim_name);
  (void)CheckAndConvertUtils::CheckInteger("size dim", SizeToLong(shape_shape.size()), kEqual, 1, prim_name);
  // Indices shape must be equal to the first dimension of var
  CheckAndConvertUtils::CheckInteger("size of values", values_shape[0], kEqual, indices_shape[0], prim_name);
  CheckAndConvertUtils::CheckInteger("size of shape", shape_shape[0], kEqual, indices_shape[1], prim_name);
  return std::make_shared<abstract::TupleShape>(
    std::vector<abstract::BaseShapePtr>{indices_shape_ptr, values_shape_ptr});
}

TuplePtr SparseReorderInferType(const PrimitivePtr &prim, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(prim);
  auto prim_name = prim->name();
  auto indices_type = input_args[0]->BuildType();
  auto values_type = input_args[1]->BuildType();
  auto shape_type = input_args[2]->BuildType();
  // Args values must be a scalar type
  const std::set<TypePtr> valid_types_values = {kBool,   kInt8,    kInt16,   kInt32,   kInt64,     kUInt8,
                                                kUInt16, kFloat16, kFloat32, kFloat64, kComplex64, kComplex128};
  const std::set<TypePtr> valid_types_indices = {kInt64};
  (void)CheckAndConvertUtils::CheckTensorTypeValid("values", values_type, valid_types_values, prim_name);
  (void)CheckAndConvertUtils::CheckTensorTypeValid("indices", indices_type, valid_types_indices, prim_name);
  (void)CheckAndConvertUtils::CheckTensorTypeValid("shape", shape_type, valid_types_indices, prim_name);
  return std::make_shared<Tuple>(std::vector<TypePtr>{indices_type, values_type});
}
}  // namespace

AbstractBasePtr SparseReorderInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t kInputsNum = 3;
  CheckAndConvertUtils::CheckInputArgs(input_args, kGreaterEqual, kInputsNum, primitive->name());
  auto infer_type = SparseReorderInferType(primitive, input_args);
  auto infer_shape = SparseReorderInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}
MIND_API_OPERATOR_IMPL(SparseReorder, BaseOperator);
REGISTER_PRIMITIVE_EVAL_IMPL(SparseReorder, prim::kPrimSparseReorder, SparseReorderInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
