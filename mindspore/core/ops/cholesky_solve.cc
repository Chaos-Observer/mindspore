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

#include "ops/cholesky_solve.h"

#include "abstract/dshape.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "utils/tensor_construct_utils.h"

namespace mindspore {
namespace ops {
namespace {
abstract::ShapePtr CholeskySolveInferShape(const PrimitivePtr &primitive,
                                           const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const size_t kDefalutRank = 2;
  const size_t kBatchRank = 3;
  const size_t kBatchIndex = 3;
  const size_t kRowIndex = 2;
  const size_t kColIndex = 1;
  auto x1_shape_map = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex0]->BuildShape());
  auto x2_shape_map = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex1]->BuildShape());
  auto x1_shape = x1_shape_map[kShape];
  auto x2_shape = x2_shape_map[kShape];
  ShapeVector out_shape = {};
  if (IsDynamicRank(x1_shape) || IsDynamicRank(x2_shape)) {
    out_shape.push_back(abstract::Shape::kShapeRankAny);
    return std::make_shared<abstract::Shape>(out_shape);
  }
  if (x1_shape.size() != kDefalutRank && x1_shape.size() != kBatchRank) {
    MS_EXCEPTION(ValueError) << "For CholeskySolve, the rank of x1 must be equal to 2 or 3"
                             << ", while got x1 rank " << x1_shape.size() << ".";
  }
  if (x2_shape.size() != kDefalutRank && x2_shape.size() != kBatchRank) {
    MS_EXCEPTION(ValueError) << "For CholeskySolve, the rank of x2 must be equal to 2 or 3"
                             << ", while got x2 rank " << x2_shape.size() << ".";
  }
  if (x1_shape.size() != x2_shape.size()) {
    MS_EXCEPTION(ValueError) << "For CholeskySolve, ranks of inputs should be equal"
                             << ", while got x1 rank " << x1_shape.size() << ", x2 rank " << x2_shape.size() << ".";
  }
  size_t rank = x1_shape.size();
  if (rank == kDefalutRank) {
    if (x1_shape[rank - kRowIndex] != x2_shape[rank - kRowIndex]) {
      MS_EXCEPTION(ValueError) << "For CholeskySolve, x1 and x2 should share the same row number"
                               << ", while row number of x1 is " << static_cast<size_t>(x1_shape[rank - kRowIndex])
                               << ", row number of x2 is " << static_cast<size_t>(x2_shape[rank - kRowIndex]);
    }
    if (x2_shape[rank - kRowIndex] != x2_shape[rank - kColIndex]) {
      MS_EXCEPTION(ValueError) << "For CholeskySolve, x2 should be square"
                               << ", but got " << static_cast<size_t>(x2_shape[rank - kRowIndex]) << " x "
                               << static_cast<size_t>(x2_shape[rank - kColIndex]) << " matrix.";
    }
  } else {
    if (x1_shape[rank - kBatchIndex] != x2_shape[rank - kBatchIndex]) {
      MS_EXCEPTION(ValueError) << "For CholeskySolve, x1 and x2 should share the same batch size"
                               << ", while x1 is of size " << static_cast<size_t>(x1_shape[rank - kBatchIndex])
                               << ", and x2 is of size " << static_cast<size_t>(x2_shape[rank - kBatchIndex]);
    }
    if (x1_shape[rank - kRowIndex] != x2_shape[rank - kRowIndex]) {
      MS_EXCEPTION(ValueError) << "For CholeskySolve, x1 and x2 should share the same row number"
                               << ", while row number of x1 is " << static_cast<size_t>(x1_shape[rank - kRowIndex])
                               << ", row number of x2 is " << static_cast<size_t>(x2_shape[rank - kRowIndex]);
    }
    if (x2_shape[rank - kRowIndex] != x2_shape[rank - kColIndex]) {
      MS_EXCEPTION(ValueError) << "For CholeskySolve, x2 should be batch squares"
                               << ", but got batch " << static_cast<size_t>(x2_shape[rank - kRowIndex]) << " x "
                               << static_cast<size_t>(x2_shape[rank - kColIndex]) << " matrices.";
    }
  }
  return std::make_shared<abstract::Shape>(x1_shape);
}

TypePtr CholeskySolveInferType(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto op_name = primitive->name();
  const std::set<TypePtr> valid_types = {kFloat32, kFloat64};
  std::map<std::string, TypePtr> args;
  (void)args.emplace("x1", input_args[kInputIndex0]->BuildType());
  (void)args.emplace("x2", input_args[kInputIndex1]->BuildType());
  (void)CheckAndConvertUtils::CheckTensorTypeSame(args, valid_types, op_name);
  return input_args[kInputIndex0]->BuildType();
}
}  // namespace

MIND_API_OPERATOR_IMPL(CholeskySolve, BaseOperator);
AbstractBasePtr CholeskySolveInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                   const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto op_name = primitive->name();
  const int64_t input_num = 2;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, input_num, op_name);
  auto infer_type = CholeskySolveInferType(primitive, input_args);
  auto infer_shape = CholeskySolveInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}

REGISTER_PRIMITIVE_EVAL_IMPL(CholeskySolve, prim::kPrimCholeskySolve, CholeskySolveInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
