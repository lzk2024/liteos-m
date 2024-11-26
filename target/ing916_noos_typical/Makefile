CMSIS_BASE=./wizard/CMSIS
CMSIS_INC=${CMSIS_BASE}/Include
ING_REL=../sdk
LITEOS_BASE=../../
ING_BUNDLE += ${ING_REL}/bundles/noos_typical/inc

OS_INC += -I ./FreeRTOS/include
OS_INC += -I ./FreeRTOS/portable/GCC/ARM_CM4F
OS_INC += -I ./src
OS_INC += -I $(LITEOS_BASE)kernel/include
OS_INC += -I $(LITEOS_BASE)arch/arm/cortex-m4/gcc
OS_INC += -I $(LITEOS_BASE)utils
OS_INC += -I $(LITEOS_BASE)arch/include
OS_INC += -I $(LITEOS_BASE)lib/libsec/include
OS_INC += -I $(LITEOS_BASE)arch/arm/common
OS_INC += -I $(LITEOS_BASE)utils
OS_INC += -I $(LITEOS_BASE)utils/internal
OS_INC += -I $(LITEOS_BASE)lib/libsec/src/
OS_INC += -I $(LITEOS_BASE)lib/libsec/src/include
OS_INC += -I $(LITEOS_BASE)components/signal

SYMDEFS=-Wl,--just-symbols=${ING_REL}/bundles/noos_typical/ING9168xx/symdefs.g

APPNAME=ing916_noos_typical
LD_NAME=ing916

STARTUP = src/gstartup_ing91600.s

ASMS_SOURCES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_dispatch.S
ASMS_SOURCES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_exc.S

APPDEFS=-D USE_STDPERIPH_DRIVER -DPOWER_SAVING -DLISTEN_TO_POWER_SAVING

PROJECT_FILES = ./src/main.c
PROJECT_FILES += ./src/gen_os_impl.c
PROJECT_FILES += ./src/profile.c
PROJECT_FILES += ./src/task_sample.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_adc.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_gpio.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_i2c.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_pinctrl.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_pwm.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_rtc.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_ssp.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_sysctrl.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_timer.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_uart.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_comparator.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_dma.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_efuse.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_i2s.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_ir.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_keyscan.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_pdm.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_pte.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_qdec.c
PROJECT_FILES += ${ING_REL}/src/FWlib/peripheral_usb.c
PROJECT_FILES += ${ING_REL}/src/Tools/trace.c
PROJECT_FILES += ${ING_REL}/src/BSP/eflash.c

OS_FILES += $(LITEOS_BASE)kernel/src/los_event.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_init.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_mux.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_queue.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_sched.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_sem.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_sortlink.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_swtmr.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_task.c
OS_FILES += $(LITEOS_BASE)kernel/src/los_tick.c

OS_FILES += $(LITEOS_BASE)kernel/src/mm/los_membox.c
OS_FILES += $(LITEOS_BASE)kernel/src/mm/los_memory.c

OS_FILES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_context.c
OS_FILES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_interrupt.c
OS_FILES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_mpu.c
OS_FILES += $(LITEOS_BASE)arch/arm/cortex-m4/gcc/los_timer.c

OS_FILES += $(LITEOS_BASE)arch/arm/common/los_common_interrupt.c

OS_FILES += $(LITEOS_BASE)utils/los_debug.c
OS_FILES += $(LITEOS_BASE)utils/los_error.c
OS_FILES += $(LITEOS_BASE)utils/los_hook.c

OS_FILES += $(LITEOS_BASE)lib/libsec/src/memcpy_s.c
OS_FILES += $(LITEOS_BASE)lib/libsec/src/memset_s.c
OS_FILES += $(LITEOS_BASE)lib/libsec/src/strncpy_s.c
OS_FILES += $(LITEOS_BASE)components/signal/los_signal.c

include makefile.conf
