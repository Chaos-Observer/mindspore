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
from mindspore import nn, context, Tensor
from mindspore.ops.operations.math_ops import Betainc
from mindspore.common import dtype as mstype
from .test_grad_of_dynamic import TestDynamicGrad


class NetBetainc(nn.Cell):
    def __init__(self):
        super().__init__()
        self.betainc = Betainc()

    def construct(self, a, b, x):
        return self.betainc(a, b, x)


def grad_dyn_case(is_dynamic_rank):
    test_dynamic = TestDynamicGrad(NetBetainc())
    shape = (3, 3)
    a = Tensor(np.random.randint(low=1, high=10, size=shape), mstype.float32)
    b = Tensor(np.random.randint(low=1, high=10, size=shape), mstype.float32)
    x = Tensor(np.random.randint(low=1, high=10, size=shape), mstype.float32)

    test_dynamic.test_dynamic_grad_net([a, b, x], is_dynamic_rank)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_grad_dynamic_shape_1():
    """
    Feature: test Betainc grad dynamic shape on CPU.
    Description: input is dynamic shape.
    Expectation: the result match with static shape
    """
    context.set_context(mode=context.PYNATIVE_MODE, device_target="CPU")
    grad_dyn_case(False)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_grad_dynamic_rank_1():
    """
    Feature: test Betainc grad dynamic rank on CPU.
    Description: input is dynamic rank.
    Expectation: the result match with static shape
    """
    context.set_context(mode=context.PYNATIVE_MODE, device_target="CPU")
    grad_dyn_case(True)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_grad_dynamic_shape_2():
    """
    Feature: test Betainc grad dynamic shape on CPU.
    Description: input is dynamic shape.
    Expectation: the result match with static shape
    """
    context.set_context(mode=context.GRAPH_MODE, device_target="CPU")
    grad_dyn_case(False)


@pytest.mark.level1
@pytest.mark.platform_x86_cpu
@pytest.mark.env_onecard
def test_grad_dynamic_rank_2():
    """
    Feature: test Betainc grad dynamic shape on CPU.
    Description: input is dynamic shape.
    Expectation: the result match with static shape
    """
    context.set_context(mode=context.GRAPH_MODE, device_target="CPU")
    grad_dyn_case(True)
