
    .text
;    .sect   ".ratConf"
;==============================================================================
;   void moved_CSL_armR5CacheEnableDCache( uint32_t enable )
;==============================================================================
    .global moved_CSL_armR5CacheEnableDCache
moved_CSL_armR5CacheEnableDCache:
    MRC     p15, #0, r1, c1, c0, #0         ; Read SCTLR
    CMP     r0, #0
    BEQ     moved_armR5CacheEnableDCaches_disable
    ORR     r1, r1, #(1<<2)                 ; Set C bit (enable L1 data caches)
    DSB
    MOV     r2, #0
    MCR     p15, #0, r2, c15, c5, #0        ; Invalidate entire data cache
    B       moved_armR5CacheEnableDCaches_00
moved_armR5CacheEnableDCaches_disable:
    BIC     r1, r1, #(1<<2)                 ; Clear C bit (disable L1 data caches)
moved_armR5CacheEnableDCaches_00:
    DSB
    MCR     p15, #0, r1, c1, c0, #0         ; Write modified SCTLR
    BX      lr


    .global moved_CSL_armR5Dsb
moved_CSL_armR5Dsb:
    DSB
    BX      lr


    .global moved_CSL_armR5CacheEnableDCache_1
moved_CSL_armR5CacheEnableDCache_1:
    MRC     p15, #0, r1, c1, c0, #0         ; Read SCTLR
    BX      lr

    .global moved_CSL_armR5CacheEnableDCache_2
moved_CSL_armR5CacheEnableDCache_2:
    MRC     p15, #0, r1, c1, c0, #0         ; Read SCTLR
    CMP     r0, #0
    BIC     r1, r1, #(1<<2)                 ; Clear C bit (disable L1 data caches)
    DSB
    BX      lr

    .global moved_CSL_armR5CacheEnableDCache_3
moved_CSL_armR5CacheEnableDCache_3:
    MRC     p15, #0, r1, c1, c0, #0         ; Read SCTLR
    CMP     r0, #0
    BIC     r1, r1, #(1<<2)                 ; Clear C bit (disable L1 data caches)
    DSB
    MCR     p15, #0, r1, c1, c0, #0         ; Write modified SCTLR
    BX      lr
    
 
 ;----------�� ��ǰ���̵� .asm�ļ��� ����:
;   uint32_t zxlCSL_armR5MpuGetState ( void )
;===============
    .global zxlCSL_armR5MpuGetState
zxlCSL_armR5MpuGetState:
    MRC     p15, #0, r0, c1, c0, #0         ; Read SCTLR (System Control Register)
    ;AND     r0, r0, #0x00000001             ; Get bit  0 ���ڷ���������SCTLR
    BX      lr								; ����ֵ��r0��

; ����MPU�е�regNum��Region��������Ϣ
;	int32_t zxlCSL_armR5MpuGetRegionInfo( uint32_t regNum, uint32_t           addr ) 
; ע�⣻ addr��ʵ�� һ���ṹ����zxlCSL_armR5MpuRegion_t�ĵ�ַ��32λ��ַ�������ڴ��3��uint32_t��ֵ 
;==============
    .global zxlCSL_armR5MpuGetRegionInfo
zxlCSL_armR5MpuGetRegionInfo:
    AND     r0, r0, #0xF					; region number should <16
    MCR     p15, #0, r0, c6, c2, #0         ; Write RGNR (MPU Region Number Register)
	MRC 	p15, #0, r3, c6, c1, #0 		; Read MPU Region Base Address Register
	;BIC	   r3, r3, #0x1F			  	    ; Base address must be 16-bit aligned
	STR 	r3, [R1]    

	MRC     p15, #0, r3, c6, c1, #2         ; Read Data MPU Region Size and Enable Register
    STR 	r3, [R1,#4]

	MRC     p15, #0, r3, c6, c1, #4         ; Read MPU Region Access Control Register
    ;BFC     r3,  #13, #18
    ;BFC     r3,  #11, #1
    ;BFC     r3,  #6, #2
	STR 	r3, [R1,#8]

    BX      lr


;   uint32_t zxlCSL_armR5PmuGetPMCR(void )
;==============
    .global zxlCSL_armR5PmuGetPMCR
zxlCSL_armR5PmuGetPMCR:
	MRC p15, #0, r0, c9, c12, #0		; Read PMCR Register
	BX	lr


;  void zxlCSL_armR5PmuGetRegs(  uint32_t addr )
;  ע�����addr  ָ��һ���ṹ�ĵ�ַ��32λ��ַ�������ڱ���Ĵ���ֵ���� zxlCSL_armR5PmuRegs_t:CPSR PMCR PMCNTENSET PMUSERENR 
;============
    .global zxlCSL_armR5PmuGetRegs
zxlCSL_armR5PmuGetRegs:
	;ָ������ʱ���г���cpsr_cf, cpsr_all, cpsr_c�Ⱥ�׺�������дָ��MSR��ô��ֻ��Ӱ���׺ָʾ��bits�����
	;		 c ָ	CPSR�е�control field ( PSR[7:0])
	;		 f ָ	flag field (PSR[31:24])
	;		 x ָ	extend field (PSR[15:8])
	;		 s ָ	status field ( PSR[23:16])
	MRS     r3, CPSR       			  ; Read CPSR register   ???
	STR 	r3, [R0]
	
	MRC p15, #0, r3, c9, c12, #0		; Read PMCR Register
	STR 	r3, [R0+4]	

	MRC p15, #0, r3, c9, c12, #1 			; Read PMCNTENSET Register
	STR 	r3, [R0,#8]

    MCR p15, #0, r3, c9, c14, #0        ; Write PMUSERENR Register
	STR 	r3, [R0,#12]

	BX		lr
   
    
    .end

