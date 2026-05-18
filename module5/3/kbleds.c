#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <linux/console_struct.h>
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define KOBJ_NAME "kbleds"
#define RESTORE_LEDS 0xFF
#define BLINK_DELAY HZ / 4

static struct kobject *kbleds_kobj;
static struct tty_driver *my_driver;
static struct timer_list blink_timer;
static char blink_state;
static unsigned int led_mask;

static void set_leds(unsigned int value)
{
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty,
                            KDSETLED, value);
}

static void blink_callback(struct timer_list *t)
{
    if (led_mask) {
        if (blink_state) {
            set_leds(0);
            blink_state = 0;
        } else {
            set_leds(led_mask);
            blink_state = 1;
        }
    } else {
        set_leds(RESTORE_LEDS);
    }

    mod_timer(&blink_timer, jiffies + BLINK_DELAY);
}

static ssize_t mask_show(struct kobject *kobj, struct kobj_attribute *attr,
                         char *buf)
{
    return sysfs_emit(buf, "%u\n", led_mask);
}

static ssize_t mask_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
        return ret;

    if (val > 7)
        val = 7;

    led_mask = val;
    return count;
}

static struct kobj_attribute mask_attribute =
    __ATTR(mask, 0660, mask_show, mask_store);

static int __init kbleds_init(void)
{
    int ret;

    my_driver = vc_cons[fg_console].d->port.tty->driver;
    if (!my_driver)
        return -ENODEV;

    kbleds_kobj = kobject_create_and_add(KOBJ_NAME, kernel_kobj);
    if (!kbleds_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(kbleds_kobj, &mask_attribute.attr);
    if (ret) {
        kobject_put(kbleds_kobj);
        return ret;
    }

    timer_setup(&blink_timer, blink_callback, 0);
    blink_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&blink_timer);

    pr_info("kbleds: loaded. Control via /sys/kernel/%s/mask\n", KOBJ_NAME);
    return 0;
}

static void __exit kbleds_exit(void)
{
    timer_delete_sync(&blink_timer);
    set_leds(RESTORE_LEDS);
    kobject_put(kbleds_kobj);
    pr_info("kbleds: unloaded\n");
}

module_init(kbleds_init);
module_exit(kbleds_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ptfn");
MODULE_DESCRIPTION("eltex module5 task3: keyboard LEDs blink via sysfs mask");
