/** 
 * Tikle kernel module
 * Copyright (C) 2009	Felipe 'ecl' Pena
 *			Gustavo 'nst' Oliveira
 * 
 * Contact us at: flipensp@gmail.com
 * 		  gmoliveira@inf.ufrgs.br
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Also thanks to Higor 'enygmata' Euripedes
 */

#ifndef LOW_DEFS_H
#define LOW_DEFS_H

#include "low_types.h"

//#include <linux/sched.h> /* struct task_struct */
//#include <linux/kthread.h>
/**
 * Macro to make errors easy to identify
 */
#define TERROR(fmt, ...) printk(KERN_ERR "tikle error: " fmt, ##__VA_ARGS_)
#define TINFO(fmt, ...) printk(KERN_INFO "tikle info: " fmt, ##__VA_ARGS_)
#define TDPRINT(fmt, ...) printk(KERN_INFO "tikle info: " fmt, ##__VA_ARGS_)
#define TDEBUG(fmt, ...) printk(KERN_DEBUG "tikle debug: " fmt, ##__VA_ARGS_)

#define NUM_FAULTLOAD_OP 10

/**
 * main thread definition
 */
extern struct task_struct *main_thread;

extern procfs_t *sysfs;

//extern faultload_op faultload[30];

#endif
