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

#include "ops/rgb_to_hsv.h"
#include <set>
#include "abstract/ops/primitive_infer_map.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
namespace {
abstract::ShapePtr RGBToHSVInferShape(const PrimitivePtr &primitive, const std::vector<AbstractBasePtr> &input_args) {
  auto input_shape = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[0]->BuildShape())[kShape];

  const int64_t input_dims = SizeToLong(input_shape.size());
  const int64_t input_last_dims = input_shape.cend()[-1];
  const int64_t numberofRGB_3 = 3;
  (void)CheckAndConvertUtils::CheckInteger("last dimension of input 'images'", input_last_dims, kEqual, numberofRGB_3,
                                           kNameRGBToHSV);
  if (input_dims < 1) {
    MS_LOG(EXCEPTION) << "For " << primitive->name() << ", the dimension of input 'images' must be 1-D or higher rank.";
  }
  return std::make_shared<abstract::Shape>(input_shape);
}

TypePtr RGBToHSVInferType(const PrimitivePtr &prim, const std::vector<AbstractBasePtr> &input_args) {
  auto input_dtype = input_args[0]->BuildType();
  const std::set<TypePtr> input_valid_types = {kFloat16, kFloat32, kFloat64};
  return CheckAndConvertUtils::CheckTensorTypeValid("type of input 'images'", input_dtype, input_valid_types,
                                                    kNameRGBToHSV);
}
}  // namespace

MIND_API_OPERATOR_IMPL(RGBToHSV, BaseOperator);
AbstractBasePtr RGBToHSVInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                              const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  const int64_t input_num = 1;
  CheckAndConvertUtils::CheckInputArgs(input_args, kEqual, input_num, primitive->name());
  auto infer_type = RGBToHSVInferType(primitive, input_args);
  auto infer_shape = RGBToHSVInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}

REGISTER_PRIMITIVE_EVAL_IMPL(RGBToHSV, prim::kPrimRGBToHSV, RGBToHSVInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
