/*----------------------------------------------------------------------------*/
/* File: linker_mcu2_0_ddr.lds                                                */
/* Description:                                                               */
/*    Link command file for J721E MCU1_0 view                                 */
/*                                                                            */
/* (c) Texas Instruments 2021-2023, All rights reserved.                           */
/*----------------------------------------------------------------------------*/
/*  History:                                                                  */
/*    Aug 26th, 2016 Original version .......................... Loc Truong   */
/*    Aug 01th, 2017 new TCM mem map  .......................... Loc Truong   */
/*    Nov 07th, 2017 Changes for R5F Init Code.................. Vivek Dhande */
/*    May 14th, 2021 running on J7VCL and HSM..... Caleb Robey & Karan Saxena */
/*    Aug 15th, 2021 port to FreeRTOS and new XIP bootflow....... Caleb Robey */
/*----------------------------------------------------------------------------*/
/* Linker Settings                                                            */
/* Standard linker options                                                    */

--retain="*(.bootCode)"
--retain="*(.startupCode)"
--retain="*(.startupData)"
--retain="*(.irqStack)"
--retain="*(.fiqStack)"
--retain="*(.abortStack)"
--retain="*(.undStack)"
--retain="*(.svcStack)"
--fill_value=0
-e __VECS_ENTRY_POINT

/*----------------------------------------------------------------------------*/
/* Memory Map                                                                 */
--define FILL_PATTERN=0xFEAA55EF
--define FILL_LENGTH=0x100

-stack  0x4000  /* SOFTWARE STACK SIZE */
-heap   0x8000  /* HEAP AREA SIZE      */

--stack_size=0x4000
--heap_size=0x8000
--entry_point=_freertosresetvectors

/*-------------------------------------------*/
/*       Stack Sizes for various modes       */
/*-------------------------------------------*/
__IRQ_STACK_SIZE   = 0x1000;
__FIQ_STACK_SIZE   = 0x0100;
__ABORT_STACK_SIZE = 0x0100;
__UND_STACK_SIZE   = 0x0100;
__SVC_STACK_SIZE   = 0x0100;


/* It is CRUCIAL that the TASK_SIZE be 0x1000 to be sure that the whole cache is filled by
    each task when it is loaded */
--define TASK_SIZE=0x1000
--define TEXT_SIZE=0x40000
--define BUF_SIZE=0x1000000

--define MAIN_OCMCRAM_START=0x03600000
--define MAIN_OCMCRAM_END=0x0367FFFF
--define MSMC_START=0x70080000
--define XIP_MCU1_0_START=0x501c0100
--define DDR_START=0x88000000
--define SBL_RUNTIME_MEM=0x41C01000
--define ATCM_START=0x0
--define NUM_TASK=0x10

MEMORY
{
    RESET_VECTORS (RWIX)	 : origin=ATCM_START	                    length=0x100
    MCU_ATCM   (RWX)         : origin=ATCM_START+0x100                  length=0x8000-0x100

    MCU0_R5F_TCMB0_VECS (X)  : origin=0x41014000                     length=0x100
    MCU0_R5F_TCMB0 (RWIX)	 : origin=0x41014100	                 length=0x4000-0x100

    BUF                     : origin=DDR_START + (BUF_SIZE*0)                    length=BUF_SIZE
    BUF_CPY                  : origin=DDR_START + (BUF_SIZE*1)                   length=BUF_SIZE
    
    TEXT_ARR                 : origin=MAIN_OCMCRAM_START  length=TEXT_SIZE 
    OCMC_RAM (RWIX)          : origin=MAIN_OCMCRAM_START + TEXT_SIZE length=(MAIN_OCMCRAM_END - (MAIN_OCMCRAM_START + TEXT_SIZE))
    MSMC (X)                 : origin=0x70080000                      length=0x80000
}

SECTIONS
{
    .freertosrstvectors                    : {} palign(8) > RESET_VECTORS
    .sbl_mcu_2_0_resetvector               : {} palign(8) > RESET_VECTORS

    .bootCode           : {} palign(8) > MCU_ATCM
    .startupCode        : {} palign(8) > MCU_ATCM
    .startupData        : {} palign(8) > MCU_ATCM, type = NOINIT

    .text_boot {
        r5_mpu_freertos.oer5f (.const:gCslR5MpuCfg)
        *rtsv7R4_A_le_v3D16_eabi.lib*<*> (.text)
    }                                > MSMC

    GROUP {
            .text.hwi       : palign(8)
            .text.cache     : palign(8)
            .text.mpu       : palign(8)
            .text.boot      : palign(8)
            .text.abort     : palign(8)
        }                              > MCU_ATCM


    .text                                  : {} palign(8)      > TEXT_ARR
    .const.devgroup                        : {*(.const.devgroup*)} align(4)     > MSMC
    .const                                 : {} palign(8)      > MSMC
    .rodata                                : {} palign(8)      > MSMC
    .cinit                                 : {} palign(8)      > MSMC
    .pinit                                 : {} palign(8)      > MSMC
    .boardcfg_data                         : {} align(4)       > MSMC
    .bss                                   : {} align(4)       > MSMC
    .data                                  : {} palign(128)    > MSMC
    .sysmem                                : {}                > MSMC
    .bss.devgroup                          : {*(.bss.devgroup*)} align(4)       > MSMC
    
    .buf                                 : {} align(4)         > BUF
    .buf_cpy                               : {} align(4)       > BUF_CPY

    .stack                                 : {} align(4)       > OCMC_RAM (HIGH)

    .irqStack   : {. = . + __IRQ_STACK_SIZE;} align(4)      > OCMC_RAM (HIGH)
    RUN_START(__IRQ_STACK_START)
    RUN_END(__IRQ_STACK_END)

    .fiqStack   : {. = . + __FIQ_STACK_SIZE;} align(4)      > OCMC_RAM (HIGH)
    RUN_START(__FIQ_STACK_START)
    RUN_END(__FIQ_STACK_END)

    .abortStack : {. = . + __ABORT_STACK_SIZE;} align(4)    > OCMC_RAM (HIGH)
    RUN_START(__ABORT_STACK_START)
    RUN_END(__ABORT_STACK_END)

    .undStack   : {. = . + __UND_STACK_SIZE;} align(4)      > OCMC_RAM (HIGH)
    RUN_START(__UND_STACK_START)
    RUN_END(__UND_STACK_END)

    .svcStack   : {. = . + __SVC_STACK_SIZE;} align(4)      > OCMC_RAM (HIGH)
    RUN_START(__SVC_STACK_START)
    RUN_END(__SVC_STACK_END)

}
