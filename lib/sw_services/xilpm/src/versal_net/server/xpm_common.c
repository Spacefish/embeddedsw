/******************************************************************************
* Copyright (c) 2018 - 2022 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/


#include "xil_io.h"

#include "xpm_common.h"
#include <stdarg.h>

#define DBG_STR_IDX(DebugType) ((((DebugType) & XPM_DEBUG_MASK) >> \
					XPM_DEBUG_SHIFT) - 1U)


u32 XPm_In32(u32 RegAddress)
{

	return Xil_In32(RegAddress);
}

void XPm_Out32(u32 RegAddress, u32 l_Val)
{

	Xil_Out32(RegAddress, l_Val);

}

void XPm_RMW32(u32 RegAddress, u32 Mask, u32 Value)
{
	u32 l_Val;

	l_Val = XPm_In32(RegAddress);
	l_Val = (l_Val & (~Mask)) | (Mask & Value);

	XPm_Out32(RegAddress, l_Val);
}

void XPm_Printf(u32 DebugType, const char *Fnstr, const char8 *Ctrl1, ...)
{
	va_list Args;
	static const char* const PrefixStr[] = {"ALERT", "ERR", "WARN", "INFO", "DBG"};
	u32 Idx = DBG_STR_IDX(DebugType);

	va_start(Args, Ctrl1);
	if ((((DebugType) & (DebugLog->LogLevel)) != (u8)FALSE) &&
		(Idx < (u32)ARRAY_SIZE(PrefixStr))) {
		xil_printf("%s %s: ", PrefixStr[Idx], Fnstr);
		xil_vprintf(Ctrl1, Args);
	}
	va_end(Args);
}