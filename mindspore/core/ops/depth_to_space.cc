/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "ops/depth_to_space.h"
#include "ops/op_utils.h"
#include "utils/check_convert_utils.h"
#include "mindapi/src/helper.h"

namespace mindspore {
namespace ops {
void DepthToSpace::set_block_size(const int64_t block_size) {
  CheckAndConvertUtils::Check(kBlockSize, block_size, kGreaterEqual, 2, this->name());
  (void)this->AddAttr(kBlockSize, api::MakeValue(block_size));
}

int64_t DepthToSpace::get_block_size() const { return GetValue<int64_t>(GetAttr(kBlockSize)); }
void DepthToSpace::set_format(const Format &format) {
  int64_t f = format;
  (void)this->AddAttr(kFormat, api::MakeValue(f));
}

Format DepthToSpace::get_format() const { return Format(GetValue<int64_t>(GetAttr(kFormat))); }

void DepthToSpace::Init(const int64_t block_size, const Format &format) {
  this->set_block_size(block_size);
  this->set_format(format);
}

namespace {
abstract::ShapePtr DepthToSpaceInferShape(const PrimitivePtr &primitive,
                                          const std::vector<AbstractBasePtr> &input_args) {
  MS_EXCEPTION_IF_NULL(primitive);
  auto prim_name = primitive->name();
  (void)CheckAndConvertUtils::CheckInteger("input number", int64_t(input_args.size()), kEqual, 1, prim_name);
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  auto input_x = input_args[kInputIndex0]->cast<abstract::AbstractTensorPtr>();
  MS_EXCEPTION_IF_NULL(input_x);

  auto shape_map = CheckAndConvertUtils::ConvertShapePtrToShapeMap(input_args[kInputIndex0]->BuildShape());
  auto x_shape = shape_map[kShape];
  if (IsDynamicRank(x_shape)) {
    return std::make_shared<abstract::Shape>(std::vector<int64_t>{-2});
  }
  auto data_format_ptr = primitive->GetAttr("format");
  int64_t format = CheckAndConvertUtils::GetAndCheckFormat(primitive->GetAttr("format"));
  primitive->AddAttr("data_format", data_format_ptr);
  const int64_t dim_0 = 0;
  const int64_t dim_1 = 1;
  const int64_t dim_2 = 2;
  const int64_t dim_3 = 3;
  const int64_t min_block_size = 2;
  const int64_t x_rank = 4;
  if (format == Format::NHWC) {
    x_shape = {x_shape[dim_0], x_shape[dim_3], x_shape[dim_1], x_shape[dim_2]};
  }
  (void)CheckAndConvertUtils::CheckInteger("x rank", SizeToLong(x_shape.size()), kEqual, x_rank, prim_name);
  int64_t block_size = GetValue<int64_t>(primitive->GetAttr(kBlockSize));
  if (block_size < min_block_size) {
    MS_EXCEPTION(ValueError) << "For DepthToSpace, block_size must greater than 2, but got the block_size is "
                             << block_size;
  }
  (void)CheckAndConvertUtils::CheckInteger("block_size", block_size % dim_1, kEqual, 0, prim_name);
  if (input_args[kInputIndex0]->BuildShape()->IsDynamic()) {
    return std::make_shared<abstract::Shape>(std::vector<int64_t>{-1});
  }
  (void)CheckAndConvertUtils::CheckInteger("x_shape[1] % (block_size*block_size)",
                                           x_shape[dim_1] % (block_size * block_size), kEqual, 0, prim_name);
  auto out_shape = x_shape;
  out_shape[dim_1] /= block_size * block_size;
  out_shape[dim_2] *= block_size;
  out_shape[dim_3] *= block_size;
  if (format == Format::NHWC) {
    out_shape = {out_shape[dim_0], out_shape[dim_2], out_shape[dim_3], out_shape[dim_1]};
  }
  return std::make_shared<abstract::Shape>(out_shape);
}

TypePtr DepthToSpaceInferType(const PrimitivePtr &prim, const std::vector<AbstractBasePtr> &input_args) {
  for (const auto &item : input_args) {
    MS_EXCEPTION_IF_NULL(item);
  }
  auto x_type = input_args[kInputIndex0]->BuildType();
  std::set<TypePtr> valid_types = {kTensorType};
  (void)CheckAndConvertUtils::CheckTensorTypeValid("x_type", x_type, valid_types, prim->name());
  return x_type;
}
}  // namespace

MIND_API_OPERATOR_IMPL(DepthToSpace, BaseOperator);
AbstractBasePtr DepthToSpaceInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                  const std::vector<AbstractBasePtr> &input_args) {
  auto infer_type = DepthToSpaceInferType(primitive, input_args);
  auto infer_shape = DepthToSpaceInferShape(primitive, input_args);
  return abstract::MakeAbstract(infer_shape, infer_type);
}
REGISTER_PRIMITIVE_EVAL_IMPL(DepthToSpace, prim::kPrimDepthToSpace, DepthToSpaceInfer, nullptr, true);
}  // namespace ops
}  // namespace mindspore
