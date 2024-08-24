#include <linux/module.h>
#include <linux/init.h>
#include <linux/tty.h>         
#include <linux/kd.h>    
#include <linux/vt.h>
#include <linux/console_struct.h>  
#include <linux/vt_kern.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>    

MODULE_DESCRIPTION("Example module illustrating the use of Keyboard LEDs with sysfs.");
MODULE_LICENSE("GPL");

static struct timer_list my_timer;
static struct tty_driver *my_driver;

static int _kbledstatus = 0;  // Текущее состояние светодиодов
static unsigned int blink_state = 0;  // Текущее состояние для мигания
#define BLINK_DELAY   HZ/5
#define RESTORE_LEDS  0xFF

// Функция таймера для мигания светодиодов
static void my_timer_func(struct timer_list *ptr) {
    blink_state = !blink_state;  // Переключаем состояние мигания

    // Устанавливаем светодиоды
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, blink_state ? _kbledstatus : RESTORE_LEDS);

    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);
}

// Функция для отображения текущего состояния светодиодов
static ssize_t kbleds_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "0x%02x\n", _kbledstatus);
}

// Функция для установки нового состояния светодиодов
static ssize_t kbleds_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    unsigned int value;

    if (sscanf(buf, "0x%x", &value) == 1) {
        _kbledstatus = value;
        blink_state = 0;
        (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, _kbledstatus);
    }
    return count;
}

static struct kobj_attribute kbleds_attribute = __ATTR(kbleds, 0664, kbleds_show, kbleds_store);

static struct kobject *kbleds_kobj;

static int __init kbleds_init(void) {
    int result;

    printk(KERN_INFO "kbleds: loading\n");

    my_driver = vc_cons[fg_console].d->port.tty->driver;

    timer_setup(&my_timer, my_timer_func, 0);
    my_timer.expires = jiffies + BLINK_DELAY;
    add_timer(&my_timer);

    kbleds_kobj = kobject_create_and_add("kbleds", kernel_kobj);
    if (!kbleds_kobj) {
        return -ENOMEM;
    }
    result = sysfs_create_file(kbleds_kobj, &kbleds_attribute.attr);
    if (result) {
        kobject_put(kbleds_kobj);
        return result;
    }
    return 0;
}

static void __exit kbleds_cleanup(void) {
    printk(KERN_INFO "kbleds: unloading...\n");
    del_timer(&my_timer);
    (my_driver->ops->ioctl)(vc_cons[fg_console].d->port.tty, KDSETLED, RESTORE_LEDS);
    sysfs_remove_file(kbleds_kobj, &kbleds_attribute.attr);
    kobject_put(kbleds_kobj);
}

module_init(kbleds_init);
module_exit(kbleds_cleanup);
