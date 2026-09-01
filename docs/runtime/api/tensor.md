# Tensor API Reference

## Module Name

`hoo.tensor`

## Import Statement

```hoo
import hoo.tensor;
```

## Module Description

The `Tensor` module provides rank-1, rank-2, and rank-3 arrays for numerical
computation. Tensors are reference-counted runtime objects supporting `bit`,
`int8`, `byte`, `int64`, `f8`, and `f64` elements, plus tensor-tensor and
tensor-scalar arithmetic and logical operations.

## Class: Tensor

### Declaration

```hoo
class Tensor
```

### Public Fields

None.

### Public Instance Functions

#### Constructor: `Tensor`

Creates a new tensor with the given shape (array of dimension sizes). The element type defaults to `int64` unless specified via explicit constructors.

```hoo
Tensor(shape: array) :Tensor
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `shape` | `array` | Array of dimension sizes (max rank 3). |

**Returns:** `Tensor` — A new tensor instance.

---

#### `length`

Returns the total number of elements in the tensor.

```hoo
length() :int64
```

**Returns:** `int64` — Total element count.

---

#### `rank`

Returns the rank (number of dimensions) of the tensor.

```hoo
rank() :int64
```

**Returns:** `int64`.

---

#### `dim`

Returns the size of a specific dimension.

```hoo
dim(axis: int64) :int64
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `axis` | `int64` | Zero‑based axis index. |

**Returns:** `int64` — Size of the requested dimension.

---

#### `element_type`

Returns the internal element type identifier.

```hoo
element_type() :int64
```

**Returns:** `int64` — One of `TENSOR_ELEMENT_BIT`, `TENSOR_ELEMENT_F64`, `TENSOR_ELEMENT_F8`, etc.

---

#### `set_value`

Stores a raw value (bits) at the given index.

```hoo
set_value(index: int64, value_bits: int64) :int64
```

**Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `int64` | Zero‑based element index. |
| `value_bits` | `int64` | Packed representation of the value. |

**Returns:** `int64` — Success flag (1) or failure (0).

---

#### `push_value`

Appends a packed value to the tensor (if capacity permits).

```hoo
push_value(value_bits: int64) :int64
```

**Returns:** `int64` — Success (1) or failure (0).

---

#### `get_int64`

Retrieves an element as a signed 64‑bit integer (converted from the stored representation).

```hoo
get_int64(index: int64) :int64
```

---

#### `get_double`

Retrieves an element as a double‑precision floating‑point value.

```hoo
get_double(index: int64) :double
```

---

#### `get_bits`

Retrieves the raw bits of an element (useful for bit‑packed tensors).

```hoo
get_bits(index: int64) :int64
```

---

#### Arithmetic Operations (binary)

These functions take two tensors of identical shape and produce a new tensor with the appropriate element type.

- `add(left: Tensor, right: Tensor) :Tensor` — Element‑wise addition.
- `sub(left: Tensor, right: Tensor) :Tensor` — Element‑wise subtraction.
- `element_mul(left: Tensor, right: Tensor) :Tensor` — Element‑wise multiplication.
- `element_div(left: Tensor, right: Tensor) :Tensor` — Element‑wise division (division‑by‑zero yields 0).
- `matmul(left: Tensor, right: Tensor) :Tensor` — Matrix multiplication for rank‑2 tensors.

Scalar broadcasting is also supported for numeric tensors:

- `tensor + scalar`, `scalar + tensor`
- `tensor - scalar`, `scalar - tensor`
- `tensor * scalar`, `scalar * tensor` (including `.*`)
- `tensor / scalar`, `scalar / tensor` (including `./`; zero denominator yields 0)

The result keeps the tensor shape. Floating-point scalars follow tensor
promotion rules; integer and low-precision tensors retain native storage
semantics where possible.

All functions return a new tensor or `null` on shape mismatch.

---

#### Comparison Operations

Each returns a bit‑tensor (boolean mask) where each element is 0 or 1.

- `eq(left: Tensor, right: Tensor) :Tensor`
- `ne(left: Tensor, right: Tensor) :Tensor`
- `lt(left: Tensor, right: Tensor) :Tensor`
- `le(left: Tensor, right: Tensor) :Tensor`
- `gt(left: Tensor, right: Tensor) :Tensor`
- `ge(left: Tensor, right: Tensor) :Tensor`

---

#### Logical Operations (bit tensors)

- `and(left: Tensor, right: Tensor) :Tensor`
- `or(left: Tensor, right: Tensor) :Tensor`
- `not(tensor: Tensor) :Tensor`

These operate on tensor elements interpreted as booleans (non‑zero = true).

---

## Usage Example

```hoo
import hoo.tensor;

func :int64 main() {
    // Create a 2x2 int64 tensor
    var t = Tensor([2, 2]);
    // Fill with values
    t.set_value(0, 1);
    t.set_value(1, 2);
    t.set_value(2, 3);
    t.set_value(3, 4);
    // Retrieve a value
    println(t.get_int64(0)); // 1
    return 0;
}
```

---

## Reference Links

- [C implementation: `hoo_tensor.cpp`](file:///Users/benoybose/Projects/hoo/src/runtime/lib/mem/hoo_tensor.cpp)
- [Collections API](file:///Users/benoybuse/Projects/hoo/docs/runtime/api/collections.md)
