/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
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
#include "los_dynlink.h"
#include "It_los_dynlink.h"

/* Test that function and value to find int dynamic_xxxxx.so */
STATIC UINT32 TestCase(VOID)
{
    VOID *handle = NULL;
    INT32 (*func)(VOID) = NULL;
    INT32 *pValueAddr = NULL;
    INT32 **ppValueAddr = NULL;
    CHAR *symbolName1 = "get_value100";
    CHAR *symbolName2 = "get_value200";
    CHAR *symbolName3 = "g_value100";
    CHAR *symbolName4 = "g_pValue100";
    CHAR *symbolName5 = "pValue200";
    CHAR *symbolName6 = "g_pValue200";
    CHAR *dsoName = DSO_FULL_PATH("dynamic_xxxxx.so");
    INT32 ret;

    handle = (VOID *)LOS_SoLoad(dsoName, NULL);
    ICUNIT_ASSERT_NOT_EQUAL(handle, NULL, handle);

    func = (INT32 (*)(VOID))LOS_FindSym(handle, symbolName1);
    ICUNIT_GOTO_NOT_EQUAL(func, NULL, func, EXIT);
    ret = func();
    ICUNIT_GOTO_EQUAL(ret, 100, ret, EXIT);

    func = (INT32 (*)(VOID))LOS_FindSym(handle, symbolName2);
    ICUNIT_GOTO_NOT_EQUAL(func, NULL, func, EXIT);
    ret = func();
    ICUNIT_GOTO_EQUAL(ret, 200, ret, EXIT);

    pValueAddr = LOS_FindSym(handle, symbolName3);
    ICUNIT_GOTO_NOT_EQUAL(pValueAddr, NULL, pValueAddr, EXIT);
    ICUNIT_GOTO_EQUAL(*pValueAddr, 100, *pValueAddr, EXIT);

    ppValueAddr = LOS_FindSym(handle, symbolName4);
    ICUNIT_GOTO_NOT_EQUAL(ppValueAddr, NULL, ppValueAddr, EXIT);
    ICUNIT_GOTO_EQUAL(**ppValueAddr, 100, **ppValueAddr, EXIT);

    pValueAddr = LOS_FindSym(handle, symbolName5);
    ICUNIT_GOTO_EQUAL(pValueAddr, NULL, pValueAddr, EXIT);

    ppValueAddr = LOS_FindSym(handle, symbolName6);
    ICUNIT_GOTO_NOT_EQUAL(ppValueAddr, NULL, ppValueAddr, EXIT);
    ICUNIT_GOTO_EQUAL(**ppValueAddr, 200, **ppValueAddr, EXIT);

EXIT:
    ret = LOS_SoUnload(handle);
    ICUNIT_ASSERT_EQUAL(ret, 0, ret);

    return LOS_OK;
}

VOID ItLosDynlink013(VOID)
{
    TEST_ADD_CASE("ItLosDynlink013", TestCase, TEST_LOS, TEST_DYNLINK, TEST_LEVEL1, TEST_FUNCTION);
}
