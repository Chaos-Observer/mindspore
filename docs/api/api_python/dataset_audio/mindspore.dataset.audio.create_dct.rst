mindspore.dataset.audio.create_dct
==================================

.. py:function:: mindspore.dataset.audio.create_dct(n_mfcc, n_mels, norm=NormMode.NONE)

    创建一个shape为(`n_mels`, `n_mfcc`)的DCT变换矩阵，并根据范数进行标准化。

    参数：
        - **n_mfcc** (int) - 要保留mfc系数的数量，该值必须大于0。
        - **n_mels** (int) - mel滤波器的数量，该值必须大于0。
        - **norm** (NormMode, 可选) - 标准化模式，可以是NormMode.NONE或NormMode.ORTHO。默认值：NormMode.NONE。

    返回：
        numpy.ndarray，DCT转换矩阵。
