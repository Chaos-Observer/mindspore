mindspore.dataset.IWSLT2017Dataset
==================================

.. py:class:: mindspore.dataset.IWSLT2017Dataset(dataset_dir, usage=None, language_pair=None, num_samples=None, shuffle=Shuffle.GLOBAL, num_shards=None, shard_id=None, num_parallel_workers=None, cache=None)

    读取和解析IWSLT2017数据集的源数据集。

    生成的数据集有两列 `[text, translation]` 。 `text` 列的数据类型是string。 `translation` 列的数据类型是string。

    参数：
        - **dataset_dir** (str) - 包含数据集文件的根目录路径。
        - **usage** (str, 可选) - 指定数据集的子集，可取值为'train'，'valid'，'test'或'all'。默认值：None，读取全部样本。
        - **language_pair** (sequence, 可选) - 包含源语和目标语的语言列表，支持的语言对有（'en'，'nl'）、（'en'，'de'）、（'en'，'it'）、（'en'，'ro'）、（'nl'，'en'，'de'）、（'nl'，'it'）、（'nl'，'ro'）、（'de'，'en'）、（'de'，'nl'）、（'de'，'it'，'it'，'en'）、（'it'，'nl'）、（'it'，'de'）、（'it'，'ro'）、（'ro'，'en'）、（'ro'，'nl'）、（'ro'，'de'）、（'ro'，'it'），默认值：（'de'，'en'）。
        - **num_samples** (int, 可选) - 指定从数据集中读取的样本数。
        - **shuffle** (Union[bool, Shuffle], 可选) - 每个epoch中数据混洗的模式，支持传入bool类型与枚举类型进行指定，默认值：mindspore.dataset.Shuffle.GLOBAL。
          如果 `shuffle` 为False，则不混洗，如果 `shuffle` 为True，等同于将 `shuffle` 设置为mindspore.dataset.Shuffle.GLOBAL。
          通过传入枚举变量设置数据混洗的模式：

          - **Shuffle.GLOBAL**：混洗文件和样本。
          - **Shuffle.FILES**：仅混洗文件。

        - **num_shards** (int, 可选) - 指定分布式训练时将数据集进行划分的分片数，默认值：None。指定此参数后， `num_samples` 表示每个分片的最大样本数。
        - **shard_id** (int, 可选) - 指定分布式训练时使用的分片ID号，默认值：None。只有当指定了 `num_shards` 时才能指定此参数。
        - **num_parallel_workers** (int, 可选) - 指定读取数据的工作线程数。默认值：None，使用mindspore.dataset.config中配置的线程数。
        - **cache** (DatasetCache, 可选) - 单节点数据缓存服务，用于加快数据集处理，详情请阅读 `单节点数据缓存 <https://www.mindspore.cn/tutorials/experts/zh-CN/master/dataset/cache.html>`_ 。默认值：None，不使用缓存。

    异常：
        - **RuntimeError** - `dataset_dir` 参数所指向的文件目录不存在或缺少数据集文件。
        - **ValueError** - `num_parallel_workers` 参数超过系统最大线程数。
        - **RuntimeError** - 指定了 `num_shards` 参数，但是未指定 `shard_id` 参数。
        - **RuntimeError** - 指定了 `shard_id` 参数，但是未指定 `num_shards` 参数。

    **关于IWSLT2016数据集：**

    IWSLT是一个专门讨论口译各个方面的重要年度科学会议。IWSLT评估活动中的MT任务被构成一个数据集，该数据集可通过wit3.fbk.eu公开获取。
    IWSLT2017数据集中有德语、英语、意大利语、荷兰语和罗马尼亚语，数据集包括其中任何两种语言的翻译。

    可以将原始IWSLT2017数据集文件解压缩到此目录结构中，并由MindSpore的API读取。解压后，还需要将要读取的数据集解压到指定文件夹中。例如，如果要读取de-en的数据集，则需要解压缩de/en目录下的tgz文件，数据集位于解压缩文件夹中。

    .. code-block::

        .
        └── iwslt2017_dataset_directory
             ├── subeval_files
             └── texts
                  ├── ar
                  │    └── en
                  │        └── ar-en
                  ├── cs
                  │    └── en
                  │        └── cs-en
                  ├── de
                  │    └── en
                  │        └── de-en
                  │            ├── IWSLT16.TED.dev2010.de-en.de.xml
                  │            ├── train.tags.de-en.de
                  │            ├── ...
                  ├── en
                  │    ├── ar
                  │    │   └── en-ar
                  │    ├── cs
                  │    │   └── en-cs
                  │    ├── de
                  │    │   └── en-de
                  │    └── fr
                  │        └── en-fr
                  └── fr
                       └── en
                           └── fr-en

    **引用：**

    .. code-block::

        @inproceedings{cettoloEtAl:EAMT2012,
        Address = {Trento, Italy},
        Author = {Mauro Cettolo and Christian Girardi and Marcello Federico},
        Booktitle = {Proceedings of the 16$^{th}$ Conference of the European Association for Machine Translation
                     (EAMT)},
        Date = {28-30},
        Month = {May},
        Pages = {261--268},
        Title = {WIT$^3$: Web Inventory of Transcribed and Translated Talks},
        Year = {2012}}


.. include:: mindspore.dataset.api_list_nlp.rst
