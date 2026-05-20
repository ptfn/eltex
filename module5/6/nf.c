#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/inet.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define KOBJ_NAME "nf_blacklist"

static struct kobject *nf_kobj;
static struct nf_hook_ops nf_out;
static char *blacklist[256];
static int blacklist_count;

static int is_ip_blacklisted(__be32 ip)
{
    int i;
    for (i = 0; i < blacklist_count; i++) {
        __be32 addr;
        if (blacklist[i] && in4_pton(blacklist[i], -1, (u8 *)&addr, -1, NULL) == 1 && addr == ip)
            return 1;
    }
    return 0;
}

static unsigned int hook_func(void *priv, struct sk_buff *skb,
                              const struct nf_hook_state *state)
{
    struct iphdr *ip_header;

    if (!skb)
        return NF_ACCEPT;

    ip_header = (struct iphdr *)skb_network_header(skb);
    if (!ip_header)
        return NF_ACCEPT;

    if (is_ip_blacklisted(ip_header->daddr)) {
        pr_info("nf: blocked %pI4\n", &ip_header->daddr);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

static ssize_t blacklist_show(struct kobject *kobj, struct kobj_attribute *attr,
                              char *buf)
{
    int pos = 0, i;
    for (i = 0; i < blacklist_count; i++)
        if (blacklist[i])
            pos += sysfs_emit_at(buf, pos, "%s\n", blacklist[i]);
    return pos;
}

static ssize_t blacklist_store(struct kobject *kobj, struct kobj_attribute *attr,
                               const char *buf, size_t count)
{
    char ip[32];
    ssize_t len = count < (ssize_t)sizeof(ip) - 1 ? count : sizeof(ip) - 1;

    memcpy(ip, buf, len);
    ip[len] = '\0';
    if (ip[len - 1] == '\n')
        ip[len - 1] = '\0';

    if (ip[0] == '+') {
        if (blacklist_count >= 256)
            return -ENOSPC;
        blacklist[blacklist_count] = kstrdup(ip + 1, GFP_KERNEL);
        if (!blacklist[blacklist_count])
            return -ENOMEM;
        blacklist_count++;
        pr_info("nf: added %s\n", ip + 1);
    } else if (ip[0] == '-') {
        int i;
        for (i = 0; i < blacklist_count; i++) {
            if (blacklist[i] && strcmp(blacklist[i], ip + 1) == 0) {
                kfree(blacklist[i]);
                blacklist[i] = blacklist[--blacklist_count];
                blacklist[blacklist_count] = NULL;
                pr_info("nf: removed %s\n", ip + 1);
                break;
            }
        }
    }

    return count;
}

static struct kobj_attribute blacklist_attribute =
    __ATTR(blacklist, 0660, blacklist_show, blacklist_store);

static int __init nf_init(void)
{
    int ret;

    nf_kobj = kobject_create_and_add(KOBJ_NAME, kernel_kobj);
    if (!nf_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(nf_kobj, &blacklist_attribute.attr);
    if (ret) {
        kobject_put(nf_kobj);
        return ret;
    }

    nf_out.hook     = hook_func;
    nf_out.hooknum  = NF_INET_LOCAL_OUT;
    nf_out.pf       = PF_INET;
    nf_out.priority = NF_IP_PRI_FIRST;

    if (nf_register_net_hook(&init_net, &nf_out)) {
        kobject_put(nf_kobj);
        return -ENOMEM;
    }

    pr_info("nf: loaded. Use /sys/kernel/%s/blacklist\n", KOBJ_NAME);
    return 0;
}

static void __exit nf_exit(void)
{
    int i;
    nf_unregister_net_hook(&init_net, &nf_out);
    for (i = 0; i < blacklist_count; i++)
        kfree(blacklist[i]);
    kobject_put(nf_kobj);
    pr_info("nf: unloaded\n");
}

module_init(nf_init);
module_exit(nf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ptfn");
MODULE_DESCRIPTION("eltex module5 task6: netfilter IP blacklist");
