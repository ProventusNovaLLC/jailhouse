/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) 2025 MediaTek
 *
 * Sharing of EINT between root and other inmate cells and control
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


#define EINT_OFFSET_STA_0               0x0000
#define EINT_OFFSET_STA_1               0x0004
#define EINT_OFFSET_STA_2               0x0008
#define EINT_OFFSET_STA_3               0x000c
#define EINT_OFFSET_STA_4               0x0010
#define EINT_OFFSET_ACK_0               0x0040
#define EINT_OFFSET_ACK_1               0x0044
#define EINT_OFFSET_ACK_2               0x0048
#define EINT_OFFSET_ACK_3               0x004c
#define EINT_OFFSET_ACK_4               0x0050
#define EINT_OFFSET_MASK_0              0x0080
#define EINT_OFFSET_MASK_1              0x0084
#define EINT_OFFSET_MASK_2              0x0088
#define EINT_OFFSET_MASK_3              0x008c
#define EINT_OFFSET_MASK_4              0x0090
#define EINT_OFFSET_MASK_SET_0          0x00c0
#define EINT_OFFSET_MASK_SET_1          0x00c4
#define EINT_OFFSET_MASK_SET_2          0x00c8
#define EINT_OFFSET_MASK_SET_3          0x00cc
#define EINT_OFFSET_MASK_SET_4          0x00d0
#define EINT_OFFSET_MASK_CLR_0          0x0100
#define EINT_OFFSET_MASK_CLR_1          0x0104
#define EINT_OFFSET_MASK_CLR_2          0x0108
#define EINT_OFFSET_MASK_CLR_3          0x010c
#define EINT_OFFSET_MASK_CLR_4          0x0110
#define EINT_OFFSET_SENS_0              0x0140
#define EINT_OFFSET_SENS_1              0x0144
#define EINT_OFFSET_SENS_2              0x0148
#define EINT_OFFSET_SENS_3              0x014c
#define EINT_OFFSET_SENS_4              0x0150
#define EINT_OFFSET_SENS_SET_0          0x0180
#define EINT_OFFSET_SENS_SET_1          0x0184
#define EINT_OFFSET_SENS_SET_2          0x0188
#define EINT_OFFSET_SENS_SET_3          0x018c
#define EINT_OFFSET_SENS_SET_4          0x0190
#define EINT_OFFSET_SENS_CLR_0          0x01c0
#define EINT_OFFSET_SENS_CLR_1          0x01c4
#define EINT_OFFSET_SENS_CLR_2          0x01c8
#define EINT_OFFSET_SENS_CLR_3          0x01cc
#define EINT_OFFSET_SENS_CLR_4          0x01d0
#define EINT_OFFSET_SOFT_0              0x0200
#define EINT_OFFSET_SOFT_1              0x0204
#define EINT_OFFSET_SOFT_2              0x0208
#define EINT_OFFSET_SOFT_3              0x020c
#define EINT_OFFSET_SOFT_4              0x0210
#define EINT_OFFSET_SOFT_SET_0          0x0240
#define EINT_OFFSET_SOFT_SET_1          0x0244
#define EINT_OFFSET_SOFT_SET_2          0x0248
#define EINT_OFFSET_SOFT_SET_3          0x024c
#define EINT_OFFSET_SOFT_SET_4          0x0250
#define EINT_OFFSET_SOFT_CLR_0          0x0280
#define EINT_OFFSET_SOFT_CLR_1          0x0284
#define EINT_OFFSET_SOFT_CLR_2          0x0288
#define EINT_OFFSET_SOFT_CLR_3          0x028c
#define EINT_OFFSET_SOFT_CLR_4          0x0290
#define EINT_OFFSET_POL_0               0x0300
#define EINT_OFFSET_POL_1               0x0304
#define EINT_OFFSET_POL_2               0x0308
#define EINT_OFFSET_POL_3               0x030c
#define EINT_OFFSET_POL_4               0x0310
#define EINT_OFFSET_POL_SET_0           0x0340
#define EINT_OFFSET_POL_SET_1           0x0344
#define EINT_OFFSET_POL_SET_2           0x0348
#define EINT_OFFSET_POL_SET_3           0x034c
#define EINT_OFFSET_POL_SET_4           0x0350
#define EINT_OFFSET_POL_CLR_0           0x0380
#define EINT_OFFSET_POL_CLR_1           0x0384
#define EINT_OFFSET_POL_CLR_2           0x0388
#define EINT_OFFSET_POL_CLR_3           0x038c
#define EINT_OFFSET_POL_CLR_4           0x0390
#define EINT_OFFSET_D0EN_0              0x0400
#define EINT_OFFSET_D0EN_1              0x0404
#define EINT_OFFSET_D0EN_2              0x0408
#define EINT_OFFSET_D0EN_3              0x040c
#define EINT_OFFSET_D0EN_4              0x0410
#define EINT_OFFSET_DBNC_3_0            0x0500
#define EINT_OFFSET_DBNC_7_4            0x0504
#define EINT_OFFSET_DBNC_B_8            0x0508
#define EINT_OFFSET_DBNC_F_C            0x050c
#define EINT_OFFSET_DBNC_9_3_0          0x0590
#define EINT_OFFSET_DBNC_9_7_4          0x0594
#define EINT_OFFSET_DBNC_SET_3_0        0x0600
#define EINT_OFFSET_DBNC_SET_7_4        0x0604
#define EINT_OFFSET_DBNC_SET_B_8        0x0608
#define EINT_OFFSET_DBNC_SET_F_C        0x060c
#define EINT_OFFSET_DBNC_SET_9_3_0      0x0690
#define EINT_OFFSET_DBNC_SET_9_7_4      0x0694
#define EINT_OFFSET_DBNC_CLR_3_0        0x0700
#define EINT_OFFSET_DBNC_CLR_7_4        0x0704
#define EINT_OFFSET_DBNC_CLR_B_8        0x0708
#define EINT_OFFSET_DBNC_CLR_F_C        0x070c
#define EINT_OFFSET_DBNC_CLR_9_3_0      0x0790
#define EINT_OFFSET_DBNC_CLR_9_7_4      0x0794
#define EINT_OFFSET_DCON                0x0800
#define EINT_OFFSET_DCON_SEL_3_0        0x0840
#define EINT_OFFSET_DCON_SEL_SET_3_0    0x0880
#define EINT_OFFSET_DCON_SEL_CLR_3_0    0x08c0
#define EINT_OFFSET_EEVT                0x0900
#define EINT_OFFSET_RAW_STA_0           0x0a00
#define EINT_OFFSET_RAW_STA_1           0x0a04
#define EINT_OFFSET_RAW_STA_2           0x0a08
#define EINT_OFFSET_RAW_STA_3           0x0a0c
#define EINT_OFFSET_RAW_STA_4           0x0a10
#define EINT_OFFSET_SECURE_EINT_EN      0x0b00
#define EINT_OFFSET_SECURE_DIR_EINT_EN  0x0b10
#define EINT_OFFSET_DCM_ON              0x0b20
#define EINT_OFFSET_SECURE_STATUS       0x0b30
#define EINT_OFFSET_SYNC_EN_0           0x0c00
#define EINT_OFFSET_SYNC_EN_1           0x0c04
#define EINT_OFFSET_SYNC_EN_SET_0       0x0d00
#define EINT_OFFSET_SYNC_EN_SET_1       0x0d04
#define EINT_OFFSET_SYNC_EN_CLR_0       0x0e00
#define EINT_OFFSET_SYNC_EN_CLR_1       0x0e04
#define EINT_OFFSET_FPGA_EMUL_0         0x0f00
#define EINT_OFFSET_FPGA_EMUL_1         0x0f04
#define EINT_OFFSET_FPGA_EMUL_2         0x0f08
#define EINT_OFFSET_FPGA_EMUL_3         0x0f0c
#define EINT_OFFSET_FPGA_EMUL_4         0x0f10

#define EINT_SIZE  (0x00001000)
#define REG_SIZE   (sizeof (u32))

/* The following two macro's are required to customize the common macro's. */
#define REG_DIST_IDX_SHIFT  (2)
#define REG_NAME(_name)     EINT_OFFSET_ ## _name


/* Access descriptor for [EINT_OFFSET_STA_0 .. EINT_OFFSET_D0EN_4] */
static const access_descr_t eint_access_descr_0 [] =
{
    /* 0x0000 */ ACCESS_DESCR (STA_0, STA_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (STA_1, STA_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (STA_2, STA_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (STA_3, STA_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0010 */ ACCESS_DESCR (STA_4, STA_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0020 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0030 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0040 */ ACCESS_DESCR (ACK_0, ACK_0, ACCESS_WO, ACCESS_ONE_BIT),           ACCESS_DESCR (ACK_1, ACK_0, ACCESS_WO, ACCESS_ONE_BIT),           ACCESS_DESCR (ACK_2, ACK_0, ACCESS_WO, ACCESS_ONE_BIT),           ACCESS_DESCR (ACK_3, ACK_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0050 */ ACCESS_DESCR (ACK_4, ACK_0, ACCESS_WO, ACCESS_ONE_BIT),           ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0060 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0070 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0080 */ ACCESS_DESCR (MASK_0, MASK_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (MASK_1, MASK_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (MASK_2, MASK_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (MASK_3, MASK_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0090 */ ACCESS_DESCR (MASK_4, MASK_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00a0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00b0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00c0 */ ACCESS_DESCR (MASK_SET_0, MASK_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_SET_1, MASK_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_SET_2, MASK_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_SET_3, MASK_SET_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x00d0 */ ACCESS_DESCR (MASK_SET_4, MASK_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00e0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x00f0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0100 */ ACCESS_DESCR (MASK_CLR_0, MASK_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_CLR_1, MASK_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_CLR_2, MASK_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (MASK_CLR_3, MASK_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0110 */ ACCESS_DESCR (MASK_CLR_4, MASK_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0120 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0130 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0140 */ ACCESS_DESCR (SENS_0, SENS_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SENS_1, SENS_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SENS_2, SENS_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SENS_3, SENS_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0150 */ ACCESS_DESCR (SENS_4, SENS_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0160 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0170 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0180 */ ACCESS_DESCR (SENS_SET_0, SENS_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_SET_1, SENS_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_SET_2, SENS_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_SET_3, SENS_SET_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0190 */ ACCESS_DESCR (SENS_SET_4, SENS_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01a0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01b0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01c0 */ ACCESS_DESCR (SENS_CLR_0, SENS_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_CLR_1, SENS_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_CLR_2, SENS_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SENS_CLR_3, SENS_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x01d0 */ ACCESS_DESCR (SENS_CLR_4, SENS_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01e0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x01f0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0200 */ ACCESS_DESCR (SOFT_0, SOFT_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SOFT_1, SOFT_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SOFT_2, SOFT_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR (SOFT_3, SOFT_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0210 */ ACCESS_DESCR (SOFT_4, SOFT_0, ACCESS_RO, ACCESS_ONE_BIT),         ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0220 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0230 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0240 */ ACCESS_DESCR (SOFT_SET_0, SOFT_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_SET_1, SOFT_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_SET_2, SOFT_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_SET_3, SOFT_SET_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0250 */ ACCESS_DESCR (SOFT_SET_4, SOFT_SET_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0260 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0270 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0280 */ ACCESS_DESCR (SOFT_CLR_0, SOFT_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_CLR_1, SOFT_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_CLR_2, SOFT_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR (SOFT_CLR_3, SOFT_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0290 */ ACCESS_DESCR (SOFT_CLR_4, SOFT_CLR_0, ACCESS_WO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02a0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02b0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02c0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02d0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02e0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x02f0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0300 */ ACCESS_DESCR (POL_0, POL_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (POL_1, POL_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (POL_2, POL_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR (POL_3, POL_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0310 */ ACCESS_DESCR (POL_4, POL_0, ACCESS_RO, ACCESS_ONE_BIT),           ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0320 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0330 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0340 */ ACCESS_DESCR (POL_SET_0, POL_SET_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_SET_1, POL_SET_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_SET_2, POL_SET_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_SET_3, POL_SET_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0350 */ ACCESS_DESCR (POL_SET_4, POL_SET_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0360 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0370 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0380 */ ACCESS_DESCR (POL_CLR_0, POL_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_CLR_1, POL_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_CLR_2, POL_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR (POL_CLR_3, POL_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),
    /* 0x0390 */ ACCESS_DESCR (POL_CLR_4, POL_CLR_0, ACCESS_WO, ACCESS_ONE_BIT),   ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03a0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03b0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03c0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03d0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03e0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x03f0 */ ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
    /* 0x0380 */ ACCESS_DESCR (D0EN_0, D0EN_0, ACCESS_RW, ACCESS_ONE_BIT),         ACCESS_DESCR (D0EN_1, D0EN_0, ACCESS_RW, ACCESS_ONE_BIT),         ACCESS_DESCR (D0EN_2, D0EN_0, ACCESS_RW, ACCESS_ONE_BIT),         ACCESS_DESCR (D0EN_3, D0EN_0, ACCESS_RW, ACCESS_ONE_BIT),
    /* 0x0390 */ ACCESS_DESCR (D0EN_4, D0EN_0, ACCESS_RW, ACCESS_ONE_BIT),         ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,                                               ACCESS_DESCR_EMPTY,
};

/* Access descriptor for [EINT_OFFSET_RAW_STA_0 .. EINT_OFFSET_RAW_STA_4] */
static const access_descr_t eint_access_descr_1 [] =
{
    /* 0x0a00 */ ACCESS_DESCR (RAW_STA_0, RAW_STA_0, ACCESS_RO, ACCESS_ONE_BIT), ACCESS_DESCR (RAW_STA_1, RAW_STA_0, ACCESS_RO, ACCESS_ONE_BIT), ACCESS_DESCR (RAW_STA_2, RAW_STA_0, ACCESS_RO, ACCESS_ONE_BIT), ACCESS_DESCR (RAW_STA_3, RAW_STA_0, ACCESS_RO, ACCESS_ONE_BIT),
    /* 0x0a10 */ ACCESS_DESCR (RAW_STA_4, RAW_STA_0, ACCESS_RO, ACCESS_ONE_BIT), ACCESS_DESCR_EMPTY,                                             ACCESS_DESCR_EMPTY,                                             ACCESS_DESCR_EMPTY,
};

static const access_descr_map_t  eint_access_descr_map [] =
{
    {
        .reg_start    = REG_NAME (STA_0),
        .reg_end      = REG_NAME (D0EN_4) + REG_SIZE,
        .access_descr = eint_access_descr_0
    },
    {
        .reg_start    = REG_NAME (RAW_STA_0),
        .reg_end      = REG_NAME (RAW_STA_4) + REG_SIZE,
        .access_descr = eint_access_descr_1
    }
};

static const u32 eint_access_descr_map_size = ARRAY_SIZE (eint_access_descr_map);


static spinlock_t lock;

static void* virt_addr = NULL;


static u32 one_bit_per_pin (access_descr_t  access_descr);
static u32 addr_to_bitmap (struct mmio_access*  mmio,
                           access_descr_t       access_descr);

static void handle_ro_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);
static void handle_wo_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);
static void handle_rw_access (struct mmio_access*  mmio,
                              access_descr_t       access_descr);

static u16 get_access_descr (struct mmio_access*  mmio);
static enum mmio_result eint_handle_access (void*                arg,
                                            struct mmio_access*  mmio);

static int get_phys_addr (struct cell*    cell,
                          unsigned long*  phys_addr);


static u32 one_bit_per_pin (access_descr_t  access_descr)
{
    u32          idx = get_access_dist_idx (access_descr);
    struct cell* cell = this_cell ();


    if (idx >= ARRAY_SIZE (cell->arch.eint_bitmap))
	{
        return (0);
	}
    
    return (cell->arch.eint_bitmap [idx]);
}

static u32 addr_to_bitmap (struct mmio_access*  mmio,
                           access_descr_t       access_descr)
{
	switch (get_access_num_of_bits (access_descr))
	{
        case ACCESS_ONE_BIT:
            return (one_bit_per_pin (access_descr));
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

    while (idx < eint_access_descr_map_size)
    {
        if ((mmio->address >= eint_access_descr_map [idx].reg_start) &&
            (mmio->address <  eint_access_descr_map [idx].reg_end))
        {
            return (eint_access_descr_map [idx].access_descr [(mmio->address - eint_access_descr_map [idx].reg_start) >> 2]);
        }

        idx++;
    }

    return (ACCESS_DESCR_EMPTY);
}

static enum mmio_result eint_handle_access (void*                arg,
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
	    	/* The remaing EINT accesses shall only be performed by the root cell. */
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
		if (vendor->type != JAILHOUSE_VENDOR_MTK_EINT)
		{
			continue;
		}

		if (((*phys_addr) != 0)                         &&
            (vendor->mtk_eint.address != (*phys_addr)))
		{
			return (-EINVAL);
		}

        (*phys_addr) = vendor->mtk_eint.address;
	}

    return (0);
}                          

static int mt8365_eint_cell_init (struct cell*  cell)
{
	size_t                         pos;
	unsigned int                   cnt;
	unsigned long                  address = 0;
	const struct jailhouse_vendor* vendor;


	FOR_EACH_VENDOR (vendor, cell->config, cnt)
	{
		if (vendor->type != JAILHOUSE_VENDOR_MTK_EINT)
		{
			continue;
		}

		/* Verify that the EINT entries in the cell description are valid. */
		if (((vendor->mtk_eint.pin_base % sizeof (vendor->mtk_eint.pin_bitmap [0])) != 0)                                       ||
		    ((vendor->mtk_eint.pin_base + (sizeof (vendor->mtk_eint.pin_bitmap) * 8)) > (sizeof (cell->arch.eint_bitmap) * 8)))
		{
			return (-EINVAL);
		}

        /* Only one EINT address per cell is supported. */
		if ((address != 0)                         &&
	        (vendor->mtk_eint.address != address))
		{
			return (-EINVAL);
		}

		address = vendor->mtk_eint.address;

		/* Copy the EINT entries. */
		for (pos = 0; pos < ARRAY_SIZE (vendor->mtk_eint.pin_bitmap); pos++)
		{
			cell->arch.eint_bitmap [(vendor->mtk_eint.pin_base / (sizeof (vendor->mtk_eint.pin_bitmap [0]) * 8)) + pos] |= vendor->mtk_eint.pin_bitmap [pos];
		}
	}

   	/* Register handler. */
    mmio_region_register (cell, address, EINT_SIZE, eint_handle_access, (void*) address);

    /* And lastly, remove the EINT entries from the root cell. */
	if (cell != &root_cell)
	{
		for (pos = 0; pos < ARRAY_SIZE (cell->arch.eint_bitmap); pos++)
		{
			root_cell.arch.eint_bitmap [pos] &= ~(cell->arch.eint_bitmap [pos]);
		}
    }

	return (0);
}

static void mt8365_eint_cell_exit (struct cell*  cell)
{
	size_t                         pos;
	unsigned int                   cnt;
	const struct jailhouse_vendor* vendor;


	if (cell != &root_cell)
	{
		/* Return the EINT entries to the root cell. */
		for (pos = 0; pos < ARRAY_SIZE (cell->arch.eint_bitmap); pos++)
		{
			root_cell.arch.eint_bitmap [pos] |= cell->arch.eint_bitmap [pos];
		}

		/* Mask out bits which were not part of the root cell. */
		FOR_EACH_VENDOR (vendor, root_cell.config, cnt)
		{
			if (vendor->type != JAILHOUSE_VENDOR_MTK_EINT)
			{
				continue;
			}	

			for (pos = 0; pos < ARRAY_SIZE (vendor->mtk_eint.pin_bitmap); pos++)
			{
				root_cell.arch.eint_bitmap [(vendor->mtk_eint.pin_base / (sizeof (vendor->mtk_eint.pin_bitmap [0]) * 8)) + pos] &= vendor->mtk_eint.pin_bitmap [pos];
			}
		}
	}
}

static unsigned int mt8365_eint_mmio_count_regions (struct cell*  cell)
{
	/* Only one MMIO EINT region handler per cell. */
	return (1);
}

static int mt8365_eint_init (void)
{
	int           ret = 0;
    unsigned long phys_addr = 0;


	if (virt_addr == NULL)
	{
		/* Create hypervisor paging for EINT access. */
        /* This is only done once with the root cell. */
        ret = get_phys_addr (&root_cell, &phys_addr);
		if (ret != 0)
		{
			return (ret);
		}

        if (phys_addr != 0)
        {
            virt_addr = paging_map_device (phys_addr, EINT_SIZE);
            if (virt_addr == NULL)
            {
                ret = -ENOMEM;
            }
        }

    	ret = mt8365_eint_cell_init (&root_cell);

    	if (ret != 0)
	    {
		    /* Cleanup if cell initialization failed. */
    		if (virt_addr != NULL)
	    	{
                paging_unmap_device (phys_addr, virt_addr, EINT_SIZE);

                virt_addr = NULL;
	    	}
	    }
    }

	return (ret);
}

static void mt8365_eint_shutdown (void)
{
    unsigned long phys_addr;


	if (virt_addr != NULL)
	{
        get_phys_addr (&root_cell, &phys_addr);

        paging_unmap_device (phys_addr, virt_addr, EINT_SIZE);

		virt_addr = NULL;
	}
}

DEFINE_UNIT (mt8365_eint, "mt8365_eint");
