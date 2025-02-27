# Copyright 2020-2022 Huawei Technologies Co., Ltd
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
# ==============================================================================
"""
Testing concatenate op
"""

import numpy as np
import pytest

import mindspore.dataset as ds
import mindspore.dataset.transforms as data_trans


def test_concatenate_op_all():
    """
    Feature: Concatenate op
    Description: Test Concatenate op with all input parameters provided
    Expectation: Output is equal to the expected output
    """
    def gen():
        yield (np.array([5., 6., 7., 8.], dtype=np.float),)

    prepend_tensor = np.array([1.4, 2., 3., 4., 4.5], dtype=np.float)
    append_tensor = np.array([9., 10.3, 11., 12.], dtype=np.float)
    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate(0, prepend_tensor, append_tensor)
    data = data.map(operations=concatenate_op, input_columns=["col"])
    expected = np.array([1.4, 2., 3., 4., 4.5, 5., 6., 7., 8., 9., 10.3,
                         11., 12.])
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], expected)


def test_concatenate_op_none():
    """
    Feature: Concatenate op
    Description: Test Concatenate op with none of the input parameters provided
    Expectation: Output is equal to the expected output
    """
    def gen():
        yield (np.array([5., 6., 7., 8.], dtype=np.float),)

    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate()

    data = data.map(operations=concatenate_op, input_columns=["col"])
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], np.array([5., 6., 7., 8.], dtype=np.float))


def test_concatenate_op_string():
    """
    Feature: Concatenate op
    Description: Test Concatenate op on array of strings
    Expectation: Output is equal to the expected output
    """
    def gen():
        yield (np.array(["ss", "ad"], dtype='S'),)

    prepend_tensor = np.array(["dw", "df"], dtype='S')
    append_tensor = np.array(["dwsdf", "df"], dtype='S')
    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate(0, prepend_tensor, append_tensor)

    data = data.map(operations=concatenate_op, input_columns=["col"])
    expected = np.array(["dw", "df", "ss", "ad", "dwsdf", "df"], dtype='S')
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], expected)


def test_concatenate_op_multi_input_string():
    """
    Feature: Concatenate op
    Description: Test Concatenate op on multi dimension array of strings
    Expectation: Output is equal to the expected output
    """
    prepend_tensor = np.array(["dw", "df"])
    append_tensor = np.array(["dwsdf", "df"])

    data = ([["1", "2", "d"]], [["3", "4", "e"]])
    data = ds.NumpySlicesDataset(data, column_names=["col1", "col2"])

    concatenate_op = data_trans.Concatenate(0, prepend=prepend_tensor, append=append_tensor)

    data = data.map(operations=concatenate_op, input_columns=["col1", "col2"], column_order=["out1"],
                    output_columns=["out1"])
    expected = np.array(["dw", "df", "1", "2", "d", "3", "4", "e", "dwsdf", "df"])
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], expected)


def test_concatenate_op_multi_input_numeric():
    """
    Feature: Concatenate op
    Description: Test Concatenate op on multi dimension array of ints
    Expectation: Output is equal to the expected output
    """
    prepend_tensor = np.array([3, 5])

    data = ([[1, 2]], [[3, 4]])
    data = ds.NumpySlicesDataset(data, column_names=["col1", "col2"])

    concatenate_op = data_trans.Concatenate(0, prepend=prepend_tensor)

    data = data.map(operations=concatenate_op, input_columns=["col1", "col2"], column_order=["out1"],
                    output_columns=["out1"])
    expected = np.array([3, 5, 1, 2, 3, 4])
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], expected)


def test_concatenate_op_type_mismatch():
    """
    Feature: Concatenate op
    Description: Test Concatenate op where the data type of the original array dataset (float) has a mismatch
        data type with tensor that will be concatenated (string)
    Expectation: Error is raised as expected
    """
    def gen():
        yield (np.array([3, 4], dtype=np.float),)

    prepend_tensor = np.array(["ss", "ad"], dtype='S')
    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate(0, prepend_tensor)

    data = data.map(operations=concatenate_op, input_columns=["col"])
    with pytest.raises(RuntimeError) as error_info:
        for _ in data:
            pass
    assert "input datatype does not match" in str(error_info.value)


def test_concatenate_op_type_mismatch2():
    """
    Feature: Concatenate op
    Description: Test Concatenate op where the data type of the original array dataset (string) has a mismatch
        data type with tensor that will be concatenated (float)
    Expectation: Error is raised as expected
    """
    def gen():
        yield (np.array(["ss", "ad"], dtype='S'),)

    prepend_tensor = np.array([3, 5], dtype=np.float)
    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate(0, prepend_tensor)

    data = data.map(operations=concatenate_op, input_columns=["col"])
    with pytest.raises(RuntimeError) as error_info:
        for _ in data:
            pass
    assert "input datatype does not match" in str(error_info.value)


def test_concatenate_op_incorrect_dim():
    """
    Feature: Concatenate op
    Description: Test Concatenate op using original dataset with incorrect dimension
    Expectation: Error is raised as expected
    """
    def gen():
        yield (np.array([["ss", "ad"], ["ss", "ad"]], dtype='S'),)

    prepend_tensor = np.array(["ss", "ss"], dtype='S')
    concatenate_op = data_trans.Concatenate(0, prepend_tensor)
    data = ds.GeneratorDataset(gen, column_names=["col"])

    data = data.map(operations=concatenate_op, input_columns=["col"])
    with pytest.raises(RuntimeError) as error_info:
        for _ in data:
            pass
    assert "only 1D input supported" in str(error_info.value)


def test_concatenate_op_wrong_axis():
    """
    Feature: Concatenate op
    Description: Test Concatenate op using wrong axis argument
    Expectation: Error is raised as expected
    """
    with pytest.raises(ValueError) as error_info:
        data_trans.Concatenate(2)
    assert "only 1D concatenation supported." in str(error_info.value)


def test_concatenate_op_negative_axis():
    """
    Feature: Concatenate op
    Description: Test Concatenate op using negative axis argument
    Expectation: Output is equal to the expected output
    """
    def gen():
        yield (np.array([5., 6., 7., 8.], dtype=np.float),)

    prepend_tensor = np.array([1.4, 2., 3., 4., 4.5], dtype=np.float)
    append_tensor = np.array([9., 10.3, 11., 12.], dtype=np.float)
    data = ds.GeneratorDataset(gen, column_names=["col"])
    concatenate_op = data_trans.Concatenate(-1, prepend_tensor, append_tensor)
    data = data.map(operations=concatenate_op, input_columns=["col"])
    expected = np.array([1.4, 2., 3., 4., 4.5, 5., 6., 7., 8., 9., 10.3,
                         11., 12.])
    for data_row in data.create_tuple_iterator(num_epochs=1, output_numpy=True):
        np.testing.assert_array_equal(data_row[0], expected)


def test_concatenate_op_incorrect_input_dim():
    """
    Feature: Concatenate op
    Description: Test Concatenate op using array that we would like to concatenate with incorrect dimensions
    Expectation: Error is raised as expected
    """
    prepend_tensor = np.array([["ss", "ad"], ["ss", "ad"]], dtype='S')

    with pytest.raises(ValueError) as error_info:
        data_trans.Concatenate(0, prepend_tensor)
    assert "can only prepend 1D arrays." in str(error_info.value)


if __name__ == "__main__":
    test_concatenate_op_all()
    test_concatenate_op_none()
    test_concatenate_op_string()
    test_concatenate_op_multi_input_string()
    test_concatenate_op_multi_input_numeric()
    test_concatenate_op_type_mismatch()
    test_concatenate_op_type_mismatch2()
    test_concatenate_op_incorrect_dim()
    test_concatenate_op_negative_axis()
    test_concatenate_op_wrong_axis()
    test_concatenate_op_incorrect_input_dim()
