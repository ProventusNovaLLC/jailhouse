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


#define GPIO_OFFSET_DIR_0				0x0000
#define GPIO_OFFSET_DIR_0_SET			0x0004
#define GPIO_OFFSET_DIR_0_CLR			0x0008
#define GPIO_OFFSET_DIR_1				0x0010
#define GPIO_OFFSET_DIR_1_SET			0x0014
#define GPIO_OFFSET_DIR_1_CLR			0x0018
#define GPIO_OFFSET_DIR_2				0x0020
#define GPIO_OFFSET_DIR_2_SET			0x0024
#define GPIO_OFFSET_DIR_2_CLR			0x0028
#define GPIO_OFFSET_DIR_3				0x0030
#define GPIO_OFFSET_DIR_3_SET			0x0034
#define GPIO_OFFSET_DIR_3_CLR			0x0038
#define GPIO_OFFSET_DIR_4				0x0040
#define GPIO_OFFSET_DIR_4_SET			0x0044
#define GPIO_OFFSET_DIR_4_CLR			0x0048
#define GPIO_OFFSET_DIR_5				0x0050
#define GPIO_OFFSET_DIR_5_SET			0x0054
#define GPIO_OFFSET_DIR_5_CLR			0x0058
#define GPIO_OFFSET_DOUT_0				0x0100
#define GPIO_OFFSET_DOUT_0_SET			0x0104
#define GPIO_OFFSET_DOUT_0_CLR			0x0108
#define GPIO_OFFSET_DOUT_1				0x0110
#define GPIO_OFFSET_DOUT_1_SET			0x0114
#define GPIO_OFFSET_DOUT_1_CLR			0x0118
#define GPIO_OFFSET_DOUT_2				0x0120
#define GPIO_OFFSET_DOUT_2_SET			0x0124
#define GPIO_OFFSET_DOUT_2_CLR			0x0128
#define GPIO_OFFSET_DOUT_3				0x0130
#define GPIO_OFFSET_DOUT_3_SET			0x0134
#define GPIO_OFFSET_DOUT_3_CLR			0x0138
#define GPIO_OFFSET_DOUT_4				0x0140
#define GPIO_OFFSET_DOUT_4_SET			0x0144
#define GPIO_OFFSET_DOUT_4_CLR			0x0148
#define GPIO_OFFSET_DOUT_5				0x0150
#define GPIO_OFFSET_DOUT_5_SET			0x0154
#define GPIO_OFFSET_DOUT_5_CLR			0x0158
#define GPIO_OFFSET_DIN_0				0x0200
#define GPIO_OFFSET_DIN_1				0x0210
#define GPIO_OFFSET_DIN_2				0x0220
#define GPIO_OFFSET_DIN_3				0x0230
#define GPIO_OFFSET_DIN_4				0x0240
#define GPIO_OFFSET_DIN_5				0x0250
#define GPIO_OFFSET_MODE_0				0x0300
#define GPIO_OFFSET_MODE_0_SET			0x0304
#define GPIO_OFFSET_MODE_0_CLR			0x0308
#define GPIO_OFFSET_MODE_0_MOD			0x030c
#define GPIO_OFFSET_MODE_1				0x0310
#define GPIO_OFFSET_MODE_1_SET			0x0314
#define GPIO_OFFSET_MODE_1_CLR			0x0318
#define GPIO_OFFSET_MODE_1_MOD			0x031c
#define GPIO_OFFSET_MODE_2				0x0320
#define GPIO_OFFSET_MODE_2_SET			0x0324
#define GPIO_OFFSET_MODE_2_CLR			0x0328
#define GPIO_OFFSET_MODE_2_MOD			0x032c
#define GPIO_OFFSET_MODE_3				0x0330
#define GPIO_OFFSET_MODE_3_SET			0x0334
#define GPIO_OFFSET_MODE_3_CLR			0x0338
#define GPIO_OFFSET_MODE_3_MOD			0x033c
#define GPIO_OFFSET_MODE_4				0x0340
#define GPIO_OFFSET_MODE_4_SET			0x0344
#define GPIO_OFFSET_MODE_4_CLR			0x0348
#define GPIO_OFFSET_MODE_4_MOD			0x034c
#define GPIO_OFFSET_MODE_5				0x0350
#define GPIO_OFFSET_MODE_5_SET			0x0354
#define GPIO_OFFSET_MODE_5_CLR			0x0358
#define GPIO_OFFSET_MODE_5_MOD			0x035c
#define GPIO_OFFSET_MODE_6				0x0360
#define GPIO_OFFSET_MODE_6_SET			0x0364
#define GPIO_OFFSET_MODE_6_CLR			0x0368
#define GPIO_OFFSET_MODE_6_MOD			0x036c
#define GPIO_OFFSET_MODE_7				0x0370
#define GPIO_OFFSET_MODE_7_SET			0x0374
#define GPIO_OFFSET_MODE_7_CLR			0x0378
#define GPIO_OFFSET_MODE_7_MOD			0x037c
#define GPIO_OFFSET_MODE_8				0x0380
#define GPIO_OFFSET_MODE_8_SET			0x0384
#define GPIO_OFFSET_MODE_8_CLR			0x0388
#define GPIO_OFFSET_MODE_8_MOD			0x038c
#define GPIO_OFFSET_MODE_9				0x0390
#define GPIO_OFFSET_MODE_9_SET			0x0394
#define GPIO_OFFSET_MODE_9_CLR			0x0398
#define GPIO_OFFSET_MODE_9_MOD			0x039c
#define GPIO_OFFSET_MODE_10				0x03a0
#define GPIO_OFFSET_MODE_10_SET     	0x03a4
#define GPIO_OFFSET_MODE_10_CLR			0x03a8
#define GPIO_OFFSET_MODE_10_MOD			0x03ac
#define GPIO_OFFSET_MODE_11				0x03b0
#define GPIO_OFFSET_MODE_11_SET			0x03b4
#define GPIO_OFFSET_MODE_11_CLR			0x03b8
#define GPIO_OFFSET_MODE_11_MOD			0x03bc
#define GPIO_OFFSET_MODE_12				0x03c0
#define GPIO_OFFSET_MODE_12_SET			0x03c4
#define GPIO_OFFSET_MODE_12_CLR			0x03c8
#define GPIO_OFFSET_MODE_12_MOD			0x03cc
#define GPIO_OFFSET_MODE_13				0x03d0
#define GPIO_OFFSET_MODE_13_SET			0x03d4
#define GPIO_OFFSET_MODE_13_CLR			0x03d8
#define GPIO_OFFSET_MODE_13_MOD			0x03dc
#define GPIO_OFFSET_MODE_14				0x03e0
#define GPIO_OFFSET_MODE_14_SET			0x03e4
#define GPIO_OFFSET_MODE_14_CLR			0x03e8
#define GPIO_OFFSET_MODE_14_MOD			0x03ec
#define GPIO_OFFSET_MODE_15				0x03f0
#define GPIO_OFFSET_MODE_15_SET			0x03f4
#define GPIO_OFFSET_MODE_15_CLR			0x03f8
#define GPIO_OFFSET_MODE_15_MOD			0x03fc
#define GPIO_OFFSET_MODE_16				0x0400
#define GPIO_OFFSET_MODE_16_SET			0x0404
#define GPIO_OFFSET_MODE_16_CLR			0x0408
#define GPIO_OFFSET_MODE_16_MOD			0x040c
#define GPIO_OFFSET_MODE_17				0x0410
#define GPIO_OFFSET_MODE_17_SET			0x0414
#define GPIO_OFFSET_MODE_17_CLR			0x0418
#define GPIO_OFFSET_MODE_17_MOD			0x041c
#define GPIO_OFFSET_MODE_18				0x0420
#define GPIO_OFFSET_MODE_18_SET			0x0424
#define GPIO_OFFSET_MODE_18_CLR			0x0428
#define GPIO_OFFSET_MODE_18_MOD			0x042c
#define GPIO_OFFSET_MODE_19				0x0430
#define GPIO_OFFSET_MODE_19_SET			0x0434
#define GPIO_OFFSET_MODE_19_CLR			0x0438
#define GPIO_OFFSET_MODE_19_MOD			0x043c
#define GPIO_OFFSET_MODE_20				0x0440
#define GPIO_OFFSET_MODE_20_SET			0x0444
#define GPIO_OFFSET_MODE_20_CLR			0x0448
#define GPIO_OFFSET_MODE_20_MOD			0x044c
#define GPIO_OFFSET_MODE_21				0x0450
#define GPIO_OFFSET_MODE_21_SET			0x0454
#define GPIO_OFFSET_MODE_21_CLR			0x0458
#define GPIO_OFFSET_MODE_21_MOD			0x045c
#define GPIO_OFFSET_MODE_22				0x0460
#define GPIO_OFFSET_MODE_22_SET			0x0464
#define GPIO_OFFSET_MODE_22_CLR			0x0468
#define GPIO_OFFSET_MODE_22_MOD			0x046c
#define GPIO_OFFSET_MISC                0x0600
#define GPIO_OFFSET_MISC_SET            0x0604
#define GPIO_OFFSET_MISC_CLR            0x0608
#define GPIO_OFFSET_DBG                 0x06d0
#define GPIO_OFFSET_BANK                0x06e0
#define GPIO_OFFSET_TPBANK              0x06f0
#define GPIO_OFFSET_MODE_CFG_CT_REG     0x0700
#define GPIO_OFFSET_MODE_CFG_CT_REG_SET 0x0704
#define GPIO_OFFSET_MODE_CFG_CT_REG_CLR 0x0708
#define GPIO_OFFSET_AP_GOOD             0x0710
#define GPIO_OFFSET_AP_GOOD_SET         0x0714
#define GPIO_OFFSET_AP_GOOD_CLR         0x0718
#define GPIO_OFFSET_SEC_EN_0            0x0a00
#define GPIO_OFFSET_SEC_EN_1            0x0a10
#define GPIO_OFFSET_SEC_EN_2            0x0a20
#define GPIO_OFFSET_SEC_EN_3            0x0a30
#define GPIO_OFFSET_SEC_EN_4            0x0a40
#define GPIO_OFFSET_SEC_EN_5            0x0a50

#define GPIO_SIZE  (0x0001000)
#define REG_SIZE   (sizeof (u32))

/* The following two macro's are required to customize the common macro's. */
#define REG_DIST_IDX_SHIFT  (4)
#define REG_NAME(_name)     GPIO_OFFSET_ ## _name


/* Access descriptor for [GPIO_OFFSET_DIR_0 .. GPIO_OFFSET_MODE_22_MOD] */
static const access_descr_t gpio_access_descr_0 [] =
{
    /* 0x0000 */ ACCESS_DESCR (DIR_0, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_0_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_0_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0010 */ ACCESS_DESCR (DIR_1, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_1_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_1_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0020 */ ACCESS_DESCR (DIR_2, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_2_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_2_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0030 */ ACCESS_DESCR (DIR_3, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_3_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_3_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0040 */ ACCESS_DESCR (DIR_4, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_4_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_4_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0050 */ ACCESS_DESCR (DIR_5, DIR_0, ACCESS_RW, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_5_SET, DIR_0_SET, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR (DIR_5_CLR, DIR_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,
    /* 0x0060 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0070 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0080 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0090 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00a0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00b0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00c0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00d0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00e0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x00f0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0100 */ ACCESS_DESCR (DOUT_0, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_0_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_0_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0110 */ ACCESS_DESCR (DOUT_1, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_1_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_1_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0120 */ ACCESS_DESCR (DOUT_2, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_2_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_2_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0130 */ ACCESS_DESCR (DOUT_3, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_3_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_3_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0140 */ ACCESS_DESCR (DOUT_4, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_4_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_4_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0150 */ ACCESS_DESCR (DOUT_5, DOUT_0, ACCESS_RW, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_5_SET, DOUT_0_SET, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR (DOUT_5_CLR, DOUT_0_CLR, ACCESS_WO, ACCESS_ONE_BIT),     ACCESS_DESCR_EMPTY,
    /* 0x0160 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0170 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0180 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0190 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01a0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01b0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01c0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01d0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01e0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x01f0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0200 */ ACCESS_DESCR (DIN_0, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0210 */ ACCESS_DESCR (DIN_1, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0220 */ ACCESS_DESCR (DIN_2, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0230 */ ACCESS_DESCR (DIN_3, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0240 */ ACCESS_DESCR (DIN_4, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0250 */ ACCESS_DESCR (DIN_5, DIN_0, ACCESS_RO, ACCESS_ONE_BIT),       ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0260 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0270 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0280 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0290 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02a0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02b0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02c0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02d0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02e0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x02f0 */ ACCESS_DESCR_EMPTY,                                           ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,                                                   ACCESS_DESCR_EMPTY,
    /* 0x0300 */ ACCESS_DESCR (MODE_0,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_0_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_0_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_0_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0310 */ ACCESS_DESCR (MODE_1,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_1_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_1_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_1_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0320 */ ACCESS_DESCR (MODE_2,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_2_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_2_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_2_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0330 */ ACCESS_DESCR (MODE_3,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_3_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_3_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_3_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0340 */ ACCESS_DESCR (MODE_4,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_4_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_4_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_4_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0350 */ ACCESS_DESCR (MODE_5,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_5_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_5_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_5_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0360 */ ACCESS_DESCR (MODE_6,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_6_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_6_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_6_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0370 */ ACCESS_DESCR (MODE_7,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_7_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_7_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_7_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0380 */ ACCESS_DESCR (MODE_8,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_8_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_8_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_8_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0390 */ ACCESS_DESCR (MODE_9,  MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_9_SET,  MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_9_CLR,  MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_9_MOD,  MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03a0 */ ACCESS_DESCR (MODE_10, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_10_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_10_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_10_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03b0 */ ACCESS_DESCR (MODE_11, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_11_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_11_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_11_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03c0 */ ACCESS_DESCR (MODE_12, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_12_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_12_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_12_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03d0 */ ACCESS_DESCR (MODE_13, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_13_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_13_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_13_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03e0 */ ACCESS_DESCR (MODE_14, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_14_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_14_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_14_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x03f0 */ ACCESS_DESCR (MODE_15, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_15_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_15_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_15_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0400 */ ACCESS_DESCR (MODE_16, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_16_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_16_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_16_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0410 */ ACCESS_DESCR (MODE_17, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_17_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_17_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_17_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0420 */ ACCESS_DESCR (MODE_18, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_18_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_18_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_18_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0430 */ ACCESS_DESCR (MODE_19, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_19_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_19_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_19_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0440 */ ACCESS_DESCR (MODE_20, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_20_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_20_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_20_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0450 */ ACCESS_DESCR (MODE_21, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_21_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_21_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_21_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
    /* 0x0460 */ ACCESS_DESCR (MODE_22, MODE_0, ACCESS_RW, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_22_SET, MODE_0_SET, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_22_CLR, MODE_0_CLR, ACCESS_WO, ACCESS_THREE_BITS), ACCESS_DESCR (MODE_22_MOD, MODE_0_MOD, ACCESS_WO, ACCESS_THREE_BITS),
};

static const access_descr_map_t  gpio_access_descr_map [] =
{
    {
        .reg_start    = REG_NAME (DIR_0),
        .reg_end      = REG_NAME (MODE_22_MOD) + REG_SIZE,
        .access_descr = gpio_access_descr_0
    }
};

static const u32 gpio_access_descr_map_size = ARRAY_SIZE (gpio_access_descr_map);

static const u32 one_to_three_bit_cnvt [] =
{
    0x00000000, 0x00000007, 0x00000070, 0x00000077, 0x00000700, 0x00000707, 0x00000770, 0x00000777,
    0x00007000, 0x00007007, 0x00007070, 0x00007077, 0x00007700, 0x00007707, 0x00007770, 0x00007777
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

/* This function assumes that each pin requires 3 bits with 1 empty bit. Hence, a single bit */
/* will be translated into the corresponding 3 bits. Since this function is executed in EL2  */
/* when a cell accesses the GPIO it needs to be efficient. Note that 3 bits per pin only     */
/* allow 8 pins to be described in a single u32 value.                                       */
static u32 three_bits_per_pin (access_descr_t  access_descr)
{
    u32          idx = (get_access_dist_idx (access_descr) * 8) / BITS_PER_U32;
    u32          shift_right = (get_access_dist_idx (access_descr) * 8) % BITS_PER_U32;
    u32          bit_map;
    struct cell* cell = this_cell ();


    if (idx >= ARRAY_SIZE (cell->arch.gpio_bitmap))
	{
        return (0);
    }

    bit_map = (cell->arch.gpio_bitmap [idx] >> shift_right);

    /* At this point, only the 8 LSB bits are of importance and  */
    /* will be converted into 3 bits per pin. For speed we use a */
    /* lookup table for every 4 bits (16 values).                */
    bit_map = one_to_three_bit_cnvt [bit_map & 0x0000000f] | ((one_to_three_bit_cnvt [(bit_map >> 4) & 0x0000000f]) << (4 * 4));

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

            mmio_perform_access (virt_addr, &curr_mmio);
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

static int mt8370_gpio_cell_init (struct cell*  cell)
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

static void mt8370_gpio_cell_exit (struct cell*  cell)
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

static unsigned int mt8370_gpio_mmio_count_regions (struct cell*  cell)
{
	/* Only one MMIO GPIO region handler per cell. */
	return (1);
}

static int mt8370_gpio_init (void)
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

    	ret = mt8370_gpio_cell_init (&root_cell);

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

static void mt8370_gpio_shutdown (void)
{
    unsigned long phys_addr;


	if (virt_addr != NULL)
	{
        get_phys_addr (&root_cell, &phys_addr);

        paging_unmap_device (phys_addr, virt_addr, GPIO_SIZE);

		virt_addr = NULL;
	}
}

DEFINE_UNIT (mt8370_gpio, "mt8370_gpio");
