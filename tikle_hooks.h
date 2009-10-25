#ifndef TIKLE_HOOKS_H
#define TIKLE_HOOKS_H

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
