// Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "paddle/pten/api/include/tensor.h"
#include "paddle/pten/common/data_type.h"
#include "paddle/pten/common/scalar.h"

namespace paddle {
namespace experimental {

Tensor full(const std::vector<int64_t>& shape,
            const Scalar& value,
            DataType dtype = DataType::FLOAT32,
            Backend backend = Backend::CPU,
            DataLayout layout = DataLayout::NCHW);

Tensor full_like(const Tensor& x,
                 const Scalar& value,
                 DataType dtype = DataType::UNDEFINED);

Tensor ones_like(const Tensor& x, DataType dtype = DataType::UNDEFINED);

Tensor zeros_like(const Tensor& x, DataType dtype = DataType::UNDEFINED);

}  // namespace experimental
}  // namespace paddle
