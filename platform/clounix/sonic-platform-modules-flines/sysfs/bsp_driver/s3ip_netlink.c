#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/string.h>

#include <net/sock.h>
#include <net/net_namespace.h>

#include "clounix/s3ip_netlink.h"

void s3ip_nl_rcv_skb(struct sk_buff *skb);

static struct sock *s3ip_sock = NULL;

static struct netlink_kernel_cfg cfg = {
    .input  = s3ip_nl_rcv_skb,
};

int send_s3ip_msg(char *pbuf, unsigned short len)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    int ret;
    
    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb)
    {
        printk(KERN_ALERT "netlink alloc failure\n");
        return -ENOMEM;
    }
    
    nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_S3IP, len, 0);
    if(nlh == NULL)
    {
        printk(KERN_ALERT "nlmsg_put failaure\n");
        nlmsg_free(nl_skb);
        return -ENOBUFS;
    }
 
    memcpy(nlmsg_data(nlh), pbuf, len);
    ret = netlink_unicast(s3ip_sock, nl_skb, S3IP_PORT, MSG_DONTWAIT);
 
    return ret;

}
EXPORT_SYMBOL_GPL(send_s3ip_msg);

void s3ip_nl_rcv_skb(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    unsigned char *msg;
    unsigned char *kmsg = "this is kernel";

    nlh = nlmsg_hdr(skb);
    msg = NLMSG_DATA(nlh);
    
    if (msg) {
        printk(KERN_ALERT "kernel rcv msg form port:%d\n", NETLINK_CB(skb).portid);
        printk(KERN_ALERT "kernel rcv: %s\n", msg);
        send_s3ip_msg(kmsg, strlen(kmsg));
    }

    return;
}

static int __init s3ip_netlink_init(void)
{
    s3ip_sock = netlink_kernel_create(&init_net, NETLINK_S3IP, &cfg);
    if (s3ip_sock == NULL)
        return -ENOMEM;

    return 0;
}

static void __exit s3ip_netlink_exit(void)
{
    netlink_kernel_release(s3ip_sock);

    return;
}

module_init(s3ip_netlink_init);
module_exit(s3ip_netlink_exit);

MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("s3ip proxy netlink");
MODULE_LICENSE("GPL v2");
