/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "osTest.h"

#if (LOS_KERNEL_MULTI_HWI_TEST == 1)
#include "it_los_hwi.h"

static int g_uwIndex;
#ifdef __RISC_V__
#define HWI_NUM_INT0 HWI_NUM_TEST
#ifdef LOS_HIFONEV320_RV32
#define TEST_MAX_NUMBER_HWI 5
#define TEST_HWI_COUNT (TEST_MAX_NUMBER_HWI - 1)
#else
#define TEST_MAX_NUMBER_HWI OS_USER_HWI_MAX
#define TEST_HWI_COUNT (TEST_MAX_NUMBER_HWI - 2)
#endif
#else
#define TEST_MAX_NUMBER_HWI (OS_USER_HWI_MAX - HWI_NUM_INT0)
#define TEST_HWI_COUNT (TEST_MAX_NUMBER_HWI - 2)
#undef  HWI_NUM_INT0
#define HWI_NUM_INT0 HWI_NUM_INT1
#endif

static VOID HwiF01(VOID)
{
    TestHwiClear(HWI_NUM_INT0 + g_uwIndex);
    g_testCount++;

    return;
}

static UINT32 Testcase(VOID)
{
    UINT32 ret;
    HWI_PRIOR_T hwiPrio = OS_HWI_PRIO_LOWEST;
    HWI_MODE_T mode = 0;
    HwiIrqParam irqParam;
    (void)memset_s(&irqParam, sizeof(HwiIrqParam), 0, sizeof(HwiIrqParam));
    irqParam.pDevId = 0;

    g_testCount = 0;

    /* Creates 3 interrupts in a row with interrupt number */
    for (g_uwIndex = 0; g_uwIndex < 3; g_uwIndex++) {
        ret = LOS_HwiCreate(HWI_NUM_INT0 + g_uwIndex, hwiPrio, mode, (HWI_PROC_FUNC)HwiF01, &irqParam);
        ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);
        TestHwiTrigger(HWI_NUM_INT0 + g_uwIndex);
    }

    /* Create interrupt starting at interrupt number HWI_NUM_INT0 + 4. */
    for (g_uwIndex = 4; g_uwIndex < TEST_MAX_NUMBER_HWI; g_uwIndex++) {
        /* if interrupt number is not HWI_NUM_INT0 + 5 */
        if (g_uwIndex != 5) {
            ret = LOS_HwiCreate(HWI_NUM_INT0 + g_uwIndex, hwiPrio, mode, (HWI_PROC_FUNC)HwiF01, &irqParam);
            ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);
            TestHwiTrigger(HWI_NUM_INT0 + g_uwIndex);
        }
    }
    ICUNIT_GOTO_EQUAL(g_testCount, TEST_HWI_COUNT, g_testCount, EXIT);

EXIT:
    for (g_uwIndex = 0; g_uwIndex < TEST_MAX_NUMBER_HWI; g_uwIndex++) {
        TestHwiDelete(HWI_NUM_INT0 + g_uwIndex);
    }

    return LOS_OK;
}

VOID ItLosHwi032(VOID) // IT_Layer_ModuleORFeature_No
{
    TEST_ADD_CASE("ItLosHwi032", Testcase, TEST_LOS, TEST_HWI, TEST_LEVEL3, TEST_PRESSURE);
}
#endif