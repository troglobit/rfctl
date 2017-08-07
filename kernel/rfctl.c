/* Transmit and record pulse and pause lengths using GPIO
 *
 * Based on rfbb.c by Tord Andersson, which in turn is based on
 * lirc_serial.c by Ralph Metzler et al.
 *
 * Copyright (C) 2010, 2012  Tord Andersson <tord.andersson@endian.se>
 * Copyright (C) 2017        Joachim Nilsson <troglobit@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, visit the Free Software Foundation
 * website at http://www.gnu.org/licenses/gpl-2.0.html or write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

#define DRIVER_VERSION       "1.0"
#define DRIVER_NAME          "rfctl"

#define HW_MODE_POWER_DOWN   0	/* Transceiver in power down mode */
#define HW_MODE_RX           1
#define HW_MODE_TX           2

/* Borrowed LIRC definitions */
#define LIRC_MODE2_SPACE     0x00000000
#define LIRC_MODE2_PULSE     0x01000000
#define LIRC_MODE2_TIMEOUT   0x03000000

#define LIRC_VALUE_MASK      0x00FFFFFF
#define LIRC_MODE2_MASK      0xFF000000

/*
 * We export one device, /dev/rfctl
 */
static struct cdev rfctl_dev;

#define NO_GPIO_PIN             -1
#define NO_RX_IRQ               -1

#define DEFAULT_GPIO_IN_PIN     NO_GPIO_PIN // 27
#define DEFAULT_GPIO_OUT_PIN    17

static int dev_major = 0;	/* use dynamic major number assignment */

static int share_irq = 0;
static int interrupt_enabled = 0;
static bool debug = false;
static int device_open = 0;
static int hw_mode = HW_MODE_POWER_DOWN;

static DEFINE_MUTEX(read_lock);

#define xstringify(s) stringify(s)
#define stringify(s) #s

#define errx(fmt, args...) \
	printk(KERN_ERR DRIVER_NAME ": " fmt, ##args)
#define warnx(fmt, args...) \
	printk(KERN_WARNING DRIVER_NAME ": " fmt, ##args)
#define info(fmt, args...) \
	printk(KERN_INFO DRIVER_NAME ": " fmt, ##args)
#define dbg(fmt, args...)						\
	if (debug)							\
		printk(KERN_DEBUG DRIVER_NAME ": %s() " fmt, __func__, ##args)

/* forward declarations */
static void set_tx_mode(void);	/* set up transceiver for transmission */
static void set_rx_mode(void);	/* set up transceiver for reception */
static void on(void);		/* TX signal on */
static void off(void);		/* TX signal off */
static void rfctl_exit_module(void);

static int gpio_out_pin  = DEFAULT_GPIO_OUT_PIN;
static int gpio_in_pin   = DEFAULT_GPIO_IN_PIN;
static int tx_ctrl_pin   = NO_GPIO_PIN; /* not used */
static int rf_enable_pin = NO_GPIO_PIN; /* not used */

#define RS_ISR_PASS_LIMIT 256

/*
 * A long pulse code from a remote might take up to 300 bytes.  The
 * daemon should read the bytes as soon as they are generated, so take
 * the number of keys you think you can push before the daemon runs
 * and multiply by 300.  The driver will warn you if you overrun this
 * buffer.  If you have a slow computer or non-busmastering IDE disks,
 * maybe you will need to increase this.
 */

#define RBUF_LEN 4096
#define WBUF_LEN 4096

static int sense = 0;		/* -1 = auto, 0 = active high, 1 = active low */

static int irq = NO_RX_IRQ;

static struct timeval lasttv = { 0, 0 };

/* static struct lirc_buffer rbuf; */

/* Use FIFO to store received pulses */
static DEFINE_KFIFO(rxfifo, int32_t, RBUF_LEN);

static int32_t wbuf[WBUF_LEN];

/* AUREL RTX-MID transceiver TX setup sequence
   will use rf_enable as well as tx_ctrl pins.
   Not used for simple TX modules */
static void set_tx_mode(void)
{
	off();
	switch (hw_mode) {
	case HW_MODE_POWER_DOWN:
		if (rf_enable_pin != NO_GPIO_PIN) {
			gpio_set_value(rf_enable_pin, 1);
			udelay(20);
		}
		if (tx_ctrl_pin != NO_GPIO_PIN) {
			gpio_set_value(tx_ctrl_pin, 1);
			udelay(400);	/* let it settle */
		}
		break;

	case HW_MODE_TX:
		/* do nothing */
		break;

	case HW_MODE_RX:
		if (rf_enable_pin != NO_GPIO_PIN) {
			gpio_set_value(rf_enable_pin, 1);
		}
		if (tx_ctrl_pin != NO_GPIO_PIN) {
			gpio_set_value(tx_ctrl_pin, 1);
			udelay(400);	/* let it settle */
		}
		break;

	default:
		errx("%s: Illegal HW mode %d\n", __func__, hw_mode);
		break;
	}

	hw_mode = HW_MODE_TX;
}

/* AUREL RTX-MID transceiver RX setup sequence */
static void set_rx_mode(void)
{
	off();
	switch (hw_mode) {
	case HW_MODE_POWER_DOWN:
		/* Note this sequence is only needed for AUREL RTX-MID */
		if (rf_enable_pin != NO_GPIO_PIN && tx_ctrl_pin != NO_GPIO_PIN) {
			gpio_set_value(rf_enable_pin, 1);
			gpio_set_value(tx_ctrl_pin, 0);
			udelay(20);
			gpio_set_value(tx_ctrl_pin, 1);
			udelay(200);
			gpio_set_value(tx_ctrl_pin, 0);
			udelay(40);
			gpio_set_value(rf_enable_pin, 0);
			udelay(20);
			gpio_set_value(rf_enable_pin, 1);
			udelay(200);
		}
		break;

	case HW_MODE_RX:
		/* do nothing */
		break;

	case HW_MODE_TX:
		if (tx_ctrl_pin != NO_GPIO_PIN) {
			gpio_set_value(tx_ctrl_pin, 0);
			udelay(40);
		}
		if (rf_enable_pin != NO_GPIO_PIN) {
			gpio_set_value(rf_enable_pin, 0);
			udelay(20);
			gpio_set_value(rf_enable_pin, 1);
			udelay(200);
		}
		break;

	default:
		errx("set_rx_mode. Illegal HW mode %d\n", hw_mode);
		break;
	}

	hw_mode = HW_MODE_RX;
}

static void on(void)
{
	gpio_set_value(gpio_out_pin, 1);
}

static void off(void)
{
	gpio_set_value(gpio_out_pin, 0);
}

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_US 5000
#else
#define MAX_UDELAY_US (MAX_UDELAY_MS*1000)
#endif

static void safe_udelay(unsigned long usecs)
{
	while (usecs > MAX_UDELAY_US) {
		udelay(MAX_UDELAY_US);
		usecs -= MAX_UDELAY_US;
	}
	udelay(usecs);
}

static void send_pulse_gpio(unsigned long length)
{
	on();
	/* dbg("%ld us\n", length); */
	safe_udelay(length);
}

static void send_space_gpio(unsigned long length)
{
	off();
	/* dbg("%ld us\n", length); */
	safe_udelay(length);
}

static irqreturn_t irq_handler(int i, void *blah)
{
	struct timeval tv;
	int status;
	long deltv;
	int32_t data = 0;
	static int old_status = -1;
	static int counter = 0;	/* to find burst problems */
	/* static int intCount = 0; */

	status = gpio_get_value(gpio_in_pin);
	if (status == old_status) {
		/* could have been a spike */
		counter++;
		if (counter > RS_ISR_PASS_LIMIT) {
			warnx("AIEEEE: " "We're caught!\n");
			counter = 0;	/* to avoid flooding warnings */
		}

		goto leave;
	}

	counter = 0;

	/* get current time */
	do_gettimeofday(&tv);

	/* New mode, written by Trent Piepho
	   <xyzzy@u.washington.edu>. */

	/*
	 * The old format was not very portable.  We now use an int to
	 * pass pulses and spaces to user space.
	 *
	 * If PULSE_BIT is set a pulse has been received, otherwise a
	 * space has been received.  The driver needs to know if your
	 * receiver is active high or active low, or the space/pulse
	 * sense could be inverted. The bits denoted by PULSE_MASK are
	 * the length in microseconds. Lengths greater than or equal to
	 * 16 seconds are clamped to PULSE_MASK.  All other bits are
	 * unused.  This is a much simpler interface for user programs,
	 * as well as eliminating "out of phase" errors with space/pulse
	 * autodetection.
	 */

	/* calc time since last interrupt in microseconds */
	deltv = tv.tv_sec - lasttv.tv_sec;
	if (tv.tv_sec < lasttv.tv_sec || (tv.tv_sec == lasttv.tv_sec && tv.tv_usec < lasttv.tv_usec)) {
		warnx("AIEEEE: your clock just jumped " "backwards\n");
		warnx("%d %lx %lx %lx %lx\n",
		       sense, tv.tv_sec, lasttv.tv_sec, tv.tv_usec, lasttv.tv_usec);
		data = status ? (data | LIRC_VALUE_MASK) : (data | LIRC_MODE2_PULSE | LIRC_VALUE_MASK);	/* handle as too long time */
	} else if (deltv > 15) {
		data = status ? (data | LIRC_VALUE_MASK) : (data | LIRC_MODE2_PULSE | LIRC_VALUE_MASK);	/* really long time */
	} else {
		data = (int32_t) (deltv * 1000000 + tv.tv_usec - lasttv.tv_usec);
	}

	/* frbwrite(status ? data : (data|PULSE_BIT)); */
	lasttv = tv;
	old_status = status;
	data = status ? data : (data | LIRC_MODE2_PULSE);
	/* dbg("Nr: %d. Pin: %d time: %ld\n", ++intCount, status, (long)(data & PULSE_MASK)); */
	kfifo_put(&rxfifo, data);
	/* wake_up_interruptible(&rbuf.wait_poll); */

leave:
	return IRQ_RETVAL(IRQ_HANDLED);
}

#define gpio_register(pin, io, nm)					\
	if (pin != NO_GPIO_PIN) {					\
		dbg("Registering %s, GPIO %d\n", nm, pin);		\
		err = gpio_request_one(pin, io, nm);			\
		if (err) {						\
			errx("Error %d requesting %s\n", err, nm);	\
			err = -EIO;					\
			goto leave;					\
		}							\
	}

#define gpio_expose(pin, dir, nm)					\
	if (pin != NO_GPIO_PIN && debug) {				\
		dbg("Exporting %s, GPIO %d to sysfs\n", nm, pin);	\
		err = gpio_export(pin, dir);				\
		if (err)						\
			errx("Error %d exporting %s\n", err, nm);	\
	}

#define gpio_direction(pin, io, nm)					\
	if (pin != NO_GPIO_PIN) {					\
		dbg("Setting %s, GPIO %d, dir %d\n", nm, pin, io);	\
		if (io)							\
			err = gpio_direction_input(pin);		\
		else							\
			err = gpio_direction_output(pin, 0);		\
		if (err)						\
			errx("Error %d setting %s dir\n", err, nm);	\
	}

static int gpio_init(void)
{
	unsigned long flags;
	int err = 0;

	/* First of all, disable all interrupts */
	local_irq_save(flags);

	/* Setup all pins */
	gpio_register(gpio_out_pin,  GPIOF_OUT_INIT_LOW, "TX");
	gpio_register(gpio_in_pin,   GPIOF_IN,           "RX");
	gpio_register(tx_ctrl_pin,   GPIOF_OUT_INIT_LOW, "TX_CTRL");
	gpio_register(rf_enable_pin, GPIOF_OUT_INIT_LOW, "RF_ENABLE");

	/* Set I/O direction */
	gpio_direction(gpio_out_pin,  0, "TX");
	gpio_direction(gpio_in_pin,   1, "RX");
	gpio_direction(tx_ctrl_pin,   0, "TX_CTRL");
	gpio_direction(rf_enable_pin, 0, "RF_ENABLE");

	/* Get interrupt for RX */
	if (gpio_in_pin != NO_GPIO_PIN) {
		irq = gpio_to_irq(gpio_in_pin);
		dbg("Interrupt %d for RX pin\n", irq);
	}

	/* Export pins and make them able to change from sysfs for troubleshooting */
	gpio_expose(gpio_out_pin,  1, "TX");
	gpio_expose(gpio_in_pin,   0, "RX");
	gpio_expose(tx_ctrl_pin,   1, "TX_CTRL");
	gpio_expose(rf_enable_pin, 1, "RF_ENABLE");

	/* Start in TX mode, avoid interrupts */
	set_tx_mode();

leave:
	local_irq_restore(flags);
	return 0;
}

static ssize_t rfctl_read(struct file *filp, char *buf, size_t length, loff_t *offset)
{
	int ret = 0;
	unsigned int copied = 0;

	set_rx_mode();
	if (!interrupt_enabled) {
		//enable_irq(irq);
		interrupt_enabled = 1;
	}

	/* might need mutex */
	ret = kfifo_to_user(&rxfifo, buf, length, &copied);

	dbg("request %zd bytes, result %d, copied bytes %u\n", length, ret, copied);

	return (ssize_t)(ret ? ret : copied);
}

static ssize_t rfctl_write(struct file *file, const char *buf, size_t n, loff_t *ppos)
{
	int i, err, count;
	unsigned long flags;

	if (interrupt_enabled) {
		//disable_irq(irq);
		interrupt_enabled = 0;
	}
	set_tx_mode();

	/* Workaround, TX pin gets reset to input in long-time test */
	gpio_direction(gpio_out_pin,  0, "TX");

	dbg("%zd bytes\n", n);

	if (n % sizeof(int32_t))
		return -EINVAL;

	count = n / sizeof(int32_t);
	if (count > WBUF_LEN) {
		errx("Too many elements (%d) in TX buffer, max %d\n", count, WBUF_LEN);
		return -EINVAL;
	}

	err = copy_from_user(wbuf, buf, n);
	if (err) {
		errx("Failed copy_from_user() TX buffer, err %d\n", err);
		return -EFAULT;
	}

	local_irq_save(flags);
	for (i = 0; i < count; i++) {
		if (wbuf[i] & LIRC_MODE2_PULSE)
			send_pulse_gpio(wbuf[i] & LIRC_VALUE_MASK);
		else
			send_space_gpio(wbuf[i] & LIRC_VALUE_MASK);
	}
	off();
	local_irq_restore(flags);

	return n;
}

static long rfctl_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static int rfctl_open(struct inode *ino, struct file *filep)
{
	int result;
	unsigned long flags;

	if (device_open) {
		errx("Already opened\n");
		return -EBUSY;
	}

	/* initialize timestamp */
	do_gettimeofday(&lasttv);

	if (irq != NO_RX_IRQ) {
		local_irq_save(flags);
		result = request_irq(irq, irq_handler,
				     IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
				     DRIVER_NAME, NULL);

		switch (result) {
		case -EBUSY:
			errx("IRQ %d busy\n", irq);
			break;

		case -EINVAL:
			errx("Bad irq number or handler\n");
			break;

		default:
			dbg("Interrupt %d obtained\n", irq);
			result = 0;
			break;
		};

		local_irq_restore(flags);
		if (result)
			return result;
	}

	if (interrupt_enabled) {
		//disable_irq(irq);
		interrupt_enabled = 0;
	}

	try_module_get(THIS_MODULE);
	device_open++;

	return 0;
}

static int rfctl_close(struct inode *node, struct file *file)
{
	off();

	if (interrupt_enabled) {
		//disable_irq(irq);
		interrupt_enabled = 0;
	}

	/* remove the RX interrupt */
	if (irq != NO_RX_IRQ) {
		free_irq(irq, NULL);
		dbg("Freed RX IRQ %d\n", irq);
	}

	/* lirc_buffer_free(&rbuf); */

	device_open--;		/* We're now ready for our next caller */
	module_put(THIS_MODULE);

	return 0;
}

static struct file_operations rfctl_fops = {
	.owner          = THIS_MODULE,
	.open           = rfctl_open,
	.release        = rfctl_close,
	.write          = rfctl_write,
	.read           = rfctl_read,
	.unlocked_ioctl = rfctl_ioctl,
};

/*
 * Set up the cdev structure for a device.
 */
static int rfctl_setup_cdev(struct cdev *dev, int minor, struct file_operations *fops)
{
	int err, devno = MKDEV(dev_major, minor);
	struct class *class;
	struct device *device;

	class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(class)) {
		err = PTR_ERR(class);
		pr_warn("Unable to create %s class; errno %d\n", DRIVER_NAME, err);
		return err;
	}

	cdev_init(dev, fops);
	dev->owner = THIS_MODULE;
	dev->ops = fops;
	err = cdev_add(dev, devno, 1);
	if (err) {
		warnx("Error %d adding /dev/rfctl %d", err, minor);
		return err;
	}

	device = device_create(class, NULL, /* no parent device   */
			       devno, NULL, /* no additional data */
			       DRIVER_NAME);
	if (IS_ERR(device)) {
		err = PTR_ERR(device);
		pr_warn("Failed creating /dev/%s, errno %d", DRIVER_NAME, err);
		cdev_del(dev);
		return err;
	}

	return 0;
}

static int rfctl_init(void)
{
	int result;
	dev_t dev = 0;

	/*
	 * Dynamic major if not set otherwise.
	 */
	if (dev_major) {
		dev = MKDEV(dev_major, 0);
		result = register_chrdev_region(dev, 1, DRIVER_NAME);
	} else {
		result = alloc_chrdev_region(&dev, 0, 1, DRIVER_NAME);
		dev_major = MAJOR(dev);
	}

	if (result < 0) {
		warnx("Failed allocating character device, major %d\n", dev_major);
		return result;
	}

	return rfctl_setup_cdev(&rfctl_dev, 0, &rfctl_fops);
}

static int rfctl_init_module(void)
{
	int result;

	result = rfctl_init();
	if (result)
		goto leave;

	result = gpio_init();
	if (result < 0)
		goto leave;

	info("%s %s registered\n", DRIVER_NAME, DRIVER_VERSION);
	dbg("dev major = %d\n", dev_major);
	dbg("IRQ = %d\n", irq);
	dbg("share_irq = %d\n", share_irq);

	return 0;

leave:
	rfctl_exit_module();
	return result;
}

static void rfctl_exit_module(void)
{
	cdev_del(&rfctl_dev);
	unregister_chrdev_region(MKDEV(dev_major, 0), 1);

	if (gpio_out_pin != NO_GPIO_PIN) {
		gpio_unexport(gpio_out_pin);
		gpio_free(gpio_out_pin);
	}

	if (tx_ctrl_pin != NO_GPIO_PIN) {
		gpio_unexport(tx_ctrl_pin);
		gpio_free(tx_ctrl_pin);
	}

	if (gpio_in_pin != NO_GPIO_PIN) {
		gpio_unexport(gpio_in_pin);
		gpio_free(gpio_in_pin);
	}

	info("%s %s unregistered\n", DRIVER_NAME, DRIVER_VERSION);
}

module_init(rfctl_init_module);
module_exit(rfctl_exit_module);

MODULE_DESCRIPTION("RF Tx/Rx driver for Raspberry Pi GPIO");
MODULE_AUTHOR("Tord Andersson, Joachim Nilsson");
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Enable debugging messages");

module_param(gpio_out_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_out_pin, "GPIO output (Tx) pin of the BCM"
		 " processor. (default " xstringify(DEFAULT_GPIO_OUT_PIN) ")");

module_param(gpio_in_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_in_pin, "GPIO input (Rx) pin number of the BCM processor."
		 " (default " xstringify(DEFAULT_GPIO_IN_PIN) ")");
