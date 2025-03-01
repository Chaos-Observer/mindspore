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
""" test_list_clear """
import pytest
import numpy as np
from mindspore import Tensor, ms_function, context


context.set_context(mode=context.GRAPH_MODE)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_list_clear_1():
    """
    Feature: list clear.
    Description: support list clear.
    Expectation: No exception.
    """
    @ms_function
    def list_net_1():
        x = [1, 2, 3, 4]
        x.clear()
        return Tensor(x)
    out = list_net_1()
    assert np.all(out.asnumpy() == ())


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_list_clear_2():
    """
    Feature: list clear.
    Description: support list clear.
    Expectation: No exception.
    """
    @ms_function
    def list_net_2():
        aa = 20
        x = ['a', ['bb', '2', 3], aa, 4]
        x.clear()
        return Tensor(x)
    out = list_net_2()
    assert np.all(out.asnumpy() == ())


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_list_clear_3():
    """
    Feature: list clear.
    Description: support list clear.
    Expectation: No exception.
    """
    @ms_function
    def list_net_3():
        aa = 20
        bb = Tensor(1)
        x = ['a', ('Michael', 'Bob', '2'), aa, 4, bb, [1, 2], Tensor(1)]
        x.clear()
        return Tensor(x)
    out = list_net_3()
    assert np.all(out.asnumpy() == ())


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_list_clear_4():
    """
    Feature: list clear.
    Description: support list clear.
    Expectation: No exception.
    """
    @ms_function
    def list_net_4():
        x = []
        x.clear()
        return Tensor(x)
    out = list_net_4()
    assert np.all(out.asnumpy() == ())
