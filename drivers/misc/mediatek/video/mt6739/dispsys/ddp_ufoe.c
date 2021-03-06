/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "UFOE"
#include "ddp_log.h"
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
#include <linux/delay.h>

#include "ddp_info.h"
#include "ddp_hal.h"
#include "ddp_reg.h"

static bool ufoe_enable;
static bool lr_mode_en;
static int compress_ratio;

static void ufoe_dump(void)
{
	DDPDUMP("== DISP UFOE REGS ==\n");
	DDPDUMP("(0x000)UFOE_START=0x%x\n", DISP_REG_GET(DISP_REG_UFO_START));
	DDPDUMP("(0x020)UFOE_PAD=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CR0P6_PAD));
	DDPDUMP("(0x050)UFOE_WIDTH=0x%x\n", DISP_REG_GET(DISP_REG_UFO_FRAME_WIDTH));
	DDPDUMP("(0x054)UFOE_HEIGHT=0x%x\n", DISP_REG_GET(DISP_REG_UFO_FRAME_HEIGHT));
	DDPDUMP("(0x100)UFOE_CFG0=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CFG_0B));
	DDPDUMP("(0x104)UFOE_CFG1=0x%x\n", DISP_REG_GET(DISP_REG_UFO_CFG_1B));
}

static int ufoe_init(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_DISP0_DISP_UFOE, "ufoe");
#else
	ddp_clk_enable(DISP0_DISP_UFOE);
#endif
	DDPMSG("ufoe_clock on CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

static int ufoe_deinit(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_DISP0_DISP_UFOE, "ufoe");
#else
	ddp_clk_disable(DISP0_DISP_UFOE);
#endif
	DDPMSG("ufoe_clock off CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

int ufoe_start(enum DISP_MODULE_ENUM module, struct cmdqRecStruct *cmdq)
{
	if (ufoe_enable)
		DISP_REG_SET_FIELD(cmdq, START_FLD_DISP_UFO_START, DISP_REG_UFO_START, 1);

	DDPMSG("ufoe_start, ufoe_start:0x%x\n", DISP_REG_GET(DISP_REG_UFO_START));
	return 0;
}


int ufoe_stop(enum DISP_MODULE_ENUM module, void *cmdq_handle)
{
	DISP_REG_SET_FIELD(cmdq_handle, START_FLD_DISP_UFO_START, DISP_REG_UFO_START, 0);
	DDPMSG("ufoe_stop, ufoe_start:0x%x\n", DISP_REG_GET(DISP_REG_UFO_START));
	return 0;
}

static int ufoe_config(enum DISP_MODULE_ENUM module, struct disp_ddp_path_config *pConfig, void *handle)
{
	LCM_PARAMS *disp_if_config = &(pConfig->dispif_config);
	LCM_DSI_PARAMS *lcm_config = &(disp_if_config->dsi);

	if (lcm_config->ufoe_enable == 1 && pConfig->dst_dirty) {
		ufoe_enable = 1;
		/* disable BYPASS ufoe */
		DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_BYPASS, DISP_REG_UFO_START, 0);
		/* DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_START, DISP_REG_UFO_START, 1); */
		if (lcm_config->ufoe_params.lr_mode_en == 1) {
			lr_mode_en = 1;
			DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_LR_EN, DISP_REG_UFO_START, 1);
		} else {
			DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_LR_EN, DISP_REG_UFO_START, 0);
			compress_ratio = lcm_config->ufoe_params.compress_ratio;
			if (lcm_config->ufoe_params.compress_ratio == 3) {
				unsigned int internal_width =
					disp_if_config->width + disp_if_config->width % 4;
				DISP_REG_SET_FIELD(handle, CFG_0B_FLD_DISP_UFO_CFG_COM_RATIO,
						DISP_REG_UFO_CFG_0B, 1);
				if (internal_width % 6 != 0) {
					DISP_REG_SET_FIELD(handle,
							CR0P6_PAD_FLD_DISP_UFO_STR_PAD_NUM,
							DISP_REG_UFO_CR0P6_PAD,
							(((internal_width / 6 + 1) * 6) - internal_width));
				}
			} else
				DISP_REG_SET_FIELD(handle, CFG_0B_FLD_DISP_UFO_CFG_COM_RATIO,
						DISP_REG_UFO_CFG_0B, 0);

			if (lcm_config->ufoe_params.vlc_disable) {
				DISP_REG_SET_FIELD(handle, CFG_0B_FLD_DISP_UFO_CFG_VLC_EN,
						DISP_REG_UFO_CFG_0B, 0);
				DISP_REG_SET(handle, DISP_REG_UFO_CFG_1B, 0x1);
			} else {
				DISP_REG_SET_FIELD(handle, CFG_0B_FLD_DISP_UFO_CFG_VLC_EN,
						DISP_REG_UFO_CFG_0B, 1);
				DISP_REG_SET(handle, DISP_REG_UFO_CFG_1B,
						(lcm_config->ufoe_params.vlc_config ==
						 0) ? 5 : lcm_config->ufoe_params.vlc_config);
			}
		}
		DISP_REG_SET(handle, DISP_REG_UFO_FRAME_WIDTH, pConfig->dst_w);
		DISP_REG_SET(handle, DISP_REG_UFO_FRAME_HEIGHT, pConfig->dst_h);
		/* ufoe_dump(); */
	}
	/* ufoe_dump(); */
	return 0;

}

static int ufoe_clock_on(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
#ifdef ENABLE_CLK_MGR
#ifdef CONFIG_MTK_CLKMGR
	enable_clock(MT_CG_DISP0_DISP_UFOE, "ufoe");
#else
	ddp_clk_enable(DISP0_DISP_UFOE);
#endif
#endif
	DDPMSG("ufoe_clock on CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

static int ufoe_clock_off(enum DISP_MODULE_ENUM module, void *handle)
{
#if 0
#ifdef ENABLE_CLK_MGR
#ifdef CONFIG_MTK_CLKMGR
	disable_clock(MT_CG_DISP0_DISP_UFOE, "ufoe");
#else
	ddp_clk_disable(DISP0_DISP_UFOE);
#endif
#endif
	DDPMSG("ufoe_clock off CG 0x%x\n", DISP_REG_GET(DISP_REG_CONFIG_MMSYS_CG_CON0));
#endif
	return 0;
}

static int ufoe_reset(enum DISP_MODULE_ENUM module, void *handle)
{
	DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_SW_RST_ENGINE, DISP_REG_UFO_START, 1);
	DISP_REG_SET_FIELD(handle, START_FLD_DISP_UFO_SW_RST_ENGINE, DISP_REG_UFO_START, 0);
	DDPMSG("ufoe reset done\n");
	return 0;
}

static int _ufoe_partial_update(enum DISP_MODULE_ENUM module, void *arg, void *handle)
{
	struct disp_rect *roi = (struct disp_rect *)arg;
	int width = roi->width;
	int height = roi->height;

	if (ufoe_enable) {
		if (lr_mode_en == 0) {
			if (compress_ratio == 3) {
				unsigned int internal_width = width + width % 4;

				if (internal_width % 6 != 0) {
					DISP_REG_SET_FIELD(handle,
							CR0P6_PAD_FLD_DISP_UFO_STR_PAD_NUM,
							DISP_REG_UFO_CR0P6_PAD,
							(((internal_width / 6 + 1) * 6) - internal_width));
				}
			}
		}
		DISP_REG_SET(handle, DISP_REG_UFO_FRAME_WIDTH, width);
		DISP_REG_SET(handle, DISP_REG_UFO_FRAME_HEIGHT, height);
	}
	return 0;
}

int ufoe_ioctl(enum DISP_MODULE_ENUM module, void *cmdq_handle,
		unsigned int ioctl_cmd, unsigned long *params)
{
	int ret = -1;
	enum DDP_IOCTL_NAME ioctl = (enum DDP_IOCTL_NAME) ioctl_cmd;

	switch (ioctl) {
	case DDP_PARTIAL_UPDATE:
		_ufoe_partial_update(module, params, cmdq_handle);
		ret = 0;
		break;
	default:
		break;
	}
	return ret;
}

/* ufoe */
struct DDP_MODULE_DRIVER ddp_driver_ufoe = {
	.init = ufoe_init,
	.deinit = ufoe_deinit,
	.config = ufoe_config,
	.start = (int (*)(enum DISP_MODULE_ENUM, void *))ufoe_start,
	.trigger = NULL,
	.stop = ufoe_stop,
	.reset = ufoe_reset,
	.power_on = ufoe_clock_on,
	.power_off = ufoe_clock_off,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = (int (*)(enum DISP_MODULE_ENUM, int))ufoe_dump,
	.bypass = NULL,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
	.ioctl = (int (*)(enum DISP_MODULE_ENUM, void *, enum DDP_IOCTL_NAME, void *))ufoe_ioctl,
};
