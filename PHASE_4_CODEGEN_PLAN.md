# Phase 4: Code Generation Updates - Detailed Implementation Plan

## Objective
Modify `LLVMCodeGenerator` to infer array types from variable/parameter declarations and use the generic `HooArray` runtime functions instead of LLVM constant arrays.

## Current Implementation (to be replaced)

### generateArrayLiteral() - Current Approach
```cpp
// Creates LLVM constant arrays like: [i64 1, i64 2, i64 3]
// Returns pointer to LLVM array constant
Value* LLVMCodeGenerator::generateArrayLiteral(const ArrayLiteral& literal) {
    // 1. Infer element type from first expression
    // 2. Create LLVM constant array
    // 3. Create global variable
    // 4. Return pointer to global
}
```

### Issues with Current Approach
1. **No Runtime Type Information**: Array type is lost after compilation
2. **No Type Checking**: Cannot distinguish `int64[]` from raw `i64*`
3. **Static Only**: Cannot create dynamic arrays
4. **No ARC Integration**: No reference counting for arrays

## New Implementation (to be added)

### Step 1: Declare Array Creation Functions

Add to `LLVMCodeGenerator.h`:
```cpp
private:
    llvm::Function* hoo_int64_array_from_buffer_func_ = nullptr;
    llvm::Function* hoo_double_array_from_buffer_func_ = nullptr;

    // Helper to declare array creation functions
    void declareArrayFunctions();
    llvm::Function* getArrayFromBufferFunc(llvm::Type* elementType);
```

Add to `LLVMCodeGenerator.cpp`:
```cpp
void LLVMCodeGenerator::declareArrayFunctions() {
    // Declare: HooInt64Array hoo_int64_array_from_buffer(const int64_t* data, int64_t length)
    if (!hoo_int64_array_from_buffer_func_) {
        auto i64Ty = LLVMType::getInt64Ty(context_);
        auto ptrTy = llvm::PointerType::get(context_, 0);

        std::vector<LLVMType*> paramTypes = {ptrTy, i64Ty};
        auto funcType = llvm::FunctionType::get(ptrTy, paramTypes, false);
        hoo_int64_array_from_buffer_func_ = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            "hoo_int64_array_from_buffer",
            module_.get()
        );
    }

    // Declare: HooDoubleArray hoo_double_array_from_buffer(const double* data, int64_t length)
    if (!hoo_double_array_from_buffer_func_) {
        auto f64Ty = LLVMType::getDoubleTy(context_);
        auto i64Ty = LLVMType::getInt64Ty(context_);
        auto ptrTy = llvm::PointerType::get(context_, 0);

        std::vector<LLVMType*> paramTypes = {ptrTy, i64Ty};
        auto funcType = llvm::FunctionType::get(ptrTy, paramTypes, false);
        hoo_double_array_from_buffer_func_ = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            "hoo_double_array_from_buffer",
            module_.get()
        );
    }
}

llvm::Function* LLVMCodeGenerator::getArrayFromBufferFunc(LLVMType* elementType) {
    if (elementType->isIntegerTy(64)) {
        return hoo_int64_array_from_buffer_func_;
    } else if (elementType->isDoubleTy()) {
        return hoo_double_array_from_buffer_func_;
    }
    return nullptr;
}
```

### Step 2: Create Global Data Buffer

Replace `createGlobalArrayConstant()` with new version:
```cpp
Value* LLVMCodeGenerator::generateArrayLiteralWithRuntime(
    const std::vector<Constant*>& elements,
    LLVMType* elementType) {

    // Create array type [N x elementType]
    auto arrayType = llvm::ArrayType::get(elementType, elements.size());

    // Create constant array initializer
    auto arrayInit = llvm::ConstantArray::get(arrayType, elements);

    // Create global variable for the array data (buffer)
    auto globalData = new llvm::GlobalVariable(
        *module_,
        arrayType,
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        arrayInit,
        ".array_data"
    );

    // Create GEP to get pointer to first element
    std::vector<llvm::Constant*> indices = {
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0),
        llvm::ConstantInt::get(LLVMType::getInt64Ty(context_), 0)
    };

    auto dataPtr = llvm::ConstantExpr::getGetElementPtr(
        arrayType,
        globalData,
        indices
    );

    // Get the array creation function
    auto arrayFunc = getArrayFromBufferFunc(elementType);
    if (!arrayFunc) {
        std::cerr << "Error: No array creation function for type" << std::endl;
        return nullptr;
    }

    // Create call: hoo_*_array_from_buffer(dataPtr, length)
    auto lengthConst = llvm::ConstantInt::get(
        LLVMType::getInt64Ty(context_),
        elements.size()
    );

    std::vector<Value*> args = {dataPtr, lengthConst};

    return builder_->CreateCall(arrayFunc, args, "hoo_arr");
}
```

### Step 3: Update generateArrayLiteral()

```cpp
Value* LLVMCodeGenerator::generateArrayLiteral(const ArrayLiteral& literal) {
    const ExpressionList* elementsList = literal.getElements();

    if (!elementsList || elementsList->getExpressions().empty()) {
        // Empty array - create empty array via runtime
        // return hoo_int64_array_new(0);  // Use default int64
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(context_, 0));
    }

    const auto& expressions = elementsList->getExpressions();
    std::vector<Constant*> constantElements;
    LLVMType* elementType = nullptr;

    // Evaluate all elements and infer type from first element
    for (const auto& expr : expressions) {
        Value* elemValue = generateLLVMExpression(*expr);
        if (!elemValue) {
            std::cerr << "Failed to generate array element expression" << std::endl;
            return nullptr;
        }

        Constant* constElem = llvm::dyn_cast<Constant>(elemValue);
        if (!constElem) {
            std::cerr << "Array literal elements must be compile-time constants" << std::endl;
            return nullptr;
        }

        if (elementType == nullptr) {
            elementType = elemValue->getType();
        } else if (elemValue->getType() != elementType) {
            std::cerr << "Array literal elements must have uniform type" << std::endl;
            return nullptr;
        }

        constantElements.push_back(constElem);
    }

    // Declare array functions if not already done
    declareArrayFunctions();

    // Create array using runtime function
    return generateArrayLiteralWithRuntime(constantElements, elementType);
}
```

## Type Inference Strategy

### Case 1: Variable Declaration with Explicit Type
```hoo
var numbers: int64[] = [1, 2, 3];
```
✅ Type is explicit in annotation - use `int64` for element type

### Case 2: Variable Declaration with Type Inference
```hoo
var numbers = [1, 2, 3];
```
✅ Infer from literal elements - element type is `int64` (from constants)

### Case 3: Function Parameter
```hoo
func process(arr: double[]) { ... }
process([1.0, 2.0, 3.0]);
```
✅ Function signature provides expected type - use `double`

### Case 4: Function Argument with Inference
```hoo
func process(arr: int64[]) { ... }
process([1, 2, 3]);
```
✅ Infer from literal - element type is `int64`

## Implementation Order

1. **Step 1**: Add function declaration methods
   - Declare hoo_int64_array_from_buffer
   - Declare hoo_double_array_from_buffer
   - Add helper methods

2. **Step 2**: Implement new array generation
   - Rewrite generateArrayLiteralWithRuntime()
   - Create global data buffers
   - Emit runtime function calls

3. **Step 3**: Update Main Function
   - Modify generateArrayLiteral() to use new approach
   - Ensure backward compatibility

4. **Step 4**: Testing & Validation
   - Test array initialization
   - Test array access/modification
   - Test with multiple element types
   - Verify ARC integration

## Files to Modify

1. **src/LLVMCodeGenerator.h**
   - Add function pointer members for array creation functions
   - Add method declarations for helpers

2. **src/LLVMCodeGenerator.cpp**
   - Implement declareArrayFunctions()
   - Implement getArrayFromBufferFunc()
   - Implement generateArrayLiteralWithRuntime()
   - Modify generateArrayLiteral()

## Testing Strategy

### Unit Tests to Add
1. Array literal with int64 elements
2. Array literal with double elements
3. Array literal type inference
4. Array literal with explicit type annotation
5. Empty array creation
6. Array operations (access, modification, push, pop)
7. Multiple array types in same program
8. Array reference counting

### Integration Tests
1. Array in variable declaration
2. Array as function parameter
3. Array in function return type
4. Array operations in loops
5. Array concatenation and slicing

## Expected Behavior After Implementation

```hoo
func main() {
    var numbers = [1, 2, 3, 4, 5];           // int64[]
    var floats = [1.0, 2.5, 3.14];           // double[]

    // At runtime, these will call:
    // hoo_int64_array_from_buffer(&data, 5)
    // hoo_double_array_from_buffer(&data, 3)

    // Arrays will have reference counting
    // Arrays can be used with array operations
    // Type safety enforced by wrapper functions
}
```

## Performance Considerations

### Trade-offs
- **Pros**: Type safety, reference counting, dynamic operations
- **Cons**: Function call overhead for array creation, memcpy for generic operations
- **Optimization**: JIT can inline array operations if needed

### Optimization Opportunities
1. LLVM JIT can inline array functions
2. Constant propagation can eliminate some operations
3. Dead code elimination can remove unused arrays

## Backward Compatibility

The implementation maintains backward compatibility:
- Existing type-specific wrappers still work
- RuntimeClassRegistry can use HooInt64Array/HooDoubleArray
- External C code using old API continues to work
- Type-specific functions delegate to generic implementation

## Success Criteria

✅ Phase 4 Complete When:
1. Array literals call hoo_*_array_from_buffer() functions
2. Array type is inferred from declarations
3. All 577 tests still pass (zero regressions)
4. New array tests pass (type inference, operations)
5. Array reference counting works correctly
6. Code compiles without warnings

## Timeline Estimate

- Implementation: 4-6 hours
- Testing: 2-3 hours
- Debugging/Fixes: 1-2 hours
- Documentation: 1 hour

**Total: 8-12 hours of development work**
