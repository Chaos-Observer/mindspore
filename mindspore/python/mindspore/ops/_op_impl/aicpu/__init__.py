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

"""aicpu ops"""
from .unique import _unique_aicpu
from .matrix_solve import _matrix_solve_aicpu
from .betainc import _betainc_aicpu
from .no_repeat_ngram import _no_repeat_ngram_aicpu
from .init_data_set_queue import _init_data_set_queue_aicpu
from .embedding_lookup import _embedding_lookup_aicpu
from .padding import _padding_aicpu
from .gather import _gather_aicpu
from .gather_grad import _gather_grad_aicpu
from .gather_d_grad_v2 import _gather_d_grad_v2_aicpu
from .gather_d import _gather_d_aicpu
from .gather_nd import _gather_nd_aicpu
from .scatter import _scatter_aicpu
from .identity import _identity_aicpu
from .edit_distance import _edit_distance_aicpu
from .unique_with_pad import _unique_with_pad_aicpu
from .sub_and_filter import _sub_and_filter_aicpu
from .pad_and_shift import _pad_and_shift_aicpu
from .dropout_genmask import _dropout_genmask_aicpu
from .dropout_genmask_v3 import _dropout_genmask_v3_aicpu
from .dropout2d import _dropout2d_aicpu
from .dropout3d import _dropout3d_aicpu
from .dynamic_stitch import _dynamic_stitch_aicpu
from .get_next import _get_next_aicpu
from .print_tensor import _print_aicpu
from .topk import _top_k_aicpu
from .log1p import _log1p_aicpu
from .asin import _asin_aicpu
from .is_finite import _is_finite_aicpu
from .is_inf import _is_inf_aicpu
from .is_nan import _is_nan_aicpu
from .reshape import _reshape_aicpu
from .flatten import _flatten_aicpu
from .cosh import _cosh_aicpu
from .sign import _sign_aicpu
from .squeeze import _squeeze_aicpu
from .acos import _acos_aicpu
from .expand import _expand_aicpu
from .expand_dims import _expand_dims_aicpu
from .randperm import _randperm_aicpu
from .random_choice_with_mask import _random_choice_with_mask_aicpu
from .rsqrt import _rsqrt_aicpu
from .search_sorted import _search_sorted_aicpu
from .stack import _stack_aicpu
from .unstack import _unstack_aicpu
from .addcmul import _addcmul_aicpu
from .uniform_candidate_sampler import _uniform_candidate_sampler_aicpu
from .log_uniform_candidate_sampler import _log_uniform_candidate_sampler_aicpu
from .compute_accidental_hits import _compute_accidental_hits_aicpu
from .ctcloss import _ctcloss_aicpu
from .reverse_sequence import _reverse_sequence_aicpu
from .log_matrix_determinant import _log_matrix_determinant_aicpu
from .crop_and_resize import _crop_and_resize_aicpu
from .acosh import _acosh_aicpu
from .rnnt_loss import _rnnt_loss_aicpu
from .random_categorical import _random_categorical_aicpu
from .tanh_grad import _tanh_grad_aicpu
from .cast import _cast_aicpu
from .mirror_pad import _mirror_pad_aicpu
from .mirror_pad_grad import _mirror_pad_grad_aicpu
from .masked_select import _masked_select_aicpu
from .masked_select_grad import _masked_select_grad_aicpu
from .matmul import _matmul_aicpu
from .mul import _mul_aicpu
from .standard_normal import _standard_normal_aicpu
from .gamma import _gamma_aicpu
from .random_gamma import _random_gamma_aicpu
from .sub import _sub_aicpu
from .not_equal import _not_equal_aicpu
from .poisson import _poisson_aicpu
from .update_cache import _update_cache_aicpu
from .cache_swap_table import _cache_swap_table_aicpu
from .uniform_int import _uniform_int_aicpu
from .uniform_real import _uniform_real_aicpu
from .standard_laplace import _standard_laplace_aicpu
from .strided_slice import _strided_slice_aicpu
from .neg import _neg_aicpu
from .strided_slice_grad import _strided_slice_grad_aicpu
from .end_of_sequence import _end_of_sequence_aicpu
from .fused_sparse_adam import _fused_sparse_adam_aicpu
from .fused_sparse_lazy_adam import _fused_sparse_lazy_adam_aicpu
from .fused_sparse_ftrl import _fused_sparse_ftrl_aicpu
from .fused_sparse_proximal_adagrad import _fused_sparse_proximal_adagrad_aicpu
from .meshgrid import _meshgrid_aicpu
from .div import _div_aicpu
from .trans_data import _trans_data_aicpu
from .stack_push_pop import _stack_init_aicpu
from .stack_push_pop import _stack_push_aicpu
from .stack_push_pop import _stack_pop_aicpu
from .asinh import _asinh_aicpu
from .stack_push_pop import _stack_destroy_aicpu
from .matrix_diag_v3 import _matrix_diag_v3_aicpu
from .matrix_diag_part_v3 import _matrix_diag_part_v3_aicpu
from .tan import _tan_aicpu
from .ctc_greedy_decoder import _ctc_greedy_decoder_aicpu
from .resize_bilinear import _resize_bilinear_aicpu
from .resize_bilinear_grad import _resize_bilinear_grad_aicpu
from .scatter_elements import _scatter_elements_aicpu
from .non_max_suppression import _non_max_suppression_aicpu
from .square import _square_aicpu
from .squared_difference import _squared_difference_aicpu
from .non_zero import _non_zero_aicpu
from .zeros_like import _zeros_like_aicpu
from .ones_like import _ones_like_aicpu
from .grid_sampler_3d import _grid_sampler_3d_aicpu
from .atanh import _atanh_aicpu
from .grid_sampler_3d_grad import _grid_sampler_3d_grad_aicpu
from .environ_create import _environ_create_aicpu
from .environ_set import _environ_set_aicpu
from .environ_get import _environ_get_aicpu
from .environ_destroy_all import _environ_destroy_all_aicpu
from .cross import _cross_aicpu
from .check_numerics import _check_numerics_aicpu
from .cumsum import _cumsum_aicpu
from .round import _round_aicpu
from .stft import _stft_aicpu
from .floor_div import _floor_div_aicpu
from .priority_replay_buffer import _prb_create_op_cpu
from .priority_replay_buffer import _prb_push_op_cpu
from .conjugate_transpose import _conjugate_transpose_aicpu
from .priority_replay_buffer import _prb_sample_op_cpu
from .priority_replay_buffer import _prb_update_op_cpu
from .equal import _equal_aicpu
from .priority_replay_buffer import _prb_destroy_op_cpu
from .right_shift import _right_shift_aicpu
from .tril import _tril_aicpu
from .linspace import _lin_space_aicpu
from .triu import _triu_aicpu
from .zeta import _zeta_aicpu
from .bias_add import _bias_add_aicpu
from .bias_add_grad import _bias_add_grad_aicpu
from .grid_sampler_2d import _grid_sampler_2d_aicpu
from .grid_sampler_2d_grad import _grid_sampler_2d_grad_aicpu
from .sparse_segment_mean_grad import _sparse_segment_mean_grad_aicpu
from .scatter_nd_max import _scatter_nd_max_aicpu
from .conj import _conj_aicpu
from .scatter_nd_min import _scatter_nd_min_aicpu
from .compare_and_bitpack import _compare_and_bitpack_aicpu
from .addcdiv import _addcdiv_aicpu
from .unique_consecutive import _unique_consecutive_aicpu
from .sparse_tensor_to_csr_sparse_matrix import _sparse_tensor_to_csr_sparse_matrix_aicpu
from .csr_sparse_matrix_to_sparse_tensor import _csr_sparse_matrix_to_sparse_tensor_aicpu
from .linear_sum_assignment import _linear_sum_assignment_aicpu
from .random_shuffle import _random_shuffle_aicpu
from .reservoir_replay_buffer import _rrb_create_op_cpu
from .reservoir_replay_buffer import _rrb_push_op_cpu
from .reservoir_replay_buffer import _rrb_sample_op_cpu
from .reservoir_replay_buffer import _rrb_destroy_op_cpu
from .parallel_concat import _parallel_concat_aicpu
from .concat_offset import _concat_offset_aicpu
from .range import _range_aicpu
