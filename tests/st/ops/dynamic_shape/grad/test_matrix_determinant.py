# Copyright 2022 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

import numpy as np
import pytest
import mindspore.nn as nn
from mindspore import Tensor, context
from mindspore.ops import operations as P
from .test_grad_of_dynamic import TestDynamicGrad


class MatrixDeterminantNet(nn.Cell):
    def __init__(self):
        super(MatrixDeterminantNet, self).__init__()
        self.matrix_determinant = P.MatrixDeterminant()

    def construct(self, x):
        return self.matrix_determinant(x)


def dynamic_shape():
    test_dynamic = TestDynamicGrad(MatrixDeterminantNet())
    x = Tensor(np.array([[[-4.5, -1.5], [7.0, 6.0]], [[2.5, 0.5], [3.0, 9.0]]]).astype(np.float32))
    test_dynamic.test_dynamic_grad_net(x)


def dynamic_rank():
    test_dynamic = TestDynamicGrad(MatrixDeterminantNet())
    x = Tensor(np.array([[[-4.5, -1.5], [7.0, 6.0]], [[2.5, 0.5], [3.0, 9.0]]]).astype(np.float32))
    test_dynamic.test_dynamic_grad_net(x, True)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_dynamic_matrix_determinant_cpu():
    """
    Feature: MatrixDeterminant Grad DynamicShape.
    Description: Test case of dynamic shape for  MatrixDeterminant grad operator on CPU.
    Expectation: success.
    """
    # Graph mode
    context.set_context(mode=context.GRAPH_MODE, device_target="CPU")
    dynamic_shape()
    dynamic_rank()
    # PyNative mode
    context.set_context(mode=context.PYNATIVE_MODE, device_target="CPU")
    dynamic_shape()
    dynamic_rank()


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.env_onecard
def test_dynamic_matrix_determinant_gpu():
    """
    Feature: MatrixDeterminant Grad DynamicShape.
    Description: Test case of dynamic shape for  MatrixDeterminant grad operator on GPU.
    Expectation: success.
    """
    # Graph mode
    context.set_context(mode=context.GRAPH_MODE, device_target="GPU")
    dynamic_shape()
    dynamic_rank()
    # PyNative mode
    context.set_context(mode=context.PYNATIVE_MODE, device_target="GPU")
    dynamic_shape()
    dynamic_rank()
