/******************************************************************************
* Copyright (c) 2019 - 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


/*****************************************************************************/
/**
*
* @file xplmi.h
*
* This file contains declarations PLMI module.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  kc   02/07/2019 Initial release
* 1.01  ma   08/01/2019 Added LPD init code
* 1.02  kc   02/19/2020 Moved code to support PLM banner from PLM app
*       bsv  04/04/2020 Code clean up
* 1.03  bsv  07/07/2020 Made functions used in single transaltion unit as
*						static
*       kc   07/28/2020 Added WDT MACRO to indicate WDT initialized
*       skd  07/29/2020 Added device copy macros
*       bm   09/08/2020 Added RunTime Configuration related registers
*       bsv  09/30/2020 Added XPLMI_CHUNK_SIZE macro
*       bm   10/14/2020 Code clean up
*       td   10/19/2020 MISRA C Fixes
* 1.04  nsk  12/14/2020 Modified xplmi_event_logging.c to use Canonical
*                       names.
* 1.05  ma   03/04/2021 Added IPI security defines
*       har  03/17/2021 Added Secure State register for authentication and
*                       encryption
*       ma   03/24/2021 Added RTCA Debug Log Address define
*       bm   03/24/2021 Added RTCA defines for Error Status registers
*       har  03/31/2021 Added RTCA defines for PDI ID
*       bm   05/05/2021 Added USR_ACCESS defines for PLD0 image
*
* </pre>
*
* @note
*
******************************************************************************/

#ifndef XPLMI_H
#define XPLMI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xplmi_generic.h"

/************************** Constant Definitions *****************************/
/* SDK release version */
#define SDK_RELEASE_YEAR	"2022"
#define SDK_RELEASE_QUARTER	"2"

/*
 * Device Copy flag related macros
 */
#define XPLMI_DEVICE_COPY_STATE_MASK		(0x7U << 5U)
#define XPLMI_DEVICE_COPY_STATE_BLK			(0x0U << 5U)
#define XPLMI_DEVICE_COPY_STATE_INITIATE	(0x1U << 5U)
#define XPLMI_DEVICE_COPY_STATE_WAIT_DONE	(0x2U << 5U)
/*
 * PMCRAM CHUNK SIZE
 */
#define XPLMI_CHUNK_SIZE	(0x10000U)

/* IPI command Secure/Non-secure flags */
#define XPLMI_CMD_SECURE					0x0U
#define XPLMI_CMD_NON_SECURE				0x1U

/**************************** Type Definitions *******************************/
#define UART_INITIALIZED	((u8)(1U << 0U))
#define LPD_INITIALIZED		((u8)(1U << 1U))
#define LPD_WDT_INITIALIZED	((u8)(1U << 2U))

/***************** Macros (Inline Functions) Definitions *********************/

/*
 * PLM RunTime Configuration Registers related defines
 */
/* PLM RunTime Configuration Area Base Address */
#define XPLMI_RTCFG_BASEADDR			(0xF2014000U)

/* Offsets of PLM Runtime Configuration Registers */
#define XPLMI_RTCFG_RTCA_ADDR			(XPLMI_RTCFG_BASEADDR + 0x0U)
#define XPLMI_RTCFG_VERSION_ADDR		(XPLMI_RTCFG_BASEADDR + 0x4U)
#define XPLMI_RTCFG_SIZE_ADDR			(XPLMI_RTCFG_BASEADDR + 0x8U)
#define XPLMI_RTCFG_DBG_LOG_BUF_ADDR	(XPLMI_RTCFG_BASEADDR + 0x10U)
#define XPLMI_RTCFG_IMGINFOTBL_ADDRLOW_ADDR	(XPLMI_RTCFG_BASEADDR + 0x40U)
#define XPLMI_RTCFG_IMGINFOTBL_ADDRHIGH_ADDR	(XPLMI_RTCFG_BASEADDR + 0x44U)
#define XPLMI_RTCFG_IMGINFOTBL_LEN_ADDR		(XPLMI_RTCFG_BASEADDR + 0x48U)
#define XPLMI_RTCFG_SECURESTATE_AHWROT_ADDR	(XPLMI_RTCFG_BASEADDR + 0x14CU)
#define XPLMI_RTCFG_SECURESTATE_SHWROT_ADDR	(XPLMI_RTCFG_BASEADDR + 0x150U)
#define XPLMI_RTCFG_PMC_ERR1_STATUS_ADDR	(XPLMI_RTCFG_BASEADDR + 0x154U)
#define XPLMI_RTCFG_PMC_ERR2_STATUS_ADDR	(XPLMI_RTCFG_BASEADDR + 0x158U)
#define XPLMI_RTCFG_PSM_ERR1_STATUS_ADDR	(XPLMI_RTCFG_BASEADDR + 0x15CU)
#define XPLMI_RTCFG_PSM_ERR2_STATUS_ADDR	(XPLMI_RTCFG_BASEADDR + 0x160U)
#define XPLMI_RTCFG_PDI_ID_ADDR			(XPLMI_RTCFG_BASEADDR + 0x164U)
#define XPLMI_RTCFG_USR_ACCESS_ADDR		(XPLMI_RTCFG_BASEADDR + 0x168U)

/* Masks of PLM RunTime Configuration Registers */
#define XPLMI_RTCFG_IMGINFOTBL_NUM_ENTRIES_MASK	(0x0000FFFFU)
#define XPLMI_RTCFG_IMGINFOTBL_CHANGE_CTR_MASK	(0xFFFF0000U)

/* Shifts of PLM RunTime Configuration Registers */
#define XPLMI_RTCFG_IMGINFOTBL_CHANGE_CTR_SHIFT	(0x10U)
/* Default Values of PLM RunTime Configuration Registers */
#define XPLMI_RTCFG_VER				(0x1U)
#define XPLMI_RTCFG_SIZE			(0x400U)
#define XPLMI_RTCFG_IMGINFOTBL_ADDR_HIGH	(0x0U)
#define XPLMI_RTCFG_IMGINFOTBL_LEN		(0x0U)
#define XPLMI_RTCFG_IDENTIFICATION		(0x41435452U)
#define XPLMI_RTCFG_SECURESTATE_AHWROT		(0xA5A5A5A5U)
#define XPLMI_RTCFG_SECURESTATE_SHWROT		(0x96969696U)
#define XPLMI_RTCFG_PDI_ID			(0x0U)

/* Values of Secure State Register */
#define XPLMI_RTCFG_SECURESTATE_EMUL_AHWROT	(0x5A5A5A5AU)
#define XPLMI_RTCFG_SECURESTATE_EMUL_SHWROT	(0x69696969U)
#define XPLMI_RTCFG_SECURESTATE_NONSECURE	(0xD2D2D2D2U)

/*
 * Using FW_IS_PRESENT to indicate Boot PDI loading is completed
 */
#define XPlmi_SetBootPdiDone()	XPlmi_UtilRMW(PMC_GLOBAL_GLOBAL_CNTRL, \
					PMC_GLOBAL_GLOBAL_CNTRL_FW_IS_PRESENT_MASK, \
					PMC_GLOBAL_GLOBAL_CNTRL_FW_IS_PRESENT_MASK)

#define XPlmi_IsLoadBootPdiDone() (((XPlmi_In32(PMC_GLOBAL_GLOBAL_CNTRL) & \
				PMC_GLOBAL_GLOBAL_CNTRL_FW_IS_PRESENT_MASK) == \
				PMC_GLOBAL_GLOBAL_CNTRL_FW_IS_PRESENT_MASK) ? \
					(TRUE) : (FALSE))

/* PLMI GENERIC MODULE Data Structures IDs */
#define XPLMI_WDT_DS_ID			(0x01U)
#define XPLMI_TRACELOG_DS_ID		(0x02U)
#define XPLMI_LPDINITIALIZED_DS_ID	(0x03U)
#define XPLMI_BANNER_DS_ID		(0x04U)
#define XPLMI_UPDATE_IPIMASK_DS_ID	(0x05U)

/************************** Function Prototypes ******************************/
int XPlmi_Init(void);
void XPlmi_LpdInit(void);
void XPlmi_ResetLpdInitialized(void);
void XPlmi_PrintPlmBanner(void);

/************************** Variable Definitions *****************************/
extern u32 LpdInitialized;

#ifdef __cplusplus
}
#endif

#endif  /* XPLMI_H */