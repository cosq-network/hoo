# ISSUE-026: Native Neural Network and AI Data Support

## 1. Overview

This issue defines a staged plan for a first-class neural-network and AI data
library for the Hoo ecosystem. The intended result is a practical, PyTorch- or
TensorFlow-like platform that can:

- define, train, evaluate, export, import, and deploy neural networks;
- support common image, text, audio, video, tabular, and multimodal workloads;
- provide reusable layers, losses, optimizers, metrics, tokenizers, datasets,
  transforms, batching, and streaming data loaders;
- load and serialize pretrained weights and model graphs in widely used industry
  formats; and
- keep the core execution path close to Hoo and HVM while allowing explicit
  native and accelerator backends where they provide a material benefit.

The implementation must be delivered incrementally. The existing tensor core is
the foundation, but it does not currently provide automatic differentiation,
model/layer abstractions, training orchestration, or model-format import/export.

## 1.0 Implementation status (2026-08-10)

The first executable foundation slice is implemented and verified:

- `hoort` now exposes a versioned status/feature ABI through `hoo_ai.h` and
  `hoo_ai.cpp` while retaining the legacy tensor constructors and accessors.
- `HooTensor` ABI version 2 supports checked dynamic-rank allocation (rank 1–64),
  row-major shape/stride/numel queries, copying, `f32` and `int32` storage, and
  typed error/status reporting. `f16`/`bf16` remain explicitly unsupported.
- ABI v2 reports contiguous strides in bytes for byte-addressable dtypes and in
  bits for packed `bit` tensors; this convention must be retained by future
  view/layout extensions.
- Tensor objects use a dedicated runtime type ID and remain opaque ARC-managed
  handles; the Buffer type ID is no longer reused by new tensor objects.
- HVM tensor arithmetic no longer relies on a private inline payload offset.
  Tensor expressions use registered runtime calls, so the HVM path is compatible
  with dynamic metadata and remains valid for C++17 `hoort` builds.
- Nested tensor literals now infer their leaf element type recursively.
- Focused runtime and HVM/JIT tensor tests pass, including dynamic-rank metadata,
  `f32` copy behavior, arithmetic, 3-D literals, and chained operations.

This slice is not an ANN framework yet: autograd, modules, optimizers, data
pipelines, tokenizers, model import/export, and neural-network layers remain
planned work described below.

## 1.1 Sufficiency Verdict

This document is sufficient as a broad roadmap, but the current `hoort` tensor
and Hoo language are **not yet sufficient to implement a full Keras/PyTorch
class of library**. The following prerequisites must be completed before the
project can claim general neural-network support:

| Capability | Current position | Required before parity claim |
| --- | --- | --- |
| Tensor rank/layout | Legacy rank 1/2/3 surface; versioned C ABI rank 1–64 metadata | Hoo source-level higher-rank types, views, layout transforms, and batched kernels |
| Tensor operators | Basic arithmetic, rank-2 matmul, reshape, transpose, softmax | Reductions, unary math, broadcasting, batched matmul, convolution, gather/scatter, masks, indexing, and stable numerics |
| Autograd | Not implemented | A typed callable/function representation, graph/tape or compiler IR, gradient rules, mutation/aliasing rules, and higher-order policy |
| Model values | Mostly `Tensor` and untyped `Array`/`any` | Typed multi-input/multi-output/state values and parameter/buffer traversal |
| Training dtypes | `f8`, `f64`, integer and bit storage | Explicit master-weight and gradient accumulation policy; `f16`/`bf16` only after ABI extension |
| Device execution | CPU/runtime and profile-gated HVM features | Device placement, transfer, synchronization, capability checks, and fallback semantics |
| Data streams | No dataset/stream runtime | Iterable datasets, backpressure, cancellation, worker lifetime, and bounded buffering |
| Model compatibility | No ANN model importer | Operator/layout/dtype mapping and validation for each supported external format |

The phases and acceptance criteria below must be read as gates. A model family
that requires an unsupported rank, operator, dtype, or callable feature is not
implemented merely because its class name exists in `hoo.nn`.

## 2. Current State and Constraints

As of 2026-08-10:

- The legacy Hoo surface has rank-1, rank-2, and rank-3 tensors with `bit`,
  `int8`, `byte`, `int64`, `f8`, and `f64` elements. The versioned runtime C ABI
  additionally supports checked dynamic-rank metadata (rank 1–64), `f32`, and
  `int32`; this is not yet exposed as a complete Hoo source-level tensor type.
- Tensor allocation, element access, arithmetic, matrix multiplication,
  reshape, transpose, softmax, comparisons, and logical operations exist in the
  runtime/JIT integration.
- Tensor arithmetic currently uses runtime implementations. In particular,
  matrix multiplication is implemented by a C++ triple loop. Tensor HVM/JIT
  expressions use registered runtime calls; private tensor payload offsets are
  not part of the HVM contract. The kernels are not yet optimized by LLVM
  loop-vectorization.
- HVMJIT translates HVM instructions to LLVM IR and has HVM-V/vector-related
  operations, but its current IR transform is not a general tensor optimizer and
  does not provide automatic GPU dispatch.
- There is no `grad`, `hoo.nn`, optimizer, dataset, tokenizer, model-format
  loader, checkpoint manager, or complete training loop.

This issue must not describe proposed functionality as if it already exists.
Every feature below is subject to capability detection, numerical tests, and a
portable fallback where practical.

## 2.1 Compatibility Contract: C++17, HVM 1.5, and `hoort`

The implementation must preserve the existing target boundaries:

### C++17

- All compiler and runtime changes must compile with the repository's required
  `CMAKE_CXX_STANDARD 17` and `CMAKE_CXX_STANDARD_REQUIRED ON` settings.
- Public runtime interfaces must remain C-compatible where they are currently
  C ABI interfaces. C++17-only implementation details must not leak into Hoo
  source or the HVM ABI.
- Do not require C++20/23 concepts, ranges, modules, `std::span`, coroutines,
  or compiler-specific language extensions. C++17 equivalents may be used
  internally.
- `hoort` must remain independent of LLVM. LLVM-dependent graph compilation,
  model import/export, and optional accelerator integrations belong in
  `hoo-core` or separately linked optional libraries.
- Optional dependencies for image, audio, video, BLAS, ONNX, or accelerator
  support must be isolated behind C++17-compatible adapters and must not make
  the minimal `hoort` target depend on those libraries.

### HVM 1.5

- HVM is a 64-bit load/store ISA, not a tensor or GPU ISA. Tensor objects are
  ARC-managed runtime objects referenced by 64-bit HVM pointers; their metadata
  and storage must not be assumed to be HVM architectural registers.
- Tensor operations must initially lower to ordinary HVM instructions plus
  registered runtime calls. A future HVM tensor lowering may use scalar
  instructions, `LOOP.SET`/`LOOP.DECBR`, or HVM-V, but each is optional and must
  retain a software fallback.
- HVM-V is profile-gated. In the hosted HVMJIT profile, `VLEN` is 64 bits and
  the supported vector element types are effectively `i64`/`u64` and `f64`.
  The plan must not claim native HVM-V vector execution for packed `bit`, `int8`,
  `byte`, or `f8` tensors until a target profile defines and tests those types.
- HVM-A `DOORBELL` is also profile-gated and is not automatic GPU support. GPU
  or accelerator execution requires a platform-specific device ABI, memory and
  synchronization contract, feature-bit validation, and a fallback path.
- Generated modules must advertise required HVM features in their module
  feature flags and loaders must reject unsupported required features. Portable
  model code must be usable without HVM-V, HVM-A, HVM-Alloc, or other optional
  profiles.
- Public calls follow the HVM ABI: scalar and pointer arguments occupy 64-bit
  registers/stack slots, pointers are 64-bit, `r4` is reserved for the thread
  pointer, and `r1` carries scalar/pointer returns. Tensor APIs must not expose
  host C++ object layout as an HVM contract.

### `hoort` tensor target

- The legacy `hoort` tensor surface supports ranks 1, 2, and 3 with `bit`,
  `int8`, `byte`, `int64`, `f8`, and `f64`; the versioned C ABI adds dynamic
  rank, `f32`, and `int32` as a foundation for ANN work. ANN Phase 1 must use
  only capability-queried types and retain the existing constructor family for
  source compatibility.
- The C ABI uses `HooTensor` as an opaque pointer. Both legacy and versioned
  implementations must be accessed through exported functions; no HVM or Hoo
  code may expose `HooTensorHeader` or depend on private storage offsets.
- Tensor storage is ARC-managed through `hoo_alloc`, `hoo_retain`, and
  `hoo_release`; every model parameter, gradient, optimizer state, and
  temporary tensor must obey those ownership rules. Views and aliases need an
  explicit ownership/lifetime contract before they are added.
- The tensor implementation currently uses runtime C++ arithmetic and returns
  `null` for several invalid operations. The ANN layer must add typed error
  reporting without breaking existing calls; compile-time diagnostics may be
  added only where the Hoo type system has enough information.
- New tensor objects use a dedicated tensor type ID (`131`) rather than the
  Buffer ID (`113`), with destructor registration performed once by the tensor
  runtime. Serialization and generic object dispatch still require explicit
  ANN-level coverage before checkpoints rely on them.
- `f16` and `bf16` are not current `hoort` tensor element types. They may be
  supported first as import/export encodings converted to `f64` or `f8`, or
  added only through a separately specified `hoort` ABI extension. They must
  not appear as implemented Hoo tensor types in the initial ANN API.

## 3. Product Scope

### 3.1 Model authoring

The library should support composable models with a small, stable core API:

- `Module`: owns parameters, buffers, child modules, training/evaluation mode,
  serialization, and device/dtype metadata;
- `Parameter`: a trainable tensor with gradient and optimizer state;
- `Sequential`, residual blocks, branching/merging graphs, and named modules;
- explicit `forward` methods and a functional API for stateless operations;
- parameter freezing, sharing, tying, initialization, and parameter groups; and
- deterministic random seeds and reproducible initialization.

The API should support both eager execution for development and a compiled
execution path for deployment. Graph capture or compilation may be introduced
after eager semantics are stable; it must not be a hidden requirement for basic
model authoring.

### 3.2 Common neural-network families

The standard library should provide tested building blocks for the following
families. Each family is a deliverable only when it has reference examples,
shape tests, gradient tests, serialization tests, and at least one end-to-end
example.

**Vision**

- dense and convolutional layers, pooling, normalization, dropout, residual
  connections, attention, and positional encodings;
- CNN, ResNet-like, U-Net-like, Vision Transformer, and image autoencoder
  components;
- image classification, multilabel classification, object detection, semantic
  segmentation, image generation, and image embedding examples.

**Text and language**

- embeddings, recurrent layers, gated recurrent layers, one-dimensional
  convolutions, self-attention, cross-attention, masking, and transformer blocks;
- text classification, sequence classification, sequence-to-sequence models,
  language modeling, text generation, question answering, and text embedding;
- greedy, beam, top-k, nucleus, temperature, repetition-penalty, and streaming
  generation utilities.

**Audio and video**

- spectrogram and mel-spectrogram transforms, waveform models, temporal
  convolution, recurrent and transformer sequence models;
- video frame decoding, frame sampling, temporal batching, 2D/3D convolution,
  optical-flow-ready data interfaces, and video transformers;
- speech/audio classification, transcription-ready preprocessing, audio
  embeddings, video classification, and multimodal audio-video examples.

**General-purpose models**

- multilayer perceptrons, tabular preprocessing, categorical embeddings,
  autoencoders, variational autoencoders, contrastive learning, and multimodal
  fusion;
- configurable custom layers and custom loss/metric functions written in Hoo.

The first release should focus on dense/conv/recurrent/attention primitives and
classification, embedding, and language-modeling examples. Detection,
segmentation, generative, and multimodal components should follow the same
interfaces rather than being built as unrelated special cases.

### 3.3 Automatic differentiation

Introduce a documented `grad` API after the scalar prototype is proven. The
preferred initial shape is a function that returns a value and a gradient
structure, for example:

```hoo
func :f64 loss_fn(x: f64, w: f64) {
    return (w * x - 1.0) * (w * x - 1.0);
}

var value_and_grad = grad(loss_fn, wrt: [1]);
```

The exact syntax is subject to grammar design; this example is illustrative
until the language feature is specified. The specification must define:

- gradients with respect to scalar values, tensors, `Parameter`s, and nested
  module fields;
- scalar-loss requirements and behavior for vector-valued outputs;
- reverse mode, forward mode, and the boundary at which each is selected;
- control flow, loops, function calls, closures, mutation, aliasing, and
  checkpointing/rematerialization;
- nondifferentiable operations and explicit stop-gradient behavior;
- gradient dtype and accumulation precision, especially for `f8` and `f16`
  storage; and
- error messages for unsupported or invalid differentiation graphs.

The implementation may begin with an operator-tape or trace-based prototype if
that produces correct gradients. A source-to-source or compiler IR transform
can then remove graph-building overhead for supported pure functions. “Zero
runtime overhead” is not an acceptance criterion until it is demonstrated by
benchmarks against the eager implementation.

### 3.4 Tensor and dtype system

Extend the existing tensor system without breaking its current API:

- keep the initial `hoort` target at rank 1/2/3 and add arbitrary-rank tensors
  only through a versioned ABI extension or a separate higher-level tensor
  representation;
- define broadcasting, reductions, strides, views, contiguity, layout, and
  device placement;
- add reductions needed for training: `sum`, `mean`, `max`, `argmax`, norms,
  and batch reductions;
- add convolution, padding, gather/scatter, concatenate, stack, split, mask,
  indexing, and attention primitives;
- add `f16`/`bf16` only after defining their C ABI element IDs, storage,
  conversion, accumulation, serialization, and HVM fallback semantics;
- preserve higher-precision accumulation for low-precision parameters where
  required; and
- provide explicit CPU, HVM, and accelerator capability queries.

The existing Hoo grammar accepts literal dimensions in `tensor<T>[N, M, ...]`
types and the runtime also stores dynamic dimensions. Shape inference must
distinguish those compile-time literals from runtime dimensions. Invalid shapes
should produce a compile-time diagnostic when provable and a typed runtime
error otherwise; silently returning `null` is not sufficient for the training
API. Any new dynamic or arbitrary-rank representation must continue to cross
the HVM boundary as an opaque 64-bit pointer and must not change existing
`HooTensor` layout assumptions.

#### 3.4.0 Proposed `hoort` ABI expansion

The ANN library needs a versioned C ABI underneath the Hoo intrinsic type. The
ABI must extend the existing `hoo_tensor_*` functions without changing their
symbol signatures or the meaning of their current rank-1/rank-2/rank-3 calls.
The following rules apply to all new APIs:

- Every public handle is opaque (`void*` at the C boundary) and is passed as a
  64-bit HVM pointer. C++ class layout, STL containers, exceptions, and
  ownership details never cross the boundary.
- Every managed handle is allocated and released through the existing ARC
  mechanism. New handle types require stable type IDs, registered destruction,
  retain/release tests, and no collision with Buffer or legacy Tensor IDs.
- New functions return a `HooStatus` and write results through explicit output
  pointers. `null` remains a valid failure result for legacy functions, but new
  functions must distinguish invalid input, unsupported capability, allocation
  failure, I/O failure, and numerical failure.
- ABI version and feature queries are mandatory. A caller must be able to test
  support before using a new dtype, rank, layout, device, operator, or file
  format.
- Input buffers and descriptor arrays are borrowed for the duration of a call
  unless a function explicitly documents ownership transfer. A function that
  retains a borrowed tensor or byte buffer must call `hoo_retain`.
- All dimensions, strides, lengths, byte offsets, and element counts use
  signed 64-bit values with checked overflow. Negative dimensions and invalid
  strides are rejected before allocation.

#### 3.4.0.1 Implemented ABI v2 foundation

The first implementation slice uses the existing opaque `HooTensor` handle and
adds versioned functions without exposing its private C++ representation:

- `hoo_ai.h/.cpp` provides `HooStatus`, ABI version, thread-local error state,
  and capability queries.
- `hoo_tensor_new_ex` supports rank 1–64 with checked dimensions and storage
  sizes. ABI v2 currently implements `f32` and `int32` in addition to the
  legacy dtypes; `f16` and `bf16` remain capability-negative.
- Shape, stride, `numel`, ABI-version, and copy queries return explicit status
  values. Contiguous strides are bytes for byte-addressable types and bits for
  packed `bit` tensors.
- New tensors use runtime type ID `131`; Buffer remains `113`. Destruction frees
  owned metadata/storage and retains any future view base.
- HVM/JIT tensor arithmetic calls runtime kernels and does not use a private
  payload offset. This is the required portable fallback while optimized
  tensor lowering is developed.

The remaining descriptor, view, device, operator, parameter, autograd, data,
and model-format APIs in this section are planned extensions. The selected
direction is to evolve `HooTensor` through versioned ABI additions; a separate
`HooTensorND` handle remains an alternative only if future ownership or device
requirements make in-place evolution unsafe.

##### ABI version, status, and capability functions

The following names are proposed for `hoo_runtime.h`/new ANN headers:

```cpp
typedef int32_t HooStatus;

enum {
    HOO_STATUS_OK = 0,
    HOO_STATUS_INVALID_ARGUMENT = 1,
    HOO_STATUS_INVALID_SHAPE = 2,
    HOO_STATUS_INVALID_DTYPE = 3,
    HOO_STATUS_UNSUPPORTED = 4,
    HOO_STATUS_OUT_OF_MEMORY = 5,
    HOO_STATUS_OUT_OF_BOUNDS = 6,
    HOO_STATUS_DEVICE_UNAVAILABLE = 7,
    HOO_STATUS_IO_ERROR = 8,
    HOO_STATUS_FORMAT_ERROR = 9,
    HOO_STATUS_NUMERICAL_ERROR = 10,
    HOO_STATUS_CANCELLED = 11
};

int32_t hoo_ai_abi_version(void);
HooStatus hoo_ai_last_status(void);
const char* hoo_ai_last_error(void); /* borrowed, thread-local */
int64_t hoo_ai_has_feature(const char* feature_name);
HooStatus hoo_ai_get_capabilities(HooBuffer* encoded_capabilities);
```

The error string is diagnostic only; programs must branch on `HooStatus` or a
capability query. The C ABI must not throw C++ exceptions across Hoo/HVM calls.

##### Tensor handle, descriptor, and dtype expansion

`HooTensor` remains the legacy handle. The new `HooTensorND` handle supports
the same scalar/tensor language type with dynamic rank and metadata:

```cpp
typedef void* HooTensorND;
typedef void* HooTensorDesc;
typedef void* HooDevice;

enum HooTensorDType {
    HOO_DTYPE_INT64 = 1,  HOO_DTYPE_F64 = 2,
    HOO_DTYPE_INT8  = 5,  HOO_DTYPE_BYTE = 6,
    HOO_DTYPE_BIT   = 8,  HOO_DTYPE_F8 = 9,
    /* New IDs are appended; existing IDs must never be reused. */
    HOO_DTYPE_F32   = 16, HOO_DTYPE_F16 = 17,
    HOO_DTYPE_BF16  = 18, HOO_DTYPE_INT32 = 19
};

HooStatus hoo_tensor_nd_new(int32_t dtype, int64_t rank,
                             const int64_t* dims, HooDevice device,
                             HooTensorND* out);
HooStatus hoo_tensor_nd_retain(HooTensorND tensor);
HooStatus hoo_tensor_nd_release(HooTensorND tensor);
HooStatus hoo_tensor_nd_rank(HooTensorND tensor, int64_t* out_rank);
HooStatus hoo_tensor_nd_shape(HooTensorND tensor, int64_t capacity,
                              int64_t* dims, int64_t* out_count);
HooStatus hoo_tensor_nd_strides(HooTensorND tensor, int64_t capacity,
                                int64_t* strides, int64_t* out_count);
HooStatus hoo_tensor_nd_dtype(HooTensorND tensor, int32_t* out_dtype);
HooStatus hoo_tensor_nd_device(HooTensorND tensor, HooDevice* out_device);
HooStatus hoo_tensor_nd_numel(HooTensorND tensor, int64_t* out_numel);
HooStatus hoo_tensor_nd_contiguous(HooTensorND tensor,
                                   HooTensorND* out_tensor);
HooStatus hoo_tensor_nd_view(HooTensorND tensor, int64_t rank,
                             const int64_t* dims, const int64_t* strides,
                             int64_t offset_bytes, HooTensorND* out);
HooStatus hoo_tensor_nd_copy(HooTensorND source, HooDevice device,
                             HooTensorND* out);
HooStatus hoo_tensor_nd_get_bits(HooTensorND tensor, int64_t index,
                                 int64_t* out_bits);
HooStatus hoo_tensor_nd_set_bits(HooTensorND tensor, int64_t index,
                                 int64_t bits);
```

The descriptor and view functions must define row-major default layout, byte
strides, bit packing order, alignment, aliasing, and whether a view keeps its
base tensor retained. `HooTensorND` rank 1/2/3 values may be converted to the
legacy `HooTensor` only when dtype, contiguous layout, device, and dimensions
are representable by the old ABI. No implicit conversion may truncate a rank or
dimension.

The Hoo type system must map the intrinsic tensor to these handles deliberately:

- existing `tensor<T>[N, ...]` continues to use the legacy path when
  representable;
- a proposed dynamic form such as `tensor<T>[]` maps to `HooTensorND` only after
  grammar, type checking, code generation, and JIT tests are added; and
- rank-4/5 image/video values use `HooTensorND`, never an undocumented extension
  of the three-dimension legacy payload.

##### Tensor operator ABI

All differentiable tensor operations must have a runtime entry point with a
consistent status/output convention. At minimum the ABI must cover:

```cpp
HooStatus hoo_tensor_nd_unary(HooTensorND input, int32_t op,
                              HooTensorND* out);
HooStatus hoo_tensor_nd_binary(HooTensorND left, HooTensorND right,
                               int32_t op, HooTensorND* out);
HooStatus hoo_tensor_nd_scalar(HooTensorND input, int32_t op,
                               int32_t scalar_dtype, int64_t scalar_bits,
                               HooTensorND* out);
HooStatus hoo_tensor_nd_reduce(HooTensorND input, int32_t op,
                               const int64_t* axes, int64_t axis_count,
                               int32_t keepdims, HooTensorND* out);
HooStatus hoo_tensor_nd_matmul(HooTensorND left, HooTensorND right,
                               HooTensorND* out);
HooStatus hoo_tensor_nd_conv(HooTensorND input, HooTensorND weight,
                             HooTensorND bias, const int64_t* stride,
                             const int64_t* padding, int64_t spatial_rank,
                             HooTensorND* out);
HooStatus hoo_tensor_nd_index(HooTensorND input, HooTensorND indices,
                              int32_t op, int64_t axis, HooTensorND* out);
```

Operation IDs, shape rules, broadcasting rules, output dtype, accumulation
dtype, NaN/Inf behavior, and gradient rules must be versioned in an operator
registry. A generic operation ID must not be used to hide incompatible calling
conventions. Native optimized implementations may replace these calls only
after producing the same result as the reference `hoort` implementation.

##### Device and transfer ABI

```cpp
HooStatus hoo_device_get(const char* kind, int64_t index, HooDevice* out);
HooStatus hoo_device_retain(HooDevice device);
HooStatus hoo_device_release(HooDevice device);
HooStatus hoo_device_kind(HooDevice device, const char** out_kind);
HooStatus hoo_device_available(HooDevice device, int64_t* out_available);
HooStatus hoo_device_supports(HooDevice device, int32_t dtype,
                              int32_t op, int64_t* out_supported);
HooStatus hoo_tensor_nd_to_device(HooTensorND input, HooDevice device,
                                  HooTensorND* out);
HooStatus hoo_device_synchronize(HooDevice device);
```

CPU and ordinary HVM runtime execution are mandatory. HVM-V, HVM-A, GPU, and
vendor devices are optional and must return `HOO_STATUS_UNSUPPORTED` or
`HOO_STATUS_DEVICE_UNAVAILABLE` rather than trap or silently run on another
device. Device transfers must retain source data until completion and define
synchronous behavior for the initial implementation.

##### Parameter, module, and autograd ABI

Hoo-defined classes may implement these concepts in Hoo, but a stable C ABI is
needed for runtime ownership, model import/export, and native adapters:

```cpp
typedef void* HooParameter;
typedef void* HooModule;
typedef void* HooGradTape;

HooStatus hoo_parameter_new(HooTensorND value, int32_t requires_grad,
                            HooParameter* out);
HooStatus hoo_parameter_value(HooParameter parameter, HooTensorND* out);
HooStatus hoo_parameter_grad(HooParameter parameter, HooTensorND* out);
HooStatus hoo_parameter_set_value(HooParameter parameter, HooTensorND value);
HooStatus hoo_parameter_set_grad(HooParameter parameter, HooTensorND grad);
HooStatus hoo_parameter_zero_grad(HooParameter parameter);
HooStatus hoo_module_register_parameter(HooModule module, const char* name,
                                        HooParameter parameter);
HooStatus hoo_module_register_buffer(HooModule module, const char* name,
                                     HooTensorND value);
HooStatus hoo_module_register_child(HooModule module, const char* name,
                                    HooModule child);
HooStatus hoo_module_set_training(HooModule module, int32_t training);
HooStatus hoo_module_state_size(HooModule module, int64_t* out_count);
HooStatus hoo_module_state_item(HooModule module, int64_t index,
                                HooBuffer* out_name, HooTensorND* out_value);

HooStatus hoo_grad_tape_new(HooGradTape* out);
HooStatus hoo_grad_tape_watch(HooGradTape tape, HooTensorND value);
HooStatus hoo_grad_tape_backward(HooGradTape tape, HooTensorND loss,
                                 HooTensorND seed_gradient);
HooStatus hoo_grad_tape_gradient(HooGradTape tape, HooTensorND value,
                                 HooTensorND* out_gradient);
HooStatus hoo_grad_tape_clear(HooGradTape tape);
HooStatus hoo_grad_tape_release(HooGradTape tape);
```

The tape must specify whether it records runtime calls, compiler-generated
operations, or both; how in-place mutation and aliasing are handled; whether a
graph is reusable; and how unsupported operations are reported. `f64` master
weights/gradients are the initial fallback for `f8` and future low-precision
storage.

##### Dataset, stream, and model-format ABI

The initial data ABI should be pull-based so it does not depend on C++ virtual
classes or function-pointer callbacks crossing the HVM boundary:

```cpp
typedef void* HooDataset;
typedef void* HooDataLoader;
typedef void* HooStream;
typedef void* HooModelReader;
typedef void* HooModelWriter;

HooStatus hoo_dataset_length(HooDataset dataset, int64_t* out_length);
HooStatus hoo_dataset_get(HooDataset dataset, int64_t index,
                          HooBuffer* out_sample);
HooStatus hoo_loader_new(HooDataset dataset, int64_t batch_size,
                         int32_t shuffle, HooDataLoader* out);
HooStatus hoo_loader_next(HooDataLoader loader, HooBuffer* out_batch,
                          int64_t* out_has_value);
HooStatus hoo_loader_reset(HooDataLoader loader, int64_t seed);
HooStatus hoo_stream_next(HooStream stream, HooBuffer* out_chunk,
                          int64_t* out_has_value);
HooStatus hoo_stream_cancel(HooStream stream);
HooStatus hoo_model_reader_open(const char* path, const char* format,
                                HooModelReader* out);
HooStatus hoo_model_reader_next_tensor(HooModelReader reader,
                                       HooBuffer* out_name,
                                       HooTensorND* out_tensor,
                                       int64_t* out_has_value);
HooStatus hoo_model_reader_verify(HooModelReader reader);
HooStatus hoo_model_writer_open(const char* path, const char* format,
                                HooModelWriter* out);
HooStatus hoo_model_writer_write_tensor(HooModelWriter writer,
                                        const char* name,
                                        HooTensorND tensor);
HooStatus hoo_model_writer_close(HooModelWriter writer);
```

Stream cancellation, worker shutdown, bounded buffering, and borrowed-buffer
lifetime must be tested. Model readers must parse into temporary state, validate
all names/shapes/dtypes/checksums, and only then mutate a module. They must never
execute code embedded in a model file.

#### 3.4.1 Minimum tensor kernel and ABI expansion

The following kernel groups are prerequisites for the advertised model families;
they are not optional convenience functions:

- unary numerics: `exp`, `log`, `log1p`, `sqrt`, `rsqrt`, `abs`, `negate`,
  `sign`, `pow`, `maximum`, `minimum`, `clamp`, `isfinite`, and stable
  `softplus`/`logsumexp`;
- reductions: sum, mean, variance, standard deviation, min/max, argmin/argmax,
  and reductions over selected axes with `keepdims`;
- shape/layout: arbitrary-rank shape metadata, strides, reshape/view, transpose,
  permute, squeeze/unsqueeze, contiguous copy, broadcast, split, concatenate,
  stack, and flatten;
- indexing: slice, gather, scatter, take, masked select, masked fill, and
  bounds-checked index updates;
- linear algebra: rank-2 matmul, batched matmul, transpose matmul, dot, and
  reductions with defined accumulation dtype;
- neural kernels: 1D/2D/3D convolution, pooling, normalization, padding,
  embedding lookup, one-hot, causal masks, and attention score/mask kernels; and
- randomness: a reproducible, thread-safe generator with explicit seed/state
  serialization and independent streams for initialization, dropout, and data
  shuffling.

The existing three-dimension `HooTensorHeader` cannot represent all of these
operations. Before implementing rank-4 image batches or rank-5 video batches,
choose one compatible design and specify it in the tensor ABI:

1. version the managed tensor payload to include rank, dynamically allocated
   dimensions, strides, layout, device, and storage/accumulation dtype; or
2. retain the existing `HooTensor` for rank 1/2/3 and introduce a separate
   opaque `HooTensorND` handle with explicit conversion rules.

The choice must preserve old binaries, define ownership for views, and provide
interpreter/JIT/runtime implementations. A class named `Conv2D` or
`VideoTransform` is not implementable until its input rank and layout contract
exists.

#### 3.4.2 Callable values and structured model values

The proposed signatures `grad(fn: any, ...)`, `fit(..., loss: any)`, and custom
Hoo transforms require language features that are not yet specified. `any` must
not be treated as an executable function pointer. Phase 0 must choose one of:

- introduce a first-class `Function`/closure value with captured environment,
  parameter and return-type metadata, invocation, lifetime, and a safe HVM
  representation; or
- define `grad`/`fit` as compiler intrinsics that accept statically named Hoo
  functions and reject dynamic callables until first-class functions exist.

The model API must also specify typed representations for multiple inputs,
multiple outputs, recurrent state, masks, and named outputs. An untyped `Array`
or `any` may be used as a boundary adapter, but it is not sufficient as the
internal representation for gradient propagation, module signatures, or model
format validation. Add a typed tuple/record value or a documented tagged-value
schema before claiming general functional-model compatibility.

### 3.5 Optimizers, losses, and training

Provide a framework-independent training loop and reusable implementations of:

- SGD, momentum, Adam, AdamW, Adagrad, RMSProp, and learning-rate schedulers;
- gradient clipping, accumulation, scaling, mixed-precision loss scaling, and
  exponential moving averages;
- cross-entropy, binary cross-entropy, focal, mean-squared-error, L1, smooth-L1,
  cosine, contrastive, triplet, CTC-ready, and KL-divergence losses; and
- accuracy, precision, recall, F1, confusion matrix, perplexity, BLEU/ROUGE
  adapters, embedding similarity, and task-specific metric hooks.

Training utilities must support minibatches, validation, early stopping,
callbacks, checkpoints, resuming, distributed-training hooks, and structured
logging. Distributed execution is initially an extension point; a single
process CPU/HVM implementation is the minimum viable backend.

### 3.6 Data preparation and streaming

Create a `hoo.data` API based on composable datasets and transforms:

- `Dataset`, `IterableDataset`, `DataLoader`, samplers, sharding, batching,
  prefetching, worker configuration, caching, shuffling, padding, and bucketing;
- text decoding, Unicode normalization, sentence/word/subword tokenization,
  vocabulary building, special tokens, truncation, packing, and attention masks;
- image decoding, resize/crop/pad, normalization, color conversion, augmentation,
  labels, bounding boxes, masks, and image-to-token patchification;
- video containers, frame decoding, timestamp/frame sampling, clip creation,
  variable-length batching, and stream backpressure;
- audio decoding, resampling, channel conversion, normalization, framing,
  spectrograms, mel features, and streaming chunk/window processing;
- CSV, JSON/JSONL, text files, binary buffers, directory datasets, and a
  documented adapter interface for external storage; and
- schema validation, missing-value handling, train/validation/test splitting,
  normalization statistics, and data provenance.

I/O and codec implementations may use native libraries behind a narrow Hoo API.
“Pure Hoo” should apply to model composition and transforms where practical, not
to every image, video, or audio codec.

### 3.7 Model formats and pretrained weights

Implement a versioned checkpoint format for Hoo models containing:

- model architecture/configuration or a registered model identifier;
- named parameters and non-parameter buffers;
- dtype, shape, layout, device, and endianness metadata;
- optimizer state, scheduler state, random state, training step, and metrics;
- schema/version information, checksums, and optional compression; and
- safe loading rules that do not execute arbitrary code from a checkpoint.

Add import/export adapters in this order:

1. Hoo’s own checkpoint format;
2. NumPy `.npy`/`.npz` tensors and common raw tensor containers;
3. ONNX graphs and initializers for interoperable inference;
4. SafeTensors for safe named-weight exchange;
5. framework-specific adapters for PyTorch state dictionaries and TensorFlow/
   Keras SavedModel or weight checkpoints, where licensing and dependency
   boundaries permit; and
6. optional GGUF or other quantized formats for supported inference models.

Each adapter must document supported operators, layouts, dtypes, quantization,
dynamic dimensions, unsupported graph nodes, and whether the result is
training-capable or inference-only. Importing a model must validate tensor names,
shapes, dtypes, and checksums before mutating a live model.

### 3.8 Execution and hardware backends

Use a layered backend design:

- a correct reference CPU/runtime implementation;
- HVM lowering for scalar and tensor operations with explicit ownership and
  bounds semantics;
- LLVM CPU optimization only after generated IR contains analyzable tensor
  loops or calls to well-specified vector primitives;
- HVM-V or equivalent vector operations where the target profile supports them;
- optional native BLAS, convolution, or FFT integrations behind capability-based
  dispatch; and
- future GPU/accelerator backends with explicit device memory, kernel launch,
  synchronization, fallback, and error semantics.

LLVM CPU vectorization must not be described as automatic GPU support. AMX,
CUDA/Hopper, Metal, Vulkan, and other accelerators require separate lowering or
integration work and must have a portable fallback.

For binary neural networks, define multiplication separately from ordinary
numeric matmul. The initial contract should specify bit packing order, padding,
XNOR behavior, accumulation type, output encoding, and the fallback algorithm.
For example, a signed binary dot product may be defined as
`2 * popcount(XNOR(a, b)) - K`, accumulated in `int32` or `int64`.

## 4. Proposed Hoo Package Layout

```text
hoo.tensor       tensor types, layouts, dtypes, indexing, reductions
hoo.nn           modules, parameters, layers, activations, model utilities
hoo.autograd     grad, jacobian, vjp/jvp, stop-gradient, checkpointing
hoo.loss         differentiable loss functions
hoo.optim        optimizers, schedulers, gradient scaling and clipping
hoo.metrics      reusable metrics and evaluation utilities
hoo.data         datasets, loaders, transforms, batching and streaming
hoo.text         tokenizers, vocabularies, text normalization and masks
hoo.vision       image/video transforms and vision model components
hoo.audio        waveform and spectrogram transforms
hoo.models       reference architectures and model registries
hoo.io           checkpoints, SafeTensors, ONNX and other format adapters
hoo.device       backend/device discovery and placement
```

The package boundaries should keep codecs, file formats, and optional native
dependencies out of the minimal compiler/runtime installation.

## 5. Proposed Public API

This section is the API contract proposal for the first library release. Names
and signatures are subject to grammar validation before implementation, but the
ownership, error, and capability rules are normative for the design.

### 5.1 Hoo API conventions

- Hoo classes have one `constructor` declaration. Alternative construction
  paths use module-level free functions such as `tokenizer_from_file` or
  `tokenizer_from_buffer`.
- Class methods are instance methods declared with `func`; the current grammar
  has no general `static` method modifier. Factories and stateless operations
  therefore remain free functions.
- `public` and `private` visibility must be used consistently. Parameters and
  buffers exposed by the API are ARC-managed opaque objects; callers do not
  access C++ implementation fields.
- `Tensor` means the existing `hoort` tensor object. Initial APIs are limited to
  rank 1/2/3 and current element types. APIs requiring rank 4/5 tensors are
  gated until the tensor ABI is extended.
- Operations that can fail return `null` or a documented status/result object;
  invalid shapes, dtypes, devices, and model files must also produce a useful
  runtime diagnostic. No API may silently reinterpret an incompatible tensor.
- The signatures below use `Tensor`, `Array`, `Buffer`, `string`, `bool`,
  `int64`, `double`, and `any` in the style of the existing Hoo APIs. Exact
  generic collection syntax must be validated against `Hooc.g4` before landing.

### 5.2 Tensor and numerical free functions

The existing `hoo.tensor` class methods remain the low-level ABI. These free
functions provide the NN layer with predictable factories and numerical
utilities:

```hoo
func :Tensor tensor_zeros(shape: Array, element_type: int64);
func :Tensor tensor_ones(shape: Array, element_type: int64);
func :Tensor tensor_full(shape: Array, value: double, element_type: int64);
func :Tensor tensor_random_uniform(shape: Array, low: double, high: double,
                                   seed: int64);
func :Tensor tensor_random_normal(shape: Array, mean: double, stddev: double,
                                  seed: int64);
func :Tensor tensor_clone(value: Tensor);
func :Tensor tensor_cast(value: Tensor, element_type: int64);
func :Tensor tensor_concat(left: Tensor, right: Tensor, axis: int64);
func :Tensor tensor_stack(values: Array, axis: int64);
func :Tensor tensor_sum(value: Tensor, axis: int64);
func :Tensor tensor_mean(value: Tensor, axis: int64);
func :Tensor tensor_max(value: Tensor, axis: int64);
func :Tensor tensor_argmax(value: Tensor, axis: int64);
func :Tensor tensor_norm(value: Tensor, p: double);
func :Tensor tensor_gather(value: Tensor, indices: Tensor, axis: int64);
func :Tensor tensor_where(mask: Tensor, yes: Tensor, no: Tensor);
func :Tensor tensor_pad(value: Tensor, before: Array, after: Array,
                        value: double);
func :Tensor tensor_reduce_sum(value: Tensor, axis: int64);
func :Tensor tensor_reduce_mean(value: Tensor, axis: int64);
func :bool tensor_all_close(left: Tensor, right: Tensor,
                            atol: double, rtol: double);
func :int64 tensor_numel(value: Tensor);
func :bool tensor_is_contiguous(value: Tensor);
```

The first implementation may expose only operations supported by the current
rank-1/rank-2/rank-3 `hoort` ABI. `tensor_concat`, `tensor_stack`, `gather`,
padding, and reductions must return a typed unsupported-operation error until
their runtime C ABI is implemented.

### 5.3 Autograd and training free functions

```hoo
func :any grad(fn: any, inputs: Array);
func :any value_and_grad(fn: any, inputs: Array);
func :any jacobian(fn: any, inputs: Array);
func :Tensor stop_gradient(value: Tensor);
func :void zero_grad(module: Module);
func :void backward(loss: Tensor);
func :double clip_grad_norm(module: Module, max_norm: double);
func :void clip_grad_value(module: Module, min_value: double,
                           max_value: double);
func :void train_mode(module: Module);
func :void eval_mode(module: Module);
func :bool is_training(module: Module);
func :void optimizer_step(optimizer: Optimizer);
func :void optimizer_zero_grad(optimizer: Optimizer);
func :void scheduler_step(scheduler: Scheduler);
func :TrainingResult fit(module: Module, loader: DataLoader,
                         optimizer: Optimizer, epochs: int64,
                         loss: any);
func :EvaluationResult evaluate(module: Module, loader: DataLoader,
                                metrics: Array);
func :Tensor predict(module: Module, inputs: Tensor);
```

`grad` and `value_and_grad` must define how `inputs` maps to tensors or
`Parameter`s, how nested results are represented, and how failures are
reported. `backward` is only valid for a scalar loss unless an explicit seed
gradient is supplied. Gradient accumulation defaults to `f64` where the
underlying parameter uses `f8`, `int8`, or `byte` storage.

#### `TrainingResult` and `EvaluationResult`

These result classes make training diagnostics explicit instead of requiring
callers to unpack an untyped `any` value:

```hoo
class TrainingResult {
    constructor(loss: double, epoch: int64, step: int64) {}
    func :double loss();
    func :int64 epoch();
    func :int64 step();
    func :Array history();
    func :bool converged();
}

class EvaluationResult {
    constructor(metrics: Array, samples: int64) {}
    func :Array metrics();
    func :int64 samples();
    func :double metric(name: string);
    func :bool contains(name: string);
}
```

### 5.4 Core model classes

#### `Module`

Base class for every trainable or composable model. It has no required state.

```hoo
class Module {
    constructor() {}
    func :Tensor forward(input: Tensor);
    func :Array parameters();
    func :Array named_parameters(prefix: string);
    func :Array buffers();
    func :Array children();
    func :void train();
    func :void eval();
    func :bool training();
    func :void requires_grad(value: bool);
    func :Array state_dict();
    func :bool load_state_dict(state: Array, strict: bool);
    func :string module_name();
}
```

`forward` is the convention for subclasses. Because the current language does
not declare abstract methods, the base implementation must either be a tested
error path or the compiler must add an explicit abstract-method rule.

#### `Parameter`

```hoo
class Parameter {
    constructor(value: Tensor, requires_grad: bool) {}
    func :Tensor value();
    func :Tensor grad();
    func :void set_value(value: Tensor);
    func :void set_grad(value: Tensor);
    func :bool requires_grad();
    func :void zero_grad();
    func :string name();
    func :int64 version();
}
```

#### `Sequential`

```hoo
class Sequential extends Module {
    constructor(layers: Array) {}
    func :Tensor forward(input: Tensor);
    func :void add(module: Module);
    func :Module at(index: int64);
    func :int64 length();
    func :Array children();
}
```

#### `Device`

```hoo
class Device {
    constructor(kind: string, index: int64) {}
    func :string kind();
    func :int64 index();
    func :bool available();
    func :bool supports(element_type: int64);
    func :string name();
}

func :Device cpu_device();
func :Device hvm_device();
func :Device accelerator_device(index: int64);
func :Array available_devices();
```

### 5.5 Layer classes

All layer classes inherit `Module` and therefore provide `forward`,
`parameters`, `state_dict`, `load_state_dict`, `train`, and `eval`. Their
constructors and additional methods are:

```hoo
class Linear extends Module {
    constructor(input_features: int64, output_features: int64, bias: bool) {}
    func :Tensor forward(input: Tensor);
    func :Parameter weight();
    func :Parameter bias();
}

class Conv2D extends Module {
    constructor(input_channels: int64, output_channels: int64,
                kernel_height: int64, kernel_width: int64,
                stride: int64, padding: int64, bias: bool) {}
    func :Tensor forward(input: Tensor);
    func :Parameter weight();
    func :Parameter bias();
}

class Pool2D extends Module {
    constructor(kernel: int64, stride: int64, mode: string) {}
    func :Tensor forward(input: Tensor);
}

class Flatten extends Module {
    constructor(start_axis: int64, end_axis: int64) {}
    func :Tensor forward(input: Tensor);
}

class Reshape extends Module {
    constructor(shape: Array) {}
    func :Tensor forward(input: Tensor);
    func :Array shape();
}

class BatchNorm extends Module {
    constructor(features: int64, epsilon: double, momentum: double) {}
    func :Tensor forward(input: Tensor);
    func :Parameter weight();
    func :Parameter bias();
    func :Tensor running_mean();
    func :Tensor running_variance();
}

class LayerNorm extends Module {
    constructor(features: int64, epsilon: double) {}
    func :Tensor forward(input: Tensor);
    func :Parameter weight();
    func :Parameter bias();
}

class Dropout extends Module {
    constructor(probability: double, seed: int64) {}
    func :Tensor forward(input: Tensor);
    func :double probability();
}

class Embedding extends Module {
    constructor(vocabulary_size: int64, embedding_size: int64) {}
    func :Tensor forward(indices: Tensor);
    func :Parameter weight();
}

class MultiHeadAttention extends Module {
    constructor(model_size: int64, heads: int64, dropout: double) {}
    func :Tensor forward(query: Tensor, key: Tensor, value: Tensor,
                         mask: Tensor);
    func :Tensor causal_mask(length: int64);
}

class RNN extends Module {
    constructor(input_size: int64, hidden_size: int64, layers: int64,
                bidirectional: bool) {}
    func :Tensor forward(input: Tensor, hidden: Tensor);
    func :Tensor initial_hidden(batch: int64);
}

class LSTM extends Module {
    constructor(input_size: int64, hidden_size: int64, layers: int64,
                bidirectional: bool) {}
    func :Array forward_state(input: Tensor, hidden: Tensor, cell: Tensor);
    func :Array initial_state(batch: int64);
}

class GRU extends Module {
    constructor(input_size: int64, hidden_size: int64, layers: int64,
                bidirectional: bool) {}
    func :Array forward_state(input: Tensor, hidden: Tensor);
    func :Tensor initial_hidden(batch: int64);
}

class TransformerEncoderLayer extends Module {
    constructor(model_size: int64, heads: int64, feedforward_size: int64,
                dropout: double) {}
    func :Tensor forward(input: Tensor, mask: Tensor);
}

class TransformerDecoderLayer extends Module {
    constructor(model_size: int64, heads: int64, feedforward_size: int64,
                dropout: double) {}
    func :Tensor forward(input: Tensor, memory: Tensor,
                         target_mask: Tensor, memory_mask: Tensor);
}

class Residual extends Module {
    constructor(module: Module) {}
    func :Tensor forward(input: Tensor);
    func :Module inner();
}
```

The initial tensor target can represent common batched sequence data as rank-3
`[batch, sequence, features]` tensors. `Conv2D` input layout and rank must be
specified before implementation. Video models that require rank-4/5 tensors
must use a documented frame/clip adapter or wait for the tensor ABI extension.

Free activation functions complement the layer classes:

```hoo
func :Tensor relu(input: Tensor);
func :Tensor sigmoid(input: Tensor);
func :Tensor tanh(input: Tensor);
func :Tensor gelu(input: Tensor);
func :Tensor softmax(input: Tensor, axis: int64);
func :Tensor log_softmax(input: Tensor, axis: int64);
func :Tensor dropout(input: Tensor, probability: double, training: bool,
                     seed: int64);
```

### 5.6 Optimizer and scheduler classes

```hoo
class Optimizer {
    constructor(parameters: Array, learning_rate: double) {}
    func :void step();
    func :void zero_grad();
    func :Array state_dict();
    func :bool load_state_dict(state: Array, strict: bool);
    func :void set_learning_rate(value: double);
    func :double learning_rate();
    func :Array parameters();
}

class SGD extends Optimizer {
    constructor(parameters: Array, learning_rate: double) {}
    func :void step();
    func :void set_momentum(value: double);
}

class Adam extends Optimizer {
    constructor(parameters: Array, learning_rate: double) {}
    func :void step();
    func :void set_betas(beta1: double, beta2: double);
    func :void set_epsilon(value: double);
}

class AdamW extends Adam {
    constructor(parameters: Array, learning_rate: double) {}
    func :void set_weight_decay(value: double);
}

class Scheduler {
    constructor(optimizer: Optimizer) {}
    func :void step();
    func :double learning_rate();
    func :Array state_dict();
    func :bool load_state_dict(state: Array, strict: bool);
}

class StepScheduler extends Scheduler {
    constructor(optimizer: Optimizer, step_size: int64, gamma: double) {}
    func :void step();
}

class ReduceOnPlateau extends Scheduler {
    constructor(optimizer: Optimizer, factor: double, patience: int64) {}
    func :void step_metric(metric: double);
}
```

Because Hoo currently permits only one constructor per class, additional
optimizer options should be configured with methods or free factory functions,
not constructor overloads.

### 5.7 Loss and metric classes/functions

Losses are stateless free functions initially; class forms are used when a loss
has configuration:

```hoo
func :Tensor mse_loss(input: Tensor, target: Tensor);
func :Tensor l1_loss(input: Tensor, target: Tensor);
func :Tensor cross_entropy(logits: Tensor, target: Tensor);
func :Tensor binary_cross_entropy(logits: Tensor, target: Tensor);
func :Tensor cosine_loss(input: Tensor, target: Tensor);
func :Tensor kl_divergence(input: Tensor, target: Tensor);

class FocalLoss {
    constructor(gamma: double, alpha: double) {}
    func :Tensor forward(input: Tensor, target: Tensor);
}

class ContrastiveLoss {
    constructor(margin: double) {}
    func :Tensor forward(left: Tensor, right: Tensor, target: Tensor);
}

class Metric {
    constructor(name: string) {}
    func :void reset();
    func :void update(prediction: Tensor, target: Tensor);
    func :double value();
    func :string name();
}

class Accuracy extends Metric {
    constructor() {}
    func :void update(prediction: Tensor, target: Tensor);
}

class Precision extends Metric {
    constructor() {}
    func :void update(prediction: Tensor, target: Tensor);
}

class Recall extends Metric {
    constructor() {}
    func :void update(prediction: Tensor, target: Tensor);
}

class F1 extends Metric {
    constructor() {}
    func :void update(prediction: Tensor, target: Tensor);
}

class Perplexity extends Metric {
    constructor() {}
    func :void update(prediction: Tensor, target: Tensor);
}
```

### 5.8 Dataset, batch, and preprocessing classes

```hoo
class Batch {
    constructor(inputs: Tensor, targets: Tensor) {}
    func :Tensor inputs();
    func :Tensor targets();
    func :int64 size();
    func :Batch to_device(device: Device);
}

class Dataset {
    constructor(size: int64) {}
    func :int64 length();
    func :Batch get(index: int64);
    func :Dataset map(transform: Transform);
    func :Dataset shuffle(seed: int64);
    func :Array split(train_ratio: double, validation_ratio: double);
}

class DataLoader {
    constructor(dataset: Dataset, batch_size: int64, shuffle: bool) {}
    func :void reset();
    func :bool has_next();
    func :Batch next();
    func :int64 length();
    func :void set_seed(seed: int64);
    func :void set_drop_last(value: bool);
}

class Transform {
    constructor(name: string) {}
    func :any apply(value: any);
    func :string name();
}

class Compose extends Transform {
    constructor(transforms: Array) {}
    func :any apply(value: any);
    func :void append(transform: Transform);
}

class TextTransform extends Transform {
    constructor(tokenizer: Tokenizer, max_length: int64) {}
    func :Tensor apply(value: string);
    func :Tensor attention_mask(value: string);
}

class ImageTransform extends Transform {
    constructor(width: int64, height: int64) {}
    func :Tensor apply(value: Buffer);
    func :Tensor normalize(value: Tensor, mean: Tensor, stddev: Tensor);
}

class AudioTransform extends Transform {
    constructor(sample_rate: int64, window_size: int64, hop_size: int64) {}
    func :Tensor apply(value: Buffer);
    func :Tensor spectrogram(value: Buffer);
    func :Tensor mel_spectrogram(value: Buffer);
}

class VideoTransform extends Transform {
    constructor(frame_count: int64, width: int64, height: int64) {}
    func :Array apply(value: Buffer);
    func :Array sample_frames(value: Buffer, start: int64, stride: int64);
}
```

`Dataset` and `DataLoader` must have an `IterableDataset`/streaming extension
before they claim unbounded live audio/video support. That extension must define
backpressure, cancellation, worker ownership, and buffer lifetime.

### 5.9 Tokenizer and media decoder classes

```hoo
class Vocabulary {
    constructor(unknown_token: string) {}
    func :int64 add(token: string);
    func :int64 id(token: string);
    func :string token(id: int64);
    func :int64 size();
    func :void freeze();
    func :Buffer serialize();
    func :bool deserialize(value: Buffer);
}

class Tokenizer {
    constructor(vocabulary: Vocabulary) {}
    func :Array tokenize(value: string);
    func :Tensor encode(value: string, max_length: int64);
    func :string decode(tokens: Tensor);
    func :Tensor attention_mask(tokens: Tensor);
    func :Vocabulary vocabulary();
}

func :Tokenizer tokenizer_from_file(path: string);
func :Tokenizer tokenizer_from_buffer(value: Buffer);

class ImageDecoder {
    constructor(format: string) {}
    func :Tensor decode(value: Buffer);
    func :Array metadata(value: Buffer);
}

class AudioDecoder {
    constructor(format: string) {}
    func :Buffer decode(value: Buffer);
    func :int64 sample_rate(value: Buffer);
    func :int64 channels(value: Buffer);
}

class VideoDecoder {
    constructor(format: string) {}
    func :Array decode_frames(value: Buffer, first: int64, count: int64);
    func :int64 frame_count(value: Buffer);
    func :double duration(value: Buffer);
}
```

Codec-backed classes are optional native adapters. Their absence must produce a
capability error, not a link failure in `hoort`.

### 5.10 Checkpoint and model-format classes

```hoo
class Checkpoint {
    constructor(path: string) {}
    func :bool save(module: Module, optimizer: Optimizer, step: int64);
    func :bool load(module: Module, optimizer: Optimizer, strict: bool);
    func :Array metadata();
    func :bool verify();
    func :string format();
}

class ModelFormat {
    constructor(name: string) {}
    func :string name();
    func :bool can_load(path: string);
    func :bool can_save(path: string);
    func :Module load(path: string);
    func :bool save(module: Module, path: string);
    func :Array unsupported_ops();
}

func :Checkpoint save_checkpoint(path: string, module: Module,
                                 optimizer: Optimizer, step: int64);
func :bool load_checkpoint(path: string, module: Module,
                           optimizer: Optimizer, strict: bool);
func :Module load_onnx(path: string);
func :Module load_safetensors(path: string);
func :bool save_safetensors(module: Module, path: string);
func :bool load_pytorch_weights(module: Module, path: string);
func :bool load_tensorflow_weights(module: Module, path: string);
func :bool export_onnx(module: Module, path: string);
```

Format loaders must validate magic/version, bounds, checksums, names, shapes,
and dtypes before updating a module. They must never deserialize executable
code. A loaded model must report whether it is inference-only or training-safe.

### 5.11 Required API unit tests

Every class and free function above requires both direct runtime tests and Hoo
source/JIT tests where it crosses the language boundary. The proposed test
files are:

- `tests/runtime/HooTensorTest.cpp`: factories, rank/dtype limits, reductions,
  broadcasting, shape errors, numerical tolerances, and ARC lifetime;
- `tests/jit/HooTensorJitTest.cpp`: tensor free functions, opaque pointer ABI,
  overload resolution, and interpreter/JIT equivalence;
- `tests/runtime/HooNNRuntimeTest.cpp`: `Module`, `Parameter`, `Sequential`,
  all layer constructors/methods, state traversal, train/eval mode, and
  parameter sharing;
- `tests/jit/HooNNJitTest.cpp`: Hoo class construction, inherited methods,
  `forward` dispatch, private/public access, field ownership, and chained calls;
- `tests/runtime/HooAutogradTest.cpp` and `tests/jit/HooAutogradJitTest.cpp`:
  scalar/tensor gradients, finite differences, accumulation, stop-gradient,
  unsupported operations, and f8-to-f64 accumulation;
- `tests/runtime/HooOptimTest.cpp` and `tests/jit/HooOptimJitTest.cpp`:
  optimizer updates, zeroing, scheduler state, clipping, checkpoint resume,
  and deterministic seeds;
- `tests/runtime/HooDataTest.cpp` and `tests/jit/HooDataJitTest.cpp`:
  dataset indexing, batching, shuffle determinism, transforms, padding,
  tokenizer round trips, and stream cancellation/lifetime;
- `tests/runtime/HooMediaTest.cpp`: optional codec capability detection and
  image/audio/video metadata, decode, sampling, and malformed-input errors;
- `tests/runtime/HooModelFormatTest.cpp` and
  `tests/jit/HooModelFormatJitTest.cpp`: Hoo checkpoint round trips,
  SafeTensors/ONNX adapters, unsupported operators, checksums, corrupt files,
  and no-code-execution guarantees; and
- `tests/hvm/HVMNNInstructionSemanticsTest.cpp`: any new HVM-L/HVM-V/HVM-A
  sequence, feature-flag rejection, fallback behavior, pointer ABI, and
  interpreter/JIT architectural equivalence.

Each class test must cover construction, every public method, valid and invalid
inputs, null/error behavior, ARC retain/release balance, serialization where
applicable, and deterministic results. Each differentiable layer additionally
requires forward-value, gradient, shape, dtype, and numerical-equivalence
tests. Tests requiring optional codecs or accelerators must be capability-gated
and must pass with those dependencies disabled.

## 6. Implementation Phases

### Phase 0: Contracts and vertical-slice prototype

1. Freeze tensor ownership, dtype, layout, shape, and error contracts for the
   existing and versioned `hoort` tensor ABI. Device contracts remain pending.
2. Resolve the Buffer/Tensor type-ID collision and add regression tests for ARC,
   runtime type checks, generic values, and serialization.
3. Add the remaining versioned capability, descriptor, device, parameter,
   module, tape, data-loader, stream, and model-reader/writer ABI contracts from
   section 3.4.0; the initial `HooStatus`/capability/tensor metadata slice is
   complete.
4. Add legacy-to-ND conversion rules and verify old `hoo_tensor_*` symbols and
   existing Hoo programs remain behaviorally compatible.
5. Define the C++17 dependency boundary and verify that `hoort` still builds
   without LLVM, codecs, model-format libraries, or accelerator SDKs.
6. Decide and specify the callable representation and structured model-value
   representation; do not use `any` as an implicit function pointer.
7. Define the operator registry: forward kernel, shape rule, dtype rule,
   derivative rule, numerical tolerances, runtime C ABI symbol, and HVM/JIT
   fallback for every differentiable tensor operation.
8. Extend the versioned `HooTensor` path for rank-4/rank-5 layout and batched
   image/video models, or document a separate handle if device/view ownership
   requirements make that safer.
9. Build a scalar/tensor autodiff prototype with finite-difference and reference
   gradient tests using only currently supported tensor dtypes.
10. Deliver one end-to-end MLP on synthetic data: forward, loss, backward,
   optimizer step, checkpoint, reload, and inference through the ordinary HVM
   runtime-call path, without requiring HVM-V or HVM-A.

### Phase 1: Tensor training foundation

1. Add the minimum tensor kernel groups in section 3.4.1, including stable
   unary functions, reductions, broadcasting, indexing, and batched matmul.
2. Implement reverse-mode autodiff for the supported tensor operators, retaining
   `f64` accumulation for low-precision `f8`/integer storage where required.
3. Add SGD, Adam/AdamW, common losses, metrics, batching, and validation.
4. Implement `hoo.nn` core layers and an MLP/transformer encoder using runtime
   calls and ordinary HVM instructions first; optimized lowering is a later
   phase.
5. Implement batched `Conv2D` only after the rank/layout ABI extension is
   complete and tested; until then, expose it as unsupported rather than
   silently flattening or copying dimensions.

### Phase 2: Data and task foundations

1. Deliver `hoo.data` datasets/loaders with deterministic batching and workers.
2. Deliver text tokenization, image transforms, audio features, and video frame
   sampling through testable adapter interfaces.
3. Add text classification and text embeddings first. Add image/audio/video
   classification only when their required tensor ranks, layouts, codecs, and
   batch contracts are implemented.
4. Add profiling, memory accounting, and reproducible experiment configuration.

### Phase 3: Model ecosystem and deployment

1. Deliver versioned Hoo checkpoints with safe validation and resumable training.
2. Add NumPy, SafeTensors, and ONNX import/export with operator coverage reports.
3. Add PyTorch and TensorFlow/Keras weight adapters where mapping is unambiguous.
4. Add compiled inference, batching, quantization hooks, and model-serving APIs.

### Phase 4: Advanced architectures

1. Add decoder-only and encoder-decoder transformers, generation, KV-cache
   interfaces, streaming generation, and text embedding pipelines.
2. Add detection, segmentation, image generation, autoencoders, multimodal
   fusion, and video/audio sequence architectures.
3. Add distributed-training and accelerator abstractions without coupling the
   core API to one vendor.

### Phase 5: Performance and specialized backends

1. Lower proven tensor kernels to ordinary HVM loops; use HVM-L only when the
   target feature flag is present and preserve the ordinary-branch fallback.
2. Add HVM-V lowering only for the element encodings defined by the target
   profile. For hosted HVMJIT, that means validating the current 64-bit
   `i64`/`f64` vector behavior rather than assuming packed low-precision lanes.
3. Benchmark and selectively enable LLVM CPU vectorization with generated IR
   validation and numerical-equivalence tests. This remains a CPU optimization,
   not GPU lowering.
4. Add optional BLAS/FFT/native-kernel dispatch and memory-planning optimizations
   behind C++17 adapter interfaces; `hoort` remains dependency-light.
5. Implement binary neural-network packing and XNOR/popcount only after its
   HVM/runtime numerical contract and fallback are finalized.
6. Add HVM-A and other GPU/accelerator backends as separately testable,
   feature-gated capabilities with explicit device-memory and synchronization
   contracts.

## 7. Acceptance Criteria

The issue is not complete until all of the following are demonstrated:

- a published capability matrix stating exactly which Keras/PyTorch model
  operators, ranks, layouts, dtypes, devices, and model formats are supported;
- a documented, compilable Hoo API for defining custom modules and parameters;
- a defined callable/function representation or a compiler-intrinsic boundary
  for `grad`, custom losses, and custom transforms;
- typed multi-input, multi-output, recurrent-state, mask, and named-output
  model values, with no implicit executable `any` values;
- correct gradients for every advertised differentiable operator, checked
  against numerical or reference implementations;
- train/resume/evaluate/export/import tests for an MLP and transformer-based
  text classifier, plus a batched CNN after rank/layout support is complete;
- deterministic data loaders and preprocessing tests for text, image, audio,
  and video inputs;
- image classification, text classification, text embedding, and text
  generation reference examples;
- safe checkpoint validation, round-trip tests, and explicit unsupported-format
  diagnostics;
- benchmarks comparing reference runtime, HVM, and any optimized backend;
- numerical-equivalence tests across supported dtypes and backends; and
- documented capability checks and fallbacks for every optional native or
  accelerator dependency.
- rank/layout/stride/view tests covering the highest supported tensor rank and
  explicit rejection of unsupported ranks;
- ABI compatibility tests covering status codes, capability negotiation,
  legacy `HooTensor` behavior, `HooTensorND` creation/views/transfers, new
  dtype IDs, opaque-handle ownership, and old-binary symbol compatibility;
- parameter/module/tape ABI tests covering registration, nested state,
  gradient accumulation, mutation/aliasing rejection, and ARC balance;
- pull-based dataset/loader/stream tests covering borrowed buffers, cancellation,
  worker shutdown, bounded buffering, and deterministic replay; and
- model reader/writer tests covering temporary-state validation, checksums,
  malformed descriptors, unsupported operators, and no executable-code loading;
- C++17 builds of `hoort`, `hoo-core`, and the HVMJIT target on supported host
  platforms, including a build with optional AI dependencies disabled;
- HVM interpreter/JIT equivalence tests for every new instruction sequence and
  runtime call, with correct module feature flags and behavior when optional
  HVM features are absent; and
- ABI tests confirming that tensor parameters and returns remain opaque 64-bit
  HVM pointers and that ARC ownership is balanced across forward, backward,
  optimizer, checkpoint, and data-loader paths.

“Keras/PyTorch-compatible” means compatible only with the published capability
matrix. Full framework parity is not implied by the existence of similarly
named classes. In particular, image/video parity cannot be claimed while the
selected tensor ABI cannot represent their required batched ranks and layouts.

Performance claims must include model, tensor shapes, dtype, hardware, batch
size, warm-up policy, and whether data preparation is included.

## 8. Compatibility References

Implementation work must be checked against the authoritative repository
contracts rather than against assumptions from external ML frameworks:

- C++ build configuration: `CMakeLists.txt` (`CMAKE_CXX_STANDARD 17` and the
  independent `hoort` target);
- HVM architecture and ABI: `docs/hvm/hvm-spec.md`,
  `docs/hvm/instructions.md`, and `docs/hvm/ho-file-format.md`;
- HVM instruction encodings and feature flags:
  `docs/hvm/hvm_instruction_set.csv` and `src/hvm/HOModule.h`;
- tensor C ABI and current storage contract: `src/runtime/lib/hoo_tensor.h` and
  `src/runtime/lib/hoo_tensor.cpp`; and
- Hoo tensor language/API behavior: `src/parsing/Hooc.g4`,
  `docs/runtime/api/tensor.md`, and the tensor JIT/runtime tests.

If this issue proposes a capability not covered by those contracts, the phase
must first add or update the relevant normative specification and compatibility
tests. The ANN plan must not silently redefine the HVM ISA, the Hoo public ABI,
or the existing `hoort` tensor representation.

## 9. Risks and Non-Goals

- Reimplementing every PyTorch or TensorFlow operator is not a prerequisite for
  the first release. Operator coverage must be explicit and discoverable.
- Native codecs, BLAS, FFT, and accelerator libraries may remain optional
  dependencies; the Hoo API must remain stable when they are unavailable.
- Dynamic graphs, arbitrary mutation, aliasing, and external side effects make
  compile-time autodiff difficult. They require documented restrictions or a
  supported eager fallback.
- Low-precision storage must not silently become low-precision gradient
  accumulation.
- Loading untrusted model files must never execute serialized code.
- Distributed training, GPU execution, and production serving are extension
  milestones, not assumptions of the initial CPU/HVM implementation.

## 10. Status

- **Date**: 2026-08-10
- **Status**: **PROPOSED**
- **Priority**: **CRITICAL**
- **Current baseline**: Tensor support is implemented; the neural-network,
  autodiff, data, checkpoint, and model-format layers remain to be built.
- **Readiness**: Roadmap-ready, but not implementation-ready for general
  Keras/PyTorch workloads until the tensor ABI, callable representation,
  minimum kernel set, and structured model-value contracts are approved.
