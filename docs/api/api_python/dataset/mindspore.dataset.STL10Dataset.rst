mindspore.dataset.STL10Dataset
==============================

.. py:class:: mindspore.dataset.STL10Dataset(dataset_dir, usage=None, num_samples=None, num_parallel_workers=None, shuffle=None, sampler=None, num_shards=None, shard_id=None, cache=None)

    读取和解析STL10数据集的源文件构建数据集。

    生成的数据集有两列: `[image, label]` 。 `image` 列的数据类型是uint8。`label` 列的数据类型是uint32。

    参数：
        - **dataset_dir** (str) - 包含数据集文件的根目录路径。
        - **usage** (str, 可选) - 指定数据集的子集，可取值为'train'、'test'、'unlabeled'、'train+unlabeled'或'all'。
          取值为'train'时将会读取5,000个样本，取值为'test'时将会读取8,000个样本，取值为'unlabeled'时将会读取100,000个样本，取值为'train+unlabeled'时将会读取10,5000个样本，
          取值为'all'时将会读取全部类型的样本。默认值：None，读取全部样本图片。
        - **num_samples** (int, 可选) - 指定从数据集中读取的样本数，可以小于数据集总数。默认值：None，读取全部样本图片。
        - **num_parallel_workers** (int, 可选) - 指定读取数据的工作线程数。默认值：None，使用mindspore.dataset.config中配置的线程数。
        - **shuffle** (bool, 可选) - 是否混洗数据集。默认值：None，下表中会展示不同参数配置的预期行为。
        - **sampler** (Sampler, 可选) - 指定从数据集中选取样本的采样器，默认值：None，下表中会展示不同配置的预期行为。
        - **num_shards** (int, 可选) - 指定分布式训练时将数据集进行划分的分片数，默认值：None。指定此参数后， `num_samples` 表示每个分片的最大样本数。
        - **shard_id** (int, 可选) - 指定分布式训练时使用的分片ID号，默认值：None。只有当指定了 `num_shards` 时才能指定此参数。
        - **cache** (DatasetCache, 可选) - 单节点数据缓存服务，用于加快数据集处理，详情请阅读 `单节点数据缓存 <https://www.mindspore.cn/tutorials/experts/zh-CN/master/dataset/cache.html>`_ 。默认值：None，不使用缓存。

    异常：
        - **RuntimeError** - `dataset_dir` 路径下不包含数据文件。
        - **ValueError** - `num_parallel_workers` 参数超过系统最大线程数。
        - **RuntimeError** - 同时指定了 `sampler` 和 `shuffle` 参数。
        - **RuntimeError** - 同时指定了 `sampler` 和 `num_shards` 参数或同时指定了 `sampler` 和 `shard_id` 参数。
        - **RuntimeError** - 指定了 `num_shards` 参数，但是未指定 `shard_id` 参数。
        - **RuntimeError** - 指定了 `shard_id` 参数，但是未指定 `num_shards` 参数。
        - **ValueError** - `usage` 参数无效。
        - **ValueError** - `shard_id` 参数错误（小于0或者大于等于 `num_shards` ）。

    .. note:: 此数据集可以指定参数 `sampler` ，但参数 `sampler` 和参数 `shuffle` 的行为是互斥的。下表展示了几种合法的输入参数组合及预期的行为。

    .. list-table:: 配置 `sampler` 和 `shuffle` 的不同组合得到的预期排序结果
       :widths: 25 25 50
       :header-rows: 1

       * - 参数 `sampler`
         - 参数 `shuffle`
         - 预期数据顺序
       * - None
         - None
         - 随机排列
       * - None
         - True
         - 随机排列
       * - None
         - False
         - 顺序排列
       * - `sampler` 实例
         - None
         - 由 `sampler` 行为定义的顺序
       * - `sampler` 实例
         - True
         - 不允许
       * - `sampler` 实例
         - False
         - 不允许

    **关于STL10数据集：**

    STL10数据集由10类组成：飞机、鸟、汽车、猫、鹿、狗、马、猴子、船、卡车。
    数据集样本均为96x96的彩色图像。
    每个类别分别有500张训练图像和800张测试图像，以及100000张没有标签的图像。
    标签索引从0开始标记，没有标签的的图像以-1作为标记。

    以下是原始STL10数据集结构。
    可以将数据集文件解压缩到此目录结构中，并由MindSpore的API读取。

    .. code-block::

        .
        └── stl10_dataset_dir
             ├── train_X.bin
             ├── train_y.bin
             ├── test_X.bin
             ├── test_y.bin
             └── unlabeled_X.bin

    **引用：**

    .. code-block::

        @techreport{Coates10,
        author       = {Adam Coates},
        title        = {Learning multiple layers of features from tiny images},
        year         = {20010},
        howpublished = {https://cs.stanford.edu/~acoates/stl10/},
        description  = {The STL-10 dataset consists of 96x96 RGB images in 10 classes,
                        with 500 training images and 800 testing images per class.
                        There are 5000 training images and 8000 test images.
                        It also has 100000 unlabeled images for unsupervised learning.
                        These examples are extracted from a similar but broader distribution of images.
                        }
        }


.. include:: mindspore.dataset.api_list_vision.rst
