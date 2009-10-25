#include <linux/kernel.h>
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/version.h>
#include <linux/netfilter.h> /* kernel protocol stack */
#include <linux/netdevice.h> /* SOCK_DGRAM, KERNEL_DS an others */
#undef __KERNEL__
#include <linux/netfilter_ipv4.h>
#include "tikle_hooks.h"
#include "tikle_defs.h"

struct nf_hook_ops tikle_pre_hook, tikle_post_hook;
int num_ips, log_size;

/**
 * after ending experiment, host must
 * send your counters to controller
 */
static void tikle_send_log(void)
{
	int tikle_err;

	if ((tikle_err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &tikle_comm->sock_log)) < 0) {
		printk(KERN_ERR "tikle alert: error %d while creating sockudp\n", -ENXIO);
	}

	memset(&tikle_comm->addr_log, 0, sizeof(struct sockaddr));
	tikle_comm->addr_log.sin_family = AF_INET;
	tikle_comm->addr_log.sin_addr.s_addr = tikle_comm->addr_recv.sin_addr.s_addr;
	tikle_comm->addr_log.sin_port = htons(PORT_LOGGING);

	if ((tikle_err = tikle_comm->sock_send->ops->connect(tikle_comm->sock_send,(struct sockaddr *)&tikle_comm->addr_send, sizeof(struct sockaddr), 0)) < 0) {
		printk(KERN_ERR "tikle alert: error %d while connecting to socket\n", -tikle_err);
		sock_release(tikle_comm->sock_send);
		tikle_comm->sock_log = NULL; 
	}

	tikle_sockudp_send(tikle_comm->sock_log, &tikle_comm->addr_log, tikle_log_counters, sizeof(unsigned long) * log_size);
}

/**
 * Setup trigger handling
 * @return void
 */
void tikle_trigger_handling(void)
{
	unsigned int i = 0, base;

	/*
	 * setting up triggers
 	 */
	printk(KERN_INFO "TRIGGERS:\n");

	for (i = 0; i < tikle_num_timers; i++) {
		init_timer(&tikle_timers[i].trigger);

		tikle_timers[i].trigger.function = tikle_flag_handling;

		/*
		 * send the next trigger ID, if actual trigger it is
		 * not the latest, else send your own trigger ID
		 */
		if (tikle_timers[i].trigger_id < tikle_timers[tikle_num_timers-1].trigger_id) {
			tikle_timers[i].trigger.data = tikle_timers[i+1].trigger_id;
		} else {
			tikle_timers[i].trigger.data = tikle_timers[i].trigger_id;
		}
		
		printk(KERN_INFO "Registering timer %d; trigger_id=%d ; trigger_data=%ld\n",
			i, tikle_timers[i].trigger_id, tikle_timers[i].trigger.data);

		/*
		 * calculate expire time to trigger
		 * based on the experiment start time
		 */		
		base = jiffies;
		tikle_timers[i].trigger.expires = base +
			msecs_to_jiffies(faultload[tikle_timers[i].trigger_id].op_value[0].num * 1000);

		if (tikle_timers[i].trigger_id == 0) { /* if timer corresponds to the first timer */

			/*
			 * register hook
			 */
			tikle_pre_hook.hook = &tikle_pre_hook_function;
			tikle_pre_hook.hooknum = NF_IP_PRE_ROUTING;
			tikle_pre_hook.pf = PF_INET;
			tikle_pre_hook.priority = NF_IP_PRI_FIRST;

			nf_register_hook(&tikle_pre_hook);

			tikle_post_hook.hook = &tikle_post_hook_function;
			tikle_post_hook.hooknum = NF_IP_POST_ROUTING;
			tikle_post_hook.pf = PF_INET;
			tikle_post_hook.priority = NF_IP_PRI_FIRST;

			nf_register_hook(&tikle_post_hook);

			//nf_register_queue_handler(PF_INET, &qh);

			/*
			 * if timer corresponds
			 * to the last timer
			 */
		} else if (tikle_timers[i].trigger_id == tikle_timers[tikle_num_timers-1].trigger_id) {			
			/*
			 * unregister hook timer
			 * this is done by a thread
			 * to avoid console freezing
			 */
			/* not handled here */
		}
		add_timer(&tikle_timers[i].trigger);
	}
	printk(KERN_INFO "--------------------------------\n");
}

/**
 * Update flags to maintain a timer execution control.
 *
 * @param id active trigger id
 * @return void
 */
void tikle_flag_handling(unsigned long id)
{
	/*
	 * set the previous timer as 'killed'
	 * and update the system flags with the
	 * running trigger informations
	 */
	if (id > 0 && tikle_timers[tikle_trigger_flag].trigger_state == 1) {
		tikle_timers[tikle_trigger_flag].trigger_state = 2;
		printk(KERN_INFO "tikle alert: killing timer %d\n", tikle_trigger_flag);

		/*
		 * Go to next timer
		 */
		tikle_trigger_flag++;	
	}
	
	printk(KERN_INFO "tikle alert: activating timer %d\n", tikle_trigger_flag);

	/* 
	 * Set state to active
	 */
	tikle_timers[tikle_trigger_flag].trigger_state = 1;

	/*
	 * stop trigger handler
	 */
	if (tikle_trigger_flag == tikle_num_timers-1) {
		nf_unregister_hook(&tikle_pre_hook);
		nf_unregister_hook(&tikle_post_hook);

		//nf_unregister_queue_handler(PF_INET, &qh);

		printk(KERN_INFO "tikle alert: killing timer %d\n\texecution ended\n", tikle_trigger_flag);

		tikle_send_log();
	}
}
