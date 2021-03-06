/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "core/gmm_helper/gmm_helper.h"
#include "core/unit_tests/helpers/default_hw_info.h"
#include "unit_tests/helpers/variable_backup.h"

#include "gtest/gtest.h"

namespace NEO {

extern GMM_INIT_IN_ARGS passedInputArgs;
extern bool copyInputArgs;

TEST(GmmHelperTest, whenCreateGmmHelperWithoutOsInterfaceThenPassedFileDescriptorIsZeroed) {
    std::unique_ptr<GmmHelper> gmmHelper;
    VariableBackup<decltype(passedInputArgs)> passedInputArgsBackup(&passedInputArgs);
    VariableBackup<decltype(copyInputArgs)> copyInputArgsBackup(&copyInputArgs, true);

    uint32_t expectedFileDescriptor = 0u;

    gmmHelper.reset(new GmmHelper(nullptr, platformDevices[0]));
    EXPECT_EQ(expectedFileDescriptor, passedInputArgs.FileDescriptor);
}

} // namespace NEO
