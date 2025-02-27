/**
 * Copyright 2019-2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_CCSRC_FRONTEND_PARALLEL_OPS_INFO_OPS_UTILS_H_
#define MINDSPORE_CCSRC_FRONTEND_PARALLEL_OPS_INFO_OPS_UTILS_H_

#include "mindspore/core/ops/core_ops.h"
#include "mindspore/core/utils/log_adapter.h"

namespace mindspore {
namespace parallel {
constexpr size_t MAX_RECURSIVE_DEPTH = 100000;
constexpr size_t PRELU_INPUTS_SIZE = 2;
constexpr size_t PRELU_OUTPUTS_SIZE = 1;
constexpr size_t PRELU_SECOND_INPUT_SIZE = 1;
constexpr int64_t MICRO_INTERLEAVED_SIZE = 2;
constexpr int64_t PRELU_CHANNEL_INDEX = 1;
constexpr int64_t PRELU_CHANNEL_STRATEGY = 1;
constexpr int64_t NO_SPLIT_MAP = -1;
constexpr int64_t NO_SPLIT_STRATEGY = 1;
constexpr int64_t SPLIT_FLAG = 1;
constexpr int64_t NO_SPLIT_FLAG = 0;
constexpr int64_t ADASUM_MIN_DIS = 8;
constexpr size_t MATMUL_ATTRS_SIZE = 2;
constexpr size_t SLICE_BEGIN_INDEX = 1;
constexpr size_t SLICE_SIZE_INDEX = 2;
constexpr size_t SLICE_INPUTS_SIZE = 3;
constexpr size_t STRIDED_SLICE_ATTRS_SIZE = 5;
constexpr size_t STRIDED_SLICE_INPUTS_SIZE = 4;
constexpr size_t STRIDED_SLICE_ARGS_SIZE = 3;
constexpr size_t STRIDED_SLICE_BEGIN_INDEX = 1;
constexpr size_t STRIDED_SLICE_END_INDEX = 2;
constexpr size_t STRIDED_SLICE_STRIDES_INDEX = 3;
constexpr size_t MATMUL_INPUTS_SIZE = 2;
constexpr size_t MATMUL_OUTPUTS_SIZE = 1;
constexpr size_t LAYERNORM_INPUTS_SIZE = 3;
constexpr size_t ONEHOT_INPUTS_SIZE = 4;
constexpr size_t UNSORTEDSEGMENTSUM_INPUTS_SIZE = 3;
constexpr size_t GATHERV2_INPUTS_SIZE = 3;
constexpr size_t DROPOUTDOMASK_INPUTS_SIZE = 3;
constexpr size_t CROP_AND_RESIZE_INPUTS_SIZE = 3;
constexpr size_t ROI_ALIGN_INPUTS_SIZE = 2;
constexpr size_t ACTIVATION_ATTR_SIZE = 1;
constexpr size_t SOFTMAX_ATTR_SIZE = 1;
constexpr size_t ACTIVATION_INPUTS_SIZE = 1;
constexpr size_t ACTIVATION_OUTPUTS_SIZE = 1;
constexpr size_t EXPANDDIMS_INPUT_SIZE = 2;
constexpr size_t CUMSUM_INPUT_SIZE = 2;
constexpr size_t CUM_OP_INPUT_SIZE = 2;
constexpr size_t DROPOUT_DO_MASK_CNODE_INPUT_SIZE = 4;
constexpr size_t DROPOUT_GEN_MASK_CNODE_INPUT_SIZE = 3;
constexpr size_t DROPOUT_GEN_MASK_INDEX = 2;
constexpr size_t DROPOUT_DO_MASK_KEEP_PROB_INDEX = 3;
constexpr size_t DSD_MATMUL_INPUTS_SIZE = 3;
constexpr size_t DSD_MATMUL_OUTPUTS_SIZE = 1;
constexpr size_t DSD_MATMUL_STRATEGY_W_SIZE = 7;
constexpr size_t DSD_MATMUL_STRATEGY_V_SIZE = 4;
constexpr size_t SoftmaxCrossEntropyWithLogitsAttrSize = 1;
constexpr size_t SoftmaxCrossEntropyWithLogitsInputsSize = 2;
constexpr size_t SoftmaxCrossEntropyWithLogitsOutputsSize = 2;
constexpr int64_t ONE_HOT_CNODE_INPUT_SIZE = 5;
constexpr size_t UNIQUE_INPUTS_SIZE = 1;
constexpr size_t UNIQUE_INPUT_SIZE = 1;
constexpr size_t UNIQUE_OUTPUTS_SIZE = 2;
constexpr size_t RESHAPE_INPUT_SIZE = 3;
constexpr size_t TRANSFER_PERMUTE_ARGS_SIZE = 5;
constexpr size_t TRANSFER_PERMUTE_SPLIT_COUNT_INDEX = 0;
constexpr size_t TRANSFER_PERMUTE_SPLIT_DIM_INDEX = 1;
constexpr size_t TRANSFER_PERMUTE_CONCAT_DIM_INDEX = 2;
constexpr size_t TRANSFER_PERMUTE_DEV_DIM_INDEX = 3;
constexpr size_t TRANSFER_PERMUTE_DEV_NUM_INDEX = 4;
constexpr size_t TRANSFER_CONCAT_ARGS_SIZE = 3;
constexpr size_t TRANSFER_CONCAT_TENSOR_DIM_INDEX = 0;
constexpr size_t TRANSFER_CONCAT_DEV_DIM_INDEX = 1;
constexpr size_t TRANSFER_CONCAT_SPLIT_COUNT_INDEX = 2;
constexpr size_t TRANSFER_SPLIT_ARGS_SIZE = 3;
constexpr size_t TUPLE_GETITEM_INDEX_POS = 2;
constexpr size_t MATMUL_DDS_INPUTS_SIZE = 4;
constexpr size_t MATMUL_DDS_OUTPUTS_SIZE = 2;
constexpr size_t MATMUL_DDS_STRATEGY_SIZE = 4;
constexpr size_t BATCH_NORM_INPUTS_SIZE = 5;
constexpr double EPS = 1e-6;
constexpr double INF = 1e20;
constexpr double COST_FACTOR = 2.0;

constexpr char AUTO_PARALLEL_RUN_ONCE_ONLY[] = "auto_parallel_run_once_only";
constexpr char SEMI_AUTO_PARALLEL_RUN_ONCE_ONLY[] = "semi_auto_parallel_run_once_only";
constexpr char CHECK_SET_STRATEGY_VALID_ONCE_ONLY[] = "check_set_strategy_valid_once_only";
constexpr char IN_STRATEGY[] = "in_strategy";
constexpr char OUT_STRATEGY[] = "out_strategy";
constexpr char STAGE_ATTR[] = "stage";
constexpr char GEN_STRATEGY[] = "gen_strategy";
constexpr char REDUCE_OP_SUM[] = "sum";
constexpr char STRATEGY_GEN_MODE[] = "strategy_gen_mode";
constexpr char REDUCE_OP_MAX[] = "max";
constexpr char REDUCE_OP_MIN[] = "min";
constexpr char REDUCE_OP_ANY[] = "sum";
constexpr char REDUCE_OP_ALL[] = "prod";
constexpr char REDUCE_OP_PROD[] = "prod";
constexpr char OP_PATH[] = "mindspore.ops.operations";
constexpr char INNER_OP_PATH[] = "mindspore.ops.operations._inner_ops";
constexpr char FUNCTIONAL_OP_PATH[] = "mindspore.ops.functional";
constexpr char GET_OP_FUNCTION_PATH[] = "mindspore.parallel._utils";
constexpr char GET_OP_FUNCTION[] = "_get_python_op";
constexpr char KEEP_DIMS[] = "keep_dims";
constexpr char OUTPUT_TYPE[] = "output_type";
constexpr char CROSS_BATCH[] = "cross_batch";
constexpr char STEP_PARALLEL_BEGIN[] = "step_parallel_begin";
constexpr char STEP_PARALLEL_END[] = "step_parallel_end";
constexpr char STEP_AUTO_PARALLEL_BEGIN[] = "step_auto_parallel_begin";
constexpr char REQUIRES_GRAD[] = "requires_grad";
constexpr char PARAM_NAME[] = "name";
constexpr char RESHAPEINFO[] = "ReshapeInfo";
constexpr char GETNEXTINFO[] = "GetNextInfo";
constexpr char VIRTUALDATASETINFO[] = "VirtualDatasetInfo";

constexpr char RELU_TYPE[] = "relu";
constexpr char RELU6_TYPE[] = "relu6";
constexpr char SIGMOID_TYPE[] = "sigmoid";
constexpr char OP[] = "op";
constexpr char IDENTITY_INFO[] = "identity_info";
constexpr char DIVISOR[] = "divisor";
constexpr char NONE[] = "None";
constexpr char DEPEND[] = "Depend";
constexpr char BATCH_PARALLEL[] = "BatchParallel";

constexpr char ACTIVATION_TYPE[] = "activation_type";
constexpr char TARGET[] = "primitive_target";
constexpr char CPU[] = "CPU";
constexpr char TRANSPOSE_A[] = "transpose_a";
constexpr char TRANSPOSE_B[] = "transpose_b";
constexpr char SHAPE[] = "shape";
constexpr char BEGIN_MASK[] = "begin_mask";
constexpr char END_MASK[] = "end_mask";
constexpr char ELLIPSIS_MASK[] = "ellipsis_mask";
constexpr char NEW_AXIS_MASK[] = "new_axis_mask";
constexpr char SHRINK_AXIS_MASK[] = "shrink_axis_mask";
constexpr char BEGIN[] = "begin";
constexpr char SIZE[] = "size";
constexpr char ALIGN_CORNERS[] = "align_corners";
constexpr char END[] = "end";
constexpr char STRIDES[] = "strides";
constexpr char GROUP[] = "group";
constexpr char FUSION[] = "fusion";
constexpr char COMM_REUSE[] = "comm_reuse";
constexpr char DO_MIRROR[] = "do_mirror";
constexpr char ADD_ACCU[] = "add_accu";
constexpr char RECOMPUTE[] = "recompute";
constexpr char RECOMPUTE_COMM_OP[] = "recompute_comm_op";
constexpr char NOT_RECOMPUTE[] = "not_recompute";
constexpr char NUM_SAMPLED[] = "num_sampled";
constexpr char NUM_TRUE[] = "num_true";
constexpr char SEED[] = "seed";
constexpr char SEED2[] = "seed2";
constexpr char RANGE_MAX[] = "range_max";
constexpr char REMOVE_ACCIDENTAL_HITS[] = "remove_accidental_hits";
constexpr char UNIQUE_STRING[] = "unique";
constexpr char AXIS[] = "axis";
constexpr char BS[] = "bs";
constexpr char HEADS[] = "heads";
constexpr char AXES[] = "axes";
constexpr char START[] = "start";
constexpr char LIMIT[] = "limit";
constexpr char DELTA[] = "delta";
constexpr char OUTPUT_NUM[] = "output_num";
constexpr char SPLIT_COUNT[] = "split_count";
constexpr char SPLIT_DIM[] = "split_dim";
constexpr char CONCAT_DIM[] = "concat_dim";
constexpr char FORWARD[] = "forward";
constexpr char BACKWARD[] = "backward";
constexpr char REDISTRIBUTION[] = "redistribution";
constexpr char SKIP_REDISTRIBUTION[] = "skip_redistribution";
constexpr char REPLACE[] = "replace";
constexpr char CONNSYMBOL[] = "/";
constexpr char INSTANCE_NAME[] = "instance_name";
constexpr char SPLIT_SENS[] = "split_sens";
constexpr char SEND_RANK_IDS[] = "send_rank_ids";
constexpr char RECV_RANK_IDS[] = "recv_rank_ids";
constexpr char RECV_SHAPES[] = "recv_shapes";
constexpr char SEND_SHAPES[] = "send_shapes";
constexpr char RECV_TYPE[] = "recv_type";
constexpr char SEND_LENS[] = "send_lens";
constexpr char RECV_LENS[] = "recv_lens";
constexpr char ORI_IMAGE_SIZE[] = "ori_image_size";
constexpr char SPLIT_SIZE[] = "split_size";
constexpr char SRC_START_W[] = "src_start_w";
constexpr char DST_START_W[] = "dst_start_w";
constexpr char SPLIT_TENSOR[] = "split_tensor";
constexpr char DEV_MAT[] = "dev_mat";
constexpr char TENSOR_MAP[] = "tensor_map";
constexpr char SEED0[] = "Seed0";
constexpr char SEED1[] = "Seed1";
constexpr char KEEP_PROB[] = "keep_prob";
constexpr char SRC[] = "src";
constexpr char CLONE_INFO[] = "clone_info";
constexpr char CLONED[] = "cloned";
constexpr char BE_CLONED[] = "be_cloned";
constexpr char CLONED_INDEX[] = "cloned_index";
constexpr char BE_CLONED_INDEX[] = "be_cloned_index";
constexpr char GROUP_RANKS[] = "group_ranks";
constexpr char IS_IN_FORWARD[] = "is_in_forward";
constexpr char DTYPE[] = "dtype";
constexpr char DEV_NUM[] = "dev_num";
constexpr char MEAN_FLAG[] = "mean_flag";
constexpr char GRAD_ACCUMULATION_STEP[] = "grad_accumulation_step";
constexpr char TYPES[] = "types";
constexpr char SHAPES[] = "shapes";
constexpr char ACCU_GRADS[] = "accu_grads";
constexpr char GETNEXT_NUM[] = "output_num";
constexpr char SHARED_NAME[] = "shared_name";
constexpr char MIRROR_OP[] = "mirror_op";
constexpr char FORWARD_OP[] = "forward_op";
constexpr char REDISTRIBUTION_OP[] = "redistribution_op";
constexpr char DARA_PARALLEL[] = "data_parallel";
constexpr char FORWARD_REDUCE_SCATTER[] = "forward_reduce_scatter";
constexpr char FIELD_SIZE[] = "field_size";
constexpr char Y[] = "Y";
constexpr char OPTIMIZER_SUB_STRING[] = "optimizer";
constexpr char DEVICE[] = "Device";
constexpr char PARALLEL_OPTIMIZER_ALLGATHER[] = "parallel_optimizer_allgather";
constexpr char PARALLEL_OPTIMIZER_ALLGATHER_NOT_COMPUTE[] = "parallel_optimizer_allgather_not_recompute";
constexpr char PARALLEL_OPTIMIZER_COMM_OP[] = "parallel_optimizer_comm_op";
constexpr char PARALLEL_GLOBALNORM[] = "PARALLEL_GLOBALNORM_IN_STAGES";
constexpr char PARALLEL_GLOBALNORM_BETWEEN[] = "PARALLEL_GLOBALNORM_BETWEEN_STAGES";
constexpr char PARALLEL_GLOBALNORM_DIV[] = "PARALLEL_GLOBALNORM_DIV";
constexpr char GRAD_SCALE[] = "grad_scale";
constexpr char CELLLIST_KEYWORD_PATTERN[] = "-CellList/(\\d+)-";

constexpr char OUT_CHANNEL[] = "out_channel";
constexpr char KERNEL_SIZE[] = "kernel_size";
constexpr char MODE[] = "mode";
constexpr char PAD_MODE[] = "pad_mode";
constexpr char PAD_LIST[] = "pad_list";
constexpr char PAD[] = "pad";
constexpr char DATA_FORMAT[] = "data_format";
constexpr char STRIDE[] = "stride";
constexpr char DILATION[] = "dilation";
constexpr char DILATIONS[] = "dilations";
constexpr char FORMAT[] = "format";
constexpr char NCHW[] = "NCHW";
constexpr char NCDHW[] = "NCDHW";
constexpr char H_DIMENSION[] = "h_dimension";
constexpr char W_DIMENSION[] = "w_dimension";
constexpr char IS_TRAINING[] = "is_training";
constexpr char EPSILON[] = "epsilon";
constexpr char MOMENTUM[] = "momentum";
constexpr char DEVICE_NUM[] = "device_num";
constexpr char GROUP_SIZE[] = "group_size";
constexpr char REPEAT_DIM_DIRECT[] = "repeat_dim_direct";
constexpr char RIGHT[] = "right";
constexpr char BOOL[] = "Bool";
constexpr char POOLED_HEIGHT[] = "pooled_height";
constexpr char POOLED_WIDTH[] = "pooled_width";
constexpr char SPATIAL_SCALE[] = "spatial_scale";
constexpr char SAMPLE_NUM[] = "sample_num";
constexpr char ROI_END_MODE[] = "roi_end_mode";
constexpr char REDUCTION[] = "reduction";
constexpr char MEAN[] = "mean";
constexpr char ATTR_NONE[] = "none";

// Operator
constexpr char VIRTUAL_DIV[] = "_VirtualDiv";
constexpr char GET_TENSOR_SLICE[] = "_GetTensorSlice";
constexpr char SPLIT[] = "Split";
constexpr char ALL_TO_ALL[] = "AlltoAll";
constexpr char NEIGHBOREXCHANGE[] = "NeighborExchange";
constexpr char NEIGHBOREXCHANGEV2[] = "NeighborExchangeV2";
constexpr char PARALLEL_RESIZE_BILINEAR[] = "ParallelResizeBilinear";
constexpr char PERMUTE_BY_AXIS[] = "PermuteByAxis";
constexpr char CONCAT_BY_AXIS[] = "ConcatByAxis";
constexpr char SPLIT_BY_AXIS[] = "SplitByAxis";
constexpr char ALL_REDUCE[] = "AllReduce";
constexpr char MIRROR_OPERATOR[] = "_MirrorOperator";
constexpr char MIRROR_MINI_STEP_OPERATOR[] = "_MirrorMiniStepOperator";
constexpr char LOCAL_STEP[] = "local_step";
constexpr char STRIDED_SLICE[] = "StridedSlice";
constexpr char ALL_GATHER[] = "AllGather";
constexpr char MINI_STEP_ALL_GATHER[] = "_MiniStepAllGather";
constexpr char MICRO_STEP_ALL_GATHER[] = "_MicroStepAllGather";
constexpr char REDUCE_SCATTER[] = "ReduceScatter";
constexpr char HOST_REDUCE_SCATTER[] = "_HostReduceScatter";
constexpr char EMBEDDING_LOOKUP[] = "EmbeddingLookup";
constexpr char CONCAT[] = "Concat";
constexpr char SELECT[] = "Select";
constexpr char SOFTMAX_CROSS_ENTROPY_WITH_LOGITS[] = "SoftmaxCrossEntropyWithLogits";
constexpr char SIGMOID_CROSS_ENTROPY_WITH_LOGITS[] = "SigmoidCrossEntropyWithLogits";
constexpr char MATMUL[] = "MatMul";
constexpr char GELU[] = "GeLU";
constexpr char FAST_GELU[] = "FastGeLU";
constexpr char TANH[] = "Tanh";
constexpr char RECEIVE[] = "Receive";
constexpr char SEND[] = "Send";
constexpr char SHAPE_OP[] = "Shape";
constexpr char SOFTMAX[] = "Softmax";
constexpr char LOG_SOFTMAX[] = "LogSoftmax";
constexpr char ACTIVATION[] = "Activation";
constexpr char PRELU[] = "PReLU";
constexpr char FLOORDIV[] = "FloorDiv";
constexpr char MAXPOOL[] = "MaxPool";
constexpr char AVGPOOL[] = "AvgPool";
constexpr char UNIFORMREAL[] = "UniformReal";
constexpr char MAXPOOLV2[] = "MaxPoolV2";
constexpr char L2_NORMALIZE[] = "L2Normalize";
constexpr char TRANSPOSE[] = "Transpose";
constexpr char RESHAPE[] = "Reshape";
constexpr char ADD[] = "Add";
constexpr char ADDN[] = "AddN";
constexpr char BIAS_ADD[] = "BiasAdd";
constexpr char SUB[] = "Sub";
constexpr char MUL[] = "Mul";
constexpr char DIV[] = "Div";
constexpr char REAL_DIV[] = "RealDiv";
constexpr char ASSIGN_SUB[] = "AssignSub";
constexpr char GREATER[] = "Greater";
constexpr char UNIFORM_CANDIDATE_SAMPLER[] = "UniformCandidateSampler";
constexpr char VIRTUAL_DATA_SET[] = "_VirtualDataset";
constexpr char VIRTUAL_OUTPUT[] = "_VirtualOutput";
constexpr char VIRTUAL_DATA_SET_INFO[] = "VirtualDatasetInfo";
constexpr char SPARSE_SOFTMAX_CROSS_ENTROPY_WITH_LOGITS[] = "SparseSoftmaxCrossEntropyWithLogits";
constexpr char RELU[] = "ReLU";
constexpr char REPEAT_ELEMENTS[] = "RepeatElements";
constexpr char TENSOR_DOT[] = "TensorDot";
constexpr char RANGE[] = "Range";
constexpr char ONEHOT[] = "OneHot";
constexpr char ONEHOT_INFO[] = "OneHotInfo";
constexpr char DROPOUT_DO_MASK[] = "DropoutDoMask";
constexpr char DROPOUT_GEN_MASK[] = "DropoutGenMask";
constexpr char TILE[] = "Tile";
constexpr char REDUCE_MAX[] = "ReduceMax";
constexpr char REDUCE_MIN[] = "ReduceMin";
constexpr char REDUCE_SUM[] = "ReduceSum";
constexpr char REDUCE_MEAN[] = "ReduceMean";
constexpr char REDUCE_PROD[] = "ReduceProd";
constexpr char REDUCE_ALL[] = "ReduceAll";
constexpr char REDUCE_ANY[] = "ReduceAny";
constexpr char ARGMAXWITHVALUE[] = "ArgMaxWithValue";
constexpr char ARGMINWITHVALUE[] = "ArgMinWithValue";
constexpr char ARGMAX[] = "Argmax";
constexpr char ARGMIN[] = "Argmin";
constexpr char ARGMINV2[] = "ArgminV2";
constexpr char CONV2D[] = "Conv2D";
constexpr char CONV3D[] = "Conv3D";
constexpr char CONV2D_BACK_PROP_INPUT[] = "Conv2DBackpropInput";
constexpr char CONV2D_TRANSPOSE[] = "Conv2DTranspose";
constexpr char CONV2D_INFO[] = "Conv2DInfo";
constexpr char CONV3D_INFO[] = "Conv3DInfo";
constexpr char CONV2D_BACK_PROP_INPUT_INFO[] = "Conv2DBackpropInputInfo";
constexpr char CONV2D_TRANSPOSE_INFO[] = "Conv2DTransposeInfo";
constexpr char FUSE_BATCH_NORM[] = "FusedBatchNorm";
constexpr char FUSE_BATCH_NORM_EX[] = "FusedBatchNormEx";
constexpr char BATCH_NORM[] = "BatchNorm";
constexpr char SYNC_BATCH_NORM[] = "SyncBatchNorm";
constexpr char LAYER_NORM[] = "LayerNorm";
constexpr char POOLING[] = "Pooling";
constexpr char CAST[] = "Cast";
constexpr char MAX_POOL_WITH_ARGMAX[] = "MaxPoolWithArgmax";
constexpr char SIMPLE_MEAN[] = "SimpleMean";
constexpr char FLATTEN[] = "Flatten";
constexpr char J[] = "J";
constexpr char SHARD[] = "Shard";
constexpr char Taylor[] = "Taylor";
constexpr char TMPIDENTITY_INFO_NAME[] = "identity_info";
constexpr char COS[] = "Cos";
constexpr char ACOS[] = "ACos";
constexpr char EXP[] = "Exp";
constexpr char LOG[] = "Log";
constexpr char SIGMOID[] = "Sigmoid";
constexpr char POW[] = "Pow";
constexpr char MAXIMUM[] = "Maximum";
constexpr char MINIMUM[] = "Minimum";
constexpr char EQUAL[] = "Equal";
constexpr char NOT_EQUAL[] = "NotEqual";
constexpr char LOGICALNOT[] = "LogicalNot";
constexpr char GATHERV2[] = "Gather";
constexpr char SPARSE_GATHERV2[] = "SparseGatherV2";
constexpr char STRIDEDSLICE[] = "StridedSlice";
constexpr char SLICE[] = "Slice";
constexpr char UNIFORM_REAL[] = "UniformReal";
constexpr char BROADCAST[] = "Broadcast";
constexpr char BROADCAST_TO[] = "BroadcastTo";
constexpr char SQRT[] = "Sqrt";
constexpr char ASSIGN[] = "Assign";
constexpr char GET_NEXT[] = "GetNext";
constexpr char MATMUL_DDS[] = "MatmulDDS";
constexpr char SQUEEZE[] = "Squeeze";
constexpr char NEG[] = "Neg";
constexpr char ABS[] = "Abs";
constexpr char ACOSH[] = "Acosh";
constexpr char ASIN[] = "Asin";
constexpr char ASINH[] = "Asinh";
constexpr char ATAN[] = "Atan";
constexpr char ATANH[] = "Atanh";
constexpr char CEIL[] = "Ceil";
constexpr char COSH[] = "Cosh";
constexpr char EXPM1[] = "Expm1";
constexpr char LOG1P[] = "Log1p";
constexpr char SIN[] = "Sin";
constexpr char SINH[] = "Sinh";
constexpr char TAN[] = "Tan";
constexpr char RSQRT[] = "Rsqrt";
constexpr char INV[] = "Inv";
constexpr char RECIPROCAL[] = "Reciprocal";
constexpr char ROUND[] = "Round";
constexpr char FLOOR[] = "Floor";
constexpr char SIGN[] = "Sign";
constexpr char ERF[] = "Erf";
constexpr char ERFC[] = "Erfc";
constexpr char ZEROSLIKE[] = "ZerosLike";
constexpr char ONESLIKE[] = "OnesLike";
constexpr char BESSELI0E[] = "BesselI0e";
constexpr char BESSELI1E[] = "BesselI1e";
constexpr char FLOORMOD[] = "FloorMod";
constexpr char ASSIGN_ADD[] = "AssignAdd";
constexpr char ATAN2[] = "Atan2";
constexpr char DIVNONAN[] = "DivNoNan";
constexpr char LOGICALAND[] = "LogicalAnd";
constexpr char LOGICALOR[] = "LogicalOr";
constexpr char ELU[] = "Elu";
constexpr char RELU6[] = "ReLU6";
constexpr char RELUV2[] = "ReLUV2";
constexpr char SOFTPLUS[] = "Softplus";
constexpr char SOFTSIGN[] = "Softsign";
constexpr char GREATEREQUAL[] = "GreaterEqual";
constexpr char LESSEQUAL[] = "LessEqual";
constexpr char LESS[] = "Less";
constexpr char APPROXIMATEEQUAL[] = "ApproximateEqual";
constexpr char MOD[] = "Mod";
constexpr char BATCH_MATMUL[] = "BatchMatMul";
constexpr char EXPAND_DIMS[] = "ExpandDims";
constexpr char SQUARE[] = "Square";
constexpr char BATCHMATMUL[] = "BatchMatMul";
constexpr char TOPK[] = "TopK";
constexpr char IN_TOPK[] = "InTopK";
constexpr char GATHER_ND[] = "GatherNd";
constexpr char UNSORTEF_SEGMENT_MIND[] = "UnsortedSegmentMinD";
constexpr char UNSORTEF_SEGMENT_PRODD[] = "UnsortedSegmentProdD";
constexpr char UNSORTED_SEGMENT_SUM[] = "UnsortedSegmentSum";
constexpr char UNSORTED_SEGMENT_MIN[] = "UnsortedSegmentMin";
constexpr char UNSORTED_SEGMENT_MAX[] = "UnsortedSegmentMax";
constexpr char UNSORTED_SEGMENT_PROD[] = "UnsortedSegmentProd";
constexpr char DEPTHWISE_CONV2D_NATIVE[] = "DepthwiseConv2dNative";
constexpr char DEPTHWISE_CONV2D[] = "DepthwiseConv2D";
constexpr char DROPOUT[] = "Dropout";
constexpr char KStridedSlice[] = "StridedSlice";
constexpr char UNIQUE[] = "Unique";
constexpr char UNIQUE_WITH_PAD[] = "UniqueWithPad";
constexpr char UNIQUE_CONSECUTIVE[] = "UniqueConsecutive";
constexpr char GATHERND[] = "GatherNd";
constexpr char SCATTER_UPDATE[] = "ScatterUpdate";
constexpr char GATHERD[] = "GatherD";
constexpr char DSD_MATMUL[] = "DSDMatmul";
constexpr char RESIZE_BILINEAR[] = "ResizeBilinear";
constexpr char RESIZE_NEAREST_NEIGHBOR[] = "ResizeNearestNeighbor";
constexpr char CUM_SUM[] = "CumSum";
constexpr char CUM_PROD[] = "CumProd";
constexpr char BOUNDING_BOX_ENCODE[] = "BoundingBoxEncode";
constexpr char IOU[] = "IOU";
constexpr char RANDOM_CHOICE_WITH_MASK[] = "RandomChoiceWithMask";
constexpr char CROP_AND_RESIZE[] = "CropAndResize";
constexpr char MASKED_FILL[] = "MaskedFill";
constexpr char ROI_ALIGN[] = "ROIAlign";
constexpr char SQUARE_SUM_ALL[] = "SquareSumAll";
constexpr char IS_FINITE[] = "IsFinite";
constexpr char RINT[] = "Rint";
constexpr char HSHRINK[] = "HShrink";
constexpr char HSIGMOID[] = "HSigmoid";
constexpr char MISH[] = "Mish";
constexpr char SELU[] = "SeLU";
constexpr char SOFT_SHRINK[] = "SoftShrink";
constexpr char XLOGY[] = "Xlogy";
constexpr char XDIVY[] = "Xdivy";
constexpr char CHECK_VALID[] = "CheckValid";
constexpr char INVERT[] = "Invert";
constexpr char POPULATION_COUNT[] = "PopulationCount";
constexpr char BITWISE_AND[] = "BitwiseAnd";
constexpr char BITWISE_OR[] = "BitwiseOr";
constexpr char BITWISE_XOR[] = "BitwiseXor";
constexpr char MUL_NO_NAN[] = "MulNoNan";
constexpr char TRUNCATE_DIV[] = "TruncateDiv";
constexpr char TRUNCATE_MOD[] = "TruncateMod";
constexpr char INPLACE_ADD[] = "InplaceAdd";
constexpr char INPLACE_SUB[] = "InplaceSub";
constexpr char INPLACE_UPDATE[] = "InplaceUpdate";
constexpr char INPLACE_UPDATEV2[] = "InplaceUpdateV2";
constexpr char CDIST[] = "Cdist";
constexpr char L2_LOSS[] = "L2Loss";
constexpr char LERP[] = "Lerp";
constexpr char SQUARED_DIFFERENCE[] = "SquaredDifference";
constexpr char ERFINV[] = "Erfinv";
constexpr char SPLITV[] = "SplitV";
constexpr char GAMMA[] = "Gamma";
constexpr char KLDIV_LOSS[] = "KLDivLoss";
constexpr char LIN_SPACE[] = "LinSpace";

// pipeline
constexpr size_t PIPELINE_FUSTION_OFFSET = 100;
constexpr char MICRO[] = "micro";
constexpr char DEST_RANK[] = "dest_rank";
constexpr char SRC_RANK[] = "src_rank";
constexpr char PIPELINE_PARAM[] = "pipeline_param";
constexpr char PIPELINE_END[] = "pipeline_end";
constexpr char PIPELINE_BEGIN[] = "pipeline_begin";
constexpr char SLICE_INDEX[] = "slice_index";
constexpr char MAIN_GRAPH[] = "main_graph";
constexpr char SR_TAG[] = "sr_tag";
constexpr char NEED_GRAD[] = "need_grad";
constexpr char GROUP_BACK[] = "group_back";
constexpr char MIRROR_MICRO_STEP_OPERATOR[] = "_MirrorMicroStepOperator";
constexpr char PARAMETER_MICRO[] = "parameter_micro";
constexpr char VIRTUAL_ASSIGN_ADD[] = "_VirtualAssignAdd";
constexpr char VIRTUAL_ACCU_GRAD[] = "_VirtualAccuGrad";
constexpr char ACCU_GRAD[] = "accu_grad";
constexpr char PARAMETER_START[] = "parameter_start";
constexpr char PARAM_INDEX[] = "param_index";
constexpr char PARAMETER[] = "parameter";
constexpr char FUNC_GRAPH_FLAG_STRIDED_SLICE[] = "strided_slice_flag";
constexpr char INTERLEAVED_NUM[] = "interleave_num";
constexpr char FORWARD_NODE_UNIQUE_ID[] = "forward_node_unique_id";
constexpr char MICRO_INTERLEAVED_INDEX[] = "micro_interleaved_index";
constexpr char MICRO_INTERLEAVED_FORWARD_COMM_ORDER[] = "micro_interleaved_forward_comm_order";

// Parallel don't care
constexpr char MAKE_TUPLE[] = "MakeTuple";
constexpr char MAKE_LIST[] = "make_list";
constexpr char MAKE_DICT[] = "make_dict";
constexpr char MAKE_SLICE[] = "make_slice";
constexpr char LIST_GETITEM[] = "list_getitem";
constexpr char ARRAY_GETITEM[] = "array_getitem";
constexpr char TUPLE_SETITEM[] = "tuple_setitem";
constexpr char TUPLE_GETITEM[] = "tuple_getitem";
constexpr char LIST_SETITEM[] = "list_setitem";
constexpr char ARRAY_SETITEM[] = "array_setitem";
constexpr char DICT_GETITEM[] = "dict_getitem";
constexpr char LIST_APPEND[] = "list_append";
constexpr char LIST_MAP[] = "list_map";
constexpr char LIST_REDUCE[] = "list_reduce";
constexpr char TUPLE_REVERSED[] = "tuple_reversed";
constexpr char TILE_SHAPE[] = "tile_shape";
constexpr char REDUCED_SHAPE[] = "reduced_shape";
constexpr char TUPLE_DIV[] = "tuple_div";
constexpr char TUPLE_TO_ARRAY[] = "tuple_to_array";
constexpr char VIRTUALLOSS[] = "VirtualLoss";
constexpr char RETURN[] = "Return";
constexpr char ENVIRONGET[] = "EnvironGet";
constexpr char IDENTITY[] = "identity";
constexpr char PARTIAL[] = "partial";
constexpr char ENVIRONSET[] = "EnvironSet";
constexpr char ENVIRONADD[] = "EnvironAdd";
constexpr char STATESETITEM[] = "state_setitem";
constexpr char SCALARSUMMARY[] = "ScalarSummary";
constexpr char IMAGESUMMARY[] = "ImageSummary";
constexpr char TENSORSUMMARY[] = "TensorSummary";
constexpr char HISTOGRAMSUMMARY[] = "HistogramSummary";
constexpr char DEBUG[] = "Debug";
constexpr char BROADCASTGRADIENTARGS[] = "BroadcastGradientArgs";
constexpr char INVERTPERMUTATION[] = "InvertPermutation";
constexpr char DOT[] = "dot";
constexpr char IM2COL[] = "im2col";
constexpr char COL2IM[] = "col2im";
constexpr char IM2COLV1[] = "im2col_v1";
constexpr char COL2IMV1[] = "col2im_v1";
constexpr char RESOLVE[] = "resolve";
constexpr char EMBED[] = "embed";
constexpr char CREATINSTANCE[] = "create_instance";
constexpr char REF_TO_EMBED[] = "RefToEmbed";
constexpr char STOP_GRADIENT[] = "stop_gradient";
constexpr char UPDATESTATE[] = "UpdateState";
constexpr char LOAD[] = "Load";
constexpr char OPPOSITE_RANK[] = "opposite_rank";
constexpr char TARGET_PARAM[] = "target_param";

// Batch parallel black list
constexpr char TENSOR_SCATTER_UPDATE[] = "TensorScatterUpdate";
constexpr char MIN_MAX_UPDATE_PER_LAYER[] = "MinMaxUpdatePerLayer";
constexpr char STACK[] = "Stack";

constexpr size_t LAST_INDEX(size_t s) {
  if (s < 1) {
    MS_LOG(EXCEPTION) << "Get last index for an empty size.";
  }
  return s - 1;
}
constexpr size_t SECOND_FROM_END(size_t s) {
  if (s < 2) {
    MS_LOG(EXCEPTION) << "Get second index from end for a size less than two.";
  }
  return s - 2;
}
constexpr size_t THIRD_FROM_END(size_t s) {
  if (s < 3) {
    MS_LOG(EXCEPTION) << "Get third index from end for a size less than three.";
  }
  return s - 3;
}
}  // namespace parallel
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_FRONTEND_PARALLEL_OPS_INFO_OPS_UTILS_H_
