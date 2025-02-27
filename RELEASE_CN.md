# MindSpore Release Notes

[View English](./RELEASE.md)

## MindSpore 1.8.1 Release Notes

### API变更

#### 算子

- [STABLE] ops.ApplyAdagradDA 新增GPU、CPU支持。
- [STABLE] ops.ApplyAdagradV2 新增CPU支持。
- [STABLE] ops.ApplyCenteredRmsProp 新增Ascend动态shape支持。
- [STABLE] ops.ApplyFtrl 新增CPU支持。
- [STABLE] ops.ApplyGradientDescent 新增CPU支持。
- [STABLE] ops.ApplyPowerSign 新增CPU支持。
- [STABLE] ops.ApplyProximalAdagrad 新增GPU、CPU支持。
- [STABLE] ops.ApplyRmsProp 新增Ascend动态shape支持。
- [STABLE] ops.max 新增functional接口。
- [STABLE] ops.atan2 新增functional接口。
- [STABLE] ops.cummax 新增GPU支持。
- [STABLE] ops.cummin 新增GPU、CPU支持。
- [STABLE] ops.diag 新增GPU支持。
- [STABLE] ops.expand_dims 新增functional接口。
- [STABLE] ops.gather_elements 新增functional接口。
- [STABLE] ops.grid_sample 新增GPU支持。
- [STABLE] ops.hardswish 新增Ascend支持。
- [BETA] ops.index_fill 新增GPU支持。
- [BETA] ops.inplace_update 新增CPU支持。
- [BETA] nn.InstanceNorm1d 新增GPU支持。
- [BETA] nn.InstanceNorm2d 新增GPU支持。
- [BETA] nn.InstanceNorm3d 新增GPU支持。
- [STABLE] ops.log1p 新增functional接口。
- [STABLE] ops.masked_fill 新增GPU、CPU支持。
- [BETA] ops.matrix_diag_part 新增GPU支持。
- [BETA] ops.matrix_diag 新增GPU支持。
- [BETA] ops.matrix_set_diag 新增GPU支持。
- [STABLE] ops.max_pool3d 新增GPU支持。
- [STABLE] ops.nll_loss 新增functional接口。
- [STABLE] ops.one_hot 新增functional接口。
- [STABLE] ops.pad 新增functional接口。
- [STABLE] ops.random_gamma 新增CPU支持。
- [STABLE] ops.amax 新增functional接口。
- [STABLE] ops.mean 新增functional接口。
- [STABLE] ops.amin 新增functional接口。
- [STABLE] ops.prod 新增functional接口。
- [STABLE] ops.renorm 新增Ascend、GPU、CPU支持。
- [BETA] ops.tensor_scatter_elements 新增Ascend、GPU、CPU支持。
- [STABLE] ops.scatter_max 新增GPU支持。
- [STABLE] ops.scatter_min 新增GPU支持。
- [STABLE] ops.scatter_nd 新增functional接口。
- [STABLE] ops.scatter_nd_max 新增GPU支持。
- [STABLE] ops.scatter_update 新增functional接口。
- [STABLE] ops.binary_cross_entropy_with_logits 新增CPU支持。
- [STABLE] ops.smooth_l1_loss 新增functional接口。
- [STABLE] ops.space_to_batch_nd 新增CPU支持。
- [STABLE] ops.SparseApplyAdagrad 新增GPU、CPU支持。
- [STABLE] ops.sparse_segment_mean 新增GPU、CPU支持。
- [STABLE] ops.squeeze 新增functional接口。
- [STABLE] ops.standard_laplace 新增CPU支持。
- [BETA] nn.ReflectionPad1d 新增Ascend、GPU、CPU支持。
- [BETA] nn.ReflectionPad2d 新增Ascend、GPU、CPU支持。
- [STABLE] nn.SiLU 新增Ascend、GPU、CPU支持。
- [STABLE] ops.transpose 新增functional接口。
- [STABLE] ops.uniform_candidate_sampler 新增CPU支持。
- [STABLE] ops.uniform 新增functional接口。
- [STABLE] ops.unique_with_pad 新增GPU支持。
- [STABLE] ops.unstack 新增functional接口。
- [BETA] ops.interpolate 新增GPU、CPU支持。
- [STABLE] ops.xdivy 新增CPU支持。
- [STABLE] ops.xlogy 新增CPU支持。

## MindSpore 1.8.0 Release Notes

### 主要特性和增强

#### FrontEnd

- [BETA]  提供`mindspore.train.Model.fit` API，增加两种callback方法 `mindspore.train.callback.EarlyStopping` 和 `mindspore.train.callback.ReduceLROnPlateau`。
- [BETA] 自定义算子支持Julia算子。
- [BETA] 自定义算子支持Hybrid DSL算子。
- [STABLE] export()接口支持自定义加密算法导出模型，load()接口支持自定义解密算法导入模型。
- [BETA]   [动静统一] [易用性] 图编译支持常量类型设置可变(1.8版本支持tuple/list/dict)。
- [BETA]   [动静统一] 常量场景下控制流内支持JIT Fallback功能。
- [STABLE] [动静统一] 支持图模式常量场景下Python raise语句。
- [STABLE] [动静统一] 支持图模式常量场景下Python assert语句。
- [STABLE] [动静统一] 支持图模式常量场景下Python print语句。
- [STABLE] [动静统一] 支持图模式str.format()方法。
- [STABLE] [动静统一] 支持图模式用slice方法对list赋值。
- [STABLE] [动静统一] 图模式支持创建和调用自定义类的实例。
- [STABLE] [动静统一] 支持从Cell数组/自定义类数组中获取类的属性。
- [STABLE] [动静统一] 图模式下isinstance支持场景扩展。
- [STABLE] 自定义算子修饰符'ms_hybrid'重名为'ms_kernel'。
- [BETA] 自定义算子Hybrid DSL支持CPU后端。
- [BETA] 自定义算子昇腾后端新增自定义调度原语语法支持。

#### PyNative

- [STABLE] 实现AdamWeightDecay算子，替代原有小算子组合方式。
- [STABLE] 动态图下使用动静结合的方式执行优化器。
- [STABLE] 优化PyNative反向图和ms_function的执行性能。

#### Auto Parallel

- [STABLE] 对接AllToAll单算子模式。在KernelByKernel的执行模式下，支持AllToAll算子调用。
- [STABLE] 整图下沉支持MPI启动。整图下沉的模式下，支持使用MPI的方式启动。
- [STABLE] 模型权重的Seed提供并行接口配置。在用户不通过mindspore.set_seed设置随机数种子时，每个参数初始化的随机数种子为当前分片索引决定。当配置随机数种子之后，相同shape以及相同切分策略的权重，其初始化的结果一致。
- [STABLE] HCCL屏蔽内部全连接/非全连接。允许一次训练过程中同时有全连接AllToAllv和分级AllToAllv。
- [BETA] CPU优化器融合。通过优化器跨参数融合，将多个优化器算子按数据类型融合成，带来性能提升。目前已在CPU AdamWeightDecay优化器上做过验证。用户可以通过网络cell类中的flatten_weights方法启用该功能。

#### Executor

- [STABLE] 开放南向芯片对接接口。
- [STABLE] 使用多Actor融合执行提升运行时的执行性能。
- [STABLE] NopOp算子(eg. Reshape)执行消除。
- [STABLE] Embedding Cache架构切换统一分布式运行时。
- [STABLE] Parameter Server训练切换统一分布式运行时。
- [STABLE] 支持CPU Parameter Server模式训练。

#### DataSet

- [STABLE] 对于数据集对象使用map操作时，同时num_parallel_workers>1并且python_multiprocessing=True时，进行了多进程的机制优化，使得数据通道与子进程一一映射，避免了过多的文件句柄占用，同时close_pool这个接口也被删除。
- [STABLE] 新增一批Vision、Text和Audio类数据增强操作。
- [STABLE] 修复数据集类的flat_map方法未将结果展平的错误。
- [STABLE] 统一数据集增强API的导入路径，提供更简单的使用方法，请参阅[最新的API用法](https://www.mindspore.cn/docs/zh-CN/master/api_python/mindspore.dataset.vision.html)。

### API变更

#### 算子

- [STABLE] ops.adaptive_avg_pool2d 新增GPU支持。
- [BETA] ops.adaptive_max_pool2d  新增Ascend、GPU、CPU支持。
- [BETA] ops.approximate_equal 新增CPU支持。
- [STABLE] ops.argmin 新增CPU支持。
- [BETA] ops.assign_sub 新增CPU支持。
- [STABLE] ops.bernoulli 新增GPU支持。
- [BETA] ops.bessel_i0 新增CPU支持。
- [BETA] ops.bessel_i0e 新增CPU支持。
- [BETA] ops.bessel_i1 新增CPU支持。
- [BETA] ops.bessel_i1e 新增CPU支持。
- [STABLE] ops.bessel_j0 新增CPU支持。
- [STABLE] ops.bessel_j1 新增CPU支持。
- [STABLE] ops.bessel_k0 新增CPU支持。
- [STABLE] ops.bessel_k0e 新增CPU支持。
- [BETA] ops.bessel_k1 新增CPU支持。
- [BETA] ops.bessel_k1e 新增CPU支持。
- [STABLE] ops.bessel_y0 新增CPU支持。
- [STABLE] ops.bessel_y1 新增CPU支持。
- [STABLE] ops.bitwise_and 新增CPU支持。
- [STABLE] ops.bitwise_or 新增CPU支持。
- [STABLE] ops.bitwise_xor 新增CPU支持。
- [STABLE] ops.broadcast_to 新增functional接口。
- [BETA] ops.ceil 新增GPU、CPU支持。
- [BETA] ops.col2im 新增GPU支持。
- [BETA] ops.concat 新增functional接口。
- [STABLE] ops.cosh 新增GPU支持。
- [STABLE] ops.ctc_greedy_decoder 新增Ascend、CPU支持。
- [BETA] ops.DataFormatDimMap 新增GPU、CPU支持。
- [BETA] ops.dropout2d 新增GPU、CPU支持。
- [BETA] ops.dropout3d 新增CPU支持。
- [BETA] ops.erf 新增CPU支持。
- [BETA] ops.erfc 新增CPU支持。
- [STABLE] ops.expand_dims 新增functional接口。
- [STABLE] ops.fast_gelu 新增GPU、CPU支持。
- [STABLE] ops.flatten Ascend动态shape支持。
- [BETA] ops.ger 新增GPU、CPU支持。
- [STABLE] ops.gumbel_softmax 新增Ascend、GPU、CPU支持。
- [BETA] ops.hardshrink 新增GPU、CPU支持。
- [BETA] ops.index_add 新增CPU支持。
- [BETA] ops.inplace_add 新增CPU支持。
- [BETA] ops.inplace_sub 新增CPU支持。
- [STABLE] ops.intopk 新增CPU支持。
- [STABLE] ops.inv 新增GPU、CPU支持。
- [STABLE] ops.invert 新增GPU、CPU支持。
- [BETA] ops.isclose 新增CPU支持。
- [STABLE] ops.lerp 新增CPU支持。
- [BETA] ops.linspace 新增CPU支持。
- [BETA] ops.log_softmax 新增functional接口。
- [BETA] ops.norm 新增Ascend、GPU、CPU支持。
- [BETA] ops.lrn 新增CPU支持。
- [BETA] ops.masked_select 新增GPU支持。
- [BETA] ops.matrix_band_part 新增GPU、CPU支持。
- [BETA] ops.matrix_solve 新增GPU、CPU支持。
- [BETA] ops.meshgrid 新增CPU支持。
- [STABLE] ops.mish 新增CPU支持。
- [BETA] ops.nonzero  新增GPU支持。
- [STABLE] ops.padding 新增GPU、CPU支持。
- [BETA] ops.pow 新增Ascend动态shape支持。
- [BETA] ops.range 新增functional接口。
- [BETA] ops.round 新增Ascend动态shape支持。
- [STABLE] ops.scatter_add 新增Ascend动态shape支持。
- [STABLE] ops.scatter_div 新增Ascend动态shape支持。
- [BETA] ops.scatter_max 新增GPU支持。
- [BETA] ops.scatter_min 新增GPU支持。
- [BETA] ops.scatter_nd_add 新增CPU支持。
- [STABLE] ops.scatter_nd_div 新增GPU、CPU支持。
- [STABLE] ops.scatter_nd_min 新增GPU、CPU支持。
- [STABLE] ops.scatter_nd_mul 新增GPU、CPU支持。
- [BETA] ops.scatter_nd_sub 新增CPU支持。
- [STABLE] ops.scatter_update 新增Ascend动态shape支持。
- [BETA] ops.select 新增Ascend动态shape支持。
- [BETA] ops.selu 新增GPU、CPU支持。
- [BETA] ops.soft_shrink 新增GPU、CPU支持。
- [BETA] ops.softsign 新增CPU支持。
- [STABLE] ops.tan 新增GPU支持。
- [BETA] ops.tensor_scatter_add 新增Ascend、CPU支持。
- [STABLE] ops.tensor_scatter_div 新增GPU、CPU支持。
- [STABLE] ops.tensor_scatter_mul 新增GPU、CPU支持。
- [BETA] ops.tensor_scatter_sub 新增Ascend、CPU支持。
- [STABLE] nn.AdaptiveAvgPool1d 新增Ascend、GPU、CPU支持。
- [STABLE] nn.AdaptiveMaxPool1d 新增Ascend、GPU、CPU支持。
- [BETA] nn.BiDense 新增Ascend、GPU、CPU支持。
- [STABLE] nn.ConstantPad1d 新增Ascend、GPU、CPU支持。
- [STABLE] nn.ConstantPad2d 新增Ascend、GPU、CPU支持。
- [STABLE] nn.ConstantPad3d 新增Ascend、GPU、CPU支持。
- [STABLE] nn.Hardtanh 新增Ascend、GPU、CPU支持。
- [STABLE] nn.HuberLoss 新增Ascend、GPU、CPU支持。
- [STABLE] nn.RReLU 新增Ascend、GPU、CPU支持。
- [STABLE] nn.Tanhshrink 新增Ascend、GPU、CPU支持。
- [STABLE] nn.Threshold 新增Ascend、GPU、CPU支持。
- [STABLE] nn.ZeroPad2d 新增Ascend、GPU、CPU支持。
- [BETA] ops.unique_consecutive 新增GPU支持。
- [STABLE] ops.unsorted_segment_max 新增CPU支持。
- [STABLE] ops.unsorted_segment_min 新增CPU支持。
- [STABLE] ops.unsorted_segment_prod 新增GPU支持。

#### 非兼容性变更

##### Python API

- 不再支持DVPP模拟算法，删除 `mindspore.dataset.vision.c_transforms.SoftDvppDecodeRandomCropResizeJpeg` 和 `mindspore.dataset.vision.c_transforms.SoftDvppDecodeResizeJpeg` 接口。
- LossMonitor中增加`on_train_epoch_end` 方法，实现在 `mindspore.train.Model.fit` 中使用时，打印epoch级别的metric信息。
- TimeMonitor打印内容变更，打印内容加入"train"或"eval"用于区分训练和推理阶段。
- load_checkpoint 接口的`filter_prefix`：不再支持空字符串("")，匹配规则由强匹配修改为模糊匹配。

#### import优化

mindspore.context、mindspore.parallel、mindspore.profiler、mindspore.train模块的接口可直接在mindspore模块使用。原有用法仍可以继续支持。

例如：

- `mindspore.context.set_context`可简化为`mindspore.set_context`。
- `mindspore.parallel.set_algo_parameters`可简化为`mindspore.set_algo_parameters`。
- `mindspore.profiler.Profiler`可简化为`mindspore.Profiler`。
- `mindspore.train.callback.Callback`可简化为`mindspore.train.Callback`。

API页面统一汇总至：<https://www.mindspore.cn/docs/zh-CN/master/api_python/mindspore.html>。

## MindSpore Lite

### 主要特性和增强

#### API

- [STABLE] 新增模型转换的C++和Python API.
- [STABLE] 新增模型推理的Python API.

#### 后量化

- [STABLE] 后量化支持PerLayer量化，同时内置CLE算法优化精度。

### 贡献者

感谢以下人员做出的贡献：

AGroupofProbiotocs, anzhengqi, askmiao, baihuawei, baiyangfan, bai-yangfan, bingyaweng, BowenK, buxue, caifubi, CaoJian, caojian05, caozhou, Cathy, changzherui, chenbo116, chenfei, chengxianbin, chenhaozhe, chenjianping, chenzomi, chenzupeng, chujinjin, cj, cjh9368, Corleone, damon0626, danish, Danish, davidmc, dayschan, doitH, dong-li001, fary86, fuzhiye, Gaoxiong, GAO_HYP_XYJ, gengdongjie, Gogery, gongdaguo, gray0v0, gukecai, guoqi, gzhcv, hangq, hanhuifeng2020, Harshvardhan, He, heleiwang, hesham, hexia, Hoai, HuangBingjian, huangdongrun, huanghui, huangxinjing, huqi, huzhifeng, hwjiaorui, Jiabin Liu, jianghui58, Jiaqi, jin-xiulang, jinyaohui, jjfeing, John, jonyguo, JulyAi, jzg, kai00, kingfo, kingxian, kpy, kswang, liuyongqi, laiyongqiang, leonwanghui, liangchenghui, liangzelang, lichen_101010, lichenever, lihongkang, lilei, limingqi107, ling, linqingke, Lin Xh, liubuyu, liuwenhao4, liuxiao78, liuxiao93, liuyang_655, liuzhongkai, Lixia, lixian, liyanliu, liyong, lizhenyu, luopengting, lvchangquan, lvliang, lz, maning202007, Margaret_wangrui, mengyuanli, Ming_blue, ms_yan, ougongchang, panfengfeng, panyifeng, Payne, Peilin, peixu_ren, Pengyongrong, qianlong, qianjiahong, r1chardf1d0, riemann_penn, rmdyh, Sheng, shenwei41, simson, Simson, Su, sunsuodong, tao_yunhao, tinazhang, VectorSL, , Wan, wandongdong, wangdongxu, wangmin,  wangyue01, wangzhe, wanyiming, Wei, wenchunjiang, wilfChen, WilliamLian, wsc, wudenggang, wukesong, wuweikang, wuxuejian, Xiao Tianci, Xiaoda, xiefangqi, xinyunfan, xuanyue, xuyongfei, yanghaitao, yanghaitao1, yanghaoran, YangLuo, yangruoqi713, yankai, yanzhenxiang2020, yao_yf, yepei6, yeyunpeng, Yi, yoni, yoonlee666, yuchaojie, yujianfeng, yuximiao, zengzitao, Zhang,  zhanghuiyao, zhanghui_china, zhangxinfeng3, zhangyihui, zhangz0911gm, zhanke, zhanyuan, zhaodezan, zhaojichen, zhaoting, zhaozhenlong, zhengjun10, zhiqwang, zhoufeng, zhousiyi, zhouyaqiang, zhouyifengCode, Zichun, Ziyan, zjun, ZPaC, wangfengwfwf, zymaa, gerayking, shu-kun-zhang.

欢迎以任何形式对项目提供贡献！

## MindSpore 1.7.0 Release Notes

### 主要特性和增强

#### OS

- [STABLE] 支持Python 3.8版本（Linux/Windows/Mac）。
- [STABLE] 简化安装，提供详细安装指南和自动化安装脚本。
- [STABLE] Windows版本支持算子多线程。
- [STABLE] GCC兼容7.3到9.x版本。

#### FrontEnd

- [STABLE] 优化器支持动态权重衰减，即训练期间权重衰减值随着step的增加而变化。
- [STABLE] 增加四种创建Tensor的方法，分别是`mindspore.numpy.rand()`、`mindspore.numpy.randn()`、`mindspore.numpy.randint()`和`mindspore.ops.arange ()`。
- [STABLE] 增加一种callback方法 `mindspore.train.callback.History`。
- [BETA] 自定义算子支持Julia算子。
- [STABLE] 通过 `mindspore.ms_class` 类装饰器，支持获取用户自定义类的属性和方法。
- [STABLE] 支持同时存在副作用算子和控制流语句的网络的训练。
- [STABLE] 支持更复杂的控制流语法，比如在while的循环体里使用for语句。
- [STABLE] 通过减少子图数量，提升包含复杂控制流语法的网络的性能。

#### PyNative

- [STABLE] 在PyNative模式下支持hook函数功能，包括前向hook接口register_forward_pre_hook、register_forward_hook和反向hook接口register_backward_hook。
- [STABLE] 优化PyNative模式执行性能，并行执行前端Python与后端C++。

#### Auto Parallel

- [STABLE] 在MoE场景中支持TopK的路由、数据并行和优化器切分。
- [STABLE] 支持AllGather/ReduceScatter通信算子融合，在DATA_PARALLEL模式支持AllReduce按数据量大小编译。
- [STABLE] 在并行模式下支持ops.clip_by_global_norm。
- [STABLE] 在并行模式下支持AdaSum优化器。
- [STABLE] 支持自动优化器切分。
- [STABLE] 支持AlltoAll可配置开启，支持自动插入VirtualDatasetCell。
- [STABLE] 在流水线并行训练中，支持自动推断可训练的参数。
- [STABLE] 支持集群的设备数目不为2的幂次方。
- [STABLE] 在自动并行模式中支持策略传播。
- [STABLE] 在统一运行时中支持异构训练。
- [STABLE] 支持CPU的Adafactor算子。
- [STABLE] 支持Conv2d/Conv2D的H/W轴切分和Transpose算子。支持ResizeBilinear、ROIAlign、CropAndResize、BoundingBoxEncode、IOU和RandomChoiceWithMask等分布式算子。

#### Executor

- [BETA] [数据并行训练容灾](https://www.mindspore.cn/tutorials/experts/zh-CN/master/parallel/train_gpu.html#%E5%AE%B9%E7%81%BE%E6%81%A2%E5%A4%8D) 支持多卡数据并行训练容灾恢复。
- [BETA] 支持在CPU下的线程数搜索，获取最优线程数来执行。整个搜索过程需要耗时50个steps，整体的性能会在50个steps后达到稳定的状态。在测试性能的时候，需要以50个steps之后的数据作为标准。

#### DataSet

- [STABLE] 增加了数据处理API的差异文档，比较TensorFlow.data与MindSpore.dataset部分算子的差异，详见 [对比文档](https://www.mindspore.cn/docs/zh-CN/master/note/api_mapping/tensorflow_api_mapping.html#tf-data)。
- [STABLE] Python多进程逻辑优化，保证不同异常场景的正常退出。
- [STABLE] 支持[自动数据加速](https://www.mindspore.cn/tutorials/experts/zh-CN/master/dataset/dataset_autotune.html)，可以自适应调节数据处理管道的执行速度。
- [BETA] [数据处理异构加速](https://www.mindspore.cn/tutorials/experts/zh-CN/master/dataset/dataset_offload.html) 支持了新的数据增强操作: RandomColorAdjust、RandomSharpness和TypeCast。
- GeneratorDataset加载自定义数据集时，当`__getitem__/__next__`方法返回单个NumPy对象，对应会输出单个数据列。
- 用户在数据预处理中使用过多的进程数/线程数情况下，会出现错误RuntimeError: can't start new thread，可以通过 `ulimit -u 10240` 增加当前用户可用的线程/进程数解决。

### API变更

#### 非兼容性变更

##### Python API

- 修改register_backward_hook功能对应hook的梯度返回值类型，将梯度返回值统一改成tuple类型。([!31876](https://gitee.com/mindspore/mindspore/pulls/31876))
- 弃用的import用法： `import mindspore.dataset.engine.datasets as ds` ，因其import目录过深且过度依赖Python目录结构。推荐使用 `import mindspore.dataset as ds` ，更多参考详见 [API文档](https://www.mindspore.cn/docs/zh-CN/master/api_python/mindspore.dataset.html)。
- 新增`mindspore.ms_class` 接口，作为用户自定义类的类装饰器，使得MindSpore能够识别用户自定义类，并且获取这些类的属性和方法。([!30855](https://gitee.com/mindspore/mindspore/pulls/30855))
- `mindspore.SparseTensor`接口废弃使用，对应新接口为`mindspore.COOTensor`。 ([!28505](https://gitee.com/mindspore/mindspore/pulls/28505))
- Tensor新增一个入参`internal`，作为框架内部使用。

## MindSpore Lite

### 主要特性和增强

#### 后量化

- [STABLE] 后量化支持动态量化算法。
- [BETA] 后量化模型支持在英伟达GPU上执行推理。

## 贡献者

感谢以下人员做出的贡献:

AGroupofProbiotocs, anzhengqi, askmiao, baihuawei, baiyangfan, bai-yangfan, bingyaweng, BowenK, buxue, caifubi, CaoJian, caojian05, caozhou, Cathy, changzherui, chenbo116, chenfei, chengxianbin, chenhaozhe, chenjianping, chenzomi, chenzupeng, chujinjin, cj, cjh9368, Corleone, damon0626, danish, Danish, davidmc, dayschan, doitH, dong-li001, fary86, fuzhiye, Gaoxiong, GAO_HYP_XYJ, gengdongjie, Gogery, gongdaguo, gray0v0, gukecai, guoqi, gzhcv, hangq, hanhuifeng2020, Harshvardhan, He, heleiwang, hesham, hexia, Hoai, HuangBingjian, huangdongrun, huanghui, huangxinjing, huqi, huzhifeng, hwjiaorui, Jiabin Liu, jianghui58, Jiaqi, jin-xiulang, jinyaohui, jjfeing, John, jonyguo, JulyAi, jzg, kai00, kingfo, kingxian, kpy, kswang, liuyongqi, laiyongqiang, leonwanghui, liangchenghui, liangzelang, lichen_101010, lichenever, lihongkang, lilei, limingqi107, ling, linqingke, Lin Xh, liubuyu, liuwenhao4, liuxiao78, liuxiao93, liuyang_655, liuzhongkai, Lixia, lixian, liyanliu, liyong, lizhenyu, luopengting, lvchangquan, lvliang, lz, maning202007, Margaret_wangrui, mengyuanli, Ming_blue, ms_yan, ougongchang, panfengfeng, panyifeng, Payne, Peilin, peixu_ren, Pengyongrong, qianlong, qianjiahong, r1chardf1d0, riemann_penn, rmdyh, Sheng, shenwei41, simson, Simson, Su, sunsuodong, tao_yunhao, tinazhang, VectorSL, , Wan, wandongdong, wangdongxu, wangmin,  wangyue01, wangzhe, wanyiming, Wei, wenchunjiang, wilfChen, WilliamLian, wsc, wudenggang, wukesong, wuweikang, wuxuejian, Xiao Tianci, Xiaoda, xiefangqi, xinyunfan, xuanyue, xuyongfei, yanghaitao, yanghaitao1, yanghaoran, YangLuo, yangruoqi713, yankai, yanzhenxiang2020, yao_yf, yepei6, yeyunpeng, Yi, yoni, yoonlee666, yuchaojie, yujianfeng, yuximiao, zengzitao, Zhang,  zhanghuiyao, zhanghui_china, zhangxinfeng3, zhangyihui, zhangz0911gm, zhanke, zhanyuan, zhaodezan, zhaojichen, zhaoting, zhaozhenlong, zhengjun10, zhiqwang, zhoufeng, zhousiyi, zhouyaqiang, zhouyifengCode, Zichun, Ziyan, zjun, ZPaC, wangfengwfwf, zymaa, gerayking.

欢迎以任何形式对项目提供贡献！
