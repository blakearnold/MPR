/*
 * sd_protocol.c - SD protocol driver
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
#include <linux/mm.h>
#include <asm/scatterlist.h>

#include <linux/mmc/mss_core.h>
#include <linux/mmc/sd_protocol.h>

#define KBPS 1
#define MBPS 1000

static u32 ts_exp[] = { 100*KBPS, 1*MBPS, 10*MBPS, 100*MBPS, 0, 0, 0, 0 };
static u32 ts_mul[] = { 0,    1000, 1200, 1300, 1500, 2000, 2500, 3000, 
			3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

static u32 sd_tran_speed(u8 ts)
{
	u32 clock = ts_exp[(ts & 0x7)] * ts_mul[(ts & 0x78) >> 3];

	dbg5("clock :%d", clock);
	return clock;
}


static int sd_unpack_r1(struct mss_cmd *cmd, struct sd_response_r1 *r1, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;

	//debug(" result in r1: %d\n", request->result);
	//if ( request->result )  return request->result;

	sd_card->errno = SD_ERROR_NONE;
	r1->cmd    = unstuff_bits(buf, 40, 8, 6);
	r1->status = unstuff_bits(buf, 8, 32, 6);

	dbg5("status 0x%x", r1->status);
	if (R1_STATUS(r1->status)) {
		if (r1->status & R1_OUT_OF_RANGE)
			sd_card->errno = SD_ERROR_OUT_OF_RANGE;
		if (r1->status & R1_ADDRESS_ERROR)
		  	sd_card->errno = SD_ERROR_ADDRESS;
		if (r1->status & R1_BLOCK_LEN_ERROR)
		    	sd_card->errno = SD_ERROR_BLOCK_LEN;
		if (r1->status & R1_ERASE_SEQ_ERROR)
		    	sd_card->errno = SD_ERROR_ERASE_SEQ;
		if (r1->status & R1_ERASE_PARAM)
			sd_card->errno = SD_ERROR_ERASE_PARAM;
		if (r1->status & R1_WP_VIOLATION)
		 	sd_card->errno = SD_ERROR_WP_VIOLATION;
		if (r1->status & R1_LOCK_UNLOCK_FAILED)
			sd_card->errno = SD_ERROR_LOCK_UNLOCK_FAILED;
		if (r1->status & R1_COM_CRC_ERROR)
		  	sd_card->errno = SD_ERROR_COM_CRC;
		if (r1->status & R1_ILLEGAL_COMMAND)
		    	sd_card->errno = SD_ERROR_ILLEGAL_COMMAND;
		if (r1->status & R1_CARD_ECC_FAILED)
		    	sd_card->errno = SD_ERROR_CARD_ECC_FAILED;
		if (r1->status & R1_CC_ERROR)
	     		sd_card->errno = SD_ERROR_CC;
		if (r1->status & R1_ERROR)
	  		sd_card->errno = SD_ERROR_GENERAL;
		if (r1->status & R1_CID_CSD_OVERWRITE)
		      	sd_card->errno = SD_ERROR_CID_CSD_OVERWRITE;
	}
	if (r1->status & R1_AKE_SEQ_ERROR)
	      	sd_card->errno = SD_ERROR_CID_CSD_OVERWRITE;

	if (r1->cmd != cmd->opcode)
	       	sd_card->errno = SD_ERROR_HEADER_MISMATCH;
	dbg5("command:0x%x", r1->cmd);
	/* This should be last - it's the least dangerous error */
	if (R1_CURRENT_STATE(r1->status) != sd_card->state ) {
		dbg5("state dismatch:r1->status:%x,state:%x\n",R1_CURRENT_STATE(r1->status),sd_card->state);
		sd_card->errno = SD_ERROR_STATE_MISMATCH;
	}
	dbg5("sd card error %d", sd_card->errno);
	if (sd_card->errno)
		return MSS_ERROR_RESP_UNPACK;
	return 0;
}

static int sd_unpack_r3(struct mss_cmd *cmd, struct sd_response_r3 *r3, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;

	sd_card->errno = SD_ERROR_NONE;
	r3->cmd = unstuff_bits(buf, 40, 8, 6);
	r3->ocr = unstuff_bits(buf, 8, 32, 6);
	dbg5("ocr=0x%x", r3->ocr);

	if (r3->cmd != 0x3f) {  
		sd_card->errno = SD_ERROR_HEADER_MISMATCH;
		return MSS_ERROR_RESP_UNPACK;
	}
	return 0;
}
	
static int sd_unpack_r6(struct mss_cmd *cmd, struct sd_response_r6 *r6, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;
	int errno = SD_ERROR_NONE;
	
	r6->cmd = unstuff_bits(buf, 40, 8, 6);
	r6->rca = unstuff_bits(buf, 24, 16, 6);
	r6->status = unstuff_bits(buf, 8, 16, 6);
	if (R6_STATUS(r6->status)) {
		if (r6->status & R6_COM_CRC_ERROR) 
			errno = SD_ERROR_COM_CRC;
		if (r6->status & R6_ILLEGAL_COMMAND) 
			errno = SD_ERROR_ILLEGAL_COMMAND;
		if (r6->status & R6_ERROR) 
			errno = SD_ERROR_CC;
	}
	if (r6->cmd != cmd->opcode) 
		errno = SD_ERROR_HEADER_MISMATCH;
	/* This should be last - it's the least dangerous error */
	if (R1_CURRENT_STATE(r6->status) != sd_card->state) 
		errno = SD_ERROR_STATE_MISMATCH;
	sd_card->errno = errno;
	if (errno)
		return MSS_ERROR_RESP_UNPACK;
	return 0 ;
}

static int sd_unpack_cid(struct mss_cmd *cmd, struct sd_cid *cid, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;

	sd_card->errno = SD_ERROR_NONE;
	if (buf[0] != 0x3f) {
		sd_card->errno = SD_ERROR_HEADER_MISMATCH;
		return MSS_ERROR_RESP_UNPACK;
	}
	buf = buf + 1;
	
	cid->mid	= unstuff_bits(buf, 120, 8, 16);
	cid->oid	= unstuff_bits(buf, 104, 16, 16);
	cid->pnm[0]	= unstuff_bits(buf, 96, 8, 16);
	cid->pnm[1]	= unstuff_bits(buf, 88, 8, 16);
	cid->pnm[2]	= unstuff_bits(buf, 80, 8, 16);
	cid->pnm[3]	= unstuff_bits(buf, 72, 8, 16);
	cid->pnm[4]	= unstuff_bits(buf, 64, 8, 16);
	cid->pnm[5]	= 0;
	cid->prv	= unstuff_bits(buf, 56, 8, 16);
	cid->psn	= unstuff_bits(buf, 24, 32, 16);
	cid->mdt	= unstuff_bits(buf, 8, 12, 16);
/*	
	DEBUG(" mid=%d oid=%d pnm=%s prv=%d.%d psn=%08x mdt=%d/%d\n",
	      cid->mid, cid->oid, cid->pnm, 
	      (cid->prv>>4), (cid->prv&0xf), 
	      cid->psn, cid->mdt&&0xf, ((cid->mdt>>4)&0xff)+2000);
*/

      	return 0;
}

static int sd_unpack_csd(struct mss_cmd *cmd, struct sd_csd *csd, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;

	sd_card->errno = SD_ERROR_NONE;
	if (buf[0] != 0x3f) {
		sd_card->errno = SD_ERROR_HEADER_MISMATCH;
		return MSS_ERROR_RESP_UNPACK;
	}
	buf = buf + 1;
	
	csd->csd_structure	= unstuff_bits(buf, 126, 2, 16);
	csd->taac		= unstuff_bits(buf, 112, 8, 16);
	csd->nsac		= unstuff_bits(buf, 104, 8, 16);
	csd->tran_speed		= unstuff_bits(buf, 96, 8, 16);
	csd->ccc		= unstuff_bits(buf, 84, 12, 16);
	csd->read_bl_len	= unstuff_bits(buf, 80, 4, 16);
	csd->read_bl_partial	= unstuff_bits(buf, 79, 1, 16);
	csd->write_blk_misalign	= unstuff_bits(buf, 78, 1, 16);
	csd->read_blk_misalign	= unstuff_bits(buf, 77, 1, 16);
	csd->dsr_imp		= unstuff_bits(buf, 76, 1, 16);
	if (csd->csd_structure == 0) {
		csd->csd.csd1.c_size		= unstuff_bits(buf, 62, 12, 16);
		csd->csd.csd1.vdd_r_curr_min	= unstuff_bits(buf, 59, 3, 16);
		csd->csd.csd1.vdd_r_curr_max	= unstuff_bits(buf, 56, 3, 16);
		csd->csd.csd1.vdd_w_curr_min	= unstuff_bits(buf, 53, 3, 16);
		csd->csd.csd1.vdd_w_curr_max	= unstuff_bits(buf, 50, 3, 16);
		csd->csd.csd1.c_size_mult	= unstuff_bits(buf, 47, 3, 16);
	}
	else if (csd->csd_structure == 1) {
		csd->csd.csd2.c_size		= unstuff_bits(buf, 48, 22, 16);
	}	
	csd->erase_blk_en	= unstuff_bits(buf, 46, 1, 16);
	csd->sector_size	= unstuff_bits(buf, 39, 7, 16);
	csd->wp_grp_size        = unstuff_bits(buf, 32, 7, 16);
	csd->wp_grp_enable      = unstuff_bits(buf, 31, 1, 16);
	csd->r2w_factor         = unstuff_bits(buf, 26, 3, 16);
	csd->write_bl_len       = unstuff_bits(buf, 22, 4, 16);
	csd->write_bl_partial   = unstuff_bits(buf, 21, 1, 16);
	csd->file_format_grp    = unstuff_bits(buf, 15, 1, 16);
	csd->copy               = unstuff_bits(buf, 14, 1, 16);
	csd->perm_write_protect = unstuff_bits(buf, 13, 1, 16);
	csd->tmp_write_protect  = unstuff_bits(buf, 12, 1, 16);
	csd->file_format        = unstuff_bits(buf, 10, 2, 16);

	if (csd->csd_structure == 0) {
	dbg5("  csd_structure=%d taac=%02x  nsac=%02x  tran_speed=%02x\n"
	      "  ccc=%04x  read_bl_len=%d  read_bl_partial=%d  write_blk_misalign=%d\n"
	      "  read_blk_misalign=%d  dsr_imp=%d  c_size=%d  vdd_r_curr_min=%d\n"
	      "  vdd_r_curr_max=%d  vdd_w_curr_min=%d  vdd_w_curr_max=%d  c_size_mult=%d\n"
	      "  erase_blk_en=%d sector_size=%d wp_grp_size=%d  wp_grp_enable=%d r2w_factor=%d\n"
	      "  write_bl_len=%d  write_bl_partial=%d  file_format_grp=%d  copy=%d\n"
	      "  perm_write_protect=%d  tmp_write_protect=%d  file_format=%d\n",
	      csd->csd_structure,csd->taac, csd->nsac, csd->tran_speed,
	      csd->ccc, csd->read_bl_len, 
	      csd->read_bl_partial, csd->write_blk_misalign,
	      csd->read_blk_misalign, csd->dsr_imp, 
	      csd->csd.csd1.c_size, csd->csd.csd1.vdd_r_curr_min,
	      csd->csd.csd1.vdd_r_curr_max, csd->csd.csd1.vdd_w_curr_min, 
	      csd->csd.csd1.vdd_w_curr_max, csd->csd.csd1.c_size_mult,
	      csd->erase_blk_en,csd->sector_size,
	      csd->wp_grp_size, csd->wp_grp_enable,
	      csd->r2w_factor, 
	      csd->write_bl_len, csd->write_bl_partial,
	      csd->file_format_grp, csd->copy, 
	      csd->perm_write_protect, csd->tmp_write_protect,
      	csd->file_format);
	}
	else if (csd->csd_structure == 1) {
      	      dbg5("  csd_structure=%d taac=%02x  nsac=%02x  tran_speed=%02x\n"
	      "  ccc=%04x  read_bl_len=%d  read_bl_partial=%d  write_blk_misalign=%d\n"
	      "  read_blk_misalign=%d  dsr_imp=%d  c_size=%d\n"
	      "  erase_blk_en=%d sector_size=%d wp_grp_size=%d  wp_grp_enable=%d r2w_factor=%d\n"
	      "  write_bl_len=%d  write_bl_partial=%d  file_format_grp=%d  copy=%d\n"
	      "  perm_write_protect=%d  tmp_write_protect=%d  file_format=%d\n",
	      csd->csd_structure,csd->taac, csd->nsac, csd->tran_speed,
	      csd->ccc, csd->read_bl_len, 
	      csd->read_bl_partial, csd->write_blk_misalign,
	      csd->read_blk_misalign, csd->dsr_imp, 
	      csd->csd.csd2.c_size,
	      csd->erase_blk_en,csd->sector_size,
	      csd->wp_grp_size, csd->wp_grp_enable,
	      csd->r2w_factor, 
	      csd->write_bl_len, csd->write_bl_partial,
	      csd->file_format_grp, csd->copy, 
	      csd->perm_write_protect, csd->tmp_write_protect,
	      csd->file_format);
	}

	return 0;
}

static int sd_unpack_swfuncstatus(char *buf, struct sw_func_status *status)
{
	int i; 

	buf += 34;
	for (i = 0; i < 5; i++) {
		status->func_busy[i] = buf[0] << 8 | buf[1];
		buf += 2;
	}
	buf += 1;
	for (i = 0; i <= 5; i = i + 2) {
		status->group_status[i] = buf[0] & 0xFF;
		status->group_status[(i + 1)] = (buf[0] >> 4) & 0xFF;
		buf += 1;
	}

	for (i = 0; i <= 5; i++) {
		status->func_support[i] = buf[0] << 8 | buf[1];
		buf += 2;
	}
	
	status->current_consumption = buf[0] << 8 | buf[1]; 

	return 0;
}

static int sd_unpack_r7(struct mss_cmd *cmd, struct sd_response_r7 *r7, u16 arg, struct sd_card *sd_card)
{
	u8 *buf = cmd->response;
	
	r7->cmd		= unstuff_bits(buf, 40, 8, 6);
	r7->ver		= unstuff_bits(buf, 20, 20, 6);
	r7->vca		= unstuff_bits(buf, 16, 4, 6);
	r7->pattern	= unstuff_bits(buf, 8, 8, 6);
	
/*	if ((r7->cmd | r7->ver | r7->vca | r7->pattern) == 0)
		return MSS_NO_RESPONSE;*/
	if (r7->cmd != SD_SEND_IF_COND || r7->ver != 0 
			|| (r7->vca | r7->pattern) != arg) {
		sd_card->errno = SD_ERROR_HEADER_MISMATCH;
		return MSS_ERROR_RESP_UNPACK;
	}
	return 0;
}

static int sd_unpack_scr(u8 *buf, struct sd_scr *scr)
{
	scr->scr_structure	= unstuff_bits(buf, 60, 4, 8);
	scr->sd_spec		= unstuff_bits(buf, 56, 4, 8);
	scr->data_stat_after_erase = unstuff_bits(buf, 55, 1, 8);
	scr->sd_security	= unstuff_bits(buf, 52, 3, 8);
	scr->sd_bus_width	= unstuff_bits(buf, 48, 4, 8);
	scr->init = 1;
	
	dbg5("scr_stru:%d, spec:%d, sata:%d, security:%d, bus:%d", scr->scr_structure, scr->sd_spec, scr->data_stat_after_erase, scr->sd_security, scr->sd_bus_width);
	return 0;
}
	
static int sd_get_status(struct mss_card *card, int *status)
{
	struct sd_response_r1 r1;
	struct sd_card *sd_card = card->prot_card;
	struct mss_host *host = card->slot->host;
	int clock, ret, retries = 4;

	clock = sd_tran_speed(sd_card->csd.tran_speed);
	mss_set_clock(card->slot->host, clock);
	while (retries--) {
		dbg5("rety");
		ret = mss_send_simple_ll_req(host, &sd_card->llreq, 
				&sd_card->cmd, SD_SEND_STATUS, 
				sd_card->rca << 16, MSS_RESPONSE_R1, 0);
		dbg5("retry ret :%d", ret);
		if (ret && !retries)
			return ret;
		else if (!ret) {
			ret = sd_unpack_r1(&sd_card->cmd, &r1, sd_card);
			if (ret) {
				if (sd_card->errno == SD_ERROR_STATE_MISMATCH) {
					sd_card->state = R1_CURRENT_STATE(r1.status);
					sd_card->errno = SD_ERROR_NONE;
				}
				else
					return ret;
			}
			else 
				break;
		}

		clock = host->ios.clock;
		clock = clock >> 1;
		if (clock < SD_CARD_CLOCK_SLOW || retries == 1)
			clock = SD_CARD_CLOCK_SLOW;
		mss_set_clock(host, clock);
	}

	*status = r1.status;
	
	return MSS_ERROR_NONE; 
}

/**
 *  The blocks requested by the kernel may or may not match what we can do.  
 *  Unfortunately, filesystems play fast and loose with block sizes, so we're 
 *  stuck with this.
 */
static void sd_fix_request_block_len(struct mss_card *card, int action, struct mss_rw_arg *arg)
{
	u16 block_len = 0;
	struct sd_card *sd_card = card->prot_card;
	struct mss_host *host = card->slot->host;

	switch(action) {
		case MSS_DATA_READ:
			block_len = 1 << sd_card->csd.read_bl_len;
			break;
		case MSS_DATA_WRITE:
			block_len = 1 << sd_card->csd.write_bl_len;
			break;
		default:
			return;
	}
	if (host->high_capacity	&& (sd_card->ocr & SD_OCR_CCS))
		block_len = 512;	
	if (block_len < arg->block_len) {
		int scale = arg->block_len / block_len;
		arg->block_len	= block_len;
		arg->block *= scale;
		arg->nob *= scale;
	}
}

static int sd_send_cmd6(struct mss_card *card, struct sw_func_status *status, int mode, u32 funcs)
{
	struct sd_response_r1 r1;
	struct sd_card *sd_card = card->prot_card;
	struct mss_ll_request *llreq = &sd_card->llreq;
	struct mss_cmd *cmd = &sd_card->cmd;
	struct mss_data *data = &sd_card->data;
	struct mss_host *host= card->slot->host;
	struct scatterlist sg;
	char *g_buffer = sd_card->buf;
	int ret;
	/* Set the argumens for CMD6. */
	/* [31]: Mode 
	 * [30:24]: reserved (all 0s)
	 * [23:20]: group 6
	 * [19:16]: group 5
	 * [15:12]: group 4
	 * [11:8]: group 3
	 * [7:4]: group 2
	 * [3:0]: group 1
	 */
	sg.page = virt_to_page(g_buffer);
	sg.offset = offset_in_page(g_buffer);
	sg.length = 8;
	
	memset(llreq, 0x0, sizeof(struct mss_ll_request));
	memset(cmd, 0x0, sizeof(struct mss_cmd));
	memset(data, 0x0, sizeof(struct mss_data));
	MSS_INIT_CMD(cmd, SD_SW_FUNC, (funcs | ((mode & 0x1) << 31)), 0, 
			MSS_RESPONSE_R1);
	MSS_INIT_DATA(data, 1, 32, MSS_DATA_READ, 1, &sg, 0);
	llreq->cmd = cmd;
	llreq->data = data;
	
	ret = mss_send_ll_req(host, llreq);
	if (ret)
		return ret;
	ret = sd_unpack_r1(cmd, &r1, sd_card);
	if (ret)
		return ret;
	sd_unpack_swfuncstatus(g_buffer, status);
	
	return 0;
}

/*****************************************************************************
 *
 *   protocol entry functions
 *
 ****************************************************************************/

static int sd_recognize_card(struct mss_card *card)
{
	struct sd_response_r1 r1;
	struct sd_response_r3 r3;
	struct sd_response_r7 r7;
	int ret;
	struct sd_card *sd_card = (struct sd_card *)card->prot_card;
	struct mss_ios ios;
	struct mss_host *host = card->slot->host;
	struct mss_ll_request *llreq = &sd_card->llreq;
	struct mss_cmd *cmd = &sd_card->cmd;

	card->state = CARD_STATE_IDLE;
	card->bus_width = MSS_BUSWIDTH_1BIT;

	memcpy(&ios, &host->ios, sizeof(struct mss_ios)); 
	ios.bus_mode = MSS_BUSMODE_OPENDRAIN;
	ios.clock = host->f_min;
	ios.bus_width = MSS_BUSWIDTH_1BIT;
	host->ops->set_ios(host, &ios);

	card->card_type = MSS_UNKNOWN_CARD;
	
	ret = mss_send_simple_ll_req(host, llreq, cmd, 
		       SD_GO_IDLE_STATE, 0, MSS_RESPONSE_NONE, MSS_CMD_INIT);
	if (ret)
		return ret;
	if (host->sd_spec == MSS_SD_SPEC_20) {
		if (!(host->vdd & MSS_VDD_27_36))
			return MSS_ERROR_NO_PROTOCOL; 
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SEND_IF_COND,
			       	0x1AA, MSS_RESPONSE_R7, 0);
		if (ret == MSS_ERROR_TIMEOUT)
			goto next;
		else if (ret)
			return ret;
		ret = sd_unpack_r7(cmd, &r7, 0x1AA, sd_card);
		if (!ret) {
			sd_card->ver = MSS_SD_SPEC_20;
			goto next;
		}
		else 
			return ret;
	}	
next:
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_APP_CMD, 0, 
			MSS_RESPONSE_R1, 0);
	if (ret)
		return ret;
	ret = sd_unpack_r1(cmd, &r1, sd_card);
	if (ret)
		return ret;

	
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SD_SEND_OP_COND, 0,
		MSS_RESPONSE_R3, 0);
	if (ret)
		return ret;
	ret = sd_unpack_r3(cmd, &r3, sd_card);	
	if (ret)
		return ret;
	
	if (r3.ocr & host->vdd) {
		card->card_type = MSS_SD_CARD;
		sd_card->ver = MSS_SD_SPEC_20;
	}
	else
		card->card_type = MSS_UNCOMPATIBLE_CARD;

	return MSS_ERROR_NONE;
}


/**
 *  sd_card_init
 *  @dev: mss_card_device
 *
 *  return value: 0: success, -1: failed
 */
static int sd_card_init(struct mss_card *card)
{
	struct sd_response_r1 r1;
	struct sd_response_r3 r3;
	struct sd_response_r6 r6;
	struct sd_response_r7 r7;
	struct sd_cid cid;
	int ret;
	struct sd_card * sd_card= (struct sd_card *)card->prot_card;
	struct mss_ios ios;
	struct mss_host *host = card->slot->host;
	struct mss_ll_request *llreq = &sd_card->llreq;
	struct mss_cmd *cmd = &sd_card->cmd;
	int hcs = 0;

	sd_card->state = CARD_STATE_IDLE;
	card->bus_width = MSS_BUSWIDTH_1BIT;
	sd_card->rca = 0;

	memcpy(&ios, &host->ios, sizeof(struct mss_ios));
	ios.bus_mode = MSS_BUSMODE_OPENDRAIN;
	ios.bus_width = MSS_BUSWIDTH_1BIT;
	ios.clock = host->f_min;
	host->ops->set_ios(host, &ios);

	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_GO_IDLE_STATE, 0, 
			MSS_RESPONSE_NONE, MSS_CMD_INIT);
	if (ret)
		return ret;
	/* 
	 * We have to send cmd 8 to 2.0 card. It will tell the card that the 
	 * host support 2.0 spec.
	 */
	if (sd_card->ver == MSS_SD_SPEC_20 && host->sd_spec == MSS_SD_SPEC_20) {
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SEND_IF_COND, 
				0x1AA, MSS_RESPONSE_R7, 0);
		if (ret)
			return ret;
		ret = sd_unpack_r7(cmd, &r7, 0x1AA, sd_card);
		if (ret)
			return ret;
		if (host->high_capacity)
			hcs = 1;
	}
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_APP_CMD, 0, 
			MSS_RESPONSE_R1, 0);
	if (ret)
		return ret;
	ret = sd_unpack_r1(cmd, &r1, sd_card);
	if (ret)
		return ret;
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SD_SEND_OP_COND, 
			hcs << 30 | host->vdd, MSS_RESPONSE_R3, 0);
	if (ret)
		return ret;
	ret = sd_unpack_r3(cmd, &r3, sd_card);
	if (ret)
		return ret;
	while (!(r3.ocr & SD_OCR_CARD_BUSY)) {
		//mdelay(20);
		msleep(15);
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_APP_CMD, 0, 
				MSS_RESPONSE_R1, 0);
		if (ret)
			return ret;
		ret = sd_unpack_r1(cmd, &r1, sd_card);
		if (ret)
			return ret;

		ret = mss_send_simple_ll_req(host, llreq, cmd, 
				SD_SD_SEND_OP_COND, hcs << 30 | host->vdd,
			       	MSS_RESPONSE_R3, 0);
		if (ret)
			return ret;
		ret = sd_unpack_r3(cmd, &r3, sd_card);
		if (ret)
			return ret;
	}
	memcpy(&sd_card->ocr, &r3.ocr, sizeof(r3.ocr));
	sd_card->state = CARD_STATE_READY;

	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_ALL_SEND_CID, 0, 
			MSS_RESPONSE_R2_CID, 0);
	if (ret)
		return ret;

	memset(&cid, 0x0, sizeof(struct sd_cid));
	ret = sd_unpack_cid(cmd, &cid, sd_card);
	if (ret)
		return ret;
	
	if (sd_card->cid.mid != 0) {
		if (sd_card->cid.mid != cid.mid || sd_card->cid.oid != cid.oid
				|| sd_card->cid.prv != cid.prv 
				|| sd_card->cid.psn != cid.psn
				|| sd_card->cid.mdt != cid.mdt
				|| memcmp(sd_card->cid.pnm, cid.pnm, 6))
			return MSS_ERROR_MISMATCH_CARD;

		if (memcmp(&cid, &sd_card->cid, sizeof(struct sd_cid)))
			return MSS_ERROR_MISMATCH_CARD;
	}
	else
		memcpy(&sd_card->cid, &cid, sizeof(struct sd_cid));
	
	sd_card->state = CARD_STATE_IDENT;
	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SEND_RELATIVE_ADDR, 
			0, MSS_RESPONSE_R6, 0);
	if (ret)
		return ret;
	ret = sd_unpack_r6(cmd, &r6, sd_card);
	if (ret)
		return ret;
	sd_card->state = CARD_STATE_STBY;
	sd_card->rca = r6.rca;

	ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SEND_CSD, 
		       sd_card->rca << 16, MSS_RESPONSE_R2_CSD, 0);
	if (ret)
		return ret;
	ret = sd_unpack_csd(cmd, &sd_card->csd, sd_card);
	if (ret)
		return ret;
	
	if (host->ops->is_slot_wp && host->ops->is_slot_wp(card->slot))
		card->state |= MSS_CARD_WP;
		
	return MSS_ERROR_NONE;
}

static int sd_read_write_entry(struct mss_card *card, int action, struct mss_rw_arg *arg, struct mss_rw_result *result)
{
	struct sd_response_r1 r1;
	struct mss_host *host = card->slot->host;
	struct mss_ios ios;
	int ret, retries = 4;
	struct sd_card *sd_card = (struct sd_card *)card->prot_card;
	struct mss_ll_request *llreq = &sd_card->llreq;
	struct mss_cmd *cmd = &sd_card->cmd;
	struct mss_data *data = &sd_card->data;
	struct scatterlist sg;
	char *g_buffer = sd_card->buf;
	int status;
	u32 clock;
	u32 cmdarg, blklen, opcode, flags;

	dbg5("block:%d, nob:%d, blok_len:%d", arg->block, arg->nob, arg->block_len);	
	ret = sd_get_status(card, &status);
	if (ret)
		return ret;

	if (status & R1_CARD_IS_LOCKED)
		return MSS_ERROR_LOCKED;

	if (action == MSS_WRITE_MEM && host->ops->is_slot_wp && 
			host->ops->is_slot_wp(card->slot))
		return MSS_ERROR_WP;
	
	if (sd_card->state == CARD_STATE_STBY) {
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_SELECT_CARD,
				sd_card->rca << 16, MSS_RESPONSE_R1B, 0);
		if (ret)
			return ret;
		ret = sd_unpack_r1(cmd, &r1, sd_card);
		if (ret)
			return ret;
	}
	sd_card->state = CARD_STATE_TRAN;
		
	sd_fix_request_block_len(card, action, arg);


	if (!sd_card->scr.init) {
		ret = mss_send_simple_ll_req(host, llreq, cmd, SD_APP_CMD,
				sd_card->rca << 16, MSS_RESPONSE_R1, 0);
		if (ret)
			return ret;
		
		sg.page = virt_to_page(g_buffer);
		sg.offset = offset_in_page(g_buffer);
		sg.length = 8;
		
		memset(llreq, 0x0, sizeof(struct mss_ll_request));
		memset(cmd, 0x0, sizeof(struct mss_cmd));
		memset(data, 0x0, sizeof(struct mss_data));
		MSS_INIT_CMD(cmd, SD_SEND_SCR, 0, 0, MSS_RESPONSE_R1);
		MSS_INIT_DATA(data, 1, 8, MSS_DATA_READ, 1, &sg, 0);
		llreq->cmd = cmd;
		llreq->data = data;

		ret = mss_send_ll_req(host, llreq);
		if (ret)
			return ret;
		ret = sd_unpack_scr(g_buffer, &sd_card->scr);
		if (ret)
			return ret;
	}
	if (sd_card->scr.sd_bus_width == SCR_BUSWIDTH_1BIT) {
		mss_set_buswidth(host, MSS_BUSWIDTH_1BIT);
		card->bus_width = MSS_BUSWIDTH_1BIT;
	}
	else {
		if (card->bus_width == MSS_BUSWIDTH_1BIT 
				&& host->bus_width == MSS_BUSWIDTH_4BIT) {
			mss_set_buswidth(host, MSS_BUSWIDTH_4BIT);
			card->bus_width = MSS_BUSWIDTH_4BIT;
			ret = mss_send_simple_ll_req(host, llreq, cmd, 
					SD_APP_CMD, sd_card->rca << 16, 
					MSS_RESPONSE_R1, 0);
			if (ret)
				return ret;
			ret = sd_unpack_r1(cmd, &r1, sd_card);
			if (ret)
				return ret;
			ret = mss_send_simple_ll_req(host, llreq, cmd, 
					SD_SET_BUS_WIDTH, 0x2, MSS_RESPONSE_R1,
					0);
			if (ret)
				return ret;
			ret = sd_unpack_r1(cmd, &r1, sd_card);
			if (ret)
				return ret;
			card->bus_width = MSS_BUSWIDTH_4BIT;
		}
	}
	memset(llreq, 0x0, sizeof(struct mss_ll_request));
	memset(cmd, 0x0, sizeof(struct mss_cmd));
	memset(data, 0x0, sizeof(struct mss_data));
	
	memcpy(&ios, &host->ios, sizeof(struct mss_ios));
	if ((sd_card->ocr & SD_OCR_CCS)	&& host->high_capacity) {
		ios.access_mode = MSS_ACCESS_MODE_SECTOR;
		cmdarg = arg->block;
		blklen = 512;
	}
	else {
		if (arg->block_len != sd_card->block_len) {
			ret = mss_send_simple_ll_req(host, llreq, cmd, 
					SD_SET_BLOCKLEN, arg->block_len, 
					MSS_RESPONSE_R1, 0);
			if (ret)
				return ret;
			ret = sd_unpack_r1(cmd, &r1, sd_card);
			if (ret)
				return ret;
			sd_card->block_len = arg->block_len;
		}
		cmdarg = arg->block * arg->block_len;
		blklen = arg->block_len;
	}
	ios.clock = sd_tran_speed(sd_card->csd.tran_speed);
	host->ops->set_ios(host, &ios);
	
	llreq->cmd = cmd;
	llreq->data = data;
	
read_write_entry:

	if (arg->nob > 1) {
		if (action == MSS_READ_MEM) {
			opcode = SD_READ_MULTIPLE_BLOCK;
			flags = MSS_DATA_READ | MSS_DATA_MULTI;
		}
		else {
			opcode = SD_WRITE_MULTIPLE_BLOCK;
			flags = MSS_DATA_WRITE | MSS_DATA_MULTI;
		}
		
		MSS_INIT_CMD(cmd, opcode, cmdarg, 0, MSS_RESPONSE_R1);
		MSS_INIT_DATA(data, arg->nob, blklen, flags, arg->sg_len, 
				arg->sg, 0);
		
		ret = mss_send_ll_req(host, llreq);
		if (!ret)
			ret = sd_unpack_r1(cmd, &r1, sd_card);
		sd_card->state = (action == MSS_WRITE_MEM) ? CARD_STATE_RCV : CARD_STATE_DATA;
		if (ret) {
			mss_send_simple_ll_req(host, llreq, cmd, 
					SD_STOP_TRANSMISSION, 0, 
					(action == MSS_WRITE_MEM) ? 
					MSS_RESPONSE_R1B : MSS_RESPONSE_R1, 0);
			sd_card->state = CARD_STATE_TRAN;
			if (--retries) {
				clock = host->ios.clock;
				clock = clock >> 1;
				if (clock < SD_CARD_CLOCK_SLOW && retries == 1)
					clock = SD_CARD_CLOCK_SLOW;
				mss_set_clock(host, clock);
				goto read_write_entry;
			}
			return ret;
		}
		ret = mss_send_simple_ll_req(host, llreq, cmd, 
				SD_STOP_TRANSMISSION, 0, 
				(action == MSS_WRITE_MEM) ? 
				MSS_RESPONSE_R1B : MSS_RESPONSE_R1, 0);
		if (ret)
			return ret;
		ret = sd_unpack_r1(cmd, &r1, sd_card);
		sd_card->state = CARD_STATE_TRAN;
		if (ret	&& (sd_card->errno != SD_ERROR_OUT_OF_RANGE)) 
			return ret;
	} else {
		if (action == MSS_READ_MEM) {
			opcode = SD_READ_SINGLE_BLOCK;
			flags = MSS_DATA_READ;
		}
		else {
			opcode = SD_WRITE_BLOCK;
			flags = MSS_DATA_WRITE;
		}
		MSS_INIT_CMD(cmd, opcode, cmdarg, 0, MSS_RESPONSE_R1);
		MSS_INIT_DATA(data, arg->nob, blklen, flags, arg->sg_len, 
				arg->sg, 0);

		ret = mss_send_ll_req(host, llreq);
		if (!ret)
			ret = sd_unpack_r1(cmd, &r1, sd_card);
		if (ret) {
			if (--retries) {
				clock = host->ios.clock;
				clock = clock >> 1;
				if (clock < SD_CARD_CLOCK_SLOW && retries == 1)
					clock = SD_CARD_CLOCK_SLOW;
				mss_set_clock(host, clock);
				goto read_write_entry;
			}
			return ret;
		}
	}
	/* Deselect the card */
	/*mmc_simple_ll_req(host, mmc_card, MMC_SELECT_CARD, 
		0, MSS_RESPONSE_NONE, 0);
	if (ret)
		return ret;
	ret = mmc_unpack_r1(&mmc_card->cmd, &r1, mmc_card);
	mmc_card->state = CARD_STATE_STBY;*/
	if (ret)
		return ret;
	result->bytes_xfered = data->bytes_xfered;

	return MSS_ERROR_NONE;
}

static int sd_query_function(struct mss_card *card, struct io_swfunc_request *r, struct sw_func_status *status)
{
	struct sd_card *sd_card = card->prot_card;
	int i;
	u32 arg = 0;

	if (card->slot->host->sd_spec != MSS_SD_SPEC_20
			|| sd_card->ver != MSS_SD_SPEC_20)
		return MSS_ERROR_ACTION_UNSUPPORTED;
	for (i = 5; i > 0; i--) {
		arg |= ((r->args[i] & 0xF) << (i << 2));
	}
	return sd_send_cmd6(card, status, 0, arg);
}

static int sd_sw_function(struct mss_card *card, struct io_swfunc_request *r)
{
	int ret, i;
	u32 arg = 0;
	struct sw_func_status status;
	struct sd_card *sd_card = card->prot_card;
	
	if (card->slot->host->sd_spec != MSS_SD_SPEC_20 
			|| sd_card->ver != MSS_SD_SPEC_20)
		return MSS_ERROR_ACTION_UNSUPPORTED;
	for (i = 0; i < 6; i++) {
		if (r->args[i] >= 0xF) {
			goto switch_err;
		}
	}
	/* Step 1: CMD6(mode = 0, func = 0xF(Don't Care) */
	ret = sd_send_cmd6(card, &status, 0, 0xFFFFFF);
	if (ret)
		goto switch_err;
	
	/* Step 2: Any Switch ? */
	for (i = 0; i < 6; i++) {
		if (!((status.func_support[i]) & (0x1 << (r->args[i])))) 
			goto switch_err;
	}

	for (i = 0; i < 6; i++) {
		if (status.group_status[i] != r->args[i]) {	
			break;
		}
	}
	if (i == 6)
		return 0;
		
	/* Step 3: CMD6(mode = 0, func= funcX */
	for (i = 5; i > 0; i--) {
		arg |= ((r->args[i]) << (i << 2));
	}
	ret = sd_send_cmd6(card, &status, 0, arg);
	if (ret)
		goto switch_err;
	if (status.current_consumption > r->current_acceptable) {
		goto switch_err;
	}
	for (i = 0; i < 6; i++) {
		if (status.group_status[i] != r->args[i]) {
			goto switch_err;
		}
	}
	ret = sd_send_cmd6(card, &status, 1, arg);
	if (ret)
		goto switch_err;
	for (i = 0; i < 6; i++) {
		if (status.group_status[i] != r->args[i]) {
			goto switch_err;
		}
	}
	return 0;
switch_err:
	sd_card->errno = SD_ERROR_SWFUNC;
	return MSS_ERROR_ACTION_UNSUPPORTED;
}
/* count by 512 bytes */
static int sd_get_capacity(struct mss_card *card, u32 *size)
{
	struct sd_card *sd_card = card->prot_card;
	int c_size = 0;
	int c_size_mult = 0;
	int blk_len = 0;

	if (sd_card->csd.csd_structure == 0) {
		c_size = sd_card->csd.csd.csd1.c_size;
		c_size_mult = sd_card->csd.csd.csd1.c_size_mult;
		blk_len = sd_card->csd.read_bl_len - 9;
	}
	/* (csize + 1) * 512 * 1024 bytes */
	else if (sd_card->csd.csd_structure == 1) {
		c_size = sd_card->csd.csd.csd2.c_size;
		c_size_mult = 7;
	       	blk_len = 2;
	}
	*size = (c_size + 1) << (2 + c_size_mult + blk_len);
	return MSS_ERROR_NONE;
}



/*****************************************************************************
 *
 *   protocol driver interface functions
 *
 ****************************************************************************/
static int sd_prot_entry(struct mss_card *card, unsigned int action, void *arg, void *result)
{
	int ret;
	u32 status;
	
	if (action != MSS_RECOGNIZE_CARD && card->card_type != MSS_SD_CARD)
		return MSS_ERROR_WRONG_CARD_TYPE;
	switch (action) {
		case MSS_RECOGNIZE_CARD:
			ret = sd_recognize_card(card);
			break;
		case MSS_INIT_CARD:
			ret = sd_card_init(card);
			break;
		case MSS_READ_MEM:
		case MSS_WRITE_MEM:
			if (!arg || !result)
				return -EINVAL;
			ret = sd_read_write_entry(card, action, arg, result);
			break;
	/*
		case MSS_LOCK_UNLOCK:
			ret = sd_lock_unlock_entry(dev);
			break;
	*/
		case MSS_SD_QUERY_FUNC:
			if (!arg || !result)
				return -EINVAL;
			ret = sd_query_function(card, arg, result);
			break;
		case MSS_SD_SW_FUNC:
			if (!arg)
				return -EINVAL;
			ret = sd_sw_function(card, arg);	
			break;
		case MSS_QUERY_CARD:
			ret = sd_get_status(card, &status);
			break;
		case MSS_GET_CAPACITY:
			ret = sd_get_capacity(card, result);
			break;
		default:
			ret = MSS_ERROR_ACTION_UNSUPPORTED;
			break;
	}
	return ret;
}

static int sd_prot_attach_card(struct mss_card *card)
{
	struct sd_card *sd_card;

#define ALIGN32(x)	(((x) + 31) & (~31))
	sd_card = kzalloc(ALIGN32(ALIGN32(sizeof(struct sd_card))) + 512, 
			GFP_KERNEL);
	card->prot_card = sd_card;
	if (sd_card) {
		sd_card->buf = (char *)ALIGN32((unsigned int)&sd_card[1]);
		return 0;
	}
	return -ENOMEM;
}

static void sd_prot_detach_card(struct mss_card *card)
{
	kfree(card->prot_card);
}

static int sd_prot_get_errno(struct mss_card *card)
{
	struct sd_card *sd_card = card->prot_card;
	
	return sd_card->errno;
}

static struct mss_prot_driver sd_protocol = {
	.name			=	SD_PROTOCOL,
	.prot_entry		=	sd_prot_entry,
	.attach_card		=	sd_prot_attach_card,
	.detach_card		=	sd_prot_detach_card,
	.get_errno		=	sd_prot_get_errno,
};

/*****************************************************************************
 *
 *   module init and exit functions
 *
 ****************************************************************************/
static int sd_protocol_init(void)
{
	register_mss_prot_driver(&sd_protocol);
	return 0;
}

static void sd_protocol_exit(void)
{
	unregister_mss_prot_driver(&sd_protocol);
}


module_init(sd_protocol_init);
module_exit(sd_protocol_exit);

MODULE_AUTHOR("Bridge Wu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SD protocol driver");
