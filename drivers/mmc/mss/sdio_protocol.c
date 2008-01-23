/*
 * sdio_protocol.c - SDIO protocol driver
 *
 * Copyright (C) 2007 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 only 
 * for now as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * derived from previous mmc code in Linux kernel 
 * Copyright (c) 2002 Hewlett-Packard Company
 * Copyright (c) 2002 Andrew Christian 
 * Copyright (c) 2006 Bridge Wu
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/sysctl.h>
#include <linux/suspend.h>
#include <linux/mm.h>
#include <asm/scatterlist.h>

#include <linux/mmc/mss_core.h>
#include <linux/mmc/sdio_protocol.h>

/*****************************************************************************
 *
 *   internal functions
 *
 ****************************************************************************/

#define KBPS 1
#define MBPS 1000

static u32 ts_exp[] = { 100*KBPS, 1*MBPS, 10*MBPS, 100*MBPS, 0, 0, 0, 0 };
static u32 ts_mul[] = { 0,    1000, 1200, 1300, 1500, 2000, 2500, 3000, 
			3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

static u32 sdio_tran_speed( u8 ts )
{
	u32 clock = ts_exp[(ts & 0x7)] * ts_mul[(ts & 0x78) >> 3];

	dbg5("clock :%d", clock);
	return clock;
}

static int sdio_unpack_r1(struct mss_cmd *cmd, struct sdio_response_r1 *r1, struct sdio_card *sdio_card)
{
	u8 *buf = cmd->response;

	//debug(" result in r1: %d\n", request->result);
	//if ( request->result )  return request->result;

	sdio_card->errno = SDIO_ERROR_NONE;
	r1->cmd    = unstuff_bits(buf, 40, 8, 6);
	r1->status = unstuff_bits(buf, 8, 32, 6);

	dbg5("status 0x%x", r1->status);
	if (R1_STATUS(r1->status)) {
		if (r1->status & R1_OUT_OF_RANGE)
			sdio_card->errno = SDIO_ERROR_OUT_OF_RANGE;
		if (r1->status & R1_COM_CRC_ERROR)
		  	sdio_card->errno = SDIO_ERROR_COM_CRC;
		if (r1->status & R1_ILLEGAL_COMMAND)
		    	sdio_card->errno = SDIO_ERROR_ILLEGAL_COMMAND;
		if (r1->status & R1_ERROR)
	  		sdio_card->errno = SDIO_ERROR_GENERAL;
	}

	if (r1->cmd != cmd->opcode)
	       	sdio_card->errno = SDIO_ERROR_HEADER_MISMATCH;
	dbg5("command:0x%x", r1->cmd);
	/* This should be last - it's the least dangerous error */	
	if (sdio_card->errno != SDIO_ERROR_NONE)
		return MSS_ERROR_RESP_UNPACK;
	return MSS_ERROR_NONE;
}


static int sdio_unpack_r4(struct mss_cmd *cmd, struct sdio_response_r4 *r4, struct sdio_card *sdio_card)
{
	u8 *buf = cmd->response;

	r4->ocr		= unstuff_bits(buf, 8, 24, 6);
	r4->ready	= unstuff_bits(buf, 39, 1, 6);
	r4->mem_present	= unstuff_bits(buf, 35, 1, 6);
	r4->func_num	= unstuff_bits(buf, 36, 3, 6);
	dbg5("ocr=%08x,ready=%d,mp=%d,fun_num:%d\n", r4->ocr, r4->ready,
r4->mem_present, r4->func_num);
	return 0;
}


static int sdio_unpack_r5(struct mss_cmd *cmd, struct sdio_response_r5 *r5, struct sdio_card *sdio_card)
{
	u8 *buf = cmd->response;

	sdio_card->errno = SDIO_ERROR_NONE;
	r5->cmd		= unstuff_bits(buf, 40, 8, 6);
	r5->status	= unstuff_bits(buf, 16, 8, 6);
	r5->data	= unstuff_bits(buf, 8, 8, 6);
	dbg5("cmd=%d status=%02x,data:%02x", r5->cmd, r5->status, r5->data);

	if (r5->status) {
		if (r5->status & R5_OUT_OF_RANGE)
			return SDIO_ERROR_OUT_OF_RANGE;
		if (r5->status & R5_COM_CRC_ERROR)
			return SDIO_ERROR_COM_CRC;
		if (r5->status & R5_ILLEGAL_COMMAND)
			return SDIO_ERROR_ILLEGAL_COMMAND;
		if (r5->status & R5_ERROR)
			return SDIO_ERROR_GENERAL;
		if (r5->status & R5_FUNCTION_NUMBER)
			return SDIO_ERROR_FUNC_NUM;
	}

	if (r5->cmd != cmd->opcode) {
		return SDIO_ERROR_HEADER_MISMATCH;
	}

	return 0;
}

static u16 sdio_unpack_r6(struct mss_cmd *cmd, struct sdio_response_r6 *r6, struct sdio_card *sdio_card)
{
	u8 *buf = cmd->response;
	int errno = SDIO_ERROR_NONE;
	
	r6->cmd = unstuff_bits(buf, 40, 8, 6);
	r6->rca = unstuff_bits(buf, 24, 16, 6);
	r6->status = unstuff_bits(buf, 8, 16, 6);
	if (R6_STATUS(r6->status)) {
		if (r6->status & R6_COM_CRC_ERROR) 
			errno = SDIO_ERROR_COM_CRC;
		if (r6->status & R6_ILLEGAL_COMMAND) 
			errno = SDIO_ERROR_ILLEGAL_COMMAND;
		if (r6->status & R6_ERROR) 
			errno = SDIO_ERROR_GENERAL;
	}
	if (r6->cmd != cmd->opcode) 
		errno = SDIO_ERROR_HEADER_MISMATCH;
	sdio_card->errno = errno;
	if (errno)
		return MSS_ERROR_RESP_UNPACK;
	return 0 ;
	
}


/*****************************************************************************
 *
 *   internal functions
 *
 ****************************************************************************/

/* sdio related function */
static u32 sdio_make_cmd52_arg(int rw, int fun_num, int raw, int address, int write_data)
{
	u32 ret;
	dbg5("rw:%d,fun:%d,raw:%d,address:%d,write_data:%d\n",
		rw, fun_num, raw, address, write_data);
	ret = (rw << 31) | (fun_num << 28) | (raw << 27) | (address << 9) | write_data;
	return ret;
}

static u32 sdio_make_cmd53_arg(int rw, int fun_num, int block_mode, int op_code, int address, int count)
{
	u32 ret;
	dbg5("rw:%d,fun:%d,block_mode:%d,op_code:%d,address:%d,count:%d\n",
		rw, fun_num, block_mode, op_code, address, count);
	ret = (rw << 31) | (fun_num << 28) | (block_mode << 27) | (op_code << 26) | (address << 9) | count;
	return ret;
}

#define SDIO_FN0_READ_REG(addr)						\
	do {								\
		arg = sdio_make_cmd52_arg(SDIO_R, 0, 0, addr, 0);	\
		ret = mss_send_simple_ll_req(host, llreq, cmd, 		\
				IO_RW_DIRECT, arg, MSS_RESPONSE_R5, 	\
				MSS_CMD_SDIO_EN);			\
		if (ret) {						\
			dbg5("CMD ERROR: ret = %d\n", ret);		\
			return ret;					\
		}							\
		ret = sdio_unpack_r5(cmd, &r5, sdio_card);		\
		if (ret) {						\
			dbg5("UNPACK ERROR: ret = %d\n", ret);		\
			return ret;					\
		}							\
	} while(0)
	
#define SDIO_FN0_WRITE_REG(addr, val)					\
	do {								\
		arg = sdio_make_cmd52_arg(SDIO_W, 0, 0, addr, val);	\
		ret = mss_send_simple_ll_req(host, llreq, cmd, 		\
				IO_RW_DIRECT, arg, MSS_RESPONSE_R5, 	\
				MSS_CMD_SDIO_EN);			\
		if (ret) {						\
			dbg5("CMD ERROR: ret = %d\n", ret);		\
			return ret;					\
		}							\
		ret = sdio_unpack_r5(cmd, &r5, sdio_card);		\
		if (ret) {						\
			dbg5("UNPACK ERROR: ret = %d\n", ret);		\
			return ret;					\
		}							\
	} while(0)
	

static int sdio_set_bus_width(struct mss_card *card, u8 buswidth)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret,arg;

	SDIO_FN0_WRITE_REG(BUS_IF_CTRL, buswidth);
	card->bus_width = MSS_BUSWIDTH_4BIT;

	return 0;
}

static int sdio_set_block_size(struct mss_card *card, u16 size, int func)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret,arg;
	u8 tmp;

	if (func == 0) {
		tmp = size & 0xff;
		SDIO_FN0_WRITE_REG(FN0_BLKSZ_1, tmp);
		
		tmp = (size & 0xff00) >> 8;
		SDIO_FN0_WRITE_REG(FN0_BLKSZ_2, tmp);
		
		sdio_card->cccr.fn0_blksz = size;
	} else {
		tmp = size & 0xff;
		SDIO_FN0_WRITE_REG(FNn_BLKSZ_1(func), tmp);
		
		tmp = (size & 0xff00) >> 8;
		SDIO_FN0_WRITE_REG(FNn_BLKSZ_2(func), tmp);
		
		sdio_card->fbr[func].fn_blksz = size;
	}	

	return 0;
}

static int sdio_io_enable(struct mss_card *card, u8 set_mask)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret, arg;

	SDIO_FN0_WRITE_REG(IO_ENABLE, set_mask);
	
	return 0;
}

static int sdio_interrupt_enable(struct mss_card *card, u8 set_mask)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret, arg;

	SDIO_FN0_WRITE_REG(INT_ENABLE, set_mask);

	return 0;
}

static int sdio_csa_enable(struct mss_card *card, int func)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int arg;
	int ret = 0;

	if(sdio_card->fbr[func].fun_support_csa == 0)
		return ret;
	
	SDIO_FN0_WRITE_REG(FNn_IF_CODE(func), 0x80);

	return 0;
}


static int get_sdio_fbr_info(struct mss_card *card, int func)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret,arg;
	u32 tmp;
	
	dbg5("get_sdio_fbr_info");
	SDIO_FN0_READ_REG(FNn_IF_CODE(func));
	sdio_card->fbr[func].fun_if_code = r5.data & 0xF;
	sdio_card->fbr[func].fun_support_csa = (r5.data >> 6) & 0x1;
	sdio_card->fbr[func].fun_csa_enable	= (r5.data >> 7) & 0x1;

	SDIO_FN0_READ_REG(FNn_EXT_IF_CODE(func));
	sdio_card->fbr[func].fun_ext_if_code = r5.data;

	SDIO_FN0_READ_REG(FNn_CIS_POINTER_1(func));
	tmp = r5.data;
	SDIO_FN0_READ_REG(FNn_CIS_POINTER_2(func));
	tmp |= r5.data << 8;
	SDIO_FN0_READ_REG(FNn_CIS_POINTER_3(func));
	tmp |= r5.data << 16;
	sdio_card->fbr[func].pfcis = tmp;

	SDIO_FN0_READ_REG(FNn_CSA_POINTER_1(func));
	tmp = r5.data;
	SDIO_FN0_READ_REG(FNn_CSA_POINTER_2(func));
	tmp |= r5.data << 8;
	SDIO_FN0_READ_REG(FNn_CSA_POINTER_3(func));
	tmp |= r5.data << 16;
	sdio_card->fbr[func].pcsa = tmp;

	SDIO_FN0_READ_REG(FNn_BLKSZ_1(func));
	tmp = r5.data;
	SDIO_FN0_READ_REG(FNn_BLKSZ_2(func));
	tmp |= r5.data << 8;
	sdio_card->fbr[func].fn_blksz = tmp;

	dbg5("func:%d, csa:0x%x, cis:0x%x, blksz:0x%x", func, sdio_card->fbr[func].pcsa, sdio_card->fbr[func].pfcis, sdio_card->fbr[func].fn_blksz);

	return 0;
}

static int get_sdio_cccr_info(struct mss_card *card)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	int ret, arg;
	u32 tmp;
	
	dbg5("get_sdio_cccr_info");
	SDIO_FN0_READ_REG(CCCR_SDIO_REVISION);
	sdio_card->cccr.sdiox = (r5.data >> 4) & 0xf;
	sdio_card->cccr.cccrx = r5.data & 0xf;
	
	SDIO_FN0_READ_REG(SD_SPEC_REVISION);
	sdio_card->cccr.sdx = r5.data & 0xf;
	
	SDIO_FN0_READ_REG(IO_ENABLE);
	sdio_card->cccr.ioex = r5.data;
	
	SDIO_FN0_READ_REG(IO_READY);
	sdio_card->cccr.iorx = r5.data;
	
	SDIO_FN0_READ_REG(INT_ENABLE);
	sdio_card->cccr.intex = r5.data;
	
	SDIO_FN0_READ_REG(INT_PENDING);
	sdio_card->cccr.intx = r5.data;
	
	SDIO_FN0_READ_REG(BUS_IF_CTRL);
	sdio_card->cccr.buswidth = r5.data & 0x3;
	sdio_card->cccr.cd = (r5.data >> 7) & 0x1;
	
	SDIO_FN0_READ_REG(CARD_CAPABILITY);
	sdio_card->cccr.sdc	= r5.data & 0x1;
	sdio_card->cccr.smb	= (r5.data >> 1) & 0x1;
	sdio_card->cccr.srw	= (r5.data >> 2) & 0x1;
	sdio_card->cccr.sbs	= (r5.data >> 3) & 0x1;
	sdio_card->cccr.s4mi	= (r5.data >> 4) & 0x1;
	sdio_card->cccr.e4mi	= (r5.data >> 5) & 0x1;
	sdio_card->cccr.lsc	= (r5.data >> 6) & 0x1;
	sdio_card->cccr.ls4b	= (r5.data >> 7) & 0x1;
	
	SDIO_FN0_READ_REG(COMMON_CIS_POINTER_1);
	tmp = r5.data;
	SDIO_FN0_READ_REG(COMMON_CIS_POINTER_2);
	tmp |= r5.data << 8;
	SDIO_FN0_READ_REG(COMMON_CIS_POINTER_3);
	tmp |= r5.data << 16;
	sdio_card->cccr.pccis	= tmp;
	
	SDIO_FN0_READ_REG(BUS_SUSPEND);
	sdio_card->cccr.bs	= r5.data & 0x1;
	sdio_card->cccr.br	= (r5.data >> 1) & 0x1;
	
	SDIO_FN0_READ_REG(FUNCTION_SELECT);
	sdio_card->cccr.fsx	= r5.data & 0xf;
	sdio_card->cccr.df	= (r5.data >> 7) & 0x1;

	SDIO_FN0_READ_REG(EXEC_FLAGS);
	sdio_card->cccr.exx	= r5.data;

	SDIO_FN0_READ_REG(READY_FLAGS);
	sdio_card->cccr.rfx	= r5.data;

	SDIO_FN0_READ_REG(FN0_BLKSZ_1);
	tmp = r5.data;
	SDIO_FN0_READ_REG(FN0_BLKSZ_2);
	tmp |= r5.data << 8;
	sdio_card->cccr.fn0_blksz = tmp;

	SDIO_FN0_READ_REG(POWER_CTRL);
	sdio_card->cccr.smpc	= r5.data & 0x1;
	sdio_card->cccr.empc	= (r5.data >> 1) & 0x1;

	dbg5("cccr, card capability: low_speed_card:%d"
		" low_speed_card_4bit:%d  int_bw_block:%d\n"
		" suspend/resume:%d read/wait:%d multiblcok:%d direct_cmd:%d\n",
		sdio_card->cccr.lsc, sdio_card->cccr.ls4b,
		sdio_card->cccr.s4mi, sdio_card->cccr.sbs, 
		sdio_card->cccr.srw, sdio_card->cccr.smb, sdio_card->cccr.sdc);
	dbg5("sdio:%d, sd:%d, cccr:%d, common cis:0x%x\n", sdio_card->cccr.sdiox, sdio_card->cccr.sdx, sdio_card->cccr.cccrx, sdio_card->cccr.pccis);
	dbg5("spmc:%d\n", sdio_card->cccr.smpc);
	dbg5("ioe:0x%x, ior:0x%x\n", sdio_card->cccr.ioex, sdio_card->cccr.iorx);
	return 0;
}

static int get_sdio_fcis_info(struct mss_card *card, int func)
{
	struct mss_host *host= card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	int ret, arg, tuple_body_len, tuple_type;
	struct sdio_response_r5 r5;
	u32 addr, next_addr;
	u32 tmp;
	u16 tmp16;

	dbg5("get_sdio_fcis_info");
	addr = sdio_card->fbr[func].pfcis;

	while(1) {
		SDIO_FN0_READ_REG(addr);
		tuple_type = r5.data;

		SDIO_FN0_READ_REG(addr + 1);
		tuple_body_len = r5.data;

		next_addr = addr + 2 + r5.data;
		switch (tuple_type) {
			case CISTPL_NULL:
			case CISTPL_END:
				dbg5("cistpl null/end\n");
				goto finish;
			case CISTPL_CHECKSUM:
				dbg5("cistpl checksum\n");
				break;
			case CISTPL_VERS_1:
				dbg5("cistpl vers level 1\n");
				break;
			case CISTPL_ALTSTR:
				dbg5("cistpl altstr\n");
				break;
			case CISTPL_MANFID:
				dbg5("cistpl manfid\n");
				break;
			case CISTPL_FUNCID:
				dbg5("cistpl funcid\n");
				SDIO_FN0_READ_REG(addr + 2);

				if (r5.data != 0x0c)
					dbg5("not a sdio card\n");
				else
					dbg5(" a sdio card\n");

				break;
			case CISTPL_FUNCE:
				/* Type of extended data, should be 1 */
				SDIO_FN0_READ_REG(addr + 2);
				if (r5.data == 0x01)
					dbg5("1 type extended tuple\n");

				/* FUNCTION_INFO */
				SDIO_FN0_READ_REG(addr + 3);
				sdio_card->fcis[func].func_info = r5.data;

				/* STD_IO_REV */
				SDIO_FN0_READ_REG(addr + 4);
				sdio_card->fcis[func].std_io_rev = r5.data;

				/* card psn */
				SDIO_FN0_READ_REG(addr + 5);
				tmp = r5.data;
				SDIO_FN0_READ_REG(addr + 6);
				tmp |= r5.data << 8;
				SDIO_FN0_READ_REG(addr + 7);
				tmp |= r5.data << 16;
				SDIO_FN0_READ_REG(addr + 8);
				tmp |= r5.data << 24;
				sdio_card->fcis[func].card_psn = tmp;

				/* card csa size */
				SDIO_FN0_READ_REG(addr + 9);
				tmp = r5.data;
				SDIO_FN0_READ_REG(addr + 10);
				tmp |= r5.data << 8;
				SDIO_FN0_READ_REG(addr + 11);
				tmp |= r5.data << 16;
				SDIO_FN0_READ_REG(addr + 12);
				tmp |= r5.data << 24;
				sdio_card->fcis[func].csa_size = tmp;

				/* csa property */
				SDIO_FN0_READ_REG(addr + 13);
				sdio_card->fcis[func].csa_property = r5.data;

				/* max_blk_size */
				SDIO_FN0_READ_REG(addr + 14);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 15);
				tmp16 |= r5.data << 8;
				sdio_card->fcis[func].max_blk_size = tmp16;

				/* ocr */	
				SDIO_FN0_READ_REG(addr + 16);
				tmp = r5.data;
				SDIO_FN0_READ_REG(addr + 17);
				tmp |= r5.data << 8;
				SDIO_FN0_READ_REG(addr + 18);
				tmp |= r5.data << 16;
				SDIO_FN0_READ_REG(addr + 19);
				tmp |= r5.data << 24;
				sdio_card->fcis[func].ocr = tmp;

				/* pwr */
				SDIO_FN0_READ_REG(addr + 20);
				sdio_card->fcis[func].op_min_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 21);
				sdio_card->fcis[func].op_avg_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 22);
				sdio_card->fcis[func].op_max_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 23);
				sdio_card->fcis[func].sb_min_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 24);
				sdio_card->fcis[func].sb_avg_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 25);
				sdio_card->fcis[func].sb_max_pwr = r5.data;

				SDIO_FN0_READ_REG(addr + 26);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 27);
				tmp16 |= r5.data << 8;
				sdio_card->fcis[func].min_bw = tmp16;

				SDIO_FN0_READ_REG(addr + 28);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 29);
				tmp16 |= r5.data << 8;
				sdio_card->fcis[func].opt_bw = tmp16;

				// SDIO1.0 is 28, and 1.1 is 36 
				if (sdio_card->cccr.sdiox == 0)
					break;
				SDIO_FN0_READ_REG(addr + 30);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 31);
				tmp16 |= r5.data << 8;
				sdio_card->fcis[func].enable_timeout_val= tmp16;
				break;
			case CISTPL_SDIO_STD:
				dbg5("sdio std\n");	
				break;
			case CISTPL_SDIO_EXT:
				dbg5("sdio std ext\n");	
				break;
			default :
				dbg5("not supported cis\n");
		}
		addr = next_addr;
	}
finish:
	dbg5("fcis %d\nfunction_info:0x%x std_io_rev:0x%x card_psn:0x%x\n" 
		"csa_size:0x%x csa_property:0x%x max_blk_size:0x%x\n"
		"ocr:0x%x op_min_pwr:0x%x op_avg_pwr:0x%x op_max_pwr:0x%x"
		"sb_min_pwr:0x%x sb_avg_pwr:0x%x sb_max_pwr:0x%x" 
		"min_bw:0x%x opt_bw:0x%x time out:%x\n",func, 
		sdio_card->fcis[func].func_info, sdio_card->fcis[func].std_io_rev, sdio_card->fcis[func].card_psn, 
		sdio_card->fcis[func].csa_size, sdio_card->fcis[func].csa_property, sdio_card->fcis[func].max_blk_size, 
		sdio_card->fcis[func].ocr, sdio_card->fcis[func].op_min_pwr, sdio_card->fcis[func].op_avg_pwr, sdio_card->fcis[func].op_max_pwr, 
		sdio_card->fcis[func].sb_min_pwr, sdio_card->fcis[func].sb_avg_pwr, sdio_card->fcis[func].sb_max_pwr, 
		sdio_card->fcis[func].min_bw, sdio_card->fcis[func].opt_bw, sdio_card->fcis[func].enable_timeout_val);
	return 0;
}

static int get_sdio_ccis_info(struct mss_card *card)
{
	struct mss_host *host= card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	int ret, arg, tuple_body_len, tuple_type;
	struct sdio_response_r5 r5;
	u32 addr, next_addr;
	u16 tmp16;

	dbg5("get_sdio_ccis_info");
	addr = sdio_card->cccr.pccis;
	while (1) {
		SDIO_FN0_READ_REG(addr);
		tuple_type = r5.data;

		SDIO_FN0_READ_REG(addr + 1);
		tuple_body_len = r5.data;
		next_addr = addr + 2 + r5.data;

		switch (tuple_type) {
			case CISTPL_NULL:
			case CISTPL_END:
				dbg5("cistpl null/end\n");
				goto finish;
			case CISTPL_CHECKSUM:
				dbg5("cistpl checksum\n");
				break;
			case CISTPL_VERS_1:
				dbg5("cistpl vers level 1\n");
				break;
			case CISTPL_ALTSTR:
				dbg5("cistpl altstr\n");
				break;
			case CISTPL_MANFID:
				dbg5("cistpl manfid\n");
				SDIO_FN0_READ_REG(addr + 2);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 3);
				tmp16 |= r5.data << 8;
				sdio_card->ccis.manufacturer = tmp16;

				SDIO_FN0_READ_REG(addr + 4);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 5);
				tmp16 |= r5.data << 8;
				sdio_card->ccis.card_id = tmp16;

				break;
			case CISTPL_FUNCID:
				dbg5("cistpl funcid\n");
				SDIO_FN0_READ_REG(addr + 2);
				if (r5.data != 0x0c)
					dbg5("not a sdio card\n");
				else
					dbg5(" a sdio card\n");

				break;
			case CISTPL_FUNCE:
				dbg5("cistpl funce\n");
				SDIO_FN0_READ_REG(addr + 2);
				if (r5.data == 0x00)
					dbg5("0 type extended tuple\n");

				SDIO_FN0_READ_REG(addr + 3);
				tmp16 = r5.data;
				SDIO_FN0_READ_REG(addr + 4);
				tmp16 = r5.data << 8;
				sdio_card->ccis.fn0_block_size = tmp16;

				SDIO_FN0_READ_REG(addr + 5);
				sdio_card->ccis.max_tran_speed = r5.data;
				//slot->tran_speed = card->ccis.max_tran_speed;
				break;
			case CISTPL_SDIO_STD:
				dbg5("sdio std\n");	
				break;
			case CISTPL_SDIO_EXT:
				dbg5("sdio std ext\n");	
				break;
			default:
				dbg5("not supported cis\n");
		}
		addr = next_addr;
	}	
finish:
	dbg5("ccis:\nmanf:0x%x card_id:0x%x block_size:0x%x\nmax_tran_speed:0x%x\n",sdio_card->ccis.manufacturer, sdio_card->ccis.card_id, sdio_card->ccis.fn0_block_size, sdio_card->ccis.max_tran_speed);
	return 0;
}

/*****************************************************************************
 *
 *   protocol entry functions
 *
 ****************************************************************************/

static int sdio_recognize_card(struct mss_card *card)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct mss_ios ios;
	struct sdio_response_r4 r4;
	int ret;

	card->card_type = MSS_UNKNOWN_CARD;
	card->state = CARD_STATE_INIT;
	card->bus_width = MSS_BUSWIDTH_1BIT;
	
	memcpy(&ios, &host->ios, sizeof(struct mss_ios)); 
	ios.clock = host->f_min;
	ios.bus_width = MSS_BUSWIDTH_1BIT;
	host->ops->set_ios(host, &ios);

	/* check if a sdio card, need not send CMD0 to reset for SDIO card */
	ret = mss_send_simple_ll_req(host, llreq, cmd, IO_SEND_OP_COND, 
			0, MSS_RESPONSE_R4, 0);
	if (ret)
		return ret;
	ret = sdio_unpack_r4(cmd, &r4, sdio_card);  
	if (ret)
		return ret;
	sdio_card->func_num = r4.func_num;
	sdio_card->mem_present = r4.mem_present;
	if (!r4.func_num)
		return MSS_ERROR_NONE;
	/*  maybe COMBO_CARD. but we can return SDIO_CARD first, 
	 *  in sdio_card_init, we will judge further.
	 */
	if(r4.ocr & host->vdd) {
		card->card_type = MSS_SDIO_CARD;
	} else {
		printk(KERN_WARNING "uncompatible card\n");
		card->card_type = MSS_UNCOMPATIBLE_CARD;
	}
	
	return MSS_ERROR_NONE;
}

static int sdio_card_init(struct mss_card *card)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r1 r1;
	struct sdio_response_r4 r4;
	struct sdio_response_r6 r6;
	struct mss_ios ios;
	int ret, tmp, i;
	u8 funcs;

	dbg5("card = %08X\n", (u32)card);
	card->state = CARD_STATE_INIT;
	card->bus_width = MSS_BUSWIDTH_1BIT;
	
	memcpy(&ios, &host->ios, sizeof(struct mss_ios)); 
	ios.clock = host->f_min;
	ios.bus_width = MSS_BUSWIDTH_1BIT;
	host->ops->set_ios(host, &ios);

	ret = mss_send_simple_ll_req(host, llreq, cmd, IO_SEND_OP_COND, 
			host->vdd, MSS_RESPONSE_R4, 0);
	if (ret)
		return ret;
	ret = sdio_unpack_r4(cmd, &r4, sdio_card);
	if (ret) 
		return ret;

	while (!r4.ready) {
		//mdelay(20);
		msleep(15);
		ret = mss_send_simple_ll_req(host, llreq, cmd, IO_SEND_OP_COND, 
				host->vdd, MSS_RESPONSE_R4, 0);
		if (ret)
			return ret;
		ret = sdio_unpack_r4(cmd, &r4, sdio_card);
		if (ret) 
			return ret;
	}
	
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SET_RELATIVE_ADDR, 
			0, MSS_RESPONSE_R6, 0);
	if (ret)
		return ret;
    	ret = sdio_unpack_r6(cmd, &r6, sdio_card);
	if (ret) 
		return ret;
	sdio_card->state = CARD_STATE_STBY;	
	sdio_card->rca = r6.rca;

	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SELECT_CARD, 
			(sdio_card->rca) << 16, MSS_RESPONSE_R1B, 0);
	if (ret)
		return ret;
	ret = sdio_unpack_r1(cmd, &r1, sdio_card);
	if (ret)
		return ret;

	/** CARD_STATE_CMD */
	//slot->state = CARD_STATE_CMD;
	//send CMD53 to let DATA BUS FREE, since bus_suspend not supported, need not to do this
	//arg = sdio_make_cmd53_arg(SDIO_READ, 0, 0, 0, 0, 1);
	//mss_simple_cmd( dev, IO_RW_EXTENDED, arg, RESPONSE_R5);


	/** CARD_STATE_TRAN */
	sdio_card->state = CARD_STATE_CMD;


	ret = get_sdio_cccr_info(card);
	if (ret)
		return ret;
	funcs = sdio_card->func_num;
	for(i = 1; i <= funcs; i++) {
		ret = get_sdio_fbr_info(card, i);
		if (ret)
			return ret;
	}

	ret = get_sdio_ccis_info(card);
	if (ret)
		return ret;
	for(i = 1; i <= funcs; i++) {
		ret = get_sdio_fcis_info(card, i);
		if (ret)
			return ret;
	}
	if(host->bus_width == MSS_BUSWIDTH_4BIT 
			&& (!sdio_card->cccr.lsc || sdio_card->cccr.ls4b)) {
		host->ios.bus_width = MSS_BUSWIDTH_4BIT;
		sdio_set_bus_width(card, 2);
	}

	/* enable function */
	tmp = 0;
	for(i = 1; i <= funcs; i++) {
		tmp |= (1 << i); 
	}
	ret = sdio_io_enable(card, tmp);
	if (ret)
		return ret;

	/* enable interrupt */
	tmp = 1; 
	for(i = 1; i <= funcs; i++) {
		tmp |= (1 << i); 
	}
	ret = sdio_interrupt_enable(card, tmp);
	if (ret)
		return ret;
	
	/* enable card capabilites */
	for (i=1; i <= funcs; i++) {
		sdio_csa_enable(card, i);
	}

	mss_set_sdio_int(card->slot->host, 1);

	return MSS_ERROR_NONE;
}

static int sdio_read_write_entry(struct mss_card *card, int action, struct mss_rw_arg *arg, struct mss_rw_result *result)
{
	struct mss_host *host = card->slot->host;
	struct sdio_card *sdio_card = card->prot_card;
	struct mss_cmd *cmd = &sdio_card->cmd;
	struct mss_data *data = &sdio_card->data;
	struct mss_ll_request *llreq = &sdio_card->llreq;
	struct sdio_response_r5 r5;
	struct sdio_response_r1 r1;
	int ret;
	u32 addr, blkmode, func, rw, rw_count, opcode, clock, cmdarg;
	int retries = 4;
	
	if (sdio_card->state == CARD_STATE_STBY) {
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SELECT_CARD,
				sdio_card->rca << 16, MSS_RESPONSE_R1B, 0);
		if (ret)
			return ret;
		ret = sdio_unpack_r1(cmd, &r1, sdio_card);
		if (ret)
			return ret;
	}

	mss_set_clock(host, sdio_tran_speed(sdio_card->ccis.max_tran_speed));
	func = arg->func;
	if (func > sdio_card->func_num)
		return MSS_ERROR_WRONG_ARG;
	
	if (arg->block_len == 0) {
		blkmode = 0;	
	}
	else {
		if (!sdio_card->cccr.smb)
			return MSS_ERROR_WRONG_ARG;
		dbg5("blkzs:%d, %d\n", arg->block_len, sdio_card->fbr[func].fn_blksz);
		if (arg->block_len != sdio_card->fbr[func].fn_blksz) {
			ret = sdio_set_block_size(card, arg->block_len, func);
			if (ret)
				return ret;
		}
		blkmode = 1;
	}
	
	rw = (action == MSS_READ_MEM) ? 0 : 1;
	addr = arg->block;
	opcode = (arg->opcode) ? 1 : 0;
	rw_count = arg->nob;
	
	memset(llreq, 0x0, sizeof(struct mss_ll_request));
	memset(cmd, 0x0, sizeof(struct mss_cmd));
	memset(data, 0x0, sizeof(struct mss_data));

read_write_entry:
	/* deal with request */
	/* if only one byte, then use CMD52 to read*/
	if (!blkmode && rw_count == 1) {
		u8 val = (rw) ? arg->val : 0;
		dbg5("use CMD52 to transfer data. rw direction: %d", rw);
		cmdarg = sdio_make_cmd52_arg(rw, func, opcode, addr, val);

		dbg5("cmdarg :0x%x\n", cmdarg);
		ret = mss_send_simple_ll_req(host, llreq, cmd, IO_RW_DIRECT, 
					cmdarg, MSS_RESPONSE_R5,
				       	MSS_CMD_SDIO_EN);
		if (!ret)
			ret = sdio_unpack_r5(cmd, &r5, sdio_card);
		if (!ret)
			result->bytes_xfered = r5.data;
		else if (ret && --retries) {
			clock = host->ios.clock;
			clock = clock >> 1;
			if (clock < SDIO_CARD_CLOCK_SLOW && retries == 1) {
				clock = SDIO_CARD_CLOCK_SLOW;
				mss_set_clock(host, clock);
				goto read_write_entry;
			}
			return ret;
		}
		dbg5("r5.data:0x%x\n",r5.data);
	}
	else {
		cmdarg= sdio_make_cmd53_arg(rw, func, blkmode, opcode, addr, 
				rw_count);
		MSS_INIT_CMD(cmd, IO_RW_EXTENDED, cmdarg, MSS_CMD_SDIO_EN, 
				MSS_RESPONSE_R5);
		MSS_INIT_DATA(data, rw_count, arg->block_len,  
				((rw) ? MSS_DATA_WRITE : MSS_DATA_READ), 
				arg->sg_len, arg->sg, 0);
		llreq->cmd = cmd;
		llreq->data = data;
		
		ret = mss_send_ll_req(host, llreq);
		if (!ret)
			ret = sdio_unpack_r5(cmd, &r5, sdio_card);
		if (ret) {
			if (--retries) {
				clock = host->ios.clock;
				clock = clock >> 1;
				if (clock < SDIO_CARD_CLOCK_SLOW 
						&& retries == 1)
					clock = SDIO_CARD_CLOCK_SLOW;
				mss_set_clock(host, clock);
				goto read_write_entry;
			}
			return ret;
		}
	
	}
	return MSS_ERROR_NONE;
}

/*****************************************************************************
 *
 *   protocol driver interface functions
 *
 ****************************************************************************/

static int sdio_prot_entry(struct mss_card *card, unsigned int action, void *arg, void *result)
{
	int ret;
	
	dbg5("action = %d\n", action);
	if (action != MSS_RECOGNIZE_CARD && card->card_type != MSS_SDIO_CARD)
		return MSS_ERROR_WRONG_CARD_TYPE;
	switch (action) {
		case MSS_RECOGNIZE_CARD:
			ret = sdio_recognize_card(card);
			break;
		case MSS_INIT_CARD:
			ret = sdio_card_init(card);
			break;
		case MSS_READ_MEM:
		case MSS_WRITE_MEM:
			if (!arg || !result)
				return -EINVAL;
			ret = sdio_read_write_entry(card, action, arg, result);
			break;
		case MSS_QUERY_CARD:
			ret = MSS_ERROR_NONE;
			break;
		default:
			ret = MSS_ERROR_ACTION_UNSUPPORTED;
			break;
	}
	
	return ret;
}

static int sdio_prot_attach_card(struct mss_card *card)
{
	struct sdio_card *sdio_card;

#define ALIGN32(x)	(((x) + 31) & (~31))
	sdio_card = kzalloc(ALIGN32(sizeof(struct sdio_card)), GFP_KERNEL);
	card->prot_card = sdio_card;
	if (sdio_card) {
		return 0;
	}
	return -ENOMEM;
}

static void sdio_prot_detach_card(struct mss_card *card)
{
	kfree(card->prot_card);
}

static int sdio_prot_get_errno(struct mss_card *card)
{
	struct sdio_card *sdio_card = card->prot_card;
	
	return sdio_card->errno;
}

static struct mss_prot_driver sdio_protocol = {
	.name			=	SDIO_PROTOCOL,
	.prot_entry		=	sdio_prot_entry,
	.attach_card		=	sdio_prot_attach_card,
	.detach_card		=	sdio_prot_detach_card,
	.get_errno		=	sdio_prot_get_errno,
};

/*****************************************************************************
 *
 *   module init and exit functions
 *
 ****************************************************************************/
static int sdio_protocol_init(void)
{
	register_mss_prot_driver(&sdio_protocol);
	return 0;
}

static void sdio_protocol_exit(void)
{
	unregister_mss_prot_driver(&sdio_protocol);
}


module_init(sdio_protocol_init);
module_exit(sdio_protocol_exit);

MODULE_AUTHOR("Bridge Wu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SDIO protocol driver");
