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

#include "ops/grad/cdist_grad.h"

#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
namespace {
constexpr size_t kCdistGradInputDimsMin = 2;

abstract::ShapePtr CdistGradInferShape(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  size_t batch_rank = 0;
  if (primitive->HasAttr(kBatchRank)) {
    auto value_ptr = primitive->GetAttr(kBatchRank);
    batch_rank = GetValue<int64_t>(value_ptr);
  }
  auto grad_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape())[kShape];
  auto x_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[1]->BuildShape())[kShape];
  auto y_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[2]->BuildShape())[kShape];
  auto cdist_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[3]->BuildShape())[kShape];
  auto x_size = x_shape.size();
  auto y_size = y_shape.size();
  if (IsDynamic(x_shape) || IsDynamic(y_shape)) {
    return std::make_shared<abstract::Shape>(x_shape);
  }
  CheckAndConvertUtils::Check("grad shape", grad_shape, kEqual, cdist_shape, prim_name, ValueError);
  if (x_size != y_size) {
    MS_EXCEPTION(ValueError) << "For 'CdistGrad', rank of input_x and input_y must be equal, but got input_x size: "
                             << x_size << ", input_y size: " << y_size << ".";
  }

  if (batch_rank == 0) {
    CheckAndConvertUtils::CheckInRange("input_x dim", x_size, kIncludeBoth, {2, 3}, "Cdist");
  }

  if (x_size < kCdistGradInputDimsMin) {
    MS_EXCEPTION(ValueError) << "For '" << primitive->name() << "', rank of input must be greater than "
                             << kCdistGradInputDimsMin << ", but got rank of input: " << x_size << ".";
  }

  if (x_size > kCdistGradInputDimsMin) {
    for (size_t i = 0; i < x_size - kCdistGradInputDimsMin; i++) {
      if (x_shape[i] != y_shape[i]) {
        MS_EXCEPTION(ValueError) << "For '" << primitive->name()
                                 << "', the batch shape of 'x' must be the same as the shape of 'y', "
                                    "but got 'x_shape["
                                 << i << "]': " << x_shape[i] << " and 'y_shape[" << i << "]': " << y_shape[i];
      }
    }
  }

  auto out_shape = x_shape;
  return std::make_shared<abstract::Shape>(out_shape);
}

TypePtr CdistGradInferType(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  const std::set<TypePtr> valid_types = {kFloat32, kFloat16};
  std::map<std::string, TypePtr> types;
  (void)types.emplace("grad", input_args[0]->BuildType());
  (void)types.emplace("input_x", input_args[1]->BuildType());
  (void)types.emplace("input_y", input_args[2]->BuildType());
  (void)types.emplace("cdist", input_args[3]->BuildType());
  return CheckAndConvertUtils::CheckTensorTypeSame(types, valid_types, primitive->name());
}
}  // namespace

float CdistGrad::get_p() const {
  auto value_ptr = this->GetAttr(kP);
  return GetValue<float>(value_ptr);
}
void CdistGrad::set_p(const float p) { (void)this->AddAttr(kP, api::MakeValue(p)); }

MIND_API_OPERATOR_IMPL(CdistGrad, BaseOperator);
AbstractBasePtr CdistGradInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                               const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t input_num = 4;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, input_num, primitive->name());
  auto infer_type = CdistGradInferType(primitive, input_args);
  auto infer_shape = CdistGradInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}
REGISTER_PRIMITIVE_EVAL_IMPL(CdistGrad, prim::kPrimCdistGrad, CdistGradInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
