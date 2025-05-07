/** \brief zxl: 查询MPU是enable还是disable
 *  \return 返回完整的SCTLR (System Control Register)值，其bit 0=0 for MPU disabled, 1 for MPU enabled
 */
extern uint32_t zxlCSL_armR5MpuGetState(void);

// For store MPU region's info
typedef struct zxlCSL_armR5MpuRegion_s
{
	uint32_t base_reg;		  /* MPU Region Base Address value */
	uint32_t size_enable_reg; /* Region Size and Enable Bits (and Subregion En) */
	uint32_t permissions_reg; /* The MPU region access control */
} zxlCSL_armR5MpuRegion_t;

/** \brief zxl:读出MPU中第regNum个Region的配置信息
 *	\param 	regNum 要读哪个region的配置信息
 *  \param	addr  指向一个结构的地址（32位地址），用于写3个uint32_t的值，例如zxlCSL_armR5MpuRegion_t：base, size, accessControl/permissions
 */
extern void zxlCSL_armR5MpuGetRegionInfo(uint32_t regNum, uint32_t addr);

/** \brief zxl: Get counter's event type
 *  \param cntrNum   which counter? 0, 1 or 2
 *  \return the event type for this counter  CSL_ArmR5PmuEventType
 */
extern uint32_t zxlCSL_armR5PmuGetCntrEventType(uint32_t cntrNum);

/** \brief zxl: Get counters enable state
 *  \return the PMCNTENSET register's value
 */
extern uint32_t zxlCSL_armR5PmuGetPMCNTENSET(void);

/** \brief zxl: Get Performance Monitor Control Register
 *  \return the PMCR register's value
 */
extern uint32_t zxlCSL_armR5PmuGetPMCR(void);

// For store CPSR and PMU regs  value
typedef struct zxlCSL_armR5PmuRegs_s
{
	uint32_t cpsr_reg;	   // you can check current processor's mode from CPSR[4:0]
	uint32_t control_reg;  // PMU's PMCR reg's value
	uint32_t enable_reg;   // PMCNTENSET reg
	uint32_t usr_mode_reg; // PMUSERENR reg
} zxlCSL_armR5PmuRegs_t;

/** \brief zxl:读出CPSR、PMU的控制、使能及访问模式寄存器
 *  \param	addr  指向一个结构的地址（32位地址），用于保存寄存器值例如 zxlCSL_armR5PmuRegs_t
 */
extern void zxlCSL_armR5PmuGetRegs(uint32_t addr);

// 把PMU的3个counter配置为指定的事件，清除溢出标志，diable计数器、配置事件、复位然后启动，注意这儿不涉及cycle counter
static void config_pmu_counters(CSL_ArmR5PmuEventType event0, CSL_ArmR5PmuEventType event1, CSL_ArmR5PmuEventType event2)
{
	uint32_t val;
	// 清除可能存在的溢出标志
	val = CSL_armR5PmuReadCntrOverflowStatus();
	val &= 0x80000007;
	CSL_armR5PmuClearCntrOverflowStatus(val);

	// 先禁止计数器，配置好后再启用，然后还要复位
	CSL_armR5PmuEnableCntr(0, 0);
	CSL_armR5PmuEnableCntr(1, 0);
	CSL_armR5PmuEnableCntr(2, 0);

	CSL_armR5PmuCfgCntr(0, event0);
	CSL_armR5PmuCfgCntr(1, event1);
	CSL_armR5PmuCfgCntr(2, event2);

	// 启用3个计数器 */
	CSL_armR5PmuEnableCntr(0, 1);
	CSL_armR5PmuEnableCntr(1, 1);
	CSL_armR5PmuEnableCntr(2, 1);

	/* 使能所有计数器 */
	CSL_armR5PmuEnableAllCntrs(1);

	CSL_armR5PmuResetCntrs(); // 虽然设置3个，现在干脆统统reset
}

// zxl: 把可以配成cachable的配成cachable
static void config_mpu_seven_regions(void)
{
	int32_t region;
	uint32_t numRegions;
	zxlCSL_armR5MpuRegion_t mpuRegionDefault, mpuRegion[7];

	// disabl MPU before config regions? 目前没有去disable 每个region
	CSL_armR5MpuEnable(0);

	mpuRegion[0].base_reg = (uint32_t)(0x00000000U);
	mpuRegion[0].size_enable_reg = (uint32_t)((0U << 8U) | (0x1FU << 1U) | (1U));								//=0x3f, 4GB, region enabled
	mpuRegion[0].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (2U << 3U) | (0U << 2U) | (0U << 0U)); //=0x1310 Executable, full access,non-share,reserved

	mpuRegion[1].base_reg = (uint32_t)(0x00000000U);
	mpuRegion[1].size_enable_reg = (uint32_t)((0U << 8U) | (0xEU << 1U) | (1U));								// ATCM,=0x1c 32KB ATCM, region enabled zxl大小改过
	mpuRegion[1].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (4U << 3U) | (0U << 2U) | (0U << 0U)); //=0x320   executable,full access,non-share,cachable meomory but Non-cach Policy

	mpuRegion[2].base_reg = (uint32_t)(0x41C00000U);
	mpuRegion[2].size_enable_reg = (uint32_t)((0U << 8U) | (0x13U << 1U) | (1U));								//=0x27 1MB OSMC MCU_MSRAM_1MB,region enabled zxl大小改过
	mpuRegion[2].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (1U << 3U) | (0U << 2U) | (3U << 0U)); //=0x30b   executable,full access,non-share,inner/outer Write-back,write-allocate

	mpuRegion[3].base_reg = (uint32_t)(0x70000000U);
	mpuRegion[3].size_enable_reg = (uint32_t)((0U << 8U) | (0x16U << 1U) | (1U));								//=0x2d, 8MB MSMC,region enabled
	mpuRegion[3].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (1U << 3U) | (0U << 2U) | (3U << 0U)); //(2U << 0U)); //=0x30b   executable,full access,non-share,resered? zxl改为 inter/outer WB WA

	mpuRegion[4].base_reg = (uint32_t)(0x80000000U);
	mpuRegion[4].size_enable_reg = (uint32_t)((0U << 8U) | (0x1EU << 1U) | (1U));								//=0x3d, 2GB DDR,region enabled
	mpuRegion[4].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (1U << 3U) | (0U << 2U) | (3U << 0U)); //=0x30b	executable,full access,non-share,inner/outer Write-back,write-allocate

	mpuRegion[5].base_reg = (uint32_t)(0x41010000U);
	mpuRegion[5].size_enable_reg = (uint32_t)((0U << 8U) | (0xEU << 1U) | (1U));								// BTCM,=0x1c 32KB BTCM, region enabled
	mpuRegion[5].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (4U << 3U) | (0U << 2U) | (0U << 0U)); //=0x320	executable,full access,non-share,cachable meomory but Non-cach Policy

	mpuRegion[6].base_reg = (uint32_t)(0x50000000U);
	mpuRegion[6].size_enable_reg = (uint32_t)((0U << 8U) | (0x1AU << 1U) | (0U));								//=0x38,512MB 这个不晓得是什么存储 region disabled
	mpuRegion[6].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (1U << 3U) | (0U << 2U) | (3U << 0U)); //=0x30b	executable,full access,non-share,inner/outer Write-back,write-allocate

	/* Default region */
	mpuRegionDefault.base_reg = (uint32_t)0U;
	mpuRegionDefault.size_enable_reg = (uint32_t)0U;
	mpuRegionDefault.permissions_reg = (uint32_t)0U;

	numRegions = CSL_armR5MpuGetNumRegions();

	// disable MPU regions[7....15]
	for (region = (numRegions - 1U); region >= 7; region--)
	{
		/* Set the regions */
		CSL_armR5MpuCfgRegion(region,
							  mpuRegionDefault.base_reg,
							  mpuRegionDefault.size_enable_reg,
							  mpuRegionDefault.permissions_reg);
	}

	// Configure MPU regions[0...6]
	for (region = 6; region >= 0; region--)
	{
		CSL_armR5MpuCfgRegion(region,
							  mpuRegion[region].base_reg,
							  mpuRegion[region].size_enable_reg,
							  mpuRegion[region].permissions_reg);
	}

	// Invalidate caches before MPU Enabled
	CSL_armR5CacheInvalidateAllCache();

	// enable MPU after regions configured
	CSL_armR5MpuEnable(1);
}

// zxl: 整个内存都配成一个 cacheable region，先disable MPU，配好后再enable MPU
static void config_mpu_whole_cacheable_regions(void)
{
	int region;
	uint32_t numRegions;

	zxlCSL_armR5MpuRegion_t mpuRegion[1];

	// disabl MPU before config regions. 目前没有disable 每个region
	CSL_armR5MpuEnable(0);

	numRegions = CSL_armR5MpuGetNumRegions();

	// first disable all MPU regions
	for (region = (numRegions - 1U); region >= 0; region--)
		CSL_armR5MpuCfgRegion(region, 0, 0, 0);

	mpuRegion[0].base_reg = (uint32_t)(0x00000000U);
	mpuRegion[0].size_enable_reg = (uint32_t)((0U << 8U) | (0x1FU << 1U) | (1U)); //=0x3f, 4GB, region enabled
	//=0x332 Execuatable,full access,non-share,cacheable memory with Write-Through,no write-allocate
	mpuRegion[0].permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (6U << 3U) | (0U << 2U) | (2U << 0U));

	CSL_armR5MpuCfgRegion(0,
						  mpuRegion[0].base_reg,
						  mpuRegion[0].size_enable_reg,
						  mpuRegion[0].permissions_reg);

	// Invalidate caches before MPU Enabled
	CSL_armR5CacheInvalidateAllCache();

	// enable MPU after regions configured
	CSL_armR5MpuEnable(1);
}

//.global CSL_armR5MpuEnable
// CSL_armR5MpuEnable:
// MRC     p15, #0, r1, c1, c0, #0         ; Read SCTLR (System Control Register)
// CMP     r0, #0
// BEQ     armR5MpuEnable_disable
// ORR     r1, r1, #0x1                    ; Set M bit to enable MPU
// B       armR5MpuEnable_00
// armR5MpuEnable_disable:
// BIC     r1, r1, #0x1                    ; Clr M bit to disable MPU
// armR5MpuEnable_00:
// DSB
// MCR     p15, #0, r1, c1, c0, #0         ; Write modified SCTLR
// ISB
// BX      lr
//

#define SIZE 1024 // 矩阵大小
static void matrix_multiply()
{
	int a[SIZE][SIZE], b[SIZE][SIZE], result[SIZE][SIZE];

	// 初始化矩阵a和矩阵b
	for (int i = 0; i < SIZE; i++)
	{
		for (int j = 0; j < SIZE; j++)
		{
			a[i][j] = 1; // 初始化矩阵a的每个元素为1
			b[i][j] = 2; // 初始化矩阵b的每个元素为2
		}
	}

	// 矩阵乘法操作
	for (int i = 0; i < SIZE; i++)
	{
		for (int j = 0; j < SIZE; j++)
		{
			result[i][j] = 0;
			for (int k = 0; k < SIZE; k++)
			{
				result[i][j] += a[i][k] * b[k][j];
			}
		}
	}

	// for (int i = 0; i < SIZE; i++) {
	// 	for (int j = 0; j < SIZE; j++) {
	// 		result[i][j] = 0;
	// 		for (int k = 0; k < SIZE; k++) {
	// 			result[i][j] += a[i][k] * b[k][j];
	// 		}
	// 	}
	// }

	// 打印结果矩阵的部分内容（为避免打印过大数据，打印前2行10列）
	printf("Result Matrix (first 2x10 block):\n");
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			printf("%d ", result[i][j]);
		}
		printf("\n");
	}
}

static void print_l1cache_info(void)
{
	uint32_t regSCTLR; // (System Control Register)
	regSCTLR = zxlCSL_armR5MpuGetState();
	// 打印 L1 cache 设置状态
	if (regSCTLR & (1 << 12)) // 位12为1 表示L1I cache enable
		printf("*************** L1I cache is enabled! ********** \n ");
	else
		printf("*************** L1I cache is disabled! ********** \n ");
	if (regSCTLR & (1 << 2)) // 位12为1 表示L1I cache enable
		printf("*************** L1D cache is enaabled! ********** \n ");
	else
		printf("*************** L1D cache is disabled! ********** \n ");
}

// zxl:  print MPU regions info
static void print_mpu_region_info(void)
{
	uint32_t regSCTLR; // (System Control Register)
	regSCTLR = zxlCSL_armR5MpuGetState();

	printf("\nSystem Control reg SCTLR = 0x%x \n", regSCTLR);

	if (regSCTLR & 0x01u)
	{ // 最低位为1 表示MPU enable
		// Mpu 状态为 enable
		printf("*************** MPU is enabled! ********** \n ");
		uint32_t numOfRegions = CSL_armR5MpuGetNumRegions();
		printf("MPU supported number of regisions is %d \n", numOfRegions);

		// 打印 MPU 中每个 region 的信息，倒序的方式即 region 0最后（它的设置参数的优先级最低，可以被其它 region 覆盖）
		zxlCSL_armR5MpuRegion_t mpuRegionInfo;

		mpuRegionInfo.base_reg = 0;
		mpuRegionInfo.size_enable_reg = 0;
		mpuRegionInfo.permissions_reg = 0;

		printf("MPU region's config (number: base, size, permission) \n");

		for (int i = numOfRegions - 1; i >= 0; i--)
		{
			zxlCSL_armR5MpuGetRegionInfo(i, (uint32_t)&mpuRegionInfo);
			printf("%d: 0x%x, 0x%x, 0x%x \n", i, mpuRegionInfo.base_reg,
				   mpuRegionInfo.size_enable_reg, mpuRegionInfo.permissions_reg);
			mpuRegionInfo.size_enable_reg = 0;
		}
	}
	else
	{
		// Mpu状态为diable
		printf("*************** MPU is disabled! ********** \n ");
	}
}

// zxl:  print CPSR and PMU‘s config info,including PMU control reg, user mode, Counter enable reg, EventType for 3 counters
static void print_pmu_conf_info(void)
{
	uint32_t regValue;
	uint32_t event0, event1, event2;
	zxlCSL_armR5PmuRegs_t regs;

	// 读出 CPSR 和 PMU 的配置信息
	printf("---- Conf info for PMU, check and configure if necessary ---- \n");
	zxlCSL_armR5PmuGetRegs((uint32_t)&regs);
	printf("CPSR reg = 0x%x \n", regs.cpsr_reg);
	printf("PMU Control PMCR reg = 0x%x \n", regs.control_reg);
	printf("PMU Counter enable  PMCNTENSET reg = 0x%x \n", regs.enable_reg);
	printf("PMU User mode enable PMUSERENR reg = 0x%x \n", regs.usr_mode_reg);

	// 读出 Performance Monitor Control Register, [3] for cycle counter's clk divider,[4] for Export events,
	regValue = zxlCSL_armR5PmuGetPMCR();
	printf("Performance Monitor Control PMCR reg = 0x%x \n", regValue);

	regValue = zxlCSL_armR5PmuGetPMCNTENSET();
	printf("Counter enable state PMCNTENSET reg = 0x%x \n", regValue);

	// print the event type for 3 event counters
	event0 = zxlCSL_armR5PmuGetCntrEventType(0);
	event1 = zxlCSL_armR5PmuGetCntrEventType(1);
	event2 = zxlCSL_armR5PmuGetCntrEventType(2);

	printf("Get counter's event: 0=0x%x, 1=0x%x, 2=0x%x \n ", event0, event1, event2);
	// 参考手册，检查PMU设置是否与预期相符
}

// zxl:  print PMU‘s 3 counter's value, 目前没有考虑 overflow，只是打印现在的overflow
static void print_pmu_counters(void)
{
	uint32_t overflow, temp0, temp1, temp2;
	overflow = CSL_armR5PmuReadCntrOverflowStatus(); // PMOVSR

	temp0 = CSL_armR5PmuReadCntr(0);
	temp1 = CSL_armR5PmuReadCntr(1);
	temp2 = CSL_armR5PmuReadCntr(2);

	//"print" will affect some event counters, so first read all event counters
	//[31]是cycle counter溢出标志, [0] for counter 0
	printf("Counters' overflow state = 0x%x \n", overflow);
	printf("Counter 0 = 0x%x \n", temp0);
	printf("Counter 1 = 0x%x \n", temp1);
	printf("Counter 2 = 0x%x \n", temp2);
}

// 测试PMU事件计数器，适合测试cache事件，可用于Cache命中率
// 注意：目前版本没有考虑Cortex-R5 PMU 32b计数器的溢出
// 可以替换core_r5_cache_test.c中的cslcore_r5_cacheTest()
void test_pmu_cache_event_counters(void)
{
	uint32_t start_cycle, end_cycle;
	// cslApp_initBoard(); 主函数应该调用过

	// 现状：display MPU Cache PMU status
	print_mpu_region_info();
	print_l1cache_info();
	print_pmu_conf_info(); // 也有CPSR寄存器的值，观察一下

	// 必要时调用CSL_armR5PmuCfg(0,1,1); // uint32_t cycleCntDiv, uint32_t exportEvents, uint32_t userEnable )

	printf("\n------ set counter event 0=DCACHE_MISS 0x03, 1=DCACHE_ACCESS 0x04, 2=ICACHE_ACCESS 0x58 \n ");
	config_pmu_counters(CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_MISS, CSL_ARM_R5_PMU_EVENT_TYPE_DCACHE_ACCESS, CSL_ARM_R5_PMU_EVENT_TYPE_ICACHE_ACCESS); // 配置PMU 3个事件计数器，除外cycle counter
	// 检查刚才的PMU配置是否有效
	print_pmu_conf_info();

	// 注意上述输出信息

	// Invalidate caches and reset event counters before testing
	CSL_armR5CacheInvalidateAllCache();
	CSL_armR5PmuResetCntrs(); // reset all event counters to 0, but not cycle counter
	start_cycle = CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM);

	// 运行一段代码、访问一些数据，产生cache事件
	matrix_multiply();
	// 读出event counters
	print_pmu_counters();
	end_cycle = CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM);
	end_cycle = end_cycle - start_cycle;
	printf("Cycles = %d \n", end_cycle); // 包括了print event counters的时间。如果被测试代码耗时长，要注意cycle counter溢出

	printf("\n-------------config MPU to a whole cacheable memory --------------\n");
	// 整个内存空间配成cachable，与实际硬件cacheable性质不符，会出错吗？
	// 出错就换成另一个函数config_mpu_seven_regions()
	config_mpu_whole_cacheable_regions();
	// 检查上步的配置是否生效
	print_mpu_region_info();

	// Another test with new MPU region's configuration
	//  Invalidate caches and reset event counters before testing
	CSL_armR5CacheInvalidateAllCache();
	CSL_armR5PmuResetCntrs(); // not including the cycle counter
	start_cycle = CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM);
	// 运行一段代码、访问一些数据，产生cache事件
	matrix_multiply();
	// 读出counter
	print_pmu_counters();
	end_cycle = CSL_armR5PmuReadCntr(CSL_ARM_R5_PMU_CYCLE_COUNTER_NUM);
	end_cycle = end_cycle - start_cycle;
	printf("Cycles = %d \n", end_cycle);
	printf("\n -----------------test_pmu_cache_event_counter() end--------------. \n");
}

// zxl: 最高优先级的region 15，把DDR=1，TCMB=2，或MSMC=3 配成non-cacheable region，先disable MPU，配好后再enable MPU
static void config_mpu_noncacheable_region(bool noncacheable, uint32_t which)
{
	zxlCSL_armR5MpuRegion_t mpuRegion;

	// 应该先同步吧
	CSL_armR5Dsb();
	// CSL_armR5Isb();

	// disabl MPU before config regions. 目前没有disable 每个region
	// printf("----------------------config_mpu_noncacheable_ddr_regions 1111111111111111---------\n\n");
	// invalidate cache before disable MPU, 如果不invalidate，CSL_armR5MpuEnable()会异常的
	uint32_t set = 0, way = 0;
	uint32_t numSets = CSL_armR5CacheGetNumSets();
	uint32_t numWays = CSL_armR5CacheGetNumWays();
	for (set = 0; set < numSets; set++)
	{
		for (way = 0; way < numWays; way++)
		{
			CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
		}
	}
	// CSL_armR5CacheInvalidateAllIcache(); //测试过没没有这行，CSL_armR5MpuEnable()不会异常 本程序没有函数放在DDR？

	// CSL_armR5CacheInvalidateAllCache(); //使用这行，有时会异常

	CSL_armR5MpuEnable(0);

	// printf("----------------------config_mpu_noncacheable_ddr_regions 2222222222---------\n\n");

	switch (which)
	{
	case 1: // DDR
		mpuRegion.base_reg = (uint32_t)(0x80000000U);
		mpuRegion.size_enable_reg = (uint32_t)((0U << 8U) | (0x1EU << 1U) | (1U)); //=0x3d, DDR,region enabled
		break;
	case 2: // TCM
		mpuRegion.base_reg = (uint32_t)(0x41010000U);
		mpuRegion.size_enable_reg = (uint32_t)((0U << 8U) | (0xEU << 1U) | (1U)); //=0x1d, TCM,region enabled
		break;
	case 3: // MSMC
		mpuRegion.base_reg = (uint32_t)(0x70000000U);
		mpuRegion.size_enable_reg = (uint32_t)((0U << 8U) | (0x16U << 1U) | (1U)); //=0x2d, TCM,region enabled
		CSL_armR5CacheInvalidateAllIcache();									   // 本程序函数放在MSMC
		break;
	default:
		break;
	}
	if (noncacheable)
		mpuRegion.permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (4U << 3U) | (0U << 2U) | (0U << 0U)); //=0x320   executable,full access,non-share,TEX C B =100 00 cacheable memory but innner and outer non-cache policy
	else
		mpuRegion.permissions_reg = (uint32_t)((0U << 12U) | (3U << 8U) | (5U << 3U) | (0U << 2U) | (1U << 0U)); //=0x329   executable,full access,non-share,TEX C B =101 01 Outer and Inner write-back, write-allocate

	CSL_armR5MpuCfgRegion(15,
						  mpuRegion.base_reg,
						  mpuRegion.size_enable_reg,
						  mpuRegion.permissions_reg);

	// Invalidate caches before MPU Enabled
	// CSL_armR5CacheInvalidateAllCache();
	// invalidate data cache and I-cache
	//         numSets = CSL_armR5CacheGetNumSets();
	//         numWays = CSL_armR5CacheGetNumWays();
	//           for (set = 0; set < numSets; set++)
	//           {
	//               for (way = 0; way < numWays; way++)
	//               {
	//                   CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
	//               }
	//           }
	//         CSL_armR5CacheInvalidateAllIcache();

	//    printf("----------------------config_mpu_noncacheable_ddr_regions 33333333333---------\n\n");

	// enable MPU after regions configured
	CSL_armR5MpuEnable(1);
	// printf("----------------------config_mpu_noncacheable_ddr_regions end-------------\n\n");
}

#define ITER_NUM 1		  // 迭代次数 ,读数组几次？
#define DDR_BUF_SIZE 2048 // 1MB
uint32_t buf_ddr[DDR_BUF_SIZE] __attribute__((section(".buf_ddr")));

// 测试一个数组，在用MPU region cacheable / non-cacheable的情况下，这个数组读写时间变化
void zxl_test_mpu_cacheable_ddr()
{
	uint32_t i, j;
	uint32_t set, way;

	printf("-------zxl testing--DDR --------  \n");
	// print_mpu_region_info(); //缺省，DDR permission 应该是0x329,cacheable WB WA

	for (j = 0; j < DDR_BUF_SIZE; ++j)
		buf_ddr[j] = j;

	// 先把cache清掉，表示新的access
	/////////////////////////////
	CSL_armR5Dsb();

	// invalidate data cache
	uint32_t numSets = CSL_armR5CacheGetNumSets();
	uint32_t numWays = CSL_armR5CacheGetNumWays();
	for (set = 0; set < numSets; set++)
	{
		for (way = 0; way < numWays; way++)
		{
			CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
		}
	}
	// CSL_armR5CacheInvalidateAllIcache(); //测试: 这行没有影响
	/////////////////////////////
	uint64_t sum1, sum2, sum3, start_time, end_time, elapse1, elapse2, elapse3;

	// try to let printf finished
	// 应该替换为延时函数
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}

	// get 3 elapse time
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum1 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum2 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum3 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU default----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1); // 第一个时间常常比后面大，可能是cache miss，可能是printf的影响？
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let noncacheable////////////////////////////

	config_mpu_noncacheable_region(true, 1); // 1表示DDR
	// print_mpu_region_info();				 // for test only
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum1 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum2 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum3 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU non-cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let cacheable  ////////////////////////////
	config_mpu_noncacheable_region(false, 1); // 1表示DDR
	// print_mpu_region_info();				  // for test
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum1 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum2 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < DDR_BUF_SIZE; j++)
			sum3 += buf_ddr[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);
}
#define TCM_BUF_SIZE 2048
uint32_t buf_tcm[TCM_BUF_SIZE] __attribute__((section(".buf_tcm")));

void zxl_test_mpu_cacheable_tcm()
{
	uint32_t set, way, i, j;

	printf("-------zxl testing--TCM --------  \n");
	// print_mpu_region_info(); // TCM permission 应该是0x320,cacheable memory with non-cache policy

	for (j = 0; j < TCM_BUF_SIZE; ++j)
		buf_tcm[j] = j;

	// 先把cache清掉，表示新的access
	/////////////////////////////
	CSL_armR5Dsb();

	// invalidate data cache

	uint32_t numSets = CSL_armR5CacheGetNumSets();
	uint32_t numWays = CSL_armR5CacheGetNumWays();
	for (set = 0; set < numSets; set++)
	{
		for (way = 0; way < numWays; way++)
		{
			CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
		}
	}
	// CSL_armR5CacheInvalidateAllIcache(); //测试: 这行没有影响
	/////////////////////////////
	uint64_t sum1, sum2, sum3, start_time, end_time, elapse1, elapse2, elapse3;

	// try to let printf finished
	// 应该替换为延时函数
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}

	// get 3 elapse time
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum1 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum2 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum3 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU default----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1); // 第一个时间常常比后面大，可能是cache miss，可能是printf的影响？
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let noncacheable////////////////////////////
	config_mpu_noncacheable_region(true, 2); // 2表示TCMB
	// print_mpu_region_info();
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum1 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum2 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum3 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU non-cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let cacheable ////////////////////////////
	config_mpu_noncacheable_region(false, 2); // 2表示TCMB
	// print_mpu_region_info();				  // for test
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum1 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time; //

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum2 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < TCM_BUF_SIZE; j++)
			sum3 += buf_tcm[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);
}

#define MSMC_BUF_SIZE 2048
uint32_t buf_msmc[MSMC_BUF_SIZE] __attribute__((section(".buf_msmc")));

void zxl_test_mpu_cacheable_msmc()
{
	uint32_t set, way, i, j;

	printf("-------zxl testing--MSMC --------  \n");
	// print_mpu_region_info(); // MSMC permission 应该是0x329,cacheable WB WA

	for (j = 0; j < MSMC_BUF_SIZE; ++j)
		buf_msmc[j] = j;

	// 先把cache清掉，表示新的access
	/////////////////////////////
	CSL_armR5Dsb();

	// invalidate data cache

	uint32_t numSets = CSL_armR5CacheGetNumSets();
	uint32_t numWays = CSL_armR5CacheGetNumWays();
	for (set = 0; set < numSets; set++)
	{
		for (way = 0; way < numWays; way++)
		{
			CSL_armR5CacheCleanInvalidateDcacheSetWay(set, way);
		}
	}
	// CSL_armR5CacheInvalidateAllIcache(); //待会测试 这行有没有影响
	/////////////////////////////
	uint64_t sum1, sum2, sum3, start_time, end_time, elapse1, elapse2, elapse3;

	// try to let printf finished
	// 应该替换为延时函数
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}

	// get 3 elapse time
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum1 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum2 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum3 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU default----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1); // 第一个时间常常比后面大，可能是cache miss，可能是printf的影响？
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let noncacheable////////////////////////////
	config_mpu_noncacheable_region(true, 3); // 3表示MSMC
	// print_mpu_region_info();
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum1 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time;

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum2 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum3 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU non-cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);

	///////////////////////// now let cacheable ////////////////////////////
	config_mpu_noncacheable_region(false, 3); // 3表示MSMC
	// print_mpu_region_info();				  // for test
	i = 0;
	while (i < 50000000)
	{
		sum1 = sum1 * (i++);
		sum1 = sum1 / 3;
	}
	sum1 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum1 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse1 = end_time - start_time; //

	sum2 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum2 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse2 = end_time - start_time;

	sum3 = 0;
	start_time = TimerP_getTimeInUsecs();
	for (i = 0; i < ITER_NUM; i++)
		for (j = 0; j < MSMC_BUF_SIZE; j++)
			sum3 += buf_msmc[j];
	end_time = TimerP_getTimeInUsecs();
	elapse3 = end_time - start_time;

	printf("---- testing-MPU cacheable----\n");
	printf("elapse1=%llu----sum1=%llu---  \n", elapse1, sum1);
	printf("elapse2=%llu----sum2=%llu---  \n", elapse2, sum2);
	printf("elapse3=%llu----sum3=%llu---  \n", elapse3, sum3);
}
