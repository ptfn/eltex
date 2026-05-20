#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31

static struct sock *nl_sk;

static void nl_recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int pid, res;

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;
    pr_info("netlink: received from pid %d\n", pid);

    skb_out = nlmsg_new(256, GFP_KERNEL);
    if (!skb_out)
        return;

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, 256, 0);
    if (!nlh) {
        kfree_skb(skb_out);
        return;
    }

    strcpy((char *)nlmsg_data(nlh), "Hello from kernel");
    NETLINK_CB(skb_out).dst_group = 0;

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res)
        pr_err("netlink: unicast failed %d\n", res);
}

static struct netlink_kernel_cfg nl_cfg = {
    .input = nl_recv_msg,
};

static int __init netlink_init(void)
{
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &nl_cfg);
    if (!nl_sk) {
        pr_err("netlink: socket create failed\n");
        return -ENOMEM;
    }

    pr_info("netlink: loaded (NETLINK_USER=%d)\n", NETLINK_USER);
    return 0;
}

static void __exit netlink_exit(void)
{
    netlink_kernel_release(nl_sk);
    pr_info("netlink: unloaded\n");
}

module_init(netlink_init);
module_exit(netlink_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ptfn");
MODULE_DESCRIPTION("eltex module5 task5: netlink example");
