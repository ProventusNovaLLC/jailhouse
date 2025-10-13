/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) 2025 MediaTek
 *
 * Sharing of GPIO between root and other inmate cells and control
 * concurrent access to shared registers.
 *
 * Authors:
 *   Felix Freimann <felix.freimann@mediatek.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <asm/mtk-common.h>
#include <jailhouse/cell.h>
#include <jailhouse/paging.h>
#include <jailhouse/percpu.h>
#include <jailhouse/printk.h>
#include <jailhouse/unit.h>


#define GPIO_OFFSET_DIN_0				0x0000
#define GPIO_OFFSET_DIN_1				0x0010
#define GPIO_OFFSET_DIN_2				0x0020
#define GPIO_OFFSET_DIN_3				0x0030
#define GPIO_OFFSET_DIN_4				0x0040
#define GPIO_OFFSET_EH_CF				0x0050
#define GPIO_OFFSET_RESL_CFG			0x0060
#define GPIO_OFFSET_PUPU_CFG_0			0x0070
#define GPIO_OFFSET_PUPU_CFG_0_SET		0x0074
#define GPIO_OFFSET_PUPU_CFG_0_CLR		0x0078
#define GPIO_OFFSET_PUPU_CFG_1			0x0080
#define GPIO_OFFSET_PUPU_CFG_1_SET		0x0084
#define GPIO_OFFSET_PUPU_CFG_1_CLR		0x0088
#define GPIO_OFFSET_PUPU_CFG_2			0x0090
#define GPIO_OFFSET_PUPU_CFG_2_SET		0x0094
#define GPIO_OFFSET_PUPU_CFG_2_CLR		0x0098
#define GPIO_OFFSET_DOUT_0				0x00a0
#define GPIO_OFFSET_DOUT_0_SET			0x00a4
#define GPIO_OFFSET_DOUT_0_CLR			0x00a8
#define GPIO_OFFSET_DOUT_1				0x00b0
#define GPIO_OFFSET_DOUT_1_SET			0x00b4
#define GPIO_OFFSET_DOUT_1_CLR			0x00b8
#define GPIO_OFFSET_DOUT_2				0x00c0
#define GPIO_OFFSET_DOUT_2_SET			0x00c4
#define GPIO_OFFSET_DOUT_2_CLR			0x00c8
#define GPIO_OFFSET_DOUT_3				0x00d0
#define GPIO_OFFSET_DOUT_3_SET			0x00d4
#define GPIO_OFFSET_DOUT_3_CLR			0x00d8
#define GPIO_OFFSET_DOUT_4				0x00e0
#define GPIO_OFFSET_DOUT_4_SET			0x00e4
#define GPIO_OFFSET_DOUT_4_CLR			0x00e8
#define GPIO_OFFSET_PUPU_CFG_3			0x00f0
#define GPIO_OFFSET_PUPU_CFG_3_SET		0x00f4
#define GPIO_OFFSET_PUPU_CFG_3_CLR		0x00f8
#define GPIO_OFFSET_DEBUG_MON_SEL		0x0100
#define GPIO_OFFSET_DIR_0				0x0140
#define GPIO_OFFSET_DIR_0_SET			0x0144
#define GPIO_OFFSET_DIR_0_CLR			0x0148
#define GPIO_OFFSET_DIR_1				0x0150
#define GPIO_OFFSET_DIR_1_SET			0x0154
#define GPIO_OFFSET_DIR_1_CLR			0x0158
#define GPIO_OFFSET_DIR_2				0x0160
#define GPIO_OFFSET_DIR_2_SET			0x0164
#define GPIO_OFFSET_DIR_2_CLR			0x0168
#define GPIO_OFFSET_DIR_3				0x0170
#define GPIO_OFFSET_DIR_3_SET			0x0174
#define GPIO_OFFSET_DIR_3_CLR			0x0178
#define GPIO_OFFSET_DIR_4				0x0180
#define GPIO_OFFSET_DIR_4_SET			0x0184
#define GPIO_OFFSET_DIR_4_CLR			0x0188
#define GPIO_OFFSET_MODE_0				0x01e0
#define GPIO_OFFSET_MODE_1				0x01f0
#define GPIO_OFFSET_MODE_2				0x0200
#define GPIO_OFFSET_MODE_3				0x0210
#define GPIO_OFFSET_MODE_4				0x0220
#define GPIO_OFFSET_MODE_5				0x0230
#define GPIO_OFFSET_MODE_6				0x0240
#define GPIO_OFFSET_MODE_7				0x0250
#define GPIO_OFFSET_MODE_8				0x0260
#define GPIO_OFFSET_MODE_9				0x0270
#define GPIO_OFFSET_MODE_A				0x0280
#define GPIO_OFFSET_MODE_B				0x0290
#define GPIO_OFFSET_MODE_C				0x02a0
#define GPIO_OFFSET_MODE_D				0x02b0
#define GPIO_OFFSET_MODE_E				0x02c0
#define GPIO_OFFSET_IES_CFG_0			0x0410
#define GPIO_OFFSET_IES_CFG_1			0x0420
#define GPIO_OFFSET_GPIO_BANK			0x0430
#define GPIO_OFFSET_GPIO_TM				0x0440
#define GPIO_OFFSET_MISC_CFG			0x0450
#define GPIO_OFFSET_MODE_CFG			0x0460
#define GPIO_OFFSET_SMT_CFG_0			0x0470
#define GPIO_OFFSET_SMT_CFG_1			0x0480
#define GPIO_OFFSET_TDSEL_CFG_0			0x0510
#define GPIO_OFFSET_TDSEL_CFG_2			0x0520
#define GPIO_OFFSET_TDSEL_CFG_3			0x0530
#define GPIO_OFFSET_TDSEL_CFG_4			0x0540
#define GPIO_OFFSET_TDSEL_CFG_5			0x0550
#define GPIO_OFFSET_TDSEL_CFG_6			0x0560
#define GPIO_OFFSET_TDSEL_CFG_1			0x05a0
#define GPIO_OFFSET_RDSEL_CFG_0			0x0610
#define GPIO_OFFSET_RDSEL_CFG_1			0x0620
#define GPIO_OFFSET_RDSEL_CFG_2			0x0630
#define GPIO_OFFSET_RDSEL_CFG_3			0x0640
#define GPIO_OFFSET_RDSEL_CFG_4			0x0650
#define GPIO_OFFSET_RDSEL_CFG_5			0x0660
#define GPIO_OFFSET_RDSEL_CFG_6			0x0670
#define GPIO_OFFSET_RDSEL_CFG_7			0x0680
#define GPIO_OFFSET_RDSEL_CFG_8			0x0690
#define GPIO_OFFSET_RDSEL_CFG_9			0x06a0
#define GPIO_OFFSET_DRV_CFG_0			0x0710
#define GPIO_OFFSET_DRV_CFG_0_SET		0x0714
#define GPIO_OFFSET_DRV_CFG_0_CLR		0x0718
#define GPIO_OFFSET_DRV_CFG_1			0x0720
#define GPIO_OFFSET_DRV_CFG_1_SET		0x0724
#define GPIO_OFFSET_DRV_CFG_1_CLR		0x0728
#define GPIO_OFFSET_DRV_CFG_2			0x0730
#define GPIO_OFFSET_DRV_CFG_2_SET		0x0734
#define GPIO_OFFSET_DRV_CFG_2_CLR		0x0738
#define GPIO_OFFSET_DRV_CFG_3			0x0740
#define GPIO_OFFSET_DRV_CFG_3_SET		0x0744
#define GPIO_OFFSET_DRV_CFG_3_CLR		0x0748
#define GPIO_OFFSET_DRV_CFG_4			0x0750
#define GPIO_OFFSET_DRV_CFG_4_SET		0x0754
#define GPIO_OFFSET_DRV_CFG_4_CLR		0x0758
#define GPIO_OFFSET_DRV_CFG_5			0x0760
#define GPIO_OFFSET_DRV_CFG_5_SET		0x0764
#define GPIO_OFFSET_DRV_CFG_5_CLR		0x0768
#define GPIO_OFFSET_DRV_CFG_6			0x0770
#define GPIO_OFFSET_DRV_CFG_6_SET		0x0774
#define GPIO_OFFSET_DRV_CFG_6_CLR		0x0778
#define GPIO_OFFSET_PULL_EN_0			0x0860
#define GPIO_OFFSET_PULL_EN_0_SET		0x0864
#define GPIO_OFFSET_PULL_EN_0_CLR		0x0868
#define GPIO_OFFSET_PULL_EN_1			0x0870
#define GPIO_OFFSET_PULL_EN_1_SET		0x0874
#define GPIO_OFFSET_PULL_EN_1_CLR		0x0878
#define GPIO_OFFSET_PULL_EN_2			0x0880
#define GPIO_OFFSET_PULL_EN_2_SET		0x0884
#define GPIO_OFFSET_PULL_EN_2_CLR		0x0888
#define GPIO_OFFSET_PULL_EN_3			0x0890
#define GPIO_OFFSET_PULL_EN_3_SET		0x0894
#define GPIO_OFFSET_PULL_EN_3_CLR		0x0898
#define GPIO_OFFSET_PULL_EN_4			0x08a0
#define GPIO_OFFSET_PULL_EN_4_SET		0x08a4
#define GPIO_OFFSET_PULL_EN_4_CLR		0x08a8
#define GPIO_OFFSET_PULL_SEL_0			0x0900
#define GPIO_OFFSET_PULL_SEL_0_SET		0x0904
#define GPIO_OFFSET_PULL_SEL_0_CLR		0x0908
#define GPIO_OFFSET_PULL_SEL_1			0x0910
#define GPIO_OFFSET_PULL_SEL_1_SET		0x0914
#define GPIO_OFFSET_PULL_SEL_1_CLR		0x0918
#define GPIO_OFFSET_PULL_SEL_2			0x0920
#define GPIO_OFFSET_PULL_SEL_2_SET		0x0924
#define GPIO_OFFSET_PULL_SEL_2_CLR		0x0928
#define GPIO_OFFSET_PULL_SEL_3			0x0930
#define GPIO_OFFSET_PULL_SEL_3_SET		0x0934
#define GPIO_OFFSET_PULL_SEL_3_CLR		0x0938
#define GPIO_OFFSET_PULL_SEL_4			0x0940
#define GPIO_OFFSET_PULL_SEL_4_SET		0x0944
#define GPIO_OFFSET_PULL_SEL_4_CLR		0x0948
#define GPIO_OFFSET_SEC_CFG_1			0x0950
#define GPIO_OFFSET_SEC_CFG_0			0x0960
#define GPIO_OFFSET_MISC_DUMMY			0x09d0
#define GPIO_OFFSET_BIAS_CFG			0x09f0

#define GPIO_SIZE  (0x0001000)
#define REG_SIZE   (sizeof (u32))

/* The following two macro's are required to customize the common macro's. */
#define REG_DIST_IDX_SHIFT  (4)
#define REG_NAME(_name)     GPIO_OFFSET_ ## _name


/* Access descriptor for [GPIO_OFFSET_DIN_0 .. GPIO_OFFSET_MODE_E] */
static const access_descr_t gpio_access_descr_0 [] =
{
    /* 0x0000 */ ACCESS_DESCR (DIN_0, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),      ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0010 */ ACCESS_DESCR (DIN_1, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),      ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0020 */ ACCESS_DESCR (DIN_2, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),      ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0030 */ ACCESS_DESCR (DIN_3, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),      ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0040 */ ACCESS_DESCR (DIN_4, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),      ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0050 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0060 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0070 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0080 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0090 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00a0 */ ACCESS_DESCR (DOUT_0, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),    ACCESS_DESCR (DOUT_0_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (DOUT_0_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x00b0 */ ACCESS_DESCR (DOUT_1, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),    ACCESS_DESCR (DOUT_1_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (DOUT_1_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x00c0 */ ACCESS_DESCR (DOUT_2, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),    ACCESS_DESCR (DOUT_2_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (DOUT_2_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x00d0 */ ACCESS_DESCR (DOUT_3, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),    ACCESS_DESCR (DOUT_3_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (DOUT_3_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x00e0 */ ACCESS_DESCR (DOUT_4, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),    ACCESS_DESCR (DOUT_4_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (DOUT_4_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x00f0 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0100 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0110 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0120 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0130 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0140 */ ACCESS_DESCR (DIR_0, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),      ACCESS_DESCR (DIR_0_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (DIR_0_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0150 */ ACCESS_DESCR (DIR_1, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),      ACCESS_DESCR (DIR_1_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (DIR_1_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0160 */ ACCESS_DESCR (DIR_2, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),      ACCESS_DESCR (DIR_2_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (DIR_2_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0170 */ ACCESS_DESCR (DIR_3, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),      ACCESS_DESCR (DIR_3_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (DIR_3_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0180 */ ACCESS_DESCR (DIR_4, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),      ACCESS_DESCR (DIR_4_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (DIR_4_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0190 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01a0 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01b0 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01c0 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01d0 */ ACCESS_DESCR_EMPTY,                                          ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01e0 */ ACCESS_DESCR (MODE_0, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01f0 */ ACCESS_DESCR (MODE_1, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0200 */ ACCESS_DESCR (MODE_2, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0210 */ ACCESS_DESCR (MODE_3, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0220 */ ACCESS_DESCR (MODE_4, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0230 */ ACCESS_DESCR (MODE_5, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0240 */ ACCESS_DESCR (MODE_6, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0250 */ ACCESS_DESCR (MODE_7, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0260 */ ACCESS_DESCR (MODE_8, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0270 */ ACCESS_DESCR (MODE_9, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0280 */ ACCESS_DESCR (MODE_A, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0290 */ ACCESS_DESCR (MODE_B, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02a0 */ ACCESS_DESCR (MODE_C, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02b0 */ ACCESS_DESCR (MODE_D, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02c0 */ ACCESS_DESCR (MODE_E, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
};

/* Access descriptor for [GPIO_OFFSET_PULL_EN_0 .. GPIO_OFFSET_PULL_SEL_4_CLR] */
static const access_descr_t gpio_access_descr_1 [] =
{
    /* 0x0860 */ ACCESS_DESCR (PULL_EN_0, PULL_EN_0, ACCESS_RW, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_0_SET, PULL_EN_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_0_CLR, PULL_EN_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0870 */ ACCESS_DESCR (PULL_EN_1, PULL_EN_0, ACCESS_RW, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_1_SET, PULL_EN_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_1_CLR, PULL_EN_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0880 */ ACCESS_DESCR (PULL_EN_2, PULL_EN_0, ACCESS_RW, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_2_SET, PULL_EN_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_2_CLR, PULL_EN_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x0890 */ ACCESS_DESCR (PULL_EN_3, PULL_EN_0, ACCESS_RW, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_3_SET, PULL_EN_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_3_CLR, PULL_EN_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x08a0 */ ACCESS_DESCR (PULL_EN_4, PULL_EN_0, ACCESS_RW, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_4_SET, PULL_EN_0_SET, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (PULL_EN_4_CLR, PULL_EN_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,
    /* 0x08b0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,
    /* 0x08c0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,
    /* 0x08d0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,
    /* 0x08e0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,
    /* 0x08f0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,                                                       ACCESS_DESCR_EMPTY,
    /* 0x0900 */ ACCESS_DESCR (PULL_SEL_0, PULL_SEL_0, ACCESS_RW, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_0_SET, PULL_SEL_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_0_CLR, PULL_SEL_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x0910 */ ACCESS_DESCR (PULL_SEL_1, PULL_SEL_0, ACCESS_RW, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_1_SET, PULL_SEL_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_1_CLR, PULL_SEL_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x0920 */ ACCESS_DESCR (PULL_SEL_2, PULL_SEL_0, ACCESS_RW, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_2_SET, PULL_SEL_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_2_CLR, PULL_SEL_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x0930 */ ACCESS_DESCR (PULL_SEL_3, PULL_SEL_0, ACCESS_RW, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_3_SET, PULL_SEL_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_3_CLR, PULL_SEL_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
    /* 0x0940 */ ACCESS_DESCR (PULL_SEL_4, PULL_SEL_0, ACCESS_RW, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_4_SET, PULL_SEL_0_SET, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (PULL_SEL_4_CLR, PULL_SEL_0_CLR, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,
};

static const access_descr_map_t  gpio_access_descr_map [] =
{
    {
        .reg_start    = REG_NAME (DIN_0),
        .reg_end      = REG_NAME (MODE_E) + REG_SIZE,
        .access_descr = gpio_access_descr_0
    },
    {
        .reg_start    = REG_NAME (PULL_EN_0),
        .reg_end      = REG_NAME (PULL_SEL_4_CLR) + REG_SIZE,
        .access_descr = gpio_access_descr_1
    }
};

static const u32 gpio_access_descr_map_size = ARRAY_SIZE (gpio_access_descr_map);

static const u32 bit_mask [] =
{
    0x00000001, 0x00000003, 0x00000007, 0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
    0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
    0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
    0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

static const u32 one_to_three_bit_cnvt [] =
{
    0x00000000, 0x00000007, 0x00000038, 0x0000003f, 0x000001c0, 0x000001c7, 0x000001f8, 0x000001ff,
    0x00000e00, 0x00000e07, 0x00000e38, 0x00000e3f, 0x00000fc0, 0x00000fc7, 0x00000ff8, 0x00000fff,
    0x00007000, 0x00007007, 0x00007038, 0x0000703f, 0x000071c0, 0x000071c7, 0x000071f8, 0x000071ff,
    0x00007e00, 0x00007e07, 0x00007e38, 0x00007e3f, 0x00007fc0, 0x00007fc7, 0x00007ff8, 0x00007fff
};


static spinlock_t lock;

static void* virt_addr = NULL;


static u32 one_bit_per_pin (access_descr_t  access_descr);
static u32 three_bits_per_pin (access_descr_t  access_descr);
static u32 addr_to_bitmap (struct mmio_access*  mmio,
                           access_descr_t       access_descr);

static void handle_ro_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);
static void handle_wo_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);
static void handle_rw_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);

static access_descr_t get_access_descr (struct mmio_access*  mmio);
static enum mmio_result gpio_handle_access (void*                arg,
									        struct mmio_access*  mmio);

static int get_phys_addr (struct cell*    cell,
                          unsigned long*  phys_addr);


static u32 one_bit_per_pin (access_descr_t  access_descr)
{
    u32          idx = get_access_dist_idx (access_descr);
    struct cell* cell = this_cell ();


    if (idx >= ARRAY_SIZE (cell->arch.gpio_bitmap))
	{
        return (0);
	}
    
    return (cell->arch.gpio_bitmap [idx]);
}

/* This function assumes that each pin requires 3 consecutive bits. Hence, a single bit will */
/* be translated into the corresponding 3 bits. Since this function is executed in EL2 when  */
/* a cell accesses the GPIO it needs to be efficient. Note that 3 bits per pin only allow    */
/* 10 pins to be described in a single u32 value. The algoritm is as follows:                */
/*                                                                                           */
/*     idx:         Contains the index into 'arch.gpio_bitmap []'. For some pin ranges       */
/*                  'arch.gpio_bitmap [idx]' and 'arch.gpio_bitmap [idx+1]' are required.    */
/*     shift_right: Contains the number of bits 'arch.gpio_bitmap [idx]' must be shifted to  */
/*                  the right.                                                               */
/*                                                                                           */
/*   pins     idx   idx+1    shift right               shift left                            */
/*                           arch.gpio_bitmap [idx]    arch.gpio_bitmap [idx+1]              */
/*   -----------------------------------------------------------------------                 */
/*    0 -   9   0     -            0                         -                               */
/*   10 -  19   0     -           10                         -                               */
/*   20 -  29   0     -           20                         -                               */
/*   30 -  39   0     1           30                         2                               */
/*   40 -  49   1     -            8                         -                               */
/*   50 -  59   1     -           18                         -                               */
/*   60 -  69   1     2           28                         4                               */
/*   70 -  79   2     -            6                         -                               */
/*   80 -  89   2     -           16                         -                               */
/*   90 -  99   2     3           26                         6                               */
/*  100 - 109   3     -            4                         -                               */
/*  110 - 119   3     -           14                         -                               */
/*  120 - 129   3     4           24                         8                               */
/*  130 - 139   4     -            2                         -                               */
/*  140 - 149   4     -           12                         -                               */
/*  150 - 159   4     -           22                         -                               */
/*  160 - 169   5     -            0                         -                               */
/*  170 - 179   5     -           10                         -                               */
/*  180 - 189   5     -           20                         -                               */
/*  190 - 199   5     6           30                         2                               */
static u32 three_bits_per_pin (access_descr_t  access_descr)
{
    u32          idx = (get_access_dist_idx (access_descr) * 10) / BITS_PER_U32;
    u32          shift_right = (get_access_dist_idx (access_descr) * 10) % BITS_PER_U32;
    u32          bit_map;
    struct cell* cell = this_cell ();


    if (idx >= ARRAY_SIZE (cell->arch.gpio_bitmap))
	{
        return (0);
    }

    bit_map = (cell->arch.gpio_bitmap [idx] >> shift_right);

    if (shift_right > (BITS_PER_U32 - 10))
    {
        bit_map &= bit_mask [BITS_PER_U32 - shift_right - 1];

        if ((idx + 1) < ARRAY_SIZE (cell->arch.gpio_bitmap))
        {
            bit_map |= (cell->arch.gpio_bitmap [idx + 1] << (BITS_PER_U32 - shift_right));
	    }
    }

    /* At this point, only the 10 LSB bits are of importance and */
    /* will be converted into 3 bits per pin. For speed we use a */
    /* lookup table for every 5 bits (32 values).                */
    bit_map = one_to_three_bit_cnvt [bit_map & 0x0000001f] | ((one_to_three_bit_cnvt [(bit_map >> 5) & 0x0000001f]) << (3 * 5));

    return (bit_map);
}

static u32 addr_to_bitmap (struct mmio_access*  mmio,
                           access_descr_t       access_descr)
{
	switch (get_access_num_of_bits (access_descr))
	{
        case ACCESS_ONE_BIT:
            return (one_bit_per_pin (access_descr));
            break;

        case ACCESS_THREE_BITS:
            return (three_bits_per_pin (access_descr));
            break;
										   
		default:
			break;
	}
								   
    return (0xffffffff);
}

static void handle_ro_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr)
{
	if (! (mmio->is_write))
	{
        u32 bitmap = addr_to_bitmap (mmio, access_descr);


        if (bitmap != 0)
        {
            mmio_perform_access (virt_addr, mmio);

      		/* Only allow the bits for which access is allowed. */
	    	mmio->value &= bitmap;
        }
        else
        {
            mmio->value = 0;
        }
    }
}

static void handle_wo_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr)
{
	if (mmio->is_write)
	{
        u32 bitmap = addr_to_bitmap (mmio, access_descr);


		/* Only allow the bits for which access is allowed. */
        if ((bitmap != 0)                            &&
            ((mmio->value & bitmap) == mmio->value))
        {
            mmio_perform_access (virt_addr, mmio);
        }
    }
}

static void handle_rw_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr)
{
    if (! (mmio->is_write))
    {
        handle_ro_access (mmio, access_descr);
    }
    else
    {
    	u32 bitmap = addr_to_bitmap (mmio, access_descr);


        if ((bitmap != 0)                            &&
            ((mmio->value & bitmap) == mmio->value))
        {
            struct mmio_access curr_mmio;


            curr_mmio.address  = mmio->address;
            curr_mmio.size     = mmio->size;
            curr_mmio.is_write = false;

    	    /* Perform a read-update-write operation. This must be done inside a lock     */
            /* since the read-update-write operation must be atomic. In addition, this    */
            /* MMIO write operation could be executed from multiple cells simmultanously. */
	        spin_lock (&lock);
	        mmio_perform_access (virt_addr, &curr_mmio);

            mmio->value |= (curr_mmio.value & ~ (bitmap));

            mmio_perform_access (virt_addr, mmio);
        	spin_unlock (&lock);
        }
    }
}

static u16 get_access_descr (struct mmio_access*  mmio)
{
    u32 idx = 0;


    if ((mmio->address & 0x00000003) != 0)
    {
        return (ACCESS_DESCR_EMPTY);
    }

    while (idx < gpio_access_descr_map_size)
    {
        if ((mmio->address >= gpio_access_descr_map [idx].reg_start) &&
            (mmio->address <  gpio_access_descr_map [idx].reg_end))
        {
            return (gpio_access_descr_map [idx].access_descr [(mmio->address - gpio_access_descr_map [idx].reg_start) >> 2]);
        }

        idx++;
    }

    return (ACCESS_DESCR_EMPTY);
}

static enum mmio_result gpio_handle_access (void*                arg,
									  	    struct mmio_access*  mmio)
{
    access_descr_t access_descr = get_access_descr (mmio);


	switch (get_access_type (access_descr))
	{
        case ACCESS_RO:
            handle_ro_access (mmio, access_descr);
            break;

        case ACCESS_WO:
            handle_wo_access (mmio, access_descr);
            break;

        case ACCESS_RW:
            handle_rw_access (mmio, access_descr);
            break;

        default:
	    	/* The remaing GPIO accesses shall only be performed by the root cell. */
            if (this_cell () == &root_cell)
			{
				/* No access lock is required since only the root cell is allowed */
				/* to perform the MMIO accesses. Hence, the root cell is responsible */
				/* for controlling access when multiple root cell programs try to */
				/* perform MMIO read / write operations. */
				mmio_perform_access (virt_addr, mmio);
			}
			else
			{
				if (! (mmio->is_write))
				{
					/* Simply return '0' for any read operation. */
					mmio->value = 0;
				}
			}
			break;
	}

	return (MMIO_HANDLED);
}

static int get_phys_addr (struct cell*    cell,
                          unsigned long*  phys_addr)
{
	unsigned int                   cnt;
	const struct jailhouse_vendor* vendor;


    (*phys_addr) = 0;

    FOR_EACH_VENDOR (vendor, cell->config, cnt)
	{
		if (vendor->type != JAILHOUSE_VENDOR_MTK_GPIO)
		{
			continue;
		}

		if (((*phys_addr) != 0)                         &&
            (vendor->mtk_gpio.address != (*phys_addr)))
		{
			return (-EINVAL);
		}

        (*phys_addr) = vendor->mtk_gpio.address;
	}

    return (0);
}                          

static int mt8365_gpio_cell_init (struct cell*  cell)
{
	size_t                         pos;
	unsigned int                   cnt;
    unsigned long                  address = 0;
	const struct jailhouse_vendor* vendor;


	FOR_EACH_VENDOR (vendor, cell->config, cnt)
	{
		if (vendor->type != JAILHOUSE_VENDOR_MTK_GPIO)
		{
			continue;
		}

		/* Verify that the GPIO entries in the cel description are valid. */
		if (((vendor->mtk_gpio.pin_base % sizeof (vendor->mtk_gpio.pin_bitmap [0])) != 0)                                       ||
		    ((vendor->mtk_gpio.pin_base + (sizeof (vendor->mtk_gpio.pin_bitmap) * 8)) > (sizeof (cell->arch.gpio_bitmap) * 8)))
		{
			return (-EINVAL);
		}

		/* Also only one GPIO address is supported. */
		if ((address != 0)                         &&
		    (vendor->mtk_gpio.address != address))
		{
			return (-EINVAL);
		}
	
        address = vendor->mtk_gpio.address;

		/* Copy the GPIO entries. */
		for (pos = 0; pos < ARRAY_SIZE (vendor->mtk_gpio.pin_bitmap); pos++)
		{
			cell->arch.gpio_bitmap [(vendor->mtk_gpio.pin_base / (sizeof (vendor->mtk_gpio.pin_bitmap [0]) * 8)) + pos] |= vendor->mtk_gpio.pin_bitmap [pos];
		}
	}

    /* Register handler. */
	mmio_region_register (cell, address, GPIO_SIZE, gpio_handle_access, (void*) address);

    /* And lastly, remove the GPIO entries from the root cell. */
	if (cell != &root_cell)
	{
		for (pos = 0; pos < ARRAY_SIZE (cell->arch.gpio_bitmap); pos++)
		{
			root_cell.arch.gpio_bitmap [pos] &= ~(cell->arch.gpio_bitmap [pos]);
		}
    }

	return (0);
}

static void mt8365_gpio_cell_exit (struct cell*  cell)
{
	size_t                         pos;
	unsigned int                   cnt;
	const struct jailhouse_vendor* vendor;


	if (cell != &root_cell)
	{
		/* Return the GPIO entries to the root cell. */
		for (pos = 0; pos < ARRAY_SIZE (cell->arch.gpio_bitmap); pos++)
		{
			root_cell.arch.gpio_bitmap [pos] |= cell->arch.gpio_bitmap [pos];
		}

		/* Mask out bits which were not part of the root cell. */
		FOR_EACH_VENDOR (vendor, root_cell.config, cnt)
		{
			if (vendor->type != JAILHOUSE_VENDOR_MTK_GPIO)
			{
				continue;
			}	

			for (pos = 0; pos < ARRAY_SIZE (vendor->mtk_gpio.pin_bitmap); pos++)
			{
				root_cell.arch.gpio_bitmap [(vendor->mtk_gpio.pin_base / (sizeof (vendor->mtk_gpio.pin_bitmap [0]) * 8)) + pos] &= vendor->mtk_gpio.pin_bitmap [pos];
			}
		}
	}
}

static unsigned int mt8365_gpio_mmio_count_regions (struct cell*  cell)
{
	/* Only one MMIO GPIO region handler per cell. */
	return (1);
}

static int mt8365_gpio_init (void)
{
	int           ret = 0;
    unsigned long phys_addr = 0;


	if (virt_addr == NULL)
	{
		/* Create hypervisor paging for GPIO access. */
        /* This is only done once with the root cell. */
        ret = get_phys_addr (&root_cell, &phys_addr);
		if (ret != 0)
		{
			return (ret);
		}

        if (phys_addr != 0)
        {
            virt_addr = paging_map_device (phys_addr, GPIO_SIZE);
            if (virt_addr == NULL)
            {
                ret = -ENOMEM;
            }
        }

    	ret = mt8365_gpio_cell_init (&root_cell);

    	if (ret != 0)
	    {
		    /* Cleanup if cell initialization failed. */
    		if (virt_addr != NULL)
	    	{
                paging_unmap_device (phys_addr, virt_addr, GPIO_SIZE);

                virt_addr = NULL;
	    	}
    	}
    }

	return (ret);
}

static void mt8365_gpio_shutdown (void)
{
    unsigned long phys_addr;


	if (virt_addr != NULL)
	{
        get_phys_addr (&root_cell, &phys_addr);

        paging_unmap_device (phys_addr, virt_addr, GPIO_SIZE);

		virt_addr = NULL;
	}
}

DEFINE_UNIT (mt8365_gpio, "mt8365_gpio");
