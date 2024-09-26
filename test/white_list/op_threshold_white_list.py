# Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

NEED_FIX_FP64_CHECK_GRAD_THRESHOLD_OP_LIST = [
    'affine_channel',
    'bilinear_interp',
    'bilinear_interp_v2',
    'bilinear_tensor_product',
    'conv2d',
    'conv3d',
    'cross_entropy',
    'depthwise_conv2d_transpose',
    'grid_sampler',
    'group_norm',
    'gru',
    'gru_unit',
    'kldiv_loss',
    'lstm',
    'max_pool2d_with_index',
    'max_pool3d_with_index',
    'fractional_max_pool2d',
    'fractional_max_pool3d',
    'norm',
    'pool3d',
    'lp_pool2d',
    'reduce_prod',
    'selu',
    'sigmoid_cross_entropy_with_logits',
    'soft_relu',
    'softmax_with_cross_entropy',
    'unpool',
    'yolov3_loss',
    'inverse',
    'bilateral_slice',
    'cudnn_lstm',
    'rnn',
    'lgamma',
    'sparse_attention',
    'svd',
    'matrix_power',
    'cholesky_solve',
    'solve',
    'qr',
    'layer_norm',
    'cumprod',
    # 'expand_v2',
]

NEED_FIX_FP64_CHECK_OUTPUT_THRESHOLD_OP_LIST = [
    'bilinear_interp',
    'bilinear_interp_v2',
    'layer_norm',
]
