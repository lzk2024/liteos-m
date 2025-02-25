/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
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

#include "los_lms_pri.h"
#include "los_config.h"
#include "los_debug.h"
#if (LOSCFG_KERNEL_SMP == 1)
#include "los_spinlock.h"
#else
#include "los_interrupt.h"
#endif

#if (LOSCFG_BACKTRACE_TYPE != 0)
#include "los_backtrace.h"
#endif
#include "los_sched.h"

LITE_OS_SEC_BSS STATIC LmsMemListNode g_lmsCheckPoolArray[LOSCFG_LMS_MAX_RECORD_POOL_NUM];
LITE_OS_SEC_BSS STATIC LOS_DL_LIST g_lmsCheckPoolList;
STATIC UINT32 g_checkDepth = 0;
LmsHook *g_lms = NULL;

#if (LOSCFG_KERNEL_SMP == 1)
LITE_OS_SEC_BSS SPIN_LOCK_INIT(g_lmsSpin);
#define LMS_LOCK(state)                 LOS_SpinLockSave(&g_lmsSpin, &(state))
#define LMS_UNLOCK(state)               LOS_SpinUnlockRestore(&g_lmsSpin, (state))
#else
#define LMS_LOCK(state)                 (state) = LOS_IntLock()
#define LMS_UNLOCK(state)               LOS_IntRestore(state)
#endif

#define OS_MEM_ALIGN_BACK(value, align) (((UINT32)(value)) & ~((UINT32)((align) - 1)))
#define IS_ALIGNED(value, align)        ((((UINTPTR)(value)) & ((UINTPTR)((align) - 1))) == 0)
#define OS_MEM_ALIGN_SIZE               sizeof(UINTPTR)
#define POOL_ADDR_ALIGNSIZE             64
#define LMS_POOL_UNUSED                 0
#define LMS_POOL_USED                   1
#define INVALID_SHADOW_VALUE            0xFFFFFFFF

STATIC UINT32 OsLmsPoolResize(UINT32 size)
{
    return OS_MEM_ALIGN_BACK(LMS_POOL_RESIZE(size), POOL_ADDR_ALIGNSIZE);
}

STATIC LmsMemListNode *OsLmsGetPoolNode(const VOID *pool)
{
    UINTPTR poolAddr = (UINTPTR)pool;
    LmsMemListNode *current = NULL;
    LOS_DL_LIST *listHead = &g_lmsCheckPoolList;

    if (LOS_ListEmpty(&g_lmsCheckPoolList)) {
        goto EXIT;
    }

    LOS_DL_LIST_FOR_EACH_ENTRY(current, listHead, LmsMemListNode, node) {
        if (current->poolAddr == poolAddr) {
            return current;
        }
    }

EXIT:
    return NULL;
}

STATIC LmsMemListNode *OsLmsGetPoolNodeFromAddr(UINTPTR addr)
{
    LmsMemListNode *current = NULL;
    LmsMemListNode *previous = NULL;
    LOS_DL_LIST *listHead = &g_lmsCheckPoolList;

    if (LOS_ListEmpty(&g_lmsCheckPoolList)) {
        return NULL;
    }

    LOS_DL_LIST_FOR_EACH_ENTRY(current, listHead, LmsMemListNode, node) {
        if ((addr < current->poolAddr) || (addr >= (current->poolAddr + current->poolSize))) {
            continue;
        }
        if ((previous == NULL) ||
            ((previous->poolAddr <= current->poolAddr) &&
            ((current->poolAddr + current->poolSize) <= (previous->poolAddr + previous->poolSize)))) {
            previous = current;
        }
    }

    return previous;
}

STATIC LmsMemListNode *OsLmsCheckPoolCreate(VOID)
{
    UINT32 i;
    LmsMemListNode *current = NULL;
    for (i = 0; i < LOSCFG_LMS_MAX_RECORD_POOL_NUM; i++) {
        current = &g_lmsCheckPoolArray[i];
        if (current->used == LMS_POOL_UNUSED) {
            current->used = LMS_POOL_USED;
            return current;
        }
    }
    return NULL;
}

UINT32 LOS_LmsCheckPoolAdd(const VOID *pool, UINT32 size)
{
    UINT32 intSave;
    UINTPTR poolAddr = (UINTPTR)pool;
    UINT32 realSize;
    LmsMemListNode *lmsPoolNode = NULL;

    if (pool == NULL) {
        return 0;
    }

    LMS_LOCK(intSave);

    lmsPoolNode = OsLmsGetPoolNode(pool);
    if (lmsPoolNode != NULL) { /* if pool already on checklist */
        /* Re-initialize the same pool, maybe with different size */
        /* delete the old node, then add a new one */
        lmsPoolNode->used = LMS_POOL_UNUSED;
        LOS_ListDelete(&(lmsPoolNode->node));
    }

    lmsPoolNode = OsLmsCheckPoolCreate();
    if (lmsPoolNode == NULL) {
        PRINT_DEBUG("[LMS]the num of lms check pool is max already !\n");
        LMS_UNLOCK(intSave);
        return 0;
    }
    realSize = OsLmsPoolResize(size);

    lmsPoolNode->poolAddr = poolAddr;
    lmsPoolNode->poolSize = realSize;
    lmsPoolNode->shadowStart = (UINTPTR)poolAddr + realSize;
    lmsPoolNode->shadowSize = poolAddr + size - lmsPoolNode->shadowStart;
    /* init shadow value */
    (VOID)memset_s((VOID *)lmsPoolNode->shadowStart, lmsPoolNode->shadowSize, LMS_SHADOW_AFTERFREE_U8,
                   lmsPoolNode->shadowSize);

    LOS_ListAdd(&g_lmsCheckPoolList, &(lmsPoolNode->node));

    LMS_UNLOCK(intSave);
    return realSize;
}

VOID LOS_LmsCheckPoolDel(const VOID *pool)
{
    UINT32 intSave;
    if (pool == NULL) {
        return;
    }

    LMS_LOCK(intSave);
    LmsMemListNode *delNode = OsLmsGetPoolNode(pool);
    if (delNode == NULL) {
        PRINT_ERR("[LMS]pool %p is not on lms checklist !\n", pool);
        goto RELEASE;
    }
    delNode->used = LMS_POOL_UNUSED;
    LOS_ListDelete(&(delNode->node));

RELEASE:
    LMS_UNLOCK(intSave);
}

VOID OsLmsInit(VOID)
{
    (VOID)memset_s(g_lmsCheckPoolArray, sizeof(g_lmsCheckPoolArray), 0, sizeof(g_lmsCheckPoolArray));
    LOS_ListInit(&g_lmsCheckPoolList);
    static LmsHook hook = {
        .init = LOS_LmsCheckPoolAdd,
        .deInit = LOS_LmsCheckPoolDel,
        .mallocMark = OsLmsLosMallocMark,
        .freeMark = OsLmsLosFreeMark,
        .simpleMark = OsLmsSimpleMark,
        .check = OsLmsCheckValid,
    };
    g_lms = &hook;
}

STATIC INLINE UINT32 OsLmsMem2Shadow(LmsMemListNode *node, UINTPTR memAddr, UINTPTR *shadowAddr, UINT32 *shadowOffset)
{
    if ((memAddr < node->poolAddr) || (memAddr >= node->poolAddr + node->poolSize)) { /* check ptr valid */
        PRINT_ERR("[LMS]memAddr %p is not in pool region [%p, %p)\n", memAddr, node->poolAddr,
            node->poolAddr + node->poolSize);
        return LOS_NOK;
    }

    UINT32 memOffset = memAddr - node->poolAddr;
    *shadowAddr = node->shadowStart + memOffset / LMS_SHADOW_U8_REFER_BYTES;
    *shadowOffset = ((memOffset % LMS_SHADOW_U8_REFER_BYTES) / LMS_SHADOW_U8_CELL_NUM) *
        LMS_SHADOW_BITS_PER_CELL; /* (memOffset % 16) / 4 */
    return LOS_OK;
}

STATIC INLINE VOID OsLmsGetShadowInfo(LmsMemListNode *node, UINTPTR memAddr, LmsAddrInfo *info)
{
    UINTPTR shadowAddr;
    UINT32 shadowOffset;
    UINT32 shadowValue;

    if (OsLmsMem2Shadow(node, memAddr, &shadowAddr, &shadowOffset) != LOS_OK) {
        return;
    }

    shadowValue = ((*(UINT8 *)shadowAddr) >> shadowOffset) & LMS_SHADOW_MASK;
    info->memAddr = memAddr;
    info->shadowAddr = shadowAddr;
    info->shadowOffset = shadowOffset;
    info->shadowValue = shadowValue;
}

VOID OsLmsSetShadowValue(LmsMemListNode *node, UINTPTR startAddr, UINTPTR endAddr, UINT8 value)
{
    UINTPTR shadowStart;
    UINTPTR shadowEnd;
    UINT32 startOffset;
    UINT32 endOffset;

    UINT8 shadowValueMask;
    UINT8 shadowValue;

    /* endAddr -1, then we mark [startAddr, endAddr) to value */
    if (OsLmsMem2Shadow(node, startAddr, &shadowStart, &startOffset) ||
        OsLmsMem2Shadow(node, endAddr - 1, &shadowEnd, &endOffset)) {
        return;
    }

    if (shadowStart == shadowEnd) { /* in the same u8 */
        /* because endAddr - 1, the endOffset falls into the previous cell,
        so endOffset + 2 is required for calculation */
        shadowValueMask = LMS_SHADOW_MASK_U8;
        shadowValueMask =
            (shadowValueMask << startOffset) & (~(shadowValueMask << (endOffset + LMS_SHADOW_BITS_PER_CELL)));
        shadowValue = value & shadowValueMask;
        *(UINT8 *)shadowStart &= ~shadowValueMask;
        *(UINT8 *)shadowStart |= shadowValue;
    } else {
        /* Adjust startAddr to left util it reach the beginning of a u8 */
        if (startOffset > 0) {
            shadowValueMask = LMS_SHADOW_MASK_U8;
            shadowValueMask = shadowValueMask << startOffset;
            shadowValue = value & shadowValueMask;
            *(UINT8 *)shadowStart &= ~shadowValueMask;
            *(UINT8 *)shadowStart |= shadowValue;
            shadowStart += 1;
        }

        /* Adjust endAddr to right util it reach the end of a u8 */
        if (endOffset < (LMS_SHADOW_U8_CELL_NUM - 1) * LMS_SHADOW_BITS_PER_CELL) {
            shadowValueMask = LMS_SHADOW_MASK_U8;
            shadowValueMask &= ~(shadowValueMask << (endOffset + LMS_SHADOW_BITS_PER_CELL));
            shadowValue = value & shadowValueMask;
            *(UINT8 *)shadowEnd &= ~shadowValueMask;
            *(UINT8 *)shadowEnd |= shadowValue;
            shadowEnd -= 1;
        }

        if (shadowEnd + 1 > shadowStart) {
            (VOID)memset((VOID *)shadowStart, value & LMS_SHADOW_MASK_U8, shadowEnd + 1 - shadowStart);
        }
    }
}

VOID OsLmsGetShadowValue(LmsMemListNode *node, UINTPTR addr, UINT32 *shadowValue)
{
    UINTPTR shadowAddr;
    UINT32 shadowOffset;
    if (OsLmsMem2Shadow(node, addr, &shadowAddr, &shadowOffset) != LOS_OK) {
        return;
    }

    *shadowValue = ((*(UINT8 *)shadowAddr) >> shadowOffset) & LMS_SHADOW_MASK;
}

VOID OsLmsSimpleMark(UINTPTR startAddr, UINTPTR endAddr, UINT32 value)
{
    UINT32 intSave;
    if (endAddr <= startAddr) {
        PRINT_DEBUG("[LMS]mark 0x%x, 0x%x, 0x%x\n", startAddr, endAddr, (UINTPTR)__builtin_return_address(0));
        return;
    }

    if (!IS_ALIGNED(startAddr, OS_MEM_ALIGN_SIZE) || !IS_ALIGNED(endAddr, OS_MEM_ALIGN_SIZE)) {
        PRINT_ERR("[LMS]mark addr is not aligned! 0x%x, 0x%x\n", startAddr, endAddr);
        return;
    }

    LMS_LOCK(intSave);

    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(startAddr);
    if (node == NULL) {
        LMS_UNLOCK(intSave);
        return;
    }

    OsLmsSetShadowValue(node, startAddr, endAddr, value);
    LMS_UNLOCK(intSave);
}

VOID OsLmsLosMallocMark(const VOID *curNodeStart, const VOID *nextNodeStart, UINT32 nodeHeadSize)
{
    UINT32 intSave;
    UINTPTR curNodeStartAddr = (UINTPTR)curNodeStart;
    UINTPTR nextNodeStartAddr = (UINTPTR)nextNodeStart;

    LMS_LOCK(intSave);
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr((UINTPTR)curNodeStart);
    if (node == NULL) {
        LMS_UNLOCK(intSave);
        return;
    }

    OsLmsSetShadowValue(node, curNodeStartAddr, curNodeStartAddr + nodeHeadSize, LMS_SHADOW_REDZONE_U8);
    OsLmsSetShadowValue(node, curNodeStartAddr + nodeHeadSize, nextNodeStartAddr, LMS_SHADOW_ACCESSIBLE_U8);
    OsLmsSetShadowValue(node, nextNodeStartAddr, nextNodeStartAddr + nodeHeadSize, LMS_SHADOW_REDZONE_U8);
    LMS_UNLOCK(intSave);
}

VOID OsLmsCheckValid(UINTPTR checkAddr, BOOL isFreeCheck)
{
    UINT32 intSave;
    UINT32 shadowValue = INVALID_SHADOW_VALUE;
    LMS_LOCK(intSave);
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(checkAddr);
    if (node == NULL) {
        LMS_UNLOCK(intSave);
        return;
    }

    OsLmsGetShadowValue(node, checkAddr, &shadowValue);
    LMS_UNLOCK(intSave);
    if ((shadowValue == LMS_SHADOW_ACCESSIBLE) || ((isFreeCheck) && (shadowValue == LMS_SHADOW_PAINT))) {
        return;
    }

    OsLmsReportError(checkAddr, MEM_REGION_SIZE_1, isFreeCheck ? FREE_ERRORMODE : COMMON_ERRMODE);
}

VOID OsLmsLosFreeMark(const VOID *curNodeStart, const VOID *nextNodeStart, UINT32 nodeHeadSize)
{
    UINT32 intSave;
    UINT32 shadowValue = INVALID_SHADOW_VALUE;

    LMS_LOCK(intSave);
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr((UINTPTR)curNodeStart);
    if (node == NULL) {
        LMS_UNLOCK(intSave);
        return;
    }

    UINTPTR curNodeStartAddr = (UINTPTR)curNodeStart;
    UINTPTR nextNodeStartAddr = (UINTPTR)nextNodeStart;

    OsLmsGetShadowValue(node, curNodeStartAddr + nodeHeadSize, &shadowValue);
    if ((shadowValue != LMS_SHADOW_ACCESSIBLE) && (shadowValue != LMS_SHADOW_PAINT)) {
        LMS_UNLOCK(intSave);
        OsLmsReportError(curNodeStartAddr + nodeHeadSize, MEM_REGION_SIZE_1, FREE_ERRORMODE);
        return;
    }

    if (*((UINT8 *)curNodeStart) == 0) { /* if merge the node has memset with 0 */
        OsLmsSetShadowValue(node, curNodeStartAddr, curNodeStartAddr + nodeHeadSize, LMS_SHADOW_AFTERFREE_U8);
    }
    OsLmsSetShadowValue(node, curNodeStartAddr + nodeHeadSize, nextNodeStartAddr, LMS_SHADOW_AFTERFREE_U8);

    if (*((UINT8 *)nextNodeStart) == 0) { /* if merge the node has memset with 0 */
        OsLmsSetShadowValue(node, nextNodeStartAddr, nextNodeStartAddr + nodeHeadSize, LMS_SHADOW_AFTERFREE_U8);
    }

    LMS_UNLOCK(intSave);
}

VOID LOS_LmsAddrProtect(UINTPTR addrStart, UINTPTR addrEnd)
{
    UINT32 intSave;
    if (addrEnd <= addrStart) {
        return;
    }
    LMS_LOCK(intSave);
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(addrStart);
    if (node != NULL) {
        OsLmsSetShadowValue(node, addrStart, addrEnd, LMS_SHADOW_REDZONE_U8);
    }
    LMS_UNLOCK(intSave);
}

VOID LOS_LmsAddrDisableProtect(UINTPTR addrStart, UINTPTR addrEnd)
{
    UINT32 intSave;
    if (addrEnd <= addrStart) {
        return;
    }
    LMS_LOCK(intSave);
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(addrStart);
    if (node != NULL) {
        OsLmsSetShadowValue(node, addrStart, addrEnd, LMS_SHADOW_ACCESSIBLE_U8);
    }
    LMS_UNLOCK(intSave);
}

STATIC UINT32 OsLmsCheckAddr(UINTPTR addr)
{
    UINT32 intSave;
    UINT32 shadowValue = INVALID_SHADOW_VALUE;
    /* do not check nested or before all cpu start */
    LMS_LOCK(intSave);
    if ((g_checkDepth != 0) || (!g_taskScheduled)) {
        LMS_UNLOCK(intSave);
        return 0;
    }

    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(addr);
    if (node == NULL) {
        LMS_UNLOCK(intSave);
        return LMS_SHADOW_ACCESSIBLE_U8;
    }

    OsLmsGetShadowValue(node, addr, &shadowValue);
    LMS_UNLOCK(intSave);
    return shadowValue;
}

#if (LOSCFG_LMS_CHECK_STRICT == 1)
STATIC INLINE UINT32 OsLmsCheckAddrRegion(UINTPTR addr, UINT32 size)
{
    UINT32 i;
    for (i = 0; i < size; i++) {
        if (OsLmsCheckAddr(addr + i)) {
            return LOS_NOK;
        }
    }
    return LOS_OK;
}

#else
STATIC INLINE UINT32 OsLmsCheckAddrRegion(UINTPTR addr, UINT32 size)
{
    if (OsLmsCheckAddr(addr) || OsLmsCheckAddr(addr + size - 1)) {
        return LOS_NOK;
    } else {
        return LOS_OK;
    }
}
#endif

VOID OsLmsPrintPoolListInfo(VOID)
{
    UINT32 count = 0;
    UINT32 intSave;
    LmsMemListNode *current = NULL;
    LOS_DL_LIST *listHead = &g_lmsCheckPoolList;

    LMS_LOCK(intSave);
    LOS_DL_LIST_FOR_EACH_ENTRY(current, listHead, LmsMemListNode, node)
    {
        count++;
        PRINT_DEBUG(
            "[LMS]memory pool[%1u]: totalsize 0x%-8x  memstart 0x%-8x memstop 0x%-8x memsize 0x%-8x shadowstart 0x%-8x "
            "shadowSize 0x%-8x\n",
            count, current->poolSize + current->shadowSize, current->poolAddr, current->poolAddr + current->poolSize,
            current->poolSize, current->shadowStart, current->shadowSize);
    }

    LMS_UNLOCK(intSave);
}

VOID OsLmsPrintMemInfo(UINTPTR addr)
{
#define LMS_DUMP_OFFSET 16
#define LMS_DUMP_RANGE_DOUBLE 2

    PRINTK("\n[LMS] Dump info around address [0x%8x]:\n", addr);
    const UINT32 printY = LMS_DUMP_OFFSET * LMS_DUMP_RANGE_DOUBLE + 1;
    const UINT32 printX = LMS_MEM_BYTES_PER_SHADOW_CELL * LMS_DUMP_RANGE_DOUBLE;
    UINTPTR dumpAddr = addr - addr % printX - LMS_DUMP_OFFSET * printX;
    UINT32 shadowValue = 0;
    UINTPTR shadowAddr = 0;
    UINT32 shadowOffset = 0;
    LmsMemListNode *nodeInfo = NULL;
    INT32 isCheckAddr, x, y;

    nodeInfo = OsLmsGetPoolNodeFromAddr(addr);
    if (nodeInfo == NULL) {
        PRINT_ERR("[LMS]addr is not in checkpool\n");
        return;
    }

    for (y = 0; y < printY; y++, dumpAddr += printX) {
        if (dumpAddr < nodeInfo->poolAddr) { /* find util dumpAddr in pool region */
            continue;
        }

        if ((dumpAddr + printX) >=
            nodeInfo->poolAddr + nodeInfo->poolSize) { /* finish if dumpAddr exceeds pool's upper region */
            goto END;
        }

        PRINTK("\n\t[0x%x]: ", dumpAddr);
        for (x = 0; x < printX; x++) {
            if ((dumpAddr + x) == addr) {
                PRINTK("[%02x]", *(UINT8 *)(dumpAddr + x));
            } else {
                PRINTK(" %02x ", *(UINT8 *)(dumpAddr + x));
            }
        }

        if (OsLmsMem2Shadow(nodeInfo, dumpAddr, &shadowAddr, &shadowOffset) != LOS_OK) {
            goto END;
        }

        PRINTK("|\t[0x%x | %2u]: ", shadowAddr, shadowOffset);

        for (x = 0; x < printX; x += LMS_MEM_BYTES_PER_SHADOW_CELL) {
            OsLmsGetShadowValue(nodeInfo, dumpAddr + x, &shadowValue);
            isCheckAddr = dumpAddr + x - (UINTPTR)addr + LMS_MEM_BYTES_PER_SHADOW_CELL;
            if ((isCheckAddr > 0) && (isCheckAddr <= LMS_MEM_BYTES_PER_SHADOW_CELL)) {
                PRINTK("[%1x]", shadowValue);
            } else {
                PRINTK(" %1x ", shadowValue);
            }
        }
    }
END:
    PRINTK("\n");
}

STATIC VOID OsLmsGetErrorInfo(UINTPTR addr, UINT32 size, LmsAddrInfo *info)
{
    LmsMemListNode *node = OsLmsGetPoolNodeFromAddr(addr);
    OsLmsGetShadowInfo(node, addr, info);
    if (info->shadowValue != LMS_SHADOW_ACCESSIBLE_U8) {
        return;
    } else {
        OsLmsGetShadowInfo(node, addr + size - 1, info);
    }
}

STATIC VOID OsLmsPrintErrInfo(LmsAddrInfo *info, UINT32 errMod)
{
    switch (info->shadowValue) {
        case LMS_SHADOW_AFTERFREE:
            PRINT_ERR("Use after free error detected\n");
            break;
        case LMS_SHADOW_REDZONE:
            PRINT_ERR("Heap buffer overflow error detected\n");
            break;
        case LMS_SHADOW_ACCESSIBLE:
            PRINT_ERR("No error\n");
            break;
        default:
            PRINT_ERR("UnKnown Error detected\n");
            break;
    }

    switch (errMod) {
        case FREE_ERRORMODE:
            PRINT_ERR("Illegal Double free address at: [0x%lx]\n", info->memAddr);
            break;
        case LOAD_ERRMODE:
            PRINT_ERR("Illegal READ address at: [0x%lx]\n", info->memAddr);
            break;
        case STORE_ERRMODE:
            PRINT_ERR("Illegal WRITE address at: [0x%lx]\n", info->memAddr);
            break;
        case COMMON_ERRMODE:
            PRINT_ERR("Common Error at: [0x%lx]\n", info->memAddr);
            break;
        default:
            PRINT_ERR("UnKnown Error mode at: [0x%lx]\n", info->memAddr);
            break;
    }

    PRINT_ERR("Shadow memory address: [0x%lx : %1u]  Shadow memory value: [%u] \n", info->shadowAddr,
        info->shadowOffset, info->shadowValue);
}

VOID OsLmsReportError(UINTPTR p, UINT32 size, UINT32 errMod)
{
    UINT32 intSave;
    LmsAddrInfo info;

    LMS_LOCK(intSave);
    g_checkDepth += 1;
    (VOID)memset_s(&info, sizeof(LmsAddrInfo), 0, sizeof(LmsAddrInfo));

    PRINT_ERR("*****  Kernel Address Sanitizer Error Detected Start *****\n");

    OsLmsGetErrorInfo(p, size, &info);

    OsLmsPrintErrInfo(&info, errMod);
#if (LOSCFG_BACKTRACE_TYPE != 0)
    LOS_BackTrace();
#endif
    OsLmsPrintMemInfo(info.memAddr);
    g_checkDepth -= 1;
    LMS_UNLOCK(intSave);
    PRINT_ERR("*****  Kernel Address Sanitizer Error Detected End *****\n");
}

#if (LOSCFG_LMS_STORE_CHECK == 1)
VOID __asan_store1_noabort(UINTPTR p)
{
    if (OsLmsCheckAddr(p) != LMS_SHADOW_ACCESSIBLE_U8) {
        OsLmsReportError(p, MEM_REGION_SIZE_1, STORE_ERRMODE);
    }
}

VOID __asan_store2_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_2) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_2, STORE_ERRMODE);
    }
}

VOID __asan_store4_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_4) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_4, STORE_ERRMODE);
    }
}

VOID __asan_store8_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_8) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_8, STORE_ERRMODE);
    }
}

VOID __asan_store16_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_16) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_16, STORE_ERRMODE);
    }
}

VOID __asan_storeN_noabort(UINTPTR p, UINT32 size)
{
    if (OsLmsCheckAddrRegion(p, size) != LOS_OK) {
        OsLmsReportError(p, size, STORE_ERRMODE);
    }
}
#else
VOID __asan_store1_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_store2_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_store4_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_store8_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_store16_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_storeN_noabort(UINTPTR p, UINT32 size)
{
    (VOID)p;
    (VOID)size;
}

#endif

#if (LOSCFG_LMS_LOAD_CHECK == 1)
VOID __asan_load1_noabort(UINTPTR p)
{
    if (OsLmsCheckAddr(p) != LMS_SHADOW_ACCESSIBLE_U8) {
        OsLmsReportError(p, MEM_REGION_SIZE_1, LOAD_ERRMODE);
    }
}

VOID __asan_load2_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_2) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_2, LOAD_ERRMODE);
    }
}

VOID __asan_load4_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_4) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_4, LOAD_ERRMODE);
    }
}

VOID __asan_load8_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_8) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_8, LOAD_ERRMODE);
    }
}

VOID __asan_load16_noabort(UINTPTR p)
{
    if (OsLmsCheckAddrRegion(p, MEM_REGION_SIZE_16) != LOS_OK) {
        OsLmsReportError(p, MEM_REGION_SIZE_16, LOAD_ERRMODE);
    }
}

VOID __asan_loadN_noabort(UINTPTR p, UINT32 size)
{
    if (OsLmsCheckAddrRegion(p, size) != LOS_OK) {
        OsLmsReportError(p, size, LOAD_ERRMODE);
    }
}
#else
VOID __asan_load1_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_load2_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_load4_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_load8_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_load16_noabort(UINTPTR p)
{
    (VOID)p;
}

VOID __asan_loadN_noabort(UINTPTR p, UINT32 size)
{
    (VOID)p;
    (VOID)size;
}
#endif
VOID __asan_handle_no_return(VOID)
{
    return;
}
