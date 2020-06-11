
/*
 *			For userspace header file
 *				
 *
 * */
#ifndef __BXROCE_HW_H__
#define __BXROCE_HW_H__

/*PGU Registers START*/
#define MPB_WRITE_ADDR      0x00
#define MPB_RW_DATA			0x100
#define PGU_BASE 			0x00 /*IN MPB, PGU IS 0x00*/

#define GENRSP 				0x2000	/*whether CQ interrup*/
#define CFGRNR 				0x2004	/*QP WQE, NIC WORK, RNR,CREDIT TIMER*/
#define CFGSIZEOFWRENTRY 		0x2008	/*size of workrequest, 16byte times*/
#define UPLINKDOWNLINK 			0x2010	/*MTU with uplink & downlink*/
#define WQERETRYCOUNT			0x2014	/*WQE retry count*/
#define WQERETRYTIMER			0x2018	/*WQE timer*/
#define INTRMASK			0x2020	/*INTR MASK*/
#define WRONGVECTOR			0x2024	/*err vector*/
#define WRONGFIELD			0x2028	/*err field*/
#define TLBINIT				0x202c  /*TLB INIT*/
#define SOCKETID			0x2030  /*SOCKET ID TO IP*/
#define SRCQP				0x2034	/*QP TO QP*/
#define DESTQP				0x2038  /*DEST QP*/
#define RC_QPMAPPING		0x203c	/*RC MAPPING*/

#define CQQUEUEUP			0x0000	/*the upper border of cq*/
#define CQQUEUEDOWN			0x0008	/*the lower border of cq*/
#define CQREADPTR			0x0010	/*cq read ptr*/
#define CQESIZE				0x0018	/*cqe size*/
#define CQWRITEPTR			0x001c 	/*cq write ptr*/

#define RxUpAddrCQE			0x0028
#define RxBaseAddrCQE		0x0030
#define RxCQEWP				0x0038
#define RxCQEOp             0x0040
#define RxCQWPT				0x0044 // WPT

#define XmitUpAddrCQE		0x0050
#define XmitBaseAddrCQE		0x0058
#define XmitCQEWP			0x0060
#define XmitCQEOp			0x0068
#define XmitCQWPT			0x006c//WPT

#define RCVQ_INF			0x2040  /*RECVQ_INF REGISTER*/
#define RCVQ_DI				0x2044  /*REVQ_DI REGISTER*/
#define RCVQ_WRRD			0x2050  /*REVQ_WRRD*/ 
#define QPLISTREADQPN			0x4000  /*READ QPLIST FOR QPN*/
#define WRITEORREADQPLIST   		0x4004  /*READ OR WIRTE QPLIST*/
#define WPFORQPLIST			0x4008  /*WRITE QPLIST DATA: WP*/
#define WPFORQPLIST2		0x400c
#define RPFORQPLIST 			0x4010	/*WRITE QPLIST DATA: RP*/
#define RPFORQPLIST2			0x4014  /*WRITE QPLIST DATA*/
#define QPNANDVALID			0x4018  /*WRITE QPLIST DATA: QPN AND QPVALID*/
#define QPNANDVALID2		0x401c	/*WRITE QPLIST DATA*/
#define QPLISTWRITEQPN			0x4020  /*WRITE QPLIST DATA: WRITE QPN*/
#define READQPLISTDATA			0x4024	/*READ QPLIST DATA*/
#define READQPLISTDATA2			0x4028
#define	READQPLISTDATA3			0x402c
#define READQPLISTDATA4			0x4030
#define WRITEQPLISTMASK			0x403c  /*MASK FOR PAGE,RP,WP*/
#define STARTINITPSN			0x8000  /*START INIT PSN*/
#define STARTINITQP			0x8004  /*START INIT QP*/
#define INITQPTABLE			0x8014  /*INIT QP TABLE,EEC TABLE,PSN TABLE*/
#define INITQP				0x8010
#define INITEECTABLE		0x8018
#define REGISTERQP			0xc000	/*register qp and qp data*/


/*GENRSP START*/
#define GENRSPCQCONFIG_POS 			0
#define GENRSPCQCONFIG_LEN 			12
#define GENCQRQTCONFIG_POS 			12
#define GENCQRQTCONFIG_LEN 			12
#define PAGESIZE_POS 				24
#define PAGESIZE_LEN 				5
/*GENRSP END*/
/*CFGRNR START*/
#define CFGENABLE_POS				0
#define CFGENABLE_LEN				2
#define RNRTIMER_POS				2
#define RNRTIMER_LEN				5
#define LOWTHRESHOLD_POS			7
#define LOWTHRESHOLD_LEN			6
#define CREDITTIMER_POS				13
#define CREDITTIMER_LEN				5
#define RESERV_POS				18
#define RESERV_LEN				14
/*CFGRNR END*/
/*CFGSIZEOFWRENTRY START*/
#define WORKREQUEST_POS 			0
#define WORKREQUEST_LEN				40
/*CFGSIZEOFWRENTRY END*/
/*UPLINKDOWNLINK START*/
#define DOWNLINKMTU_POS				0
#define DOWNLINKMTU_LEN				14
#define UPLINKMTU_POS				14
#define UPLINKMTU_LEN				14
#define RESERVE_POS				28
#define RESERVE_LEN				4
/*UPLINKDOWNLINK END*/
/*WQERETRYCOUNT START*/
#define RETRYCOUNT_POS				0
#define RETRYCOUNT_LEN				32
/*WQERETRYCOUNT END*/
/*WQERETRYTIMER START*/
#define RETRYTIMER_POS				0
#define RETRYTIMER_LEN				40
/*WQERETRYTIMER END*/
/*INTRMASK START*/
#define MASK_POS				0
#define MASK_LEN				32
/*INTRMASK END*/
/*WRONGVECTOR START*/
#define ERRVECTOR_POS				0
#define ERRVECTOR_LEN				32
/*WRONGVECTOR END*/
/*WRONGFIELD START*/
#define ERRFIELD_POS				0
#define ERRFIELD_LEN				32
/*WRONGFIELD END*/
/*TLBINIT START*/
#define TLBINIT_POS				0
#define TLBINIT_LEN				32
/*TLBINIT END*/
/*SOCKETIP START*/
#define SOCKETIP_POS				0
#define SOCKETIP_LEN				32
/*SOCKETIP END*/
/*QPTOQP START*/
#define SRC_QP_POS				0
#define SRC_QP_LEN				32
#define DSC_QP_POS				32 /*h2038*/
#define DSC_QP_LEN				32
#define RC_QPMAPPING_POS			64 /*h203c*/
#define RC_QPMAPPING_LEN			8
/*QPTOQP END*/
/*CQQUEUEUP START*/
#define TXUPADDRCQE_L_POS			0
#define TXUPADDRCQE_L_LEN			32
#define TXUPADDRCQE_H_POS			32 /*h0004*/
#define TXUPADDRCQE_H_LEN			32
/*CQQUEUEUP END*/
/*CQQUEUEDOWN START*/
#define TXBASEADDRCQE_L_POS			0  /*h0008*/
#define TXBASEADDRCQE_L_LEN			32 
#define TXBASEADDRCQE_H_POS			32 /*h000c*/
#define TXBASEADDRCQE_H_LEN			32
/*CQQUEUEDOWN END*/
/*CQREADPTR START*/
#define TXCQEWP_L_POS				0  /*h0010*/
#define TXCQEWP_L_LEN				32
#define TXCQEWP_H_POS				32 /*h0014*/
#define TXCQEWP_H_LEN				32
/*CQREADPTR END*/
/*CQESIZE START*/
#define TXCQRDWRVALID_POS			0  /*h0018*/
#define TXCQRDWRVALID_LEN			1
#define TXCQRDWR_POS				1
#define TXCQRDWR_LEN				1
#define TXCQQPN_POS				2
#define TXCQQPN_LEN				8
/*CQESIZE END*/
/*CQWRITEPTR START*/
#define CQWRITEPTR_POS				0  /*h001c*/
#define CQWRITEPTR_LEN				40
/*CQWRITEPTR END*/
/*RCVQ_INF START*/
#define RCVQ_WADDR_POS				0  /*h2040*/
#define RCVQ_WADDR_LEN				10
#define RCVQ_WP_POS				10
#define RCVQ_WP_LEN				8
#define RCVQ_RP_POS				18
#define RCVQ_RP_LEN				8
/*RCVQ_INF END*/
/*RCVQ_DI START*/
#define RCVQ_DI_L_POS				0  /*h2044*/
#define RCVQ_DI_L_LEN				32
#define RCVQ_DI_H_POS				32 /*h2048*/
#define RCVQ_DI_H_LEN				32
/*RCVQ_DI END*/
/*RCVQ_WRRD START*/
#define RCVQ_WRRD_POS				0  /*h2050*/
#define RCVQ_WRRD_LEN				6
/*RCVQ_WRRD END*/
/*QPLISTREADQPN START*/
#define QPLISTREADQPN_POS 			0  /*h4000*/
#define QPLISTREADQPN_LEN			32
/*QPLISTREADQPN END*/
/*WRITEORREADQPLIST START*/
#define WRITEORREADQPLIST_POS			0  /*h4004*/
#define WRITEORREADQPLIST_LEN			32
/*WRITEORREADQPLIST END*/
/*WPFORQPLIST START*/
#define WPFORQPLIST_POS				0  /*h4008*/
#define WPFORQPLIST_LEN				40
/*WPFORQPLIST END*/
/*RPFORQPLIST START*/
#define RPFORQPLIST_POS				0  /*h4010*/
#define RPFORQPLIST_LEN				40
/*RPFORQPLIST END*/
/*QPNANDVALID START*/
#define QPNANDVALID_POS				0  /*h4018*/
#define QPNANDVALID_LEN				40
/*QPNANDVALID END*/
/*QPLISTWRITEQPN START*/
#define QPLISTWRITEQPN_POS			0  /*h4020*/
#define QPLISTWRITEQPN_LEN			32
/*QPLISTWRITEQPN END*/
/*READQPLISTDATA START*/
#define READQPLISTDATA_POS			0  /*h4024*/
#define READQPLISTDATA_LEN			168
/*READQPLISTDATA END*/
/*WRITEQPLISTMASK START*/
#define WRITEQPLISTMASK_POS			0  /*h403c*/
#define WRITEQPLISTMASK_LEN			32
/*WRITEQPLISTMASK END*/
/*STARTINITPSN START*/
#define STARTINITPSN_POS			0  /*h8000*/
#define STARTINITPSN_LEN			32
/*STARTINITPSN END*/
/*STARTINITQP START*/
#define STARTINITQP_POS				0  /*h8004*/
#define STARTINITQP_LEN				32
/*STARTINITQP END*/
/*INITQPTABLE START*/
#define INITQPTABLE_POS				0  /*h8008*/
#define INITQPTABLE_LEN				32
/*INITQPTABLE END*/
/*REGISTERQP START*/
#define REGISTERQP_POS				0  /*hc000*/
#define REGISTERQP_LEN				200
/*REGISTERQP END*/
/*PGU Registers END*/


/*PHD Registers START*/
#define PHD_BASE_0 					0x50000000  /*IN MPB PHD IS 0x50000000 */
#define PHD_BASE_1					0x60000000  /*IN MPB PHD IS 0x60000000*/

#define PHDSTART					0x0000
#define PHDBYTEORDER				0x0001
#define PHDTXDESCTAILPTR_L			0x0002
#define PHDTXDESCTAILPTR_H			0x0003
#define PHDTXDESCTAILPTR_THRESDHOLD 0x0004
#define PHDRXDESCTAILPTR_THRESDHOLD 0x0005
#define PHDRXDESCTAILPTR_INCR_STEP  0x0006
#define PHDRXDESCTAILPTR_L			0x0007
#define PHDRXDESCTAILPTR_H			0x0008
#define PHDMACSOURCEADDR_L			0x0009
#define PHDMACSOURCEADDR_H			0x000a
#define PHDMACTYPEIPV4				0x000b
#define PHDMACTYPEIPV6				0x000c
#define PHDIPV4VERSION				0x000d
#define PHDIPV4HEADER_LEN			0x000e
#define PHDIPV4TOS					0x000f
#define PHDIPV4ID					0x0010
#define PHDIPV4FLAG					0x0011
#define PHDIPV4OFFSET				0x0012
#define PHDIPV4TTL					0x0013
#define PHDIPV4PROTOCOL				0x0014
#define PHDIPV4HEADER_CHECKSUM		0x0015
#define PHDIPV4SOURCEADDR			0x0016
#define PHDIPV6VERSION				0x0017
#define PHDIPV6CLASS				0x0018
#define PHDIPV6FLOWLABLE			0x0019
#define PHDIPV6NEXTHEADER			0x001a
#define PHDIPV6HOPLIMIT				0x001b
#define PHDIPV6SOURCEADDR_0			0x001c
#define PHDIPV6SOURCEADDR_1			0x001d
#define PHDIPV6SOURCEADDR_2			0x001e
#define PHDIPV6SOURCEADDR_3			0x001f
#define PHDUDPSOURCEPORT			0x0020
#define PHDUDPDESTPORT				0x0021
#define PHDUDPCHECKSUM				0x0022
#define PHDCONTEXT_TDES0			0x0023
#define PHDCONTEXT_TDES1			0x0024
#define PHDCONTEXT_TDES2			0x0025
#define PHDCONTEXT_TDES3			0x0026
#define PHDNORMAL_TDES1				0x0027
#define PHDNORMAL_TDES2				0x0028
#define PHDNORMAL_TDES3				0x0029
#define	PHDNORMAL_RDES1				0x002a
#define PHDNORMAL_RDES2				0x002b
#define PHDNORMAL_RDES3				0x002c
#define PHDSRAM_RMC					0x002d
#define PHDMACTYPEIPV6RECV			0x002e

#define PHD_START_POS				0
#define PHD_START_LEN				1

#define PHDBYTEORDER_BITINVERT_POS		0
#define PHDBYTEORDER_BITINVERT_LEN		1
#define PHDBYTEORDER_ENDIANEXCH_POS		1
#define PHDBYTEORDER_ENDIANEXCH_LEN		1
#define PHDBYTEORDER_BITINVERTRD_POS	2
#define PHDBYTEORDER_BITINVERTRD_LEN	1
#define PHDBYTEORDER_ENDIANEXCHRD_POS	3
#define PHDBYTEORDER_ENDIANEXCHRD_LEN	1

#define PHDTXDESCTLPTR_ADDRL_POS		0
#define PHDTXDESCTLPTR_ADDRL_LEN		32

#define PHDTXDESCTLPTR_ADDRH_POS		0
#define PHDTXDESCTLPTR_ADDRH_LEN		17

#define PHDTXDESCTPTR_THRES_POS		0
#define PHDTXDESCTPTR_THRES_LEN		8

#define PHDRXDESCTPTR_THRES_POS		0
#define PHDRXDESCTPTR_THRES_LEN		8

#define PHDRXDESCTPTR_INCRST_POS	1
#define PHDRXDESCTPTR_INCRST_LEN	1

#define PHDRXDESCTLPTR_ADDRL_POS	0
#define PHDRXDESCTLPTR_ADDRL_LEN	32

#define PHDRXDESCTLPTR_ADDRH_POS	0
#define PHDRXDESCTLPTR_ADDRH_LEN	17

#define PHDMACSOURCE_ADDRL_POS		0
#define PHDMACSOURCE_ADDRL_LEN		32

#define PHDMACSOURCE_ADDRH_POS		0
#define PHDMACSOURCE_ADDRH_LEN		16

#define PHDMACTYPEIPV4_POS			0
#define PHDMACTYPEIPV4_LEN			16

#define PHDMACTYPEIPV6_POS			0
#define PHDMACTYPEIPV6_LEN			16

#define PHDIPV4VERSION_POS			0
#define PHDIPV4VERSION_LEN			4

#define PHDIPV4HEADER_LEN_POS		0
#define PHDIPV4HEADER_LEN_LEN		4

#define PHDIPV4TOS_POS				0
#define PHDIPV4TOS_LEN				8

#define PHDIPV4ID_POS				0
#define PHDIPV4ID_LEN				16

#define PHDIPV4FLAG_POS				0
#define PHDIPV4FLAG_LEN				3

#define PHDIPV4OFFSET_POS			0
#define PHDIPV4OFFSET_LEN			13

#define PHDIPV4TTL_POS				0
#define PHDIPV4TTL_LEN				8

#define PHDIPV4PROTOCOL_POS			0
#define PHDIPV4PROTOCOL_LEN			8

#define PHDIPV4HEADER_CHECKSUM_POS	0
#define PHDIPV4HEADER_CHECKSUM_LEN	16

#define PHDIPV4SOURCE_ADDR_POS		0
#define PHDIPV4SOURCE_ADDR_LEN		32

#define PHDIPV6VERSION_POS			0
#define PHDIPV6VERSION_LEN			4

#define PHDIPV6CLASS_POS			0
#define PHDIPV6CLASS_LEN			8

#define PHDIPV6FLOWLABEL_POS		0
#define PHDIPV6FLOWLABEL_LEN		20

#define PHDIPV6NEXTHEADER_POS		0
#define PHDIPV6NEXTHEADER_LEN		8

#define PHDIPV6HOPLIMIT_POS			0
#define PHDIPV6HOPLIMIT_LEN			32

#define PHDIPV6SOURCE_ADDR0_POS		0
#define PHDIPV6SOURCE_ADDR0_LEN		32

#define PHDIPV6SOURCE_ADDR1_POS		0
#define PHDIPV6SOURCE_ADDR1_LEN		32

#define PHDIPV6SOURCE_ADDR2_POS		0
#define PHDIPV6SOURCE_ADDR2_LEN		32

#define PHDIPV6SOURCE_ADDR3_POS		0
#define PHDIPV6SOURCE_ADDR3_LEN		32

#define PHDUDPSOURCE_PORT_POS		0
#define PHDUDPSOURCE_PORT_LEN		32

#define PHDUDPDEST_PORT_POS			0
#define PHDUDPDEST_PORT_LEN			32

#define PHDUDP_CHECKSUM_POS			0
#define PHDUDP_CHECKSUM_LEN			16

#define PHDCONTEXT_TDES0_POS		0
#define PHDCONTEXT_TDES0_LEN		32

#define PHDCONTEXT_TDES1_POS		0
#define PHDCONTEXT_TDES1_LEN		32

#define PHDCONTEXT_TDES2_POS		0
#define PHDCONTEXT_TDES2_LEN		32

#define PHDCONTEXT_TDES3_POS		0
#define PHDCONTEXT_TDES3_LEN		32

#define PHDNORMAL_TDES1_POS			0
#define PHDNORMAL_TDES1_LEN			32

#define PHDNORMAL_TDES2_POS			0
#define PHDNORMAL_TDES2_LEN			32

#define PHDNORMAL_TDES3_POS			0
#define PHDNORMAL_TDES3_LEN			32

#define PHDNORMAL_RDES1_POS			0
#define PHDNORMAL_RDES1_LEN			32

#define PHDNORMAL_RDES2_POS			0
#define PHDNORMAL_RDES2_LEN			32

#define PHDNORMAL_RDES3_POS			0
#define PHDNORMAL_RDES3_LEN			32

#define PHDSRAMRM_POS				0
#define PHDSRAMRM_LEN				4
#define PHDSRAMRME_POS				4
#define PHDSRAMRME_LEN				1
#define PHDSRAMRES_POS				5
#define PHDSRAMRES_LEN				27

#define PHDMACTYPEIPV6RECV_POS		0
#define PHDMACTYPEIPV6RECV_LEN		32	
/*PHD REGISTER END*/

/*CM REGISTER START*/
#define CM_BASE						0x30000000 /*IN MPB CM_MSG IS 0X30000000*/
#define CM_CFG						0x20000000 /*IN MPB CM_CFG IS 0x20000000*/

#define CMLOGEN						0x0000
#define CMERREN						0x0001
#define CMINTEN						0x0002
#define CMERRINTSTA					0x0003
#define CMERRINTSTA_SET				0x0004
#define CMERRINTSTA_CLR				0x0005
#define CMERRINTCNT_CLR				0x0006
#define CMMSGADDRERRINFO_FIRST		0x0007
#define CMMSGADDRERRCNT				0x0008
#define CMMSGSENDSRAMSTATE			0x0010
#define CMMSGSENDMSG_4BYTELEN		0x0011
#define CMMSGSENDMSG_LLPINFO_0		0x0012
#define CMMSGSENDMSG_LLPINFO_1		0x0013
#define CMMSGSENDMSG_LLPINFO_2		0x0014
#define CMMSGSENDMSG_LLPINFO_3		0x0015
#define CMMSGSENDMSG_LLPINFO_4		0x0016
#define CMMSGSENDMSG_LLPINFO_5		0x0017
#define CMMSGRECEIVESRAMSTATE		0x0020
#define CMMSGRECEIVEMSG_4BYTELEN	0x0021
#define CMMSGRECEIVEMSG_LLPINFO_0	0x0022
#define CMMSGRECEIVEMSG_LLPINFO_1	0x0023
#define CMMSGRECEIVEMSG_LLPINFO_2	0x0024
#define CMMSGRECEIVEMSG_LLPINFO_3	0x0025
#define CMMSGRECEIVEMSG_LLPINFO_4	0x0026
#define CMMSGRECEIVEMSG_LLPINFO_5	0x0027
#define CMMSGSRAMOPERATE_FINISH		0x0030

#define CMLOGEN_RECSRAM_NEMP_POS	0
#define CMLOGEN_RECSRAM_NEMP_LEN	1
#define CMLOGEN_CMMSGERR_POS		1
#define CMLOGEN_CMMSGERR_LEN		1

#define CMERREN_RECSRAM_NEMP_POS	0
#define CMERREN_RECSRAM_NEMP_LEN	1
#define CMERREN_CMMSGERR_POS		1
#define CMERREN_CMMSGERR_LEN		1

#define CMINTEN_RECSRAM_NEMP_POS	0
#define CMINTEN_RECSRAM_NEMP_LEN	1
#define CMINTEN_CMMSGERR_POS		1
#define CMINTEN_CMMSGERR_LEN		1

#define CMERRINTSTA_RECSRAM_NEMP_POS	0
#define CMERRINTSTA_RECSRAM_NEMP_LEN	1
#define CMERRINTSTA_CMMSGERR_POS		1
#define CMERRINTSTA_CMMSGERR_LEN		1

#define CMERRINTSTASET_RECSRAM_NEMP_POS	0
#define CMERRINTSTASET_RECSRAM_NEMP_LEN	1
#define CMERRINTSTASET_CMMSGERR_POS		1
#define CMERRINTSTASET_CMMSGERR_LEN		1

#define CMERRINTSTACLR_RECSRAM_NEMP_POS	0
#define CMERRINTSTACLR_RECSRAM_NEMP_LEN	1
#define CMERRINTSTACLR_CMMSGERR_POS		1
#define CMERRINTSTACLR_CMMSGERR_LEN		1

#define CMERRINTCNTCLR_CMMSGERR_POS		1
#define CMERRINTCNTCLR_CMMSGERR_LEN		1

#define CMMSG_ERRINFOFIRST_POS			0
#define CMMSG_ERRINFOFIRST_LEN			32

#define CMMSG_ERRCNT_POS				0
#define CMMSG_ERRCNT_LEN				32

#define CMMSGSSRAMSTA_REMAINFLITNUM_POS			0
#define CMMSGSSRAMSTA_REMAINFLITNUM_LEN			9
#define CMMSGSSRAMSTA_MSG_CNT_POS				9
#define CMMSGSSRAMSTA_MSG_CNT_LEN				9

#define CMMSGSMSG4BYTELEN_POS					0
#define CMMSGSMSG4BYTELEN_LEN					13

#define CMMSGSMSGLLPINFO0_POS					0
#define CMMSGSMSGLLPINFO0_LEN					32

#define CMMSGSMSGLLPINFO1_POS					0
#define CMMSGSMSGLLPINFO1_LEN					32

#define CMMSGSMSGLLPINFO2_POS					0
#define CMMSGSMSGLLPINFO2_LEN					32

#define CMMSGSMSGLLPINFO3_POS					0
#define CMMSGSMSGLLPINFO3_LEN					32

#define CMMSGSMSGLLPINFO4_POS					0
#define CMMSGSMSGLLPINFO4_LEN					32

#define CMMSGSMSGLLPINFO5_DMAC_H_POS			0
#define CMMSGSMSGLLPINFO5_DMAC_H_LEN			16
#define CMMSGSMSGLLPINFO5_ISIPV6_POS			16
#define CMMSGSMSGLLPINFO5_ISIPV6_LEN			1
#define CMMSGSMSGLLPINFO5_PORTID_POS			17
#define CMMSGSMSGLLPINFO5_PORTID_LEN			1

#define CMMSGRECVSRAM_EMPTY_POS					0
#define CMMSGRECVSRAM_EMPTY_LEN					1

#define CMMSGRECVMSG4BYTELEN_POS				0
#define CMMSGRECVMSG4BYTELEN_LEN				13

#define CMMSGRECVMSGLLPINFO0_IPADDR0_POS		0
#define CMMSGRECVMSGLLPINFO0_IPADDR0_LEN		32

#define CMMSGRECVMSGLLPINFO1_IPADDR1_POS		0
#define CMMSGRECVMSGLLPINFO1_IPADDR1_LEN		32

#define CMMSGRECVMSGLLPINFO2_IPADDR2_POS		0
#define CMMSGRECVMSGLLPINFO2_IPADDR2_LEN		32

#define CMMSGRECVMSGLLPINFO3_IPADDR3_POS		0
#define CMMSGRECVMSGLLPINFO3_IPADDR3_LEN		32

#define CMMSGRECVMSGLLPINFO4_IPADDR4_POS		0
#define CMMSGRECVMSGLLPINFO4_IPADDR4_LEN		32

#define CMMSGRECVMSGLLPINFO5_SMAC_H_POS			0
#define CMMSGRECVMSGLLPINFO5_SMAC_H_LEN			16
#define CMMSGRECVMSGLLPINFO5_ISIPV6_POS			16
#define CMMSGRECVMSGLLPINFO5_ISIPV6_LEN			1
#define CMMSGRECVMSGLLPINFO5_PORTID_POS			17
#define CMMSGRECVMSGLLPINFO5_PORTID_LEN			1

#define CMMSGSRAMOPERATEFIN_SMSG_WR_FIN_POS		0
#define CMMSGSRAMOPERATEFIN_SMSG_WR_FIN_LEN		1
#define CMMSGSRAMOPERATEFIN_RMSG_RD_FIN_POS		1
#define CMMSGSRAMOPERATEFIN_RMSG_RD_FIN_LEN		1

/*CM REGISTER END*/

/*MPP REGISTER START*/
#define MPP_BASE					0x6000000  /*MPP'S OFFSET FORM BASE ADDR*/

#define MPPMAC0_STATE0				0x0000
#define MPPMAC0_STATE1				0x0004
#define MPPMAC0_STATE2				0x0008
#define MPPMAC1_STATE0				0x000c
#define MPPMAC1_STATE1				0x0010
#define MPPMAC1_STATE2				0x0014
#define MPPPCS0_STATE				0x0018
#define MPPPCS1_STATE				0x001c
#define MPPPHY_STATE				0x0020
#define MPPMPP_PARAM				0x0024
#define MPPMAC0_PARAM				0x0028
#define MPPMAC1_PARAM				0x002c
#define MPPPCS0_PARAM				0x0030
#define MPPPCS1_PARAM				0x0034
#define MPPPHY_PARAM				0x0038


#define MAC0LOOPBACK_MODE_POS			0
#define MAC0LOOPBACK_MODE_LEN			1
#define MAC0_SPEED_POS					1
#define MAC0_SPEED_LEN					3

#define MAC0PTP_TIMESTAMP_L_POS			0
#define MAC0PTP_TIMESTAMP_L_LEN			32

#define MAC0PTP_TIMESTAMP_H_POS			0
#define MAC0PTP_TIMESTAMP_H_LEN			32

#define MAC1LOOPBACK_MODE_POS			0
#define MAC1LOOPBACK_MODE_LEN			1
#define MAC1_SPEED_POS					1
#define MAC1_SPEED_LEN					3

#define MAC1PTP_TIMESTAMP_L_POS			0
#define MAC1PTP_TIMESTAMP_L_LEN			32

#define MAC1PTP_TIMESTAMP_H_POS			0
#define MAC1PTP_TIMESTAMP_H_LEN			32

#define PCS0RX_LANE_STATUS_POS			0
#define PCS0RX_LANE_STATUS_LEN			1
#define PCS0TX_LANE_STATUS_POS			1
#define PCS0TX_LANE_STATUS_LEN			1

#define PCS1RX_LANE_STATUS_POS			0
#define PCS1RX_LANE_STATUS_LEN			1
#define PCS1TX_LANE_STATUS_POS			1
#define PCS1TX_LANE_STATUS_LEN			1

#define PHYREF_CLK_REQ_POS				0
#define PHYREF_CLK_REQ_LEN				1
#define PHYMPLLB_FORCE_ACK_POS			1
#define PHYMPLLB_FORCE_ACK_LEN			1
#define PHYMPLLA_FORCE_ACK_POS			2
#define PHYMPLLA_FORCE_ACK_LEN			1
#define PHYPCS_PWR_EN_POS				3
#define PHYPCS_PWR_EN_LEN				1
#define PHYPMA_PWR_EN_POS				4
#define PHYPMA_PWR_EN_LEN				1

#define MPPPORT_0_EN_POS				0
#define MPPPORT_0_EN_LEN				1
#define MPPPORT_0_RST_N_POS				1
#define MPPPORT_0_RST_N_LEN				1
#define MPPPORT_1_EN_POS				2
#define MPPPORT_1_EN_LEN				1
#define MPPPORT_1_RST_N_POS				3
#define MPPPORT_1_RST_N_LEN				1
#define MPPPHY_CTL_SEL_POS				4
#define MPPPHY_CTL_SEL_LEN				1
#define MPPPHY_RST_MSK_POS				5
#define MPPPHY_RST_MSK_LEN				1	

#define MAC0SBD_FLOWCTRL_POS			0
#define MAC0SBD_FLOWCTRL_LEN			8
#define MAC0PTP_AUX_TS_TRIG_POS			8
#define MAC0PTP_AUX_TS_TRIG_LEN			1
#define MAC0PWR_CLAMP_CTRL_POS			9
#define MAC0PWR_CLAMP_CTRL_LEN			1
#define MAC0PWR_ISOLATE_POS				10
#define MAC0PWR_ISOLATE_LEN				1
#define MAC0PWR_DOWN_CTRL_POS			11
#define MAC0PWR_DOWN_CTRL_LEN			1

#define MAC1SBD_FLOWCTRL_POS			0
#define MAC1SBD_FLOWCTRL_LEN			8
#define MAC1PTP_AUX_TS_TRIG_POS			8
#define MAC1PTP_AUX_TS_TRIG_LEN			1
#define MAC1PWR_CLAMP_CTRL_POS			9
#define MAC1PWR_CLAMP_CTRL_LEN			1
#define MAC1PWR_ISOLATE_POS				10
#define MAC1PWR_ISOLATE_LEN				1
#define MAC1PWR_DOWN_CTRL_POS			11
#define MAC1PWR_DOWN_CTRL_LEN			1

#define PCS0XLGPCS_RX_LANE_EN_POS		0
#define PCS0XLGPCS_RX_LANE_EN_LEN		4
#define PCS0XLGPCS_TX_LANE_EN_POS		4
#define PCS0XLGPCS_TX_LANE_EN_LEN		4

#define PCS1XLGPCS_RX_LANE_EN_POS		0
#define PCS1XLGPCS_RX_LANE_EN_LEN		4
#define PCS1XLGPCS_TX_LANE_EN_POS		4
#define PCS1XLGPCS_TX_LANE_EN_LEN		4

#define PHYNOMINAL_VP_SEL_POS			0
#define PHYNOMINAL_VP_SEL_LEN			2
#define PHYNOMINAL_VPH_SEL_POS			2
#define PHYNOMINAL_VPH_SEL_LEN			2
#define PHYPCS_PWR_STABLE_POS			4
#define PHYPCS_PWR_STABLE_LEN			1
#define PHYPMA_PWR_STABLE_POS			5
#define PHYPMA_PWR_STABLE_LEN			1
#define PHYPG_MODE_EN_POS				6
#define PHYPG_MODE_EN_LEN				1
#define PHYPG_RESET_POS					7
#define PHYPG_RESET_LEN					1
#define PHYMPLLA_FORCE_EN_POS			8
#define PHYMPLLA_FORCE_EN_LEN			1
#define PHYMPLLB_FORCE_EN_POS			9
#define PHYMPLLB_FORCE_EN_LEN			1


/*MPP REGISTER END*/

/*SOME IMPORTANT DEFINITION*/
#define QPNUM		10
#define DESTIQP      QPNUM
#define EECNXT		QPNUM
#define OPCODE		8
#define SOCKET		48
#define ADDRLEN		64
#define DMALEN		32
#define QKEY		32
#define L_KEY		32
#define RKEY		32
#define PKEY		16
#define IMMDT		32


/*Transport mode definition*/
#define UD				0x6
#define UC				0x2
#define RC				0x0
#define RD				0x4
#define SEND			0x4
#define SEND_WITH_IMM	0x5
#define SEND_WITH_INV   0x6
#define RDMA_WRITE			0x8
#define WRITE_WITH_IMM	0x9
#define RDMA_READ			0x0

#endif