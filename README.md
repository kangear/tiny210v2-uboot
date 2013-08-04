##2013-08-03 Support 16bit-ECC write


2013-07-27 支持tftp下载内核nfs挂载根文件系统
###
		step1: setenv machid 0xd8a
		step2: setenv bootargs root=/dev/nfs nfsroot=192.168.1.229:/work/rootfs_dir/nfs_rootfs/rootfs_qtopia_qt4 ip=192.168.1.230:192.168.1.229:192.168.1.1:255.255.255.0::eth0:off console=ttySAC0,115200 mem=512M
		step3: tftp uImage_308
		step4: bootm 
说明: 
> 
 - 0xd8a是tiny210v2内核用的机器码
> 
 - bootargs中的路径和ip与你实际情况更改
> 
 - rootfs_qtopia_qt4是rootfs_qtopia_qt4-20130222.tar.gz通过命令行解压得来的 
> 
 - uImage_308是友善光盘中的3.08内核make uImage得来的

###
		[Ver130726-TINY210v2]# tftp uImage_308
		dm9000 i/o: 0x88001000, id: 0x90000a46 
		DM9000: running in 16 bit mode
		MAC: 00:40:5c:26:0a:5b
		operating at 100M full duplex mode
		Using dm9000 device
		TFTP from server 192.168.1.229; our IP address is 192.168.1.230
		Filename 'uImage_308'.
		Load address: 0x21000000
		Loading: #################################################################
         ###################################T ##############################
         #################################################################
         #################################################################
         #################################################################
         ###
		done
		Bytes transferred = 4811624 (496b68 hex)
		[Ver130726-TINY210v2]# bootm  
		## Booting kernel from Legacy Image at 21000000 ...
			Image Name:   Linux-3.0.8-FriendlyARM
			Image Type:   ARM Linux Kernel Image (uncompressed)
			Data Size:    4811560 Bytes = 4.6 MiB
			Load Address: 20008000
			Entry Point:  20008000
			Verifying Checksum ... OK
			Loading Kernel Image ... OK
		OK
		Using machid 0xd8a from environment
		
		Starting kernel ...
		
		Uncompressing Linux... done, booting the kernel.
		[    0.000000] Initializing cgroup subsys cpu
		[    0.000000] Linux version 3.0.8-FriendlyARM (root@kangear) (gcc version 4.5.1 (ctng-1.8.1-FA) ) #1 PREEMPT Fri Jul 26 22:26:05 CST 2013
		[    0.000000] CPU: ARMv7 Processor [412fc082] revision 2 (ARMv7), cr=10c53c7f
		[    0.000000] CPU: VIPT nonaliasing data cache, VIPT aliasing instruction cache
		[    0.000000] Machine: MINI210
		[    0.000000] Memory policy: ECC disabled, Data cache writeback
		.........
		.........
		[   12.274252] libertas_sdio: Unknown symbol lbs_resume (err 0)
		[   12.332482] libertas_sdio: Libertas SDIO driver
		[   12.332536] libertas_sdio: Copyright Pierre Ossman
		Try to bring eth0 interface up......NFS root ...Done

		Please press Enter to activate this console. 
		[root@FriendlyARM /]# ls
		bin      etc      linuxrc  proc     sdcard   usr
		data     home     mnt      root     sys      var
		dev      lib      opt      sbin     tmp      www
		[root@FriendlyARM /]# 



2013-07-26 支持go命令引导内核
###
		BL1 Ver:1307020
		Start cp 
		NAND: 2GB(MLC2) ID:ECD59476
		Running! 
		K

		U-Boot 2013.01-rc2-g17590e5-dirty (Jul 26 2013 - 20:52:08) for TINY210(Nand:K9GAG08U0F)

		CPU:    S5PC110@1000MHz

		Board:   FriendlyARM-TINY210
		DRAM:  512 MiB
		WARNING: Caches not enabled

		PWM Moudle Initialized.
		GPD0CON  : 1111, GPD0DAT  : e
		NAND:  2048 MiB
		MMC:   SAMSUNG SD/MMC: 0, SAMSUNG SD/MMC: 1
		In:    serial
		Out:   serial
		Err:   serial
		Net:   dm9000
		[Ver130726-TINY210v2]# tftp zImage
		dm9000 i/o: 0x88001000, id: 0x90000a46 
		DM9000: running in 16 bit mode
		MAC: 00:40:5c:26:0a:5b
		operating at 100M full duplex mode
		Using dm9000 device
		TFTP from server 192.168.1.229; our IP address is 192.168.1.230
		Filename 'zImage'.
		Load address: 0x21000000
		Loading: #################################################################
				#################################################################
				#################################################################
				#################################################################
				#################################################################
				###
		done
		Bytes transferred = 4811556 (496b24 hex)
		[Ver130726-TINY210v2]# go 21000000
		## Starting application at 0x21000000 ...

		Starting kernel ...

		Uncompressing Linux... done, booting the kernel.
		[    0.000000] Initializing cgroup subsys cpu
		[    0.000000] Linux version 3.0.8-FriendlyARM (root@friendlyarm.tzs) (gcc version 4.5.1 (ctng-1.8.1-FA) ) #1 PREEMPT Sat Feb 2 12:19:04 CST 2013
		[    0.000000] CPU: ARMv7 Processor [412fc082] revision 2 (ARMv7), cr=10c53c7f
		[    0.000000] CPU: VIPT nonaliasing data cache, VIPT aliasing instruction cache
		[    0.000000] Machine: MINI210
		[    0.000000] MINI210: S70 selected
		[    0.000000] INITRD: 0x0000000a+0x3ffb0fd1 is not a memory region - disabling initrd
		[    0.000000] Memory policy: ECC disabled, Data cache writeback
		[    0.000000] CPU S5PV210/S5PC110 (id 0x43110220)
		[    0.000000] S3C24XX Clocks, Copyright 2004 Simtec Electronics
		[    0.000000] S5PV210: PLL settings, A=1000000000, M=667000000, E=80000000 V=54000000
		[    0.000000] S5PV210: ARMCLK=1000000000, HCLKM=200000000, HCLKD=166750000
		[    0.000000] HCLKP=133400000, PCLKM=100000000, PCLKD=83375000, PCLKP=66700000
		[    0.000000] sclk_dmc: source is sclk_a2m (0), rate is 200000000
		[    0.000000] sclk_onenand: source is hclk_dsys (1), rate is 166750000
		[    0.000000] uclk1: source is mout_mpll (6), rate is 66700000


2013-07-23 修改oobsize大小

2013-07-20 用"BL1"代替“spl”，使得BL1和u-boot代码脱离，便于移植(环境变量在SD卡中，请插SD卡)

2013-06-27 Nand/SD卡启动，u-boot.bin/spl二合一。

2013-06-26 开始移植 适用于tiny210v2 NandFlash:K9GAG08U0F
