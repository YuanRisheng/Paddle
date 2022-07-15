// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
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

#include "paddle/phi/kernels/activation_grad_kernel.h"

#include "paddle/phi/backends/onednn/onednn_context.h"
#include "paddle/phi/common/bfloat16.h"
#include "paddle/phi/common/place.h"
#include "paddle/phi/core/kernel_registry.h"
#include "paddle/phi/kernels/funcs/activation_functor.h"
#include "paddle/phi/kernels/funcs/onednn/mkldnn_reuse.h"

namespace phi {

using dnnl::memory;
using dnnl::primitive;
using dnnl::stream;

#define DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPX(name, functor_class) \
  template <typename T, typename Context>                              \
  void name##GradKernel(const Context& dev_ctx,                        \
                        const DenseTensor& x,                          \
                        const DenseTensor& dout,                       \
                        DenseTensor* dx) {                             \
    functor_class<T> functor;                                          \
    functor(dev_ctx, x, dout, 0, 0, dx);                               \
  }

#define DEFINE_ONEDNN_ACT_GRAD_KERNEL_WITH_ONE_ATTRS_DEPX( \
    name, functor_class, attr)                             \
  template <typename T, typename Context>                  \
  void name##GradKernel(const Context& dev_ctx,            \
                        const DenseTensor& x,              \
                        const DenseTensor& dout,           \
                        float attr,                        \
                        DenseTensor* dx) {                 \
    functor_class<T> functor;                              \
    functor(dev_ctx, x, dout, attr, 0, dx);                \
  }

#define DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(name, functor_class) \
  template <typename T, typename Context>                                \
  void name##GradKernel(const Context& dev_ctx,                          \
                        const DenseTensor& out,                          \
                        const DenseTensor& dout,                         \
                        DenseTensor* dx) {                               \
    functor_class<T> functor;                                            \
    functor(dev_ctx, out, dout, 0, 0, dx);                               \
  }

#define DEFINE_ONEDNN_ACT_GRAD_KERNEL_WITH_ONE_ATTRS_DEPOUT( \
    name, functor_class, attr)                               \
  template <typename T, typename Context>                    \
  void name##GradKernel(const Context& dev_ctx,              \
                        const DenseTensor& out,              \
                        const DenseTensor& dout,             \
                        float attr,                          \
                        DenseTensor* dx) {                   \
    functor_class<T> functor;                                \
    functor(dev_ctx, out, dout, attr, 0, dx);                \
  }

template <typename T>
void eltwise_grad(const OneDNNContext& dev_ctx,
                  const DenseTensor& x,
                  const DenseTensor& dout,
                  float alpha,
                  float beta,
                  DenseTensor* dx,
                  dnnl::algorithm algorithm) {
  const auto& mkldnn_engine = dev_ctx.GetEngine();

  funcs::ActivationMKLDNNHandler<T> handler(
      algorithm, alpha, beta, mkldnn_engine, dev_ctx.GetPlace(), &x, &dout);

  auto src_memory_p = handler.AcquireBackwardSrcMemory(&x);
  auto diff_dst_memory_p = handler.AcquireDiffDstMemory(&dout);
  auto diff_src_memory_p = handler.AcquireDiffSrcMemory(dx);
  auto activation_backward_p = handler.AcquireBackwardPrimitive();

  auto& astream = OneDNNContext::tls().get_stream();
  activation_backward_p->execute(astream,
                                 {{DNNL_ARG_SRC, *src_memory_p},
                                  {DNNL_ARG_DIFF_DST, *diff_dst_memory_p},
                                  {DNNL_ARG_DIFF_SRC, *diff_src_memory_p}});
  astream.wait();

  dx->set_mem_desc(diff_src_memory_p->get_desc());
}

template <typename T>
void eltwise_grad_use_out(const OneDNNContext& dev_ctx,
                          const DenseTensor& out,
                          const DenseTensor& dout,
                          float alpha,
                          float beta,
                          DenseTensor* dx,
                          dnnl::algorithm algorithm) {
  const auto& mkldnn_engine = dev_ctx.GetEngine();

  funcs::ActivationMKLDNNHandler<T> handler(
      algorithm, alpha, beta, mkldnn_engine, dev_ctx.GetPlace(), &out, &dout);

  auto dst_memory_p = handler.AcquireBackwardSrcMemory(&out);
  auto diff_dst_memory_p = handler.AcquireDiffDstMemory(&dout);
  auto diff_src_memory_p = handler.AcquireDiffSrcMemory(dx);
  auto activation_backward_p = handler.AcquireBackwardPrimitive();

  auto& astream = OneDNNContext::tls().get_stream();
  activation_backward_p->execute(astream,
                                 {{DNNL_ARG_DST, *dst_memory_p},
                                  {DNNL_ARG_DIFF_DST, *diff_dst_memory_p},
                                  {DNNL_ARG_DIFF_SRC, *diff_src_memory_p}});
  astream.wait();

  dx->set_mem_desc(diff_src_memory_p->get_desc());
}

template <typename T, dnnl::algorithm algorithm>
struct MKLDNNActivationGradFunc : public funcs::BaseActivationFunctor<T> {
  void operator()(const OneDNNContext& dev_ctx,
                  const DenseTensor& x,
                  const DenseTensor& dout,
                  float alpha,
                  float beta,
                  DenseTensor* dx) const {
    eltwise_grad<T>(dev_ctx, x, dout, alpha, beta, dx, algorithm);
  }
};

template <typename T, dnnl::algorithm algorithm>
struct MKLDNNActivationGradUseOutFunc : public funcs::BaseActivationFunctor<T> {
  void operator()(const OneDNNContext& dev_ctx,
                  const DenseTensor& out,
                  const DenseTensor& dout,
                  float alpha,
                  float beta,
                  DenseTensor* dx) const {
    eltwise_grad_use_out<T>(dev_ctx, out, dout, alpha, beta, dx, algorithm);
  }
};

template <typename T>
using ReluMKLDNNGradFunctor =
    MKLDNNActivationGradFunc<T, dnnl::algorithm::eltwise_relu>;

template <typename T>
using SwishMKLDNNGradFunctor =
    MKLDNNActivationGradFunc<T, dnnl::algorithm::eltwise_swish>;

template <typename T>
using HardSwishMKLDNNGradFunctor =
    MKLDNNActivationGradFunc<T, dnnl::algorithm::eltwise_hardswish>;

template <typename T>
using MishMKLDNNGradFunctor =
    MKLDNNActivationGradFunc<T, dnnl::algorithm::eltwise_mish>;

template <typename T>
using SigmoidMKLDNNGradUseOutFunctor = MKLDNNActivationGradUseOutFunc<
    T,
    dnnl::algorithm::eltwise_logistic_use_dst_for_bwd>;

template <typename T>
using TanhMKLDNNGradUseOutFunctor = MKLDNNActivationGradUseOutFunc<
    T,
    dnnl::algorithm::eltwise_tanh_use_dst_for_bwd>;

template <typename T>
using SqrtMKLDNNGradUseOutFunctor = MKLDNNActivationGradUseOutFunc<
    T,
    dnnl::algorithm::eltwise_sqrt_use_dst_for_bwd>;

template <typename T>
using EluMKLDNNGradUseOutFunctor = MKLDNNActivationGradUseOutFunc<
    T,
    dnnl::algorithm::eltwise_elu_use_dst_for_bwd>;

template <typename T>
using ExpMKLDNNGradUseOutFunctor = MKLDNNActivationGradUseOutFunc<
    T,
    dnnl::algorithm::eltwise_exp_use_dst_for_bwd>;

DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(Tanh, TanhMKLDNNGradUseOutFunctor);
DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(Sqrt, SqrtMKLDNNGradUseOutFunctor);
DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(Sigmoid,
                                            SigmoidMKLDNNGradUseOutFunctor);
DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(Exp, ExpMKLDNNGradUseOutFunctor);
DEFINE_ONEDNN_ACTIVATION_GRAD_KERNEL_DEPOUT(Relu, ReluMKLDNNGradFunctor);

DEFINE_ONEDNN_ACT_GRAD_KERNEL_WITH_ONE_ATTRS_DEPX(LeakyRelu,
                                                  ReluMKLDNNGradFunctor,
                                                  alpha);
DEFINE_ONEDNN_ACT_GRAD_KERNEL_WITH_ONE_ATTRS_DEPX(Mish,
                                                  MishMKLDNNGradFunctor,
                                                  threshold);
DEFINE_ONEDNN_ACT_GRAD_KERNEL_WITH_ONE_ATTRS_DEPX(Swish,
                                                  SwishMKLDNNGradFunctor,
                                                  beta);
template <typename T, typename Context>
void HardSwishGradKernel(const Context& dev_ctx,
                         const DenseTensor& x,
                         const DenseTensor& dout,
                         float threshold,
                         float scale,
                         float offset,
                         DenseTensor* dx) {
  HardSwishMKLDNNGradFunctor<T> functor;
  functor(dev_ctx, x, dout, threshold, 0, dx);
}

template <typename T, typename Context>
void EluGradKernel(const Context& dev_ctx,
                   const DenseTensor& x,
                   const DenseTensor& out,
                   const DenseTensor& dout,
                   float alpha,
                   DenseTensor* dx) {
  EluMKLDNNGradUseOutFunctor<T> functor;
  functor(dev_ctx, out, dout, alpha, 0, dx);
}

}  // namespace phi

PD_REGISTER_KERNEL(relu_grad,
                   OneDNN,
                   ALL_LAYOUT,
                   phi::ReluGradKernel,
                   float,
                   phi::dtype::bfloat16) {}

#define PD_REGISTER_ACTIVATION_GRAD_KERNEL(name, func) \
  PD_REGISTER_KERNEL(                                  \
      name, OneDNN, ALL_LAYOUT, phi::func, float, phi::dtype::bfloat16) {}

PD_REGISTER_ACTIVATION_GRAD_KERNEL(elu_grad, EluGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(exp_grad, ExpGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(hard_swish_grad, HardSwishGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(leaky_relu_grad, LeakyReluGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(mish_grad, MishGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(sigmoid_grad, SigmoidGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(sqrt_grad, SqrtGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(swish_grad, SwishGradKernel)
PD_REGISTER_ACTIVATION_GRAD_KERNEL(tanh_grad, TanhGradKernel)
