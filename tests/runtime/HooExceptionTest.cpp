#include <gtest/gtest.h>
#include "runtime/lib/exception/hoo_exception.h"

class HooExceptionTest : public ::testing::Test {
};

TEST_F(HooExceptionTest, BasicException) {
    HooException exc = hoo_exception_runtime("Test error");
    ASSERT_NE(exc, nullptr);
    EXPECT_EQ(hoo_exception_get_type_id(exc), HOO_EXCEPTION_RUNTIME);
    EXPECT_STREQ(hoo_exception_get_message(exc), "Test error");
    EXPECT_STREQ(hoo_exception_get_type_name(exc), "RuntimeException");
    
    hoo_exception_release(exc);
}

TEST_F(HooExceptionTest, ExceptionWithCause) {
    HooException cause = hoo_exception_invalid_cast("Cause message");
    HooException exc = hoo_exception_create_with_cause(HOO_EXCEPTION_RUNTIME, "Top error", cause);
    
    EXPECT_EQ(hoo_exception_has_cause(exc), 1);
    HooException fetchedCause = hoo_exception_get_cause(exc);
    EXPECT_STREQ(hoo_exception_get_message(fetchedCause), "Cause message");
    
    hoo_exception_release(exc);
    hoo_exception_release(cause);
}

TEST_F(HooExceptionTest, CustomException) {
    HooException exc = hoo_exception_custom("MyError", "Custom message");
    EXPECT_STREQ(hoo_exception_get_type_name(exc), "MyError");
    EXPECT_STREQ(hoo_exception_get_message(exc), "Custom message");
    
    hoo_exception_release(exc);
}

TEST_F(HooExceptionTest, TypeCompatibilityMatchesBaseAndExactTypes) {
    HooException runtime = hoo_exception_runtime("runtime");
    HooException custom = hoo_exception_custom("MyError", "custom");

    EXPECT_EQ(hoo_exception_matches_type(runtime, 100), 1); // Exception base
    EXPECT_EQ(hoo_exception_matches_type(runtime, HOO_EXCEPTION_RUNTIME), 1);
    EXPECT_EQ(hoo_exception_matches_type(runtime, HOO_EXCEPTION_INVALID_CAST), 0);
    EXPECT_EQ(hoo_exception_matches_type(custom, 100), 1);
    EXPECT_EQ(hoo_exception_matches_type(custom, HOO_EXCEPTION_CUSTOM), 1);

    hoo_exception_release(runtime);
    hoo_exception_release(custom);
}

TEST_F(HooExceptionTest, ARC) {
    HooException exc = hoo_exception_runtime("Msg");
    EXPECT_EQ(hoo_exception_refcount(exc), 1);
    
    hoo_exception_retain(exc);
    EXPECT_EQ(hoo_exception_refcount(exc), 2);
    
    hoo_exception_release(exc);
    EXPECT_EQ(hoo_exception_refcount(exc), 1);
    
    hoo_exception_release(exc);
}
