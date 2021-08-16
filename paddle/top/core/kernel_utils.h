//   Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
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

#include "paddle/top/core/kernel_context.h"
#include "paddle/top/core/kernel_def.h"

// See Note [ Why still include the fluid headers? ]
#include "paddle/fluid/platform/device_context.h"
#include "paddle/fluid/platform/enforce.h"

namespace pt {

// TODO(chenweihang): replaced by new DeviceContext later
using CPUContext = paddle::platform::CPUDeviceContext;
#ifdef PADDLE_WITH_CUDA
using CUDAContext = paddle::platform::CUDADeviceContext;
#endif
#ifdef PADDLE_WITH_MKLDNN
using MKLDNNContext = paddle::platform::MKLDNNDeviceContext;
#endif
#ifdef PADDLE_WITH_ASCEND_CL
using NPUContext = paddle::platform::NPUDeviceContext;
#endif
#ifdef PADDLE_WITH_XPU
using XPUContext = paddle::platform::XPUDeviceContext;
#endif

#define PT_KERNEL(...) \
  ::pt::OpKernelImpl<decltype(&__VA_ARGS__), &__VA_ARGS__>::Compute

#define PT_SPECIALIZE_OpKernelCallHelper_FOR_DEVICE_CONTEXT(dev_ctx)         \
  template <typename... Tail>                                                \
  struct OpKernelCallHelper<const dev_ctx&, Tail...> {                       \
    template <int dev_ctx_idx,                                               \
              int in_idx,                                                    \
              int attr_idx,                                                  \
              int out_idx,                                                   \
              typename... PreviousArgs>                                      \
    static void Compute(OpKernelContext* ctx, PreviousArgs&... pargs) {      \
      static_assert(in_idx == 0,                                             \
                    "Kernel's DeviceContext should appear before Inputs.");  \
      static_assert(                                                         \
          attr_idx == 0,                                                     \
          "Kernel's DeviceContext should appear before Attributes.");        \
      static_assert(out_idx == 0,                                            \
                    "Kernel's DeviceContext should appear before Outputs."); \
      const dev_ctx& arg = ctx->GetDeviceContext<dev_ctx>();                 \
      OpKernelCallHelper<Tail...>::                                          \
          template Compute<dev_ctx_idx + 1, in_idx, attr_idx, out_idx>(      \
              ctx, pargs..., arg);                                           \
    }                                                                        \
  }

template <typename T>
struct TypeTag {};

template <typename Fn, Fn fn>
struct OpKernelImpl;

template <typename Return, typename... Args, Return (*kernel_fn)(Args...)>
struct OpKernelImpl<Return (*)(Args...), kernel_fn> {
  static void Compute(OpKernelContext* ctx) {
    OpKernelCallHelper<Args..., TypeTag<int>>::template Compute<0, 0, 0, 0>(
        ctx);
  }

 private:
  template <typename... RemainingArgs>
  struct OpKernelCallHelper;

  /* DeviceContext Helpers */

  PT_SPECIALIZE_OpKernelCallHelper_FOR_DEVICE_CONTEXT(CPUContext);
#ifdef PADDLE_WITH_CUDA
  PT_SPECIALIZE_OpKernelCallHelper_FOR_DEVICE_CONTEXT(CUDAContext);
#endif
#ifdef PADDLE_WITH_XPU
  PT_SPECIALIZE_OpKernelCallHelper_FOR_DEVICE_CONTEXT(XPUContext);
#endif

  /* Input Helpers */

  template <typename... Tail>
  struct OpKernelCallHelper<const DenseTensor&, Tail...> {
    template <int dev_ctx_idx,
              int in_idx,
              int attr_idx,
              int out_idx,
              typename... PreviousArgs>
    static void Compute(OpKernelContext* ctx, PreviousArgs&... pargs) {
      static_assert(attr_idx == 0,
                    "Kernel's Input should appear before Attributes.");
      static_assert(out_idx == 0,
                    "Kernel's Input should appear before Outputs.");
      const DenseTensor& arg = ctx->InputAt<DenseTensor>(in_idx);
      OpKernelCallHelper<Tail...>::
          template Compute<dev_ctx_idx, in_idx + 1, attr_idx, out_idx>(
              ctx, pargs..., arg);
    }
  };

  /* Attribute Helpers */

  /* Output Helpers */

  template <typename... Tail>
  struct OpKernelCallHelper<DenseTensor*, Tail...> {
    template <int dev_ctx_idx,
              int in_idx,
              int attr_idx,
              int out_idx,
              typename... PreviousArgs>
    static void Compute(OpKernelContext* ctx, PreviousArgs&... pargs) {
      DenseTensor* arg = ctx->MutableOutputAt<DenseTensor>(out_idx);
      OpKernelCallHelper<Tail...>::
          template Compute<dev_ctx_idx, in_idx, attr_idx, out_idx + 1>(
              ctx, pargs..., arg);
    }
  };

  /* End case */
  template <typename T>
  struct OpKernelCallHelper<TypeTag<T>> {
    template <int dev_ctx_idx, int in_idx, int attr_idx, int out_idx>
    static void Compute(OpKernelContext* ctx, Args&... args) {
      static_assert(dev_ctx_idx > 0,
                    "Kernel should pass DeviceContext as argument.");
      static_assert(out_idx > 0, "Kernel should have output argument.");
      // TODO(chenweihang): check dev_ctx, in, attr, out number
      return kernel_fn(args...);
    }
  };
};

}  // namespace pt
