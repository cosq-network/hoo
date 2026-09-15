#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <thread>
#include "runtime/lib/core/hoo_ai.h"
#include "runtime/lib/core/hoo_runtime.h"
#include "runtime/lib/mem/hoo_tensor.h"

class HooAiTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Leave the thread-local state pristine for the next test.
        hoo_ai_clear_last_error();
    }
};

TEST_F(HooAiTest, AbiVersionIsReported) {
    EXPECT_EQ(hoo_ai_abi_version(), HOO_TENSOR_ABI_VERSION);
    EXPECT_EQ(hoo_ai_abi_version(), 2);
}

TEST_F(HooAiTest, FeatureQueriesReportImplementedCapabilities) {
    EXPECT_EQ(hoo_ai_has_feature("tensor_abi_v2"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_dynamic_rank"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_f32"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_int32"), 1);
    EXPECT_EQ(hoo_ai_has_feature("tensor_f16"), 0);
    EXPECT_EQ(hoo_ai_has_feature("tensor_bf16"), 0);
}

TEST_F(HooAiTest, UnknownAndNullFeatureNamesReturnZero) {
    EXPECT_EQ(hoo_ai_has_feature("unknown_feature"), 0);
    EXPECT_EQ(hoo_ai_has_feature("tensor_f32typo"), 0);
    EXPECT_EQ(hoo_ai_has_feature(""), 0);
    EXPECT_EQ(hoo_ai_has_feature(nullptr), 0);
}

TEST_F(HooAiTest, DtypeSupportTableAgreesWithFeatureStrings) {
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_INT64), hoo_ai_has_feature("tensor_int64"));
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F64), hoo_ai_has_feature("tensor_f64"));
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F32), hoo_ai_has_feature("tensor_f32"));
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_INT32), hoo_ai_has_feature("tensor_int32"));
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F16), hoo_ai_has_feature("tensor_f16"));
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_BF16), hoo_ai_has_feature("tensor_bf16"));
}

TEST_F(HooAiTest, DtypeSupportTableMatchesImplementedDtypes) {
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_INT64), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F64), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_INT8), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_BYTE), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_BIT), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F8), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F32), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_INT32), 1);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_F16), 0);
    EXPECT_EQ(hoo_ai_dtype_supported(HOO_TENSOR_DTYPE_BF16), 0);
    EXPECT_EQ(hoo_ai_dtype_supported(999), 0);
    EXPECT_EQ(hoo_ai_dtype_supported(0), 0);
}

TEST_F(HooAiTest, TensorValidationAgreesWithCapabilityTable) {
    // Every dtype the tensor module accepts must be reported as supported
    // (except the retained legacy id 3 which is private to the tensor API).
    int64_t implemented_ids[] = {
        HOO_TENSOR_DTYPE_INT64, HOO_TENSOR_DTYPE_F64, HOO_TENSOR_DTYPE_INT8,
        HOO_TENSOR_DTYPE_BYTE, HOO_TENSOR_DTYPE_BIT, HOO_TENSOR_DTYPE_F8,
        HOO_TENSOR_DTYPE_F32, HOO_TENSOR_DTYPE_INT32, /* legacy */ 3
    };
    for (int64_t id : implemented_ids) {
        const int64_t dims[2] = {2, 2};
        HooTensor t = nullptr;
        const HooStatus status = hoo_tensor_new_ex(id, 2, dims, &t);
        ASSERT_EQ(status, HOO_STATUS_OK) << "dtype " << id;
        if (t) hoo_release(t);
        EXPECT_EQ(hoo_ai_dtype_supported(id), id != 3) << "dtype " << id;
    }
    // Feature-negative dtypes must be rejected by the tensor module.
    const int64_t rejected_ids[] = { HOO_TENSOR_DTYPE_F16, HOO_TENSOR_DTYPE_BF16, 999 };
    for (int64_t id : rejected_ids) {
        const int64_t dims[2] = {2, 2};
        HooTensor t = nullptr;
        EXPECT_EQ(hoo_tensor_new_ex(id, 2, dims, &t), HOO_STATUS_INVALID_DTYPE) << "dtype " << id;
        EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_INVALID_DTYPE) << "dtype " << id;
    }
}

TEST_F(HooAiTest, StatusStartsCleanAndClearRestoresBaseline) {
    ASSERT_EQ(hoo_ai_last_status(), HOO_STATUS_OK);
    EXPECT_EQ(strcmp(hoo_ai_last_error(), ""), 0);
    hoo_ai_set_last_error(HOO_STATUS_NUMERICAL_ERROR, "transient");
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_NUMERICAL_ERROR);
    EXPECT_STREQ(hoo_ai_last_error(), "transient");
    hoo_ai_clear_last_error();
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_OK);
    EXPECT_EQ(strcmp(hoo_ai_last_error(), ""), 0);
}

TEST_F(HooAiTest, ErrorMessageIsBoundedAndNullSafe) {
    hoo_ai_set_last_error(HOO_STATUS_INVALID_ARGUMENT, nullptr);
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(strcmp(hoo_ai_last_error(), ""), 0);

    const std::string long_message(1024, 'x');  // exceeds the 256-byte slot
    hoo_ai_set_last_error(HOO_STATUS_IO_ERROR, long_message.c_str());
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_IO_ERROR);
    EXPECT_EQ(strlen(hoo_ai_last_error()), 255u);
    EXPECT_EQ(std::string(hoo_ai_last_error(), 3), "xxx");
    EXPECT_EQ(hoo_ai_last_error()[60], 'x');
}

TEST_F(HooAiTest, ThreadLocalStatesAreIndependentPerThread) {
    // Reset here, then corrupt the state on a fresh thread; the main
    // thread must not observe the other thread's error.
    hoo_ai_clear_last_error();
    ASSERT_EQ(hoo_ai_last_status(), HOO_STATUS_OK);
    std::thread worker([]() {
        hoo_ai_set_last_error(HOO_STATUS_CANCELLED, "from other thread");
    });
    worker.join();
    EXPECT_EQ(hoo_ai_last_status(), HOO_STATUS_OK);
}
