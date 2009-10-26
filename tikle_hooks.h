/* 
 * Tikle kernel module
 * Copyright (C) 2009  Felipe 'ecl' Pena
 *                     Gustavo 'nst' Oliveira
 * 
 * Contact us at: #c2zlabs@freenode.net - www.c2zlabs.com
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

#ifndef TIKLE_HOOKS_H
#define TIKLE_HOOKS_H

#define tikle_log_daddr(_i) _i*5
#define tikle_log_saddr(_i) _i*5+1
#define tikle_log_event(_i) _i*5+2
#define tikle_log_in(_i)    _i*5+3
#define tikle_log_out(_i)   _i*5+4

unsigned int tikle_pre_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *));

unsigned int tikle_post_hook_function(unsigned int hooknum,
				 struct sk_buff *pskb,
				 const struct net_device *indev,
				 const struct net_device *outdev,
				 int (*okfn)(struct sk_buff *));
				 
#endif /* TIKLE_HOOKS_H */
