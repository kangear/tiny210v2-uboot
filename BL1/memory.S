#include "s5pv210.h"

// OK MemControl	BL=4, 1Chip, DDR2 Type, dynamic self refresh, force precharge, dynamic power down off
#define DMC0_MEMCONTROL			0x00202400
// OK MemConfig0	256MB config, 8 banks,Mapping Method[12:15]0:linear, 1:linterleaved, 2:Mixed
#define DMC0_MEMCONFIG_0		0x20E00323	
// MemConfig1
#define DMC0_MEMCONFIG_1		0x20E00323	

// OK TimingAref   7.8us*133MHz=1038(0x40E), 100MHz=780(0x30C), 20MHz=156(0x9C), 10MHz=78(0x4E)
#define DMC0_TIMINGA_REF        0x00000618
// OK TimingRow    for @200MHz
#define DMC0_TIMING_ROW         0x2B34438A
// OK TimingData   CL=3
#define DMC0_TIMING_DATA        0x24240000
// OK TimingPower
#define DMC0_TIMING_PWR         0x0BDC0343      

.globl mem_init
mem_init:
	// 1. 设置DMC0 Drive Strength (Setting 2X)
	ldr	r0, =ELFIN_GPIO_BASE
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_0DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_1DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_2DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_3DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_4DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_5DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_6DRV_SR_OFFSET]
	ldr	r1, =0x0000AAAA
	str	r1, [r0, #MP1_7DRV_SR_OFFSET]
	ldr	r1, =0x00002AAA
	str	r1, [r0, #MP1_8DRV_SR_OFFSET]

	// 2. 初始化PHY DLL
	ldr	r0, =APB_DMC_0_BASE
	//step 3: PhyControl0 DLL parameter setting, manual 0x00101000
	ldr	r1, =0x00101000					
	str	r1, [r0, #DMC_PHYCONTROL0]
	//PhyControl1 DLL parameter setting, LPDDR/LPDDR2 Case
	ldr	r1, =0x00000086					
	str	r1, [r0, #DMC_PHYCONTROL1]
	
	//step 2: PhyControl0 DLL on
	ldr	r1, =0x00101002					
	str	r1, [r0, #DMC_PHYCONTROL0]
	//step 4: PhyControl0 DLL start
	ldr	r1, =0x00101003					
	str	r1, [r0, #DMC_PHYCONTROL0]

find_lock_val:
	//Loop until DLL is locked
	ldr	r1, [r0, #DMC_PHYSTATUS]			
	and	r2, r1, #0x7
	cmp	r2, #0x7					
	bne	find_lock_val

	//Force Value locking
	and	r1, #0x3fc0
	mov	r2, r1, LSL #18
	orr	r2, r2, #0x100000
	orr	r2 ,r2, #0x1000
	orr	r1, r2, #0x3					
	str	r1, [r0, #DMC_PHYCONTROL0]

	// 3. 初始化DMC0
	//step 5: ConControl auto refresh off
	ldr	r1, =0x0FFF2010					
	str	r1, [r0, #DMC_CONCONTROL]
	//step 6: MemControl BL=4, 1 chip, DDR2 type, dynamic self refresh, force precharge, dynamic power down off
	ldr	r1, =DMC0_MEMCONTROL				
	str	r1, [r0, #DMC_MEMCONTROL]
	//step 7: MemConfig0 256MB config, 8 banks,Mapping Method[12:15]0:linear, 1:linterleaved, 2:Mixed
	ldr	r1, =DMC0_MEMCONFIG_0				
	str	r1, [r0, #DMC_MEMCONFIG0]
	//MemConfig1
	ldr	r1, =DMC0_MEMCONFIG_1				
	str	r1, [r0, #DMC_MEMCONFIG1]
	//step 8:PrechConfig
	ldr	r1, =0xFF000000					
	str	r1, [r0, #DMC_PRECHCONFIG]
	//step 9:TimingAref	7.8us//133MHz=1038(0x40E), 100MHz=780(0x30C), 20MHz=156(0x9C), 10MHz=78(0x4E)
	ldr	r1, =DMC0_TIMINGA_REF				
	str	r1, [r0, #DMC_TIMINGAREF]
	//TimingRow	for //200MHz
	ldr	r1, =DMC0_TIMING_ROW				
	str	r1, [r0, #DMC_TIMINGROW]
	//TimingData	CL=4
	ldr	r1, =DMC0_TIMING_DATA				
	str	r1, [r0, #DMC_TIMINGDATA]
	//TimingPower
	ldr	r1, =DMC0_TIMING_PWR				
	str	r1, [r0, #DMC_TIMINGPOWER]

	// 4. 初始化DDR2 DRAM
	//DirectCmd	chip0 Deselect
	ldr	r1, =0x07000000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//step 16:DirectCmd	chip0 PALL
	ldr	r1, =0x01000000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//step 17:DirectCmd	chip0 EMRS2
	ldr	r1, =0x00020000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//step 18:DirectCmd	chip0 EMRS3
	ldr	r1, =0x00030000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//step 19:DirectCmd	chip0 EMRS1 (MEM DLL on, DQS# disable)
	ldr	r1, =0x00010400					
	str	r1, [r0, #DMC_DIRECTCMD]
	//step 20:DirectCmd	chip0 MRS (MEM DLL reset) CL=4, BL=4
	ldr	r1, =0x00000542					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 PALL
	ldr	r1, =0x01000000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 REFA
	ldr	r1, =0x05000000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 REFA
	ldr	r1, =0x05000000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 MRS (MEM DLL unreset)
	ldr	r1, =0x00000442					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 EMRS1 (OCD default)
	ldr	r1, =0x00010780					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip0 EMRS1 (OCD exit)
	ldr	r1, =0x00010400					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 Deselect
	ldr	r1, =0x07100000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 PALL
	ldr	r1, =0x01100000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 EMRS2
	ldr	r1, =0x00120000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 EMRS3
	ldr	r1, =0x00130000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 EMRS1 (MEM DLL on, DQS# disable)
	ldr	r1, =0x00110400					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 MRS (MEM DLL reset) CL=4, BL=4
	ldr	r1, =0x00100542					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 PALL
	ldr	r1, =0x01100000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 REFA
	ldr	r1, =0x05100000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 REFA
	ldr	r1, =0x05100000					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 MRS (MEM DLL unreset)
	ldr	r1, =0x00100442					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 EMRS1 (OCD default)
	ldr	r1, =0x00110780					
	str	r1, [r0, #DMC_DIRECTCMD]
	//DirectCmd	chip1 EMRS1 (OCD exit)
	ldr	r1, =0x00110400					
	str	r1, [r0, #DMC_DIRECTCMD]
	//ConControl	auto refresh on
	ldr	r1, =0x0FF02030					
	str	r1, [r0, #DMC_CONCONTROL]
	//PwrdnConfig
	ldr	r1, =0xFFFF00FF					
	str	r1, [r0, #DMC_PWRDNCONFIG]
	//MemControl	BL=4, 1 chip, DDR2 type, dynamic self refresh, force precharge, dynamic power down off
	ldr	r1, =0x00202400					
	str	r1, [r0, #DMC_MEMCONTROL]

	mov	pc, lr
