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
"""test python built-in functions in graph mode"""
import operator
import pytest
import numpy as np
from mindspore import Tensor, context, ms_function
from mindspore import dtype as mstype

context.set_context(mode=context.GRAPH_MODE)


@pytest.mark.level0
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_min_with_tensor():
    """
    Feature: Support the type of the input of built-in function max is tensor.
    Description: Support the type of the input of built-in function max is tensor.
    Expectation: No exception.
    """
    @ms_function
    def foo(x, y):
        return max(x, y), min(x, y)

    max_out, min_out = foo(Tensor([1]), Tensor([2]))
    assert operator.eq(max_out, 2)
    assert operator.eq(min_out, 1)


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_min_with_multiple_tensor():
    """
    Feature: Support the type of the input of built-in function max is tensor.
    Description: Support the type of the input of built-in function max is tensor.
    Expectation: No exception.
    """
    @ms_function
    def foo(x, y, z):
        return max(x, y, z), min(x, y, z)

    max_out, min_out = foo(Tensor([1]), Tensor([2]), Tensor([3]))
    assert operator.eq(max_out, 3)
    assert operator.eq(min_out, 1)


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_min_with_tensor_numpy():
    """
    Feature: Support the type of the input of built-in function min is tensor.
    Description: Support the type of the input of built-in function min is tensor.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        x = Tensor(np.array([1, 2, 3, 4, 5]), dtype=mstype.float32)
        return min(x), max(x)

    min_out, max_out = foo()
    assert operator.eq(min_out, 1)
    assert operator.eq(max_out, 5)


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_min_with_tensor_list():
    """
    Feature: Support the type of the input of built-in function min is tensor.
    Description: Support the type of the input of built-in function min is tensor.
    Expectation: No exception.
    """
    @ms_function
    def foo(x):
        return min(x), max(x)

    min_out, max_out = foo(Tensor([1, 2, 3, 4, 5], dtype=mstype.float32))
    assert operator.eq(min_out, 1)
    assert operator.eq(max_out, 5)


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_with_tuple_tensor():
    """
    Feature: Support the type of the input of built-in function max min is tensor tuple.
    Description: Support the type of the input of built-in function max min is tensor tuple.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        tuple_x = (Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32"))
        sum_max_x = Tensor(0).astype("float32")
        sum_min_x = Tensor(0).astype("float32")
        for _ in range(3):
            max_x = max(tuple_x)
            min_x = min(tuple_x)
            sum_max_x += max_x
            sum_min_x += min_x
        return sum_max_x, sum_min_x

    ret = foo()
    assert ret[0] == 150
    assert ret[1] == 30


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_with_list_tensor():
    """
    Feature: Support the type of the input of built-in function max min is tensor tuple.
    Description: Support the type of the input of built-in function max min is tensor tuple.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        list_x = [Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32")]
        sum_max_x = Tensor(0).astype("float32")
        sum_min_x = Tensor(0).astype("float32")
        for _ in range(3):
            max_x = max(list_x)
            min_x = min(list_x)
            sum_max_x += max_x
            sum_min_x += min_x
        return sum_max_x, sum_min_x

    ret = foo()
    assert ret[0] == 150
    assert ret[1] == 30


@pytest.mark.skip(reason="Not support tensor list or tuple nested")
def test_builtin_function_max_min_with_tuple_tuple_tensor():
    """
    Feature: Support the type of the input of built-in function max min is tensor tuple.
    Description: Support the type of the input of built-in function max min is tensor tuple.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        tuple_x = ((Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32")),)
        return max(tuple_x), min(tuple_x)

    ret = foo()
    assert ret[0] == (Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32"))
    assert ret[1] == (Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32"))


@pytest.mark.skip(reason="Not support tensor list or tuple nested")
def test_builtin_function_max_min_with_list_list_tensor():
    """
    Feature: Support the type of the input of built-in function max min is tensor list.
    Description: Support the type of the input of built-in function max min is tensor list.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        tuple_x = [[Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32")],]
        return max(tuple_x), min(tuple_x)

    ret = foo()
    assert ret[0] == (Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32"))
    assert ret[1] == (Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32"))


@pytest.mark.skip(reason="Not support tensor list or tuple nested")
def test_builtin_function_max_min_with_list_list_tensor_2():
    """
    Feature: Support the type of the input of built-in function max min is tensor list.
    Description: Support the type of the input of built-in function max min is tensor list.
    Expectation: No exception.
    """
    @ms_function
    def foo():
        tuple_x = [[Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32")],
                   [Tensor(20).astype("float32"), Tensor(40).astype("float32"), Tensor(60).astype("float32")]]
        return max(tuple_x), min(tuple_x)

    ret = foo()
    assert ret[0] == [Tensor(20).astype("float32"), Tensor(40).astype("float32"), Tensor(60).astype("float32")]
    assert ret[1] == [Tensor(10).astype("float32"), Tensor(30).astype("float32"), Tensor(50).astype("float32")]


@pytest.mark.skip(reason="Not support tensor list or tuple nested")
def test_builtin_function_max_min_with_list_list_tensor_out():
    """
    Feature: Support the type of the input of built-in function max min is tensor list.
    Description: Support the type of the input of built-in function max min is tensor list.
    Expectation: No exception.
    """
    @ms_function
    def foo(x):
        tuple_x = [[x / 4, 2 * x],
                   [x / 2, 3 * x]]
        return max(tuple_x), min(tuple_x)

    input_x = Tensor(20)
    ret = foo(input_x)
    assert ret[0] == [10, 60]
    assert ret[1] == [5, 40]


@pytest.mark.level1
@pytest.mark.platform_x86_gpu_training
@pytest.mark.platform_arm_ascend_training
@pytest.mark.platform_x86_ascend_training
@pytest.mark.env_onecard
def test_builtin_function_max_with_out_tensor():
    """
    Feature: Support the type of the input of built-in function max min is tensor tuple.
    Description: Support the type of the input of built-in function max min is tensor tuple.
    Expectation: No exception.
    """
    @ms_function
    def foo(x):
        y = x - Tensor(3)
        tuple_x = (2 * x, y)
        return max(tuple_x), min(tuple_x)

    x = Tensor(10).astype("float32")
    ret = foo(x)
    assert ret[0] == 20
    assert ret[1] == 7
