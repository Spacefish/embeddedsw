/******************************************************************************
* Copyright (c) 2018 - 2022 Xilinx, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
*
* @file xplm_pm.c
*
* This file contains the wrapper code xilpm
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date        Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00  kc   07/20/2018 Initial release
* 1.01  rp   08/08/2019 Added code to send PM notify callback through IPI
* 1.02  kc   03/23/2020 Minor code cleanup
* 1.03  kc   08/04/2020 Initialized IpiMask to zero for PMC CDO commands
*       kc   08/04/2020 Added default NPLL configuration for master SLR devices
* 1.04  ma   01/12/2021 Initialize SlrType to invalid SLR type
*                       Call PMC state clear function when error occurs while
*                        processing PLM CDO
*       bm   02/08/2021 Added SysmonInit after processing PMC CDO
*       skd  03/16/2021 Added code to monitor if psm is alive or not
*       rama 03/22/2021 Added hook for STL periodic execution and
*                       FTTI configuration support for keep alive task
*       bm   04/10/2021 Updated scheduler function calls
*       bsv  04/16/2021 Add provision to store Subsystem Id in XilPlmi
*       bm   04/27/2021 Updated priority of XPlm_KeepAliveTask
*
* </pre>
*
* @note
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xplm_default.h"
#include "xplm_pm.h"
#ifndef PLM_PM_EXCLUDE
#include "xpm_api.h"
#include "xpm_ipi.h"
#include "xpm_psm.h"
#include "xpm_subsystem.h"
#endif
#include "xplmi_scheduler.h"
#include "xplmi_util.h"
#include "xloader.h"
#include "xplmi_sysmon.h"
#ifdef PLM_ENABLE_STL
#include "xplm_stl.h"
#endif

/************************** Constant Definitions *****************************/
/**
 * NPLL CFG params
 * LOCK_DLY[31:25]=0x3f, LOCK_CNT[22:13]=0x2EE, LFHF[11:10]=0x3,
 * CP[8:5]=0x3, RES[3:0]=0x5
 */
#define XPLM_NOCPLL_CFG_VAL		(0x7E5DCC65U)

/**
 * NPLL CTRL params
 * POST_SRC[26:24]=0x0, PRE_SRC[22:20]=0x0, CLKOUTDIV[17:16]=0x3,
 * FBDIV[15:8]=0x48, BYPASS[3]=0x1, RESET[0]=0x1
 */
#define XPLM_NOCPLL_CTRL_VAL		(0x34809U)
#define NOCPLL_TIMEOUT			(100000U)
#ifdef PLM_PM_EXCLUDE
#define PM_SUBSYS_PMC				(0x1c000001U)
#endif

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
#ifndef PLM_PM_EXCLUDE
static void XPlm_PmRequestCb(const u32 IpiMask, const XPmApiCbId_t EventId, u32 *Payload);
#endif
static int XPlm_ConfigureDefaultNPll(void);
#ifdef XPAR_XIPIPSU_0_DEVICE_ID
static u32 XPlm_UpdateCounterVal(u8 Val);
static int XPlm_SendKeepAliveEvent(void);
static int XPlm_KeepAliveTask(void *Arg);
static u8 XPlm_SetAliveStsVal(u8 Val);
#endif /* XPAR_XIPIPSU_0_DEVICE_ID */

/************************** Variable Definitions *****************************/

#ifndef PLM_PM_EXCLUDE
/*****************************************************************************/
/**
* @brief This function is registered with XilPm to send any data to IPI master
* when a event comes.
*
* @param	IpiMask IPI master ID
* @param	EventId Id of the event as defined in XilPm
* @param	Payload is pointer to the Data that needs to be sent to the IPI
* 		master
*
* @return	None
*
*****************************************************************************/
static void XPlm_PmRequestCb(const u32 IpiMask, const XPmApiCbId_t EventId, u32 *Payload)
{
#ifdef XPAR_XIPIPSU_0_DEVICE_ID
	XStatus Status = XST_FAILURE;

	if ((PM_INIT_SUSPEND_CB == EventId) || (PM_NOTIFY_CB == EventId)) {
		Status = XPlmi_IpiWrite(IpiMask, Payload, XPLMI_CMD_RESP_SIZE,
					XIPIPSU_BUF_TYPE_MSG);
		if (XST_SUCCESS != Status) {
			XPlmi_Printf(DEBUG_GENERAL,
			 "%s Error in IPI write: %d\r\n", __func__, Status);
		} else {
			Status = XPlmi_IpiTrigger(IpiMask);
			if (XST_SUCCESS != Status) {
				XPlmi_Printf(DEBUG_GENERAL,
				"%s Error in IPI trigger: %d\r\n", __func__, Status);
			}
		}
	} else {
		XPlmi_Printf(DEBUG_GENERAL,
		 "%s Error: Unsupported EventId: %d\r\n", __func__, EventId);
	}
#else
	XPlmi_Printf(DEBUG_GENERAL, "%s Error: IPI is not defined\r\n", __func__);
#endif /* XPAR_XIPIPSU_0_DEVICE_ID */
}
#endif

/*****************************************************************************/
/**
* @brief It calls the XilPm initialization API to initialize its structures.
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
int XPlm_PmInit(void)
{
	int Status = XST_FAILURE;
#ifndef PLM_PM_EXCLUDE
	/**
	 * Initialize the XilPm component. It registers the callback handlers,
	 * variables, events
	 */
	Status = XPm_Init(XPlm_PmRequestCb, &XLoader_RestartImage);
	if (Status != XST_SUCCESS)
	{
		Status = XPlmi_UpdateStatus(XPLM_ERR_PM_MOD, Status);
		goto END;
	}
#else
	Status = XST_SUCCESS;
	goto END;
#endif

END:
	return Status;
}

/*****************************************************************************/
/**
* @brief This function configures the NPLL equal to slave SLR ROM NPLL
*        frequency. It is only required for master SLR devices.
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
static int XPlm_ConfigureDefaultNPll(void)
{
	int Status = XST_FAILURE;

	/* Set the PLL helper Data */
	Xil_Out32(CRP_NOCPLL_CFG, XPLM_NOCPLL_CFG_VAL);

	/* Set the PLL Basic Controls */
	Xil_Out32(CRP_NOCPLL_CTRL, XPLM_NOCPLL_CTRL_VAL);

	/* De-assert the PLL Reset; PLL is still in bypass mode only */
	XPlmi_UtilRMW(CRP_NOCPLL_CTRL, CRP_NOCPLL_CTRL_RESET_MASK, 0x0U);

	/* Check for NPLL lock */
	Status = XPlmi_UtilPoll(CRP_PLL_STATUS,
			CRP_PLL_STATUS_NOCPLL_LOCK_MASK,
			CRP_PLL_STATUS_NOCPLL_LOCK_MASK,
			NOCPLL_TIMEOUT);
	if (Status != XST_SUCCESS) {
		Status = XPlmi_UpdateStatus(XPLM_ERR_NPLL_LOCK, 0);
		goto END;
	}

	/* Release the bypass mode */
	XPlmi_UtilRMW(CRP_NOCPLL_CTRL, CRP_NOCPLL_CTRL_BYPASS_MASK, 0x0U);

END:
	return Status;
}

/*****************************************************************************/
/**
* @brief This function executes the PMC CDO present in PMC RAM.
*
* @param	Arg Not used in the function
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
int XPlm_ProcessPmcCdo(void *Arg)
{
	int Status = XST_FAILURE;
	XPlmiCdo Cdo;
	u32 SlrType = XLOADER_SSIT_INVALID_SLR;

	XPlmi_Printf(DEBUG_DETAILED, "%s\n\r", __func__);
	(void )Arg;

	/**
	 * Configure NoC frequency equivalent to the frequency ROM sets in
	 * Slave devices
	 */
	SlrType = XPlmi_In32(PMC_TAP_SLR_TYPE) &
			PMC_TAP_SLR_TYPE_VAL_MASK;
	if (SlrType == XLOADER_SSIT_MASTER_SLR) {
		Status = XPlm_ConfigureDefaultNPll();
		if (Status != XST_SUCCESS) {
			goto END;
		}
	}

	/**
	 *  Pass the PLM CDO to CDO parser, PLM CDO contains
	 *  - Device topology
	 *  - PMC block configuration
	 */

	/* Process the PLM CDO */
	Status = XPlmi_InitCdo(&Cdo);
	if (Status != XST_SUCCESS) {
		goto END;
	}
	Cdo.BufPtr = (u32 *)XPLMI_PMCRAM_BASEADDR;
	Cdo.BufLen = XPLMI_PMCRAM_LEN;
	Cdo.SubsystemId = PM_SUBSYS_PMC;
	Status = XPlmi_ProcessCdo(&Cdo);

	if (XST_SUCCESS != Status) {
		XLoader_PMCStateClear();
	}

	Status = XPlmi_SysMonInit();
	if (Status != XST_SUCCESS) {
		goto END;
	}
END:
	return Status;
}

#ifdef XPAR_XIPIPSU_0_DEVICE_ID
/*****************************************************************************/
/**
* @brief	This function updates the counter value
*
* @param	Val to Increment or Clear the CounterVal variable
*
* @return	CounterVal
*
*****************************************************************************/
static u32 XPlm_UpdateCounterVal(u8 Val)
{
	static u32 CounterVal = 0U;

	if(Val == XPLM_PSM_COUNTER_INCREMENT) {
		/* Increment the counter value */
		CounterVal++;
	}else if(Val == XPLM_PSM_COUNTER_CLEAR){
		/* Clear the counter value */
		CounterVal = 0U;
	} else{
		/* To avoid Misra-C violation  */
	}

	return CounterVal;
}

/*****************************************************************************/
/**
* @brief	This function sends keep alive IPI event to PSM
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
static int XPlm_SendKeepAliveEvent(void)
{
	int Status = XST_FAILURE;
	u32 Payload[XPLMI_IPI_MAX_MSG_LEN] = {0U};

	/* Assign PSM keep alive API ID to IPI payload[0] */
	Payload[0U] = XPLM_PSM_API_KEEP_ALIVE;

#ifndef PLM_PM_EXCLUDE
	/* Send IPI for keep alive event to PSM */
	Status = XPm_IpiSend(PSM_IPI_INT_MASK, Payload);
	if (XST_SUCCESS != Status) {
		XPlmi_Printf(DEBUG_GENERAL, "%s Error in IPI send: %0x\r\n",
				__func__, Status);
		/* Update status in case of error */
		Status = XPlmi_UpdateStatus(XPLM_ERR_IPI_SEND, Status);
	}
#else
	Status = XST_SUCCESS;
#endif

	return Status;
}

/*****************************************************************************/
/**
* @brief	This function updates the keep alive status variable
*
* @param	Val to set the status as started or not started or error
*
* @return	PsmKeepAliveStatus
*
*****************************************************************************/
static u8 XPlm_SetAliveStsVal(u8 Val)
{
	static u8 PsmKeepAliveStatus = XPLM_PSM_ALIVE_NOT_STARTED;

	if(Val != XPLM_PSM_ALIVE_RETURN) {
		/* Update the Keep Alive Status */
		PsmKeepAliveStatus = Val;
	}

	return PsmKeepAliveStatus;
}

#ifndef PLM_PM_EXCLUDE
/*****************************************************************************/
/**
* @brief	This function checks if PSM is alive and healthy.
*
* @param	Arg Not used in the function currently
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
static int XPlm_KeepAliveTask(void *Arg)
{
	int Status = XST_FAILURE;
	u32 ActualCounterValue;
#ifdef PLM_ENABLE_STL
	int StlStatus = XST_FAILURE;
#endif

	(void)Arg;

	/**
	 * Check if PSM is running and PSMFW is loaded and no error occurred
	 * from PSM Keep alive event.
	 */
	if (((u8)TRUE == XPmPsm_FwIsPresent()) && (XPLM_PSM_ALIVE_ERR !=
	    XPlm_SetAliveStsVal(XPLM_PSM_ALIVE_RETURN))) {
		/**
		 * Check if the keep alive task called for first time then skip
		 * comparing keep alive counter value.
		 */
		if (XPLM_PSM_ALIVE_STARTED == XPlm_SetAliveStsVal(XPLM_PSM_ALIVE_RETURN)) {
			/**
			 * Read keep alive counter value from RTCA(Run time
			 * configuration area) register.
			 */
			ActualCounterValue = XPlmi_In32(XPLM_PSM_ALIVE_COUNTER_ADDR);
			/* Increment expected keep alive counter value */
			(void)XPlm_UpdateCounterVal(XPLM_PSM_COUNTER_INCREMENT);
			/**
			 * Check if PSM incremented keep alive counter value or
			 * not. Return error if counter value is not matched
			 * with expected value.
			 */
			if (ActualCounterValue != XPlm_UpdateCounterVal(XPLM_PSM_COUNTER_RETURN)) {
				XPlmi_Printf(DEBUG_GENERAL, "%s ERROR: PSM is not alive\r\n",
						__func__);
				/* Clear RTCA register */
				XPlmi_Out32(XPLM_PSM_ALIVE_COUNTER_ADDR,
						0U);
				/* Clear expected counter value */
				(void)XPlm_UpdateCounterVal(XPLM_PSM_COUNTER_CLEAR);
				/* Update PSM keep alive status for error */
				(void)XPlm_SetAliveStsVal(XPLM_PSM_ALIVE_ERR);
				/* Remove Keep alive task in case of error */
				Status = XPlm_RemoveKeepAliveTask();
				/* Update the error status */
				Status = XPlmi_UpdateStatus(XPLM_ERR_PSM_NOT_ALIVE,
								Status);
				goto END;
			}
		}

		/* Send keep alive IPI event to PSM */
		Status = XPlm_SendKeepAliveEvent();
		if (XST_SUCCESS != Status) {
			/* Remove Keep alive task in case of error */
			(void)XPlm_RemoveKeepAliveTask();
			goto END;
		}

		/* Update PSM keep alive status as successfully started */
		(void)XPlm_SetAliveStsVal(XPLM_PSM_ALIVE_STARTED);
	}

	Status = XST_SUCCESS;

END:
#ifdef PLM_ENABLE_STL
	/* Execute STL periodic Tasks */
	StlStatus = XPlm_PeriodicStlHook();
	if (XST_SUCCESS == Status) {
		Status = StlStatus;
	}
#endif
	return Status;
}
#endif

/*****************************************************************************/
/**
* @brief This function creates keep alive scheduler task
*
* @param PtrMilliSeconds periodicity of the task (must be > 10ms)
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
int XPlm_CreateKeepAliveTask(void *PtrMilliSeconds)
{
	int Status = XST_FAILURE;
#ifndef PLM_PM_EXCLUDE
	u32 MilliSeconds = *(u32*)PtrMilliSeconds;

	/**
	 * Validate input parameter (MilliSeconds) which needs to be greater
	 * than minimum FTTI time (10ms).
	 */
	if (XPLM_MIN_FTTI_TIME > MilliSeconds) {
		Status = XPlmi_UpdateStatus(XPLM_ERR_KEEP_ALIVE_TASK_CREATE,
						XST_INVALID_PARAM);
		goto END;
	}

	/* Clear keep alive counter and status as not started */
	XPlmi_Out32(XPLM_PSM_ALIVE_COUNTER_ADDR, 0U);
	(void)XPlm_UpdateCounterVal(XPLM_PSM_COUNTER_CLEAR);
	XPlm_SetAliveStsVal(XPLM_PSM_ALIVE_NOT_STARTED);

	/**
	 * Add keep alive task in scheduler which runs at every
	 * XPLM_DEFAULT_FTTI_TIME period.
	 */
	Status = XPlmi_SchedulerAddTask(XPLM_PSM_HEALTH_CHK, XPlm_KeepAliveTask,
			MilliSeconds, XPLM_TASK_PRIORITY_0, NULL,
			XPLMI_PERIODIC_TASK);
	if (XST_SUCCESS != Status) {
		Status = XPlmi_UpdateStatus(XPLM_ERR_KEEP_ALIVE_TASK_CREATE,
						Status);
	}
#else
	(void)PtrMilliSeconds;
	Status = XST_SUCCESS;
	goto END;
#endif

END:
	return Status;
}

/*****************************************************************************/
/**
* @brief This function remove keep alive scheduler task
*
* @return	Status as defined in xplmi_status.h
*
*****************************************************************************/
int XPlm_RemoveKeepAliveTask(void)
{
	int Status = XST_FAILURE;

#ifndef PLM_PM_EXCLUDE
	/* Remove keep alive task from scheduler */
	Status = XPlmi_SchedulerRemoveTask(XPLM_PSM_HEALTH_CHK,
			XPlm_KeepAliveTask, 0U, NULL);
	if (XST_SUCCESS != Status) {
		/* Update minor error value to status */
		Status = (int)XPLM_PSM_ALIVE_REMOVE_TASK_ERR;
	}
#else
	Status = XST_SUCCESS;
#endif

	return Status;
}
#endif /* XPAR_XIPIPSU_0_DEVICE_ID */