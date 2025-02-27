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

#ifndef MINDSPORE_CORE_OPS_CROP_AND_RESIZE_GRAD_IMAGE_H_
#define MINDSPORE_CORE_OPS_CROP_AND_RESIZE_GRAD_IMAGE_H_

#include <vector>
#include <memory>

#include "ops/primitive_c.h"
#include "ops/base_operator.h"
#include "abstract/abstract_value.h"
#include "utils/check_convert_utils.h"
#include "mindapi/base/types.h"

namespace mindspore {
namespace ops {
constexpr auto kNameCropAndResizeGradImage = "CropAndResizeGradImage";
/// \brief Computes the gradient of the crop_and_resize op wrt the input images tensor .
/// Refer to Python API @ref mindspore.ops.CropAndResizeGradImage for more details.
class MIND_API CropAndResizeGradImage : public BaseOperator {
 public:
  MIND_API_BASE_MEMBER(CropAndResizeGradImage);
  /// \brief Constructor.

  CropAndResizeGradImage() : BaseOperator(kNameCropAndResizeGradImage) {
    InitIOName({"grads", "boxes", "box_index", "image_size"}, {"y"});
  }

  /// \brief Init. Refer to the parameters of Python API @ref mindspore.ops.CropAndResizeGradImage for the inputs.
  void Init(ResizeMethod method);

  /// \brief Set method.
  void set_method(ResizeMethod method);

  /// \brief Get method.
  ResizeMethod get_method() const;
};
AbstractBasePtr CropAndResizeGradImageInfer(const abstract::AnalysisEnginePtr &, const PrimitivePtr &primitive,
                                            const std::vector<AbstractBasePtr> &input_args);
using PrimCropAndResizeGradImagePtr = std::shared_ptr<CropAndResizeGradImage>;
}  // namespace ops
}  // namespace mindspore

#endif  // MINDSPORE_CORE_OPS_CROP_AND_RESIZE_GRAD_IMAGE_H_
