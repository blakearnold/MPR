/*
 * mss_core.c - MMC/SD/SDIO Core driver
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

#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/types.h>

#include <linux/mmc/mss_core.h>


static LIST_HEAD(mss_protocol_list);
static LIST_HEAD(mss_host_list);

static void mss_power_up(struct mss_host *host)
{
	struct mss_ios ios;
	
	memcpy(&ios, &host->ios, sizeof(ios));
	ios.vdd = host->vdd;
	ios.chip_select = MSS_CS_NO_CARE;
	ios.power_mode = MSS_POWER_UP;
	host->ops->set_ios(host, &ios);

	msleep(1);

	ios.clock = host->f_min;
	ios.power_mode = MSS_POWER_ON;
	host->ops->set_ios(host, &ios);

	msleep(2);
}

static void mss_power_off(struct mss_host *host)
{
	struct mss_ios ios;
	
	memcpy(&ios, &host->ios, sizeof(ios));
	ios.clock = 0;
	ios.chip_select = MSS_CS_NO_CARE;
	ios.power_mode = MSS_POWER_OFF;
	host->ops->set_ios(host, &ios);
}

static void mss_idle_cards(struct mss_host *host)
{
	struct mss_ios ios;
	
	memcpy(&ios, &host->ios, sizeof(ios));
	ios.chip_select = MSS_CS_HIGH;
	host->ops->set_ios(host, &ios);
	msleep(1);
	ios.chip_select = MSS_CS_NO_CARE;
	host->ops->set_ios(host, &ios);
	msleep(1);
}

/*
 * Only after card is initialized by protocol and be registed to mmc_bus, the
 * state is changed to MSS_CARD_REGISTERED.
 */
static int mmc_bus_match(struct device *dev, struct device_driver *drv)
{
	struct mss_card *card;

	card = container_of(dev, struct mss_card, dev);

	dbg(card->slot,	"bus match driver %s", drv->name);
	/* when card->state is MSS_CARD_REGISTERED,it is accepted by protocol */
	if (card->prot_driver && (card->state & MSS_CARD_REGISTERED))
		return 1;
	dbg(card->slot,	"bus match driver %s fail", drv->name);
	return 0;		
}

//static int mmc_bus_hotplug(struct device *dev, char **envp, int num_envp, char *buffer, int buffer_size)
//{
//	printk(KERN_INFO "*** HOT PLUG ***\n");
//	return 0;
//}

static int mmc_bus_suspend(struct device * dev, pm_message_t state)
{
	int ret = 0;
	struct mss_card *card;
	
	card = container_of(dev, struct mss_card, dev);

	if (card->state & MSS_CARD_HANDLEIO)
		return -EAGAIN;
	if (card->state & MSS_CARD_SUSPENDED)
		return 0;
	
	if (dev->driver && dev->driver->suspend) {
		ret = dev->driver->suspend(dev, state);//, SUSPEND_DISABLE);
		if (ret == 0)
			ret = dev->driver->suspend(dev, state);//, SUSPEND_SAVE_STATE);
		if (ret == 0)
			ret = dev->driver->suspend(dev, state);//, SUSPEND_POWER_DOWN);
	}
	/* mark MSS_CARD_SUSPEND here */
	card->state |= MSS_CARD_SUSPENDED;
	return ret;
}
/* 
 * card may be removed or replaced by another card from the mmc_bus when it is 
 * sleeping, and the slot may be inserted a card when it is sleeping.
 * The controller resume function need to take care about it.
 */
static int mmc_bus_resume(struct device * dev)
{
	int ret = 0;
	struct mss_card *card;
	
	card = container_of(dev, struct mss_card, dev);
	
	/* it is new instered card or replaced card */
	if (!(card->state & MSS_CARD_SUSPENDED))
		return 0;
	
	card->state &= ~MSS_CARD_SUSPENDED;
	if (dev->driver && dev->driver->resume) {
		ret = dev->driver->resume(dev);//, RESUME_POWER_ON);
		if (ret == 0)	
			ret = dev->driver->resume(dev);//, RESUME_RESTORE_STATE);
		if (ret == 0)
			ret = dev->driver->resume(dev);//, RESUME_ENABLE);
	}

	return ret;
}

static struct bus_type mmc_bus_type = {
	.name		=	"mmc_bus",
	.match		=	mmc_bus_match,
//	.hotplug	=	mmc_bus_hotplug,
	.suspend	=	mmc_bus_suspend,
	.resume		=	mmc_bus_resume,
};

static void mss_card_device_release(struct device *dev)
{
	struct mss_card *card = container_of(dev, struct mss_card, dev);

	kfree(card);
}

static void mss_claim_host(struct mss_host *host, struct mss_card *card)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long flags;

	/*
	spin_lock_irqsave(&host->lock, flags);
	while (host->active_card != NULL) {
		spin_unlock_irqrestore(&host->lock, flags);
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&host->wq, &wait);
		schedule();
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&host->wq, &wait);
		spin_lock_irqsave(&host->lock, flags);
	}
	*/

	spin_lock_irqsave(&host->lock, flags);
	while (host->active_card != NULL) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		spin_unlock_irqrestore(&host->lock, flags);
		add_wait_queue(&host->wq, &wait);
		schedule();
		remove_wait_queue(&host->wq, &wait);
		spin_lock_irqsave(&host->lock, flags);
	}
	set_current_state(TASK_RUNNING);
	host->active_card = card;
	spin_unlock_irqrestore(&host->lock, flags);
}

static void mss_release_host(struct mss_host* host)
{
	unsigned long flags;

	BUG_ON(host->active_card == NULL);

	spin_lock_irqsave(&host->lock, flags);
	host->active_card = NULL;
	spin_unlock_irqrestore(&host->lock, flags);

	wake_up(&host->wq);

}

int mss_card_get(struct mss_card *card)
{
	if ((card->state & MSS_CARD_REMOVING) 
			|| !(card->state & MSS_CARD_REGISTERED))
		return -ENXIO;
	if (!get_device(&card->dev))
		return -ENXIO;
	return 0;
}

void mss_card_put(struct mss_card *card)
{
	put_device(&card->dev);
}

/*
 *  finish handling a request.
 */
static void mss_finish_request(struct mss_request *req)
{
	struct mss_driver *drv;
	struct mss_card *card = req->card;

	drv = container_of(card->dev.driver, struct mss_driver, driver);
	if (drv && drv->request_done)
		drv->request_done(req);
}

/*
 * Loop all the protocol in the mss_protocol_list, and find one protocol that
 * can successful recognize and init the card.
 */
static int mss_attach_protocol(struct mss_card *card)
{
	struct list_head *item;
	struct mss_prot_driver *pdrv = NULL;
	struct mss_host *host = card->slot->host;
	int ret;

	/* loop all the protocol, and find one that match the card */
	list_for_each(item, &mss_protocol_list) {
		pdrv = list_entry(item, struct mss_prot_driver, node);
		dbg(card->slot, "try protocol:%s", pdrv->name);

		ret = pdrv->attach_card(card);
		if (ret)
			continue;
		dbg(card->slot, "protocol:%s allocate spec card", pdrv->name);

		mss_claim_host(host, card);
		ret = pdrv->prot_entry(card, MSS_RECOGNIZE_CARD, NULL, NULL);
		mss_release_host(host);
		dbg(card->slot, "protocol:%s identy ret:%d, card type:%d\n", pdrv->name, ret, card->card_type);
		if (ret)
			continue;
		switch (card->card_type) {
			case MSS_MMC_CARD:
			//case MSS_CE_ATA:
			case MSS_SD_CARD:
			case MSS_SDIO_CARD:
			case MSS_COMBO_CARD:
			//dbg("identified card type: %d", card_type);
				goto identified;
			/* 
			 * The card can be recognized, but it deos not fit the 
			 * controller.
			 */
			case MSS_UNCOMPATIBLE_CARD:
				pdrv->detach_card(card);
				return MSS_ERROR_NO_PROTOCOL;
			/* The card can not be recognized by the protocl */
			case MSS_UNKNOWN_CARD:
				pdrv->detach_card(card);
				break;	
			default:
				pdrv->detach_card(card);
				printk(KERN_WARNING "protocol driver :%s return unknown value when recognize the card\n", pdrv->name);
				break;
		}
	}
	
	return MSS_ERROR_NO_PROTOCOL;
identified:
	card->prot_driver = pdrv;
	return 0;
}

/* Initialize card by the protocol */
int mss_init_card(struct mss_card *card)
{
	int ret;
	struct mss_host *host = card->slot->host;
	
	if (!card || !card->prot_driver)
		return -EINVAL;
	mss_claim_host(host, card);
	ret = card->prot_driver->prot_entry(card, MSS_INIT_CARD, NULL, NULL);
	mss_release_host(host);

	dbg5("return :%d", ret);
	return ret;
}

int mss_query_card(struct mss_card *card)
{	
	int ret;
	struct mss_host *host = card->slot->host;
	
	if (!card || !card->prot_driver)
		return -EINVAL;
	mss_claim_host(host, card);
	ret = card->prot_driver->prot_entry(card, MSS_QUERY_CARD, NULL, NULL);
	mss_release_host(host);

	return ret;
}

static int __mss_insert_card(struct mss_card *card) 
{
	int ret;

	dbg(card->slot, "attaching protocol");
	/* Step 1: Recognize the card */
	ret = mss_attach_protocol(card);
	if (ret)
		return ret;
	
	dbg(card->slot, "protocol%s attached", card->prot_driver->name);
	/* Step 2, initialize the card */
	ret = mss_init_card(card);
	if (ret) { 
		goto detach_prot;
	}

	dbg(card->slot, "protocol%s inited", card->prot_driver->name);
	/* Step 3, register the card to mmc bus */
	card->dev.release = mss_card_device_release;
	card->dev.parent = card->slot->host->dev;
	/* set bus_id and name */
	snprintf(&card->dev.bus_id[0], sizeof(card->dev.bus_id), "mmc%d%d", 
			card->slot->host->id, card->slot->id); 
	card->dev.bus = &mmc_bus_type; 
	
	card->state |= MSS_CARD_REGISTERED;
	ret = device_register(&card->dev); /* will call mss_card_probe */	
	if (ret) {
		ret = MSS_ERROR_REGISTER_CARD;
		card->state &= ~MSS_CARD_REGISTERED;
		goto detach_prot;
	}
	dbg(card->slot, "protocol%s registered\n", card->prot_driver->name);
	return MSS_ERROR_NONE;
	
detach_prot:
	card->prot_driver->detach_card(card);
	card->prot_driver = NULL;
	return ret;
}

/*
 * After knowing a card has been inserted into the slot, this function should 
 * be invoked. At last, load card driver in card (done by card_driver->probe).
 */
static int mss_insert_card(struct mss_slot *slot)
{
	struct mss_card * card;
	int ret;

	BUG_ON(slot->card);
	dbg(slot, "card is inserting");
	card = kzalloc(sizeof(struct mss_card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	card->slot = slot;
	slot->card = card;

	dbg(slot, "allocate card :0x%p", card);	
	ret = __mss_insert_card(card);
	dbg(slot, "insert ret :%d", ret);
	if (ret) {
		dbg(slot, "free card");
		slot->card = NULL;
		kfree(card);
	}
	return ret;
}

static int __mss_eject_card(struct mss_card *card)
{
	card->state |= MSS_CARD_REMOVING;

	dbg(card->slot, "card state 0x%x", card->state);

	if (card->prot_driver) {
		card->prot_driver->detach_card(card);
		card->prot_driver = NULL;	
	}
	if (card->state & MSS_CARD_REGISTERED) {
		device_unregister(&(card->dev));
		//card->state &= ~MSS_CARD_REGISTERED;
	}
	
	return 0;
}

/*
 * After knowing a card has been ejected from the slot, this function should 
 * be invoked. At last, unload card driver in card(done by card_driver->remove).
 */
static int mss_eject_card(struct mss_card *card)
{
	BUG_ON(!card);

	dbg(card->slot, "eject card 0x%x", card);	
	__mss_eject_card(card);
	
	card->slot->card = NULL;
	card->slot = NULL;
	//kfree(card);
	
	return 0;
}

unsigned int mss_get_capacity(struct mss_card *card)
{
	int ret; 
	u32 cap;
	
	mss_claim_host(card->slot->host, card);
	ret = card->prot_driver->prot_entry(card, MSS_GET_CAPACITY, NULL, &cap);
	mss_release_host(card->slot->host);
	dbg5("get capcacity:0x%x, ret:%d",cap, ret); 
	if (ret)
		cap = 0;
	return cap;
}

int mss_scan_slot(struct work_struct *work)
{
	struct mss_card *card;
	struct mss_host *host;
	struct mss_slot *slot;
	int ret = 0;

	slot = container_of(work, struct mss_slot, card_detect.work); 

	card = slot->card;
	host = slot->host;


	printk("begin scan slot, id = %d card_type = %d\n", slot->id, 
			(card)?(card->card_type):0);	

	/* slot has card in it before, and the card is resuming back */
	if (card && (card->state & MSS_CARD_SUSPENDED)) { 
		dbg(slot, "suspend, card state is 0x%x", card->state);
		printk("suspend, card state is 0x%x", card->state);
		/* card was ejected when it is suspended */
		if (host->ops->is_slot_empty 
				&& host->ops->is_slot_empty(slot)) {
			card->state |= MSS_CARD_REMOVING;
			ret = MSS_ERROR_CARD_REMOVING;
		}
		else {
			/* 
			 * if host provides is_slot_empty, and it 
			 * indicates that the card is in slot, then 
			 * we try to init it.
			 * else, we try to init it directly. Obvisouly
			 * that if there is no card in the slot, the 
			 * init will fail
			 */
			dbg(slot, "no is_slot_empty");
			ret = mss_init_card(card);
			if (ret)
				card->state |= MSS_CARD_INVALID;
		}
	}
	else if (card && (card->state & MSS_CARD_INVALID)) {

		dbg(slot, "2222222222222");
		
		card->state &= ~MSS_CARD_INVALID;
		ret = mss_eject_card(card);
		
		if (!host->ops->is_slot_empty 
			|| (host->ops->is_slot_empty && 
				!host->ops->is_slot_empty(slot))) {
			ret = mss_insert_card(slot);
		}
			
	}
	/* slot has card in it before, and no suspend happens */
	else if (card && (card->state & MSS_CARD_REGISTERED)) {
		dbg(slot, "33333333333");
		
		dbg(slot, "register, card state is %d", card->state);
		//printk("register, card state is %d", card->state);
		if (host->ops->is_slot_empty) {
			dbg(slot, "has is_slot_empty");
			/* card has been ejected */
			if (host->ops->is_slot_empty(slot))
				ret = mss_eject_card(card);
		}
		else {
			/*
			 * We try to send the status query command.
			 * If card->state has set MSS_CARD_REGISRTEED,
			 * it indicates that the card has finished 
			 * identification process, and it will response
			 * the SEND_STATUS command.
			 */
			dbg(slot, "no is_slot_empty");
			if (mss_query_card(card))
				/* Card has been ejected */
				ret = mss_eject_card(card);
		}
	}
	/* slot has card in it, but the card is not registered */
	else if (card) {
		/* This should never be happens, because when insert fail, we will delete the card */
		BUG();
	}
	/* slot has no card in it before */
	else if (!card) {
		dbg(slot, "no card in it before");
		
		mss_power_off(host);
		mss_power_up(host);
		mss_idle_cards(host);
		if (host->ops->is_slot_empty) {
			/* slot is not empty */
			if (!host->ops->is_slot_empty(slot))
				ret = mss_insert_card(slot);
		}
		else {
			 /* try to insert a card */
			ret = mss_insert_card(slot);
		}				
	}
	else {
		printk(KERN_ERR "Unexpected situation when scan host:%d"
				", slot:%d, card state:0x%x\n", host->id, 
				slot->id, card ? card->state : 0x0);
		BUG();
	}

	return ret;
}

void mss_detect_change(struct mss_host *host, unsigned long delay, unsigned int id)
{
	struct mss_slot *slot;

	slot = &host->slots[id];

	//printk(" enter mss_detect_change(): delay = %d, id = %d, slot = 0x%xi\n",
	//		delay, id, slot);

	if (delay)
		schedule_delayed_work(&slot->card_detect, delay);
	else
		schedule_work(&slot->card_detect);
	
	//printk("%s(): done and exit\n", __FUNCTION__);	
}

void mss_scan_host(struct mss_host *host)
{
	struct mss_slot *slot;
	int i;
	
	for (i = 0; i < host->slot_num; i++) { 
		slot = &host->slots[i];
		mss_scan_slot(&slot->card_detect.work);
	}
}

void mss_force_card_remove(struct mss_card *card)
{
	mss_eject_card(card);
}

static void mss_wait_done(struct mss_ll_request *llreq)
{
	complete(llreq->done_data);
}

int mss_send_ll_req(struct mss_host *host, struct mss_ll_request *llreq)
{
	DECLARE_COMPLETION(complete);

	llreq->done = mss_wait_done;
	llreq->done_data = &complete;

	llreq->cmd->llreq = llreq;
	llreq->cmd->error = MSS_ERROR_NONE;
	if (llreq->data) 
		llreq->cmd->data = llreq->data;
/*	if (llreq->data && llreq->stop) {
		llreq->stop->llreq = llreq;
		llreq->stop->error = 0;
	}*/
	host->ops->request(host, llreq);
	wait_for_completion(&complete);
/*	if (llreq->cmd->error || (llreq->stop && llreq->stop-error))*/
	
	return llreq->cmd->error;
}

int mss_send_simple_ll_req(struct mss_host *host, struct mss_ll_request *llreq, struct mss_cmd *cmd, u32 opcode, u32 arg, u32 rtype, u32 flags)
{
	memset(llreq, 0x0, sizeof(struct mss_ll_request));
	memset(cmd, 0x0, sizeof(struct mss_cmd));

	cmd->opcode = opcode;
	cmd->arg = arg;
	cmd->rtype = rtype;
	cmd->flags = flags;

	llreq->cmd = cmd;

	return mss_send_ll_req(host, llreq);
}	


/*
 * add controller into mss_host_list
 */
int register_mss_host(struct mss_host *host)
{
	list_add_tail(&host->node, &mss_host_list);
	return 0;
}

/*
 *  delete controller from mss_controller_list
 */
void unregister_mss_host(struct mss_host *host)
{
	
	list_del(&host->node);
}

/*****************************************************************************
 *
 *   functions for protocol driver
 *
 ****************************************************************************/

/*
 * add protocol driver into mss_protocol_list
 */
int register_mss_prot_driver(struct mss_prot_driver *drv)
{
	struct list_head *item;
	struct mss_host *host;
	struct mss_slot *slot;
	int i;

	list_add(&drv->node, &mss_protocol_list); 

	list_for_each(item, &mss_host_list) {
		host = list_entry(item, struct mss_host, node);
		for (i = 0; i < host->slot_num; i++) {
			slot = &host->slots[i];
			if (!slot->card) 
				mss_scan_slot(&slot->card_detect.work);
		}
	}
	return 0;
}

/*
 * delete protocol driver from mss_protocol_list
 */
void unregister_mss_prot_driver(struct mss_prot_driver *drv)
{
	struct mss_slot *slot;
	struct list_head *item;
	struct mss_host *host;
	int i;

	list_del(&drv->node);

	list_for_each(item, &mss_host_list) {
		host = list_entry(item, struct mss_host, node);
		for (i = 0; i < host->slot_num; i++) {
			slot = &host->slots[i];
			if (slot->card && slot->card->prot_driver == drv)
				mss_eject_card(slot->card);
		}
	}

}

/*****************************************************************************
 *
 *   interfaces for card driver
 *
 ****************************************************************************/

/*
 * register card driver onto MMC bus
 */
int register_mss_driver (struct mss_driver *drv)
{
	drv->driver.bus = &mmc_bus_type;
	return driver_register(&drv->driver); /* will call card_driver->probe */
}

/*
 * unregister card driver from MMC bus
 */
void unregister_mss_driver (struct mss_driver *drv)
{
	driver_unregister(&drv->driver);
}

/*
 * enable SDIO interrupt, used by SDIO application driver
 */
void mss_set_sdio_int(struct mss_host *host, int sdio_en)
{
	struct mss_ios ios;

	dbg5("mss_set_sdio_int = %s", (sdio_en)?"ENABLE":"DISABLE");
	memcpy(&ios, &host->ios, sizeof(ios));
	ios.sdio_int = sdio_en;
	host->ops->set_ios(host, &ios);
}

void mss_set_clock(struct mss_host *host, int clock)
{
	struct mss_ios ios;

	memcpy(&ios, &host->ios, sizeof(ios));
	if (clock > host->f_max)
		clock = host->f_max;
	else if (clock < host->f_min)
		clock = host->f_min;
	ios.clock = clock;
	host->ops->set_ios(host, &ios);
}

void mss_set_buswidth(struct mss_host *host, int buswidth)
{
	struct mss_ios ios;

	memcpy(&ios, &host->ios, sizeof(ios));
	ios.bus_width = buswidth;
	host->ops->set_ios(host, &ios);
}

void mss_set_busmode(struct mss_host *host, int busmode)
{
	struct mss_ios ios;

	memcpy(&ios, &host->ios, sizeof(ios));
	ios.bus_mode = busmode;
	host->ops->set_ios(host, &ios);
}

int mss_send_request(struct mss_request *req)
{
	struct mss_host *host; 
	struct mss_card *card;
//	unsigned long flags;
	int ret;

	if (!req->card || !req->card->slot)
		return -ENODEV;
	card = req->card;
	host = card->slot->host;
	if (req->card->state & MSS_CARD_REMOVING)
		return MSS_ERROR_CARD_REMOVING;
	card->state |= MSS_CARD_HANDLEIO;
	dbg5("claim host");
	mss_claim_host(host, card);
	ret = card->prot_driver->prot_entry(card, req->action, req->arg, 
			req->result);
	mss_release_host(host);
	dbg5("release host");
	card->state &= ~MSS_CARD_HANDLEIO;

	if (ret)
		req->errno = card->prot_driver->get_errno(card);
	
	return ret;
}

struct mss_host * mss_alloc_host(unsigned int slot_num, unsigned int id, unsigned int private_size)
{
	struct mss_host *host;
	struct mss_slot *slot;
	int i = 0, size;

	printk("%s(): enter\n", __FUNCTION__);	

	size = sizeof(struct mss_host) + sizeof(struct mss_slot) * slot_num 
		+ private_size;
	host = (struct mss_host *)kzalloc(size, GFP_KERNEL);
	if (!host)
		return NULL;
        memset(host, 0x0, size);
	host->id = id;
	host->slot_num = slot_num;
	while(i < slot_num) {
		slot = &host->slots[i];
		slot->id = i;
		slot->host = host;
		INIT_DELAYED_WORK(&slot->card_detect, (void (*)(void *))mss_scan_slot);
		i++;
	}
	host->private = (void *)&host->slots[slot_num];
	host->active_card = NULL;
	init_waitqueue_head(&host->wq);
	spin_lock_init(&host->lock);

	return host;
}

void mss_free_host(struct mss_host *host)
{
	kfree(host);	
}

struct mss_host *mss_find_host(int id)
{
	struct list_head *pos;
	struct mss_host *host;

	list_for_each(pos, &mss_host_list) {
		host = list_entry(pos, struct mss_host, node);
		if (host->id == id)
			return host;
	}
	return NULL;
}

/*****************************************************************************
 *
 *   module init and exit functions
 *
 ****************************************************************************/

static int mss_core_driver_init(void)
{
	return bus_register(&mmc_bus_type);
}

static void mss_core_driver_exit(void)
{
	bus_unregister(&mmc_bus_type);
}


EXPORT_SYMBOL_GPL(mss_detect_change);
EXPORT_SYMBOL_GPL(mss_scan_slot);
EXPORT_SYMBOL_GPL(mss_scan_host);
EXPORT_SYMBOL_GPL(mss_alloc_host);
EXPORT_SYMBOL_GPL(mss_free_host);
EXPORT_SYMBOL_GPL(mss_find_host);
EXPORT_SYMBOL_GPL(mss_force_card_remove);
EXPORT_SYMBOL_GPL(register_mss_host);
EXPORT_SYMBOL_GPL(unregister_mss_host);

EXPORT_SYMBOL_GPL(register_mss_driver);
EXPORT_SYMBOL_GPL(unregister_mss_driver);
EXPORT_SYMBOL_GPL(mss_send_request);
EXPORT_SYMBOL_GPL(mss_get_capacity);

EXPORT_SYMBOL_GPL(register_mss_prot_driver);
EXPORT_SYMBOL_GPL(unregister_mss_prot_driver);
EXPORT_SYMBOL_GPL(mss_send_ll_req);
EXPORT_SYMBOL_GPL(mss_send_simple_ll_req);
EXPORT_SYMBOL_GPL(mss_set_sdio_int);
EXPORT_SYMBOL_GPL(mss_set_buswidth);
EXPORT_SYMBOL_GPL(mss_set_clock);
EXPORT_SYMBOL_GPL(mss_set_busmode);
EXPORT_SYMBOL_GPL(mss_card_get);
EXPORT_SYMBOL_GPL(mss_card_put);

module_init(mss_core_driver_init);
module_exit(mss_core_driver_exit);

MODULE_AUTHOR("Bridge Wu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Core driver for MMC/SD/SDIO card");
