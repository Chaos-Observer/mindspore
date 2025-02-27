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

"""Defines spectral operators with functional form."""

from mindspore.ops import operations as P
from mindspore.common import dtype as mstype
from .._primitive_cache import _get_cache_prim


def blackman_window(window_length, periodic=True, dtype=mstype.float32):
    r"""
    Blackman window function.

    The input `window_length` is a tensor that datatype must be a integer, which
    controlling the returned window size. In particular, If `window_length` is equal to `1`,
    the returned window contains a single value `1`.
    Attr `periodic` determines whether the returned window trims off the last duplicate value
    from the symmetric window and is ready to be used as a periodic window with functions.
    Therefore, if attr `periodic` is true, the :math:`N` in formula is in fact :math:`window\_length + 1`.

    .. math::

        w[n] = 0.42 - 0.5 cos(\frac{2\pi n}{N - 1}) + 0.08 cos(\frac{4\pi n}{N - 1})

        \text{where : N is the full window size.}

    Args:
        window_length (Tensor): the size of returned window, with data type int32, int64.
            The input data should be an integer with a value of [0, 1000000].
        periodic (bool): If True, returns a window to be used as periodic function.
            If False, return a symmetric window. Default: True.
        dtype (mindspore.dtype): the desired data type of returned tensor. Only float16, float32 and float64 is allowed.
            Default: mindspore.float32.

    Returns:
        A 1-D tensor of size `window_length` containing the window. Its datatype is set by the attr `dtype`.

    Raises:
        TypeError: If `window_length` is not a Tensor.
        TypeError: If `periodic` is not a bool.
        TypeError: If `dtype` is not one of: float16, float32, float64.
        TypeError: If the type of `window_length` is not one of: int32, int64.
        ValueError: If the value range of `window_length` is not [0,1000000].
        ValueError: If the dimension of `window_length` is not 0.

    Supported Platforms:
        ``Ascend`` ``CPU`` ``GPU``

    Examples:
        >>> window_length = Tensor(10, mindspore.int32)
        >>> output = ops.blackman_window(window_length, periodic=True, dtype=mindspore.float32)
        >>> print(output)
        [-2.9802322e-08  4.0212840e-02  2.0077014e-01  5.0978714e-01
          8.4922993e-01  1.0000000e+00  8.4922981e-01  5.0978690e-01
          2.0077008e-01  4.0212870e-02]
    """

    blackman_window_op = _get_cache_prim(P.BlackmanWindow)(periodic, dtype)
    return blackman_window_op(window_length)


__all__ = [
    'blackman_window'
]

__all__.sort()
