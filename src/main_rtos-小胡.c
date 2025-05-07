/* =============================================================================
 *   Copyright (c) Texas Instruments Incorporated 2021-2023
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* FreeRTOS Header files */
#include <ti/osal/osal.h>
#include <ti/osal/src/nonos/Nonos_config.h>
#include <ti/osal/TaskP.h>
#include <ti/osal/HwiP.h>

/* CSL Header files */
#include <ti/csl/soc.h>
#include <ti/csl/arch/r5/csl_arm_r5.h>
#include <ti/csl/arch/r5/csl_arm_r5_pmu.h>
#include <ti/csl/csl_gpio.h>
#include <ti/csl/hw_types.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* BOARD Header files */
#include <ti/board/board.h>

/* UART Header files */
#include <ti/drv/uart/UART.h>

#if (defined(BUILD_MCU1_0) && (defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J742S2))) && !(defined(NO_SCISERVER))
#include <ti/drv/sciclient/sciserver.h>
#include <ti/drv/sciclient/sciserver_tirtos.h>
#endif

#ifndef UART_ENABLED

#ifdef BUILD_MCU2_0
#define UART0_ADDR ((int)0x02800000)
#else
#define UART0_ADDR ((int)0x40A00000)
#endif

#define UART_RHR ((int)0x00U)
#define UART_LSR ((int)0x14U)
#define UART_RHR_REG_ADDR ((volatile unsigned int *)(UART0_ADDR + UART_RHR))
#define UART_LSR_REG_ADDR ((volatile unsigned int *)(UART0_ADDR + UART_LSR))

#ifdef BUILD_MCU2_0
__attribute__((section("sbl_mcu_2_0_resetvector"))) void sbl_putc(unsigned char c);
__attribute__((section("sbl_mcu_2_0_resetvector"))) void sbl_puts(char *str);
#elif BUILD_MCU1_0
__attribute__((section("sbl_mcu_1_0_resetvector"))) void sbl_putc(unsigned char c);
__attribute__((section("sbl_mcu_1_0_resetvector"))) void sbl_puts(char *str);
#endif

void sbl_putc(unsigned char c)
{
    while ((*UART_LSR_REG_ADDR & 0x20) == 0)
        ;
    *UART_RHR_REG_ADDR = c;
}
void sbl_puts(char *str)
{
    for (; *str != '\0'; str++)
        sbl_putc(*str);
}
#endif
#include <ti/drv/uart/UART_stdio.h>
static void AppUtils_Printf(const char *pcString, ...);
Board_STATUS Board_uartStdioInit(void);

uint32_t AppUtils_getElapsedTimeInUsec(uint32_t startTime);
uint32_t AppUtils_getCurTimeInUsec(void);

void memoryBenchmarking_initSciclient();

#define APP_TSK_STACK_MAIN (32U * 1024U)
#define APP_SCISERVER_INIT_TSK_STACK (32U * 1024U)

#if defined(BUILD_MCU1_0)
static uint8_t gAppTskStackMain[APP_TSK_STACK_MAIN];
static uint8_t gSciserverInitTskStack[APP_SCISERVER_INIT_TSK_STACK];
#endif

/**
 * *********************************
 *             在此修改模式          *
 * *********************************
 */
#define READ_MODE 0
#define WRITE_MODE 1
#define COPY_MODE 2
// test_mode is used to control the type of test case to be run.
//  0 - Read Mode
//  1 - Write Mode
//  2 - Copy Mode
uint32_t test_mode = COPY_MODE;

#define NUM_TASK 16

#if defined(BUILD_DDR)
#define BUF_SIZE 0x400000 // buf_size/sizeof(uint32_t) 16mb 4194304
#define NUM_TEST 20
/* For each test case the  size of the memcpy buffer can be defined
 * individually. Lesser the value more is the number of cache misses per
 * second but reduced run time as the number of instructions executed
 * reduces and the memcpy instructions still remain in the cache */
uint32_t mem_size_arr[NUM_TEST] = {50, 100, 200, 500, 750, 1000, 1250,
                                   1500, 2048, 4096, 8192, 16384, 32768,
                                   65536, 131072, 262144, 524288, 1048576, 2097152, BUF_SIZE};
#else
#define BUF_SIZE 0x8000 // buf_size/sizeof(uint32_t) 128kb
#define NUM_TEST 16
/* For each test case the  size of the memcpy buffer can be defined
 * individually. Lesser the value more is the number of cache misses per
 * second but reduced run time as the number of instructions executed
 * reduces and the memcpy instructions still remain in the cache */
uint32_t mem_size_arr[NUM_TEST] = {50, 100, 200, 500, 750, 1000, 1250,
                                   1500, 2048, 4096, 8192, 16384, BUF_SIZE};
#endif

#define BUFFER_IN_USE 2
#define TASK_STACK_SIZE 0x1000
#define ITERATION 100

/* Array to hold the bandwidth result in Byte/sec*/
uint32_t bandwidths[NUM_TEST];
/* Array to hold the execution time result in usec*/
uint32_t times[NUM_TEST];

/* Variable to pick up value from the mem_size_arr for each test*/
uint32_t mem_size = 0;

/* Counter for the number of sysbios task switches that occur during the
 * execution of the code
 */
uint32_t num_switches = 0;

TaskP_Handle main_task[NUM_TASK];

/* Array to hold the address of all the source buffers*/
uint32_t *buf[NUM_TASK];
uint32_t sum = 0;
#if defined(BUILD_MCU1_0)

void memoryBenchmarking_setupSciServer(void *arg0, void *arg1)
{

    Sciserver_TirtosCfgPrms_t appPrms;
    int32_t ret = CSL_PASS;

    ret = Sciserver_tirtosInitPrms_Init(&appPrms);

    appPrms.taskPriority[SCISERVER_TASK_USER_LO] = 4;
    appPrms.taskPriority[SCISERVER_TASK_USER_HI] = 5;

    if (ret == CSL_PASS)
    {
        ret = Sciserver_tirtosInit(&appPrms);
    }

    if (ret != CSL_PASS)
    {
        AppUtils_Printf("Starting Sciserver..... FAILED\n");
    }

    return;
}

#endif

/* Source buffers for all the memcpy operations. They can lie in the OCMC
 * or the same memory as the code like flash. The location can be changed
 * from the linker file
 */
uint32_t buf_0[BUF_SIZE] __attribute__((section(".buf_0")));

/* The target buffer for all the memcpy operations */
uint32_t dst[BUF_SIZE] __attribute__((section(".buf_cpy")));

static uint8_t MainApp_TaskStack[TASK_STACK_SIZE] __attribute__((aligned(32)));

#ifdef BUILD_MCU2_0
#if (defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J742S2))
void _system_post_cinit(void) __attribute__((section(".ratConf")));
void _system_post_cinit(void)
{
    osalArch_Config_t cfg;

    cfg.disableIrqOnInit = true;
    osalArch_Init(&cfg);

    /* Setup RAT to load data into MCU2_0 OCM RAM for MCU1_0 */
    /* This is mapping the OCM RAM for MCU2_0 (a 40 bit address) Main domain to 0xD0000000 */
#define RAT_BASE (0x0FF90000)
#define REGION_ID (0x0)
    *(unsigned int *)(RAT_BASE + 0x44 + (REGION_ID * 0x10)) = 0xD0000000; // IN ADDRESS
    *(unsigned int *)(RAT_BASE + 0x48 + (REGION_ID * 0x10)) = 0x02000000;
    *(unsigned int *)(RAT_BASE + 0x4C + (REGION_ID * 0x10)) = 0x0000004F; // Upper 16 bits of the real physical address.
    *(unsigned int *)(RAT_BASE + 0x40 + (REGION_ID * 0x10)) = 0x80000013;
}
#endif
#endif

/* Get a random number depending upon the number of tasks*/
uint32_t get_rand()
{
    return (rand() % NUM_TASK);
}

uint32_t hrs, mins, secs, durationInSecs, usecs;
uint32_t startTime, elapsedTime;
/* Master task which will call the slave tasks randomly and */
void MasterTask(void *a0, void *a1)
{
    AppUtils_Printf("\n\rmaster_task\n\r");

    if (test_mode == COPY_MODE)
    {
        AppUtils_Printf("Copy Mode\n\r");
    }
    else if (test_mode == READ_MODE)
    {
        AppUtils_Printf("Read Mode\n\r");
    }
    else if (test_mode == WRITE_MODE)
    {
        AppUtils_Printf("Write Mode\n\r");
    }

    int i, j;
    uint32_t set = 0, way = 0;
    unsigned int bandwidth = 0;

    AppUtils_Printf("\n\rmaster_task -- start sending\n\r");

    uint32_t count = 0;
    /* this loop is for NUM_TEST with the specified task calls and memcpy_size*/
    for (i = 0; i < NUM_TEST; ++i)
    {
        /* counter to track the task calls already made*/
        count = 0;
        /*reset the value of task switch for that test*/
        num_switches = 0;
        /* size of the memcpy to be performed by each task*/
        mem_size = mem_size_arr[i];
        /*invalidate all the cache to get fresh and reliable data*/
        uint32_t numSets = CSL_armR5CacheGetNumSets();
        uint32_t numWays = CSL_armR5CacheGetNumWays();
        for (set = 0; set < numSets; set++)
        {
            for (way = 0; way < numWays; way++)
            {
                CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
            }
        }

        CSL_armR5CacheInvalidateAllIcache();

        /* reset the PMU counters to get relevant data */
        CSL_armR5PmuResetCntrs();

        startTime = AppUtils_getCurTimeInUsec();
        /*start sending signals and messages to tasks*/
        while (count < ITERATION)
        {
            /*invalidate all the cache to get fresh and reliable data*/
            numSets = CSL_armR5CacheGetNumSets();
            numWays = CSL_armR5CacheGetNumWays();
            for (set = 0; set < numSets; set++)
            {
                for (way = 0; way < numWays; way++)
                {
                    CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
                }
            }
            CSL_armR5CacheInvalidateAllIcache();

            switch (test_mode)
            {
            case READ_MODE:
                for (j = 0; j < mem_size; ++j)
                    sum += buf[0][j];
                break;
            case WRITE_MODE:
                for (j = 0; j < mem_size; ++j)
                    buf[0][j] = 0xDEADBEEF;
                break;
            case COPY_MODE:
                for (j = 0; j < mem_size; ++j)
                    dst[j] = buf[0][j];
                break;
            default:
                AppUtils_Printf("Invalid mem_type! Slave task do nothing. \r\n");
                return;
            }
            count++;
        }

        elapsedTime = AppUtils_getElapsedTimeInUsec(startTime);
        durationInSecs = ((elapsedTime) / 1000U);
        hrs = durationInSecs / (60U * 60U);
        mins = (durationInSecs / 60U) - (hrs * 60U);
        secs = durationInSecs - (hrs * 60U * 60U) - (mins * 60U);
        usecs = elapsedTime - (((hrs * 60U * 60U) + (mins * 60U) + secs) * 1000000U);

        AppUtils_Printf("\nMem Size    => %d\n", (unsigned int)mem_size);
        AppUtils_Printf("Start Time in Usec => %d\n", (unsigned int)startTime);
        AppUtils_Printf("Exec Time in Usec => %d\n", (unsigned int)elapsedTime);
        AppUtils_Printf("Iter            => %d\n", (unsigned int)ITERATION);

        AppUtils_Printf("Bandwidth(Byte/s)  => %d\n", (unsigned int)(1000000 * ((float)(mem_size * ITERATION * 4.0) / (elapsedTime * 1.0))));
        bandwidth = (unsigned int)(1000000 * ((float)(mem_size * ITERATION * 4.0) / (elapsedTime * 1.0)));
        bandwidths[i] = bandwidth;
        times[i] = elapsedTime;

        if (test_mode == WRITE_MODE)
        {
            AppUtils_Printf("Reseting buffers...\n");
            for (j = 0; j < BUF_SIZE; ++j)
            {
                buf[0][j] = j;
            }
        }
        else if (test_mode == COPY_MODE)
        {
            AppUtils_Printf("Reseting buffers...\n");
            for (j = 0; j < BUF_SIZE; ++j)
            {
                dst[j] = 0;
            }
        }
    }
    AppUtils_Printf("\n*****All bandwidths and times*****\n");
    AppUtils_Printf("Total Bytes:(x/4=array_size)\n");
    for (i = 0; i < NUM_TEST; i++)
    {
        AppUtils_Printf("%d\n", mem_size_arr[i]);
    }
    AppUtils_Printf("bandwidths(Byte/s):\n");
    for (i = 0; i < NUM_TEST; i++)
    {
        AppUtils_Printf("%d\n", bandwidths[i]);
    }
    AppUtils_Printf("times(us)):\n");
    for (i = 0; i < NUM_TEST; i++)
    {
        AppUtils_Printf("%d\n", times[i]);
    }
    AppUtils_Printf("\nAll tests have passed\n");

    OS_stop();
}

int do_main(void)
{

    /* refer to r5 csl for PMU API references*/
    CSL_armR5PmuCfg(0, 0, 1);
    CSL_armR5PmuEnableAllCntrs(1);

    CSL_armR5PmuCfgCntr(0, CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_MISS);
    CSL_armR5PmuCfgCntr(1, CSL_ARM_R5_PMU_EVENT_TYPE_I_X);
    CSL_armR5PmuCfgCntr(2, CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_ACCESS);

    CSL_armR5PmuEnableCntrOverflowIntr(0, 0);
    CSL_armR5PmuEnableCntrOverflowIntr(1, 0);
    CSL_armR5PmuEnableCntrOverflowIntr(2, 0);

    CSL_armR5PmuResetCntrs();

    CSL_armR5PmuEnableCntr(0, 1);
    CSL_armR5PmuEnableCntr(1, 1);
    CSL_armR5PmuEnableCntr(2, 1);

    int j;

    Board_STATUS boardInitStatus = 0;

#if defined(UART_ENABLED) && defined(BUILD_MCU1_0) && defined(MULTICORE)
    Board_initCfg cfg = BOARD_INIT_UART_STDIO | BOARD_INIT_PINMUX_CONFIG_MCU | BOARD_INIT_MODULE_CLOCK_MCU;
#ifdef MULTICORE
    cfg = BOARD_INIT_UNLOCK_MMR | BOARD_INIT_UART_STDIO |
          BOARD_INIT_MODULE_CLOCK | BOARD_INIT_PINMUX_CONFIG;
#endif
    /* Use below when trying to use CCS to boot MCU2_0 from DDR or MSMC */
    boardInitStatus = Board_init(cfg);
    // Board_initCfg cfg = BOARD_INIT_UART_STDIO | BOARD_INIT_PINMUX_CONFIG | BOARD_INIT_MODULE_CLOCK;
#else
    boardInitStatus = Board_uartStdioInit();
#endif

#if defined(BUILD_OCMC)
    AppUtils_Printf("Calculating Benchmarks for OCMC memory ");
#elif defined(BUILD_MSMC)
    AppUtils_Printf("Calculating Benchmarks for MSMC memory ");
#elif defined(BUILD_DDR)
    AppUtils_Printf("Calculating Benchmarks for DDR memory ");
#elif defined(BUILD_XIP)
    AppUtils_Printf("Calculating Benchmarks for XIP memory ");
#else
    AppUtils_Printf("Unknown Memory Configuration \n");
#endif

#if defined(BUILD_MCU1_0)
    AppUtils_Printf("on MCU1_0 ");
#elif defined(BUILD_MCU2_0)
    AppUtils_Printf("on MCU2_0 ");
#else
    AppUtils_Printf("Unknown Core Configuration \n");
#endif

#if defined(MULTICORE)
    AppUtils_Printf("(multicore) \n");
#else
    AppUtils_Printf("(single core) \n");
#endif

    if (boardInitStatus != BOARD_SOK)
    {
        AppUtils_Printf("\nBoard_init failure\n");
        return (0);
    }
    AppUtils_Printf("\nBoard_init success\n");

    // Countdown for board init to be fully done
    unsigned long long cnt = 0;
    AppUtils_Printf("Countdown...\n");
    while (cnt++ < 5000000)
    {
        if ((cnt % 1000000) == 0)
        {
            AppUtils_Printf("%d/5\n", (int)(cnt / 1000000));
        }
    }

    buf[0] = buf_0;

    /* Filling up the buffers with if they do not lie in the flash */
    AppUtils_Printf("Filling up the buffers\n");
    for (j = 0; j < BUF_SIZE; ++j)
    {
        buf[0][j] = j;
    }

    /* Creating a task parameter */
    TaskP_Params taskParams;
    /* populating it with default values*/
    TaskP_Params_init(&taskParams);

    /* all tasks will have same priority*/
    taskParams.priority = 1;
    taskParams.stack = MainApp_TaskStack;
    taskParams.stacksize = sizeof(MainApp_TaskStack);

    /* creating master and slave tasks*/
    main_task[0] = TaskP_create(&MasterTask, &taskParams);

    CSL_armR5PmuResetCntrs();
    uint32_t Val0 = CSL_armR5PmuReadCntr(0);
    uint32_t Val2 = CSL_armR5PmuReadCntr(2);
    uint32_t Val1 = CSL_armR5PmuReadCntr(1);

    /* Sanity checking for PMU counters*/
    AppUtils_Printf("Inst Cache Miss: %u\n", Val0);
    AppUtils_Printf("Inst Cache Access: %u\n", Val2);
    AppUtils_Printf("Data Cache Miss: %u\n", Val1);

#if !defined(BUILD_MCU1_0)
    /* Start the BIOS tasks*/
    OS_start();
#endif

    return 0;
}

#if defined(BUILD_MCU1_0)
static void taskFxn(void *a0, void *a1)
{
    Board_initCfg boardCfg;

    /* Initialize SCI Client - It must be called before board init */
    memoryBenchmarking_initSciclient();

    boardCfg = BOARD_INIT_UART_STDIO | BOARD_INIT_PINMUX_CONFIG;

    Board_init(boardCfg);

#if (defined(BUILD_MCU1_0) && (defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J742S2)))
    TaskP_Handle sciserverInitTask;
    TaskP_Params sciserverInitTaskParams;

    /* Initialize SCI Client Server */
    TaskP_Params_init(&sciserverInitTaskParams);
    sciserverInitTaskParams.priority = 6;
    sciserverInitTaskParams.stack = gSciserverInitTskStack;
    sciserverInitTaskParams.stacksize = sizeof(gSciserverInitTskStack);

    sciserverInitTask = TaskP_create(&memoryBenchmarking_setupSciServer, &sciserverInitTaskParams);
    if (NULL == sciserverInitTask)
    {
        OS_stop();
    }
#endif

    do_main();
}
#endif

#if defined(BUILD_MCU2_0) && defined(BUILD_OCMC)
/* giving main a section in this case avoids _system_post_cinit
   becoming a trampoline from OCMRAM that isn't yet mapped by RAT translation
 */
int main(void) __attribute__((section(".main_text")));
#endif

int main(void)
{
#if defined(BUILD_MCU1_0)
    TaskP_Handle task;
    TaskP_Params taskParams;
#endif

    OS_init();

#if defined(BUILD_MCU1_0)
    /* Initialize the task params */
    TaskP_Params_init(&taskParams);
    /* Set the task priority higher than the default priority (1) */
    taskParams.priority = 2;
    taskParams.stack = gAppTskStackMain;
    taskParams.stacksize = sizeof(gAppTskStackMain);

    task = TaskP_create(&taskFxn, &taskParams);
    if (NULL == task)
    {
        OS_stop();
    }
    OS_start(); /* does not return */
#else
    do_main();
#endif
}

/**
 *  \brief Printf utility
 *
 */
#define APP_UTILS_PRINT_MAX_STRING_SIZE (2000U)
void AppUtils_Printf(const char *pcString, ...)
{
    static char printBuffer[APP_UTILS_PRINT_MAX_STRING_SIZE];
    va_list arguments;

    /* Start the varargs processing. */
    va_start(arguments, pcString);
    vsnprintf(printBuffer, sizeof(printBuffer), pcString, arguments);

    {
/* UART Prints do not work in XIP mode */
#ifdef UART_ENABLED
        UART_printf("%s", printBuffer);
#elif defined(CCS)
        printf("%s", printBuffer);
#else
        sbl_puts("\r");
        sbl_puts(printBuffer);
        sbl_puts("\n");
#endif
    }
    /* End the varargs processing. */
    va_end(arguments);

    return;
}

#if defined(BUILD_MCU1_0)

void memoryBenchmarking_initSciclient()
{
    int32_t ret = CSL_PASS;
    Sciclient_ConfigPrms_t config;

    /* Now reinitialize it as default parameter */
    ret = Sciclient_configPrmsInit(&config);
    if (ret != CSL_PASS)
    {
        AppUtils_Printf("Sciclient_configPrmsInit Failed\n");
    }

#if (defined(BUILD_MCU1_0) && (defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4) || defined(SOC_J742S2)))
    if (ret == CSL_PASS)
    {
        ret = Sciclient_boardCfgParseHeader(
            (uint8_t *)SCISERVER_COMMON_X509_HEADER_ADDR,
            &config.inPmPrms, &config.inRmPrms);
        if (ret != CSL_PASS)
        {
            AppUtils_Printf("Sciclient_boardCfgParseHeader Failed\n");
        }
    }
#endif

    if (ret == CSL_PASS)
    {
        ret = Sciclient_init(&config);
        if (ret != CSL_PASS)
        {
            AppUtils_Printf("Sciclient_init Failed\n");
        }
    }
}

#endif

uint32_t AppUtils_getCurTimeInUsec(void)
{
    uint64_t curTimeUsec = 0;

    curTimeUsec = TimerP_getTimeInUsecs();
    return ((uint32_t)curTimeUsec);
}

uint32_t AppUtils_getElapsedTimeInUsec(uint32_t startTime)
{
    uint32_t elapsedTimeUsec = 0U, currTime;

    currTime = AppUtils_getCurTimeInUsec();
    if (currTime < startTime)
    {
        /* Counter overflow occured */
        elapsedTimeUsec = (0xFFFFFFFFU - startTime) + currTime + 1U;
    }
    else
    {
        elapsedTimeUsec = currTime - startTime;
    }

    return (elapsedTimeUsec);
}
