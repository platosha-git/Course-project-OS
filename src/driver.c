#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "config.h"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

struct usb_mouse 
{
	char name[128];
	char phys[64];

	struct usb_device *usbdev;
	struct input_dev *dev;
	struct urb *irq;

	signed char *data;
	dma_addr_t data_dma;
};

struct task_struct *play_task;

struct button_click
{
	bool left;
	bool middle;
	bool right;
};

static struct button_click click = {
	.left = false,
	.middle = false,
	.right = false
};

struct button_status
{
	bool left;
	bool middle;
	bool right;
};

static struct button_status play = {
	.left = false,
	.middle = false,
	.right = false
};

void play_sound(const bool pressed)
{
	if (pressed) {
		call_usermodehelper(press_argv[0], press_argv, envp, UMH_NO_WAIT);
		printk(KERN_INFO "+ sound played due to press!\n");
	}
	else {
		call_usermodehelper(release_argv[0], release_argv, envp, UMH_NO_WAIT);
		printk(KERN_INFO "+ sound played due to release!\n");
	}
}

static int play_handler(void *arg) 
{
	while (!kthread_should_stop()) {
		if (play.left) {
			play_sound(click.left);
			play.left = false;
		}

		if (play.middle) {
			play_sound(click.middle);
			play.middle = false;
		}

		if (play.right) {
			play_sound(click.right);
			play.right = false;
		}

		/* Поток засыпает, чтобы освободить процессор */
		usleep_range(DELAY_LO, DELAY_HI);
	}

    return 0;
}

void set_mouse_status(const int cur_btn)
{
	if (click.left && !(cur_btn & LEFT_BTN_BIT)) {
		click.left = false;
		play.left  = true;
	} 
	else if (!click.left && (cur_btn & LEFT_BTN_BIT)) {
		click.left = true;
		play.left  = true;
	}

	if (click.middle && !(cur_btn & MIDL_BTN_BIT)) {
		click.middle = false;
		play.middle  = true;
	} 
	else if (!click.middle && (cur_btn & MIDL_BTN_BIT)) {
		click.middle = true;
		play.middle  = true;
	}

	if (click.right && !(cur_btn & RGHT_BTN_BIT)) {
		click.right = false;
		play.right  = true;
	} 
	else if (!click.right && (cur_btn & RGHT_BTN_BIT)) {
		click.right = true;
		play.right  = true;
	}
}

static void resubmit(struct urb *urb, struct usb_mouse *mouse)
{
	int status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		dev_err(&mouse->usbdev->dev, "can't resubmit intr, %s-%s/input0, status %d\n",
				mouse->usbdev->bus->bus_name,
				mouse->usbdev->devpath, status);
	}
}

static void usb_mouse_irq(struct urb *urb) 
{
	struct usb_mouse *mouse = urb->context;
	signed char *data = mouse->data;
	struct input_dev *dev = mouse->dev;

	switch (urb->status) {
	case 0:
		break;

	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		return;

	default:
		resubmit(urb, mouse);
		return;
	}

	set_mouse_status(data[0]);

	input_report_key(dev, BTN_LEFT,   data[0] & LEFT_BTN_BIT);
	input_report_key(dev, BTN_RIGHT,  data[0] & RGHT_BTN_BIT);
	input_report_key(dev, BTN_MIDDLE, data[0] & MIDL_BTN_BIT);
	input_report_key(dev, BTN_SIDE,   data[0] & 0x08);
	input_report_key(dev, BTN_EXTRA,  data[0] & 0x10);

	input_report_rel(dev, REL_X,     data[1]);
	input_report_rel(dev, REL_Y,     data[2]);
	input_report_rel(dev, REL_WHEEL, data[3]);

	input_sync(dev);

	resubmit(urb, mouse);
}

static int usb_mouse_open(struct input_dev *dev) 
{
	struct usb_mouse *mouse = input_get_drvdata(dev);

	mouse->irq->dev = mouse->usbdev;
	if (usb_submit_urb(mouse->irq, GFP_KERNEL)) {
		return -EIO;
	}

	printk(KERN_INFO "+ usb mouse was opened!\n");
	return 0;
}

static void usb_mouse_close(struct input_dev *dev) 
{
	struct usb_mouse *mouse = input_get_drvdata(dev);
	usb_kill_urb(mouse->irq);
	printk(KERN_INFO "+ usb mouse was closed!\n");
}

static void free_devices(struct input_dev *input_dev, struct usb_mouse *mouse,
	struct usb_device *dev)
{
	input_free_device(input_dev);
	kfree(mouse);

	if (dev != NULL) {
		usb_free_coherent(dev, 8, mouse->data, mouse->data_dma);
	}
}

static int usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id) 
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_mouse *mouse;
	struct input_dev *input_dev;
	int pipe, maxp, error;

	interface = intf->cur_altsetting;
	if (interface->desc.bNumEndpoints != 1) {
		return -ENODEV;
	}

	/* Получение информации о конечной точке*/
	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint)) {
		return -ENODEV;
	}

	/* Получение максимального значения пакетных данных */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	/* Аллокация структуры mouse и устройства ввода */
	mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!mouse || !input_dev) {
		free_devices(input_dev, mouse, NULL);
		return -ENOMEM;
	}

	/* Выделение начальной буфферной памяти USB-данных мыши */
	mouse->data = usb_alloc_coherent(dev, 8, GFP_ATOMIC, &mouse->data_dma);
	if (!mouse->data) {
		free_devices(input_dev, mouse, NULL);
		return -ENOMEM;
	}

	/* Аллокация urb */
	mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!mouse->irq) {
		free_devices(input_dev, mouse, dev);
		return -ENOMEM;
	}

	/* Заполнение полей структуры mouse */
	mouse->usbdev = dev;
	mouse->dev = input_dev;

	if (dev->manufacturer) {
		strlcpy(mouse->name, dev->manufacturer, sizeof(mouse->name));
	}

	if (dev->product) {
		if (dev->manufacturer) {
			strlcat(mouse->name, " ", sizeof(mouse->name));
		}
		strlcat(mouse->name, dev->product, sizeof(mouse->name));
	}

	if (!strlen(mouse->name)) {
		snprintf(mouse->name, sizeof(mouse->name),
			 "USB HIDBP Mouse %04x:%04x",
			 le16_to_cpu(dev->descriptor.idVendor),
			 le16_to_cpu(dev->descriptor.idProduct));
	}

	/* Установка имени пути устройства */
	usb_make_path(dev, mouse->phys, sizeof(mouse->phys));
	strlcat(mouse->phys, "/input0", sizeof(mouse->phys));

	input_dev->name = mouse->name;
	input_dev->phys = mouse->phys;
	usb_to_input_id(dev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y);
	input_dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
		BIT_MASK(BTN_EXTRA);
	input_dev->relbit[0] |= BIT_MASK(REL_WHEEL);

	input_set_drvdata(input_dev, mouse);

	input_dev->open = usb_mouse_open;
	input_dev->close = usb_mouse_close;

	/* Инициализация urb */
	usb_fill_int_urb(mouse->irq, dev, pipe, mouse->data, (maxp > 8 ? 8 : maxp),
		usb_mouse_irq, mouse, endpoint->bInterval);
	mouse->irq->transfer_dma = mouse->data_dma;
	mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	error = input_register_device(mouse->dev);
	if (error) {
		usb_free_urb(mouse->irq);
		free_devices(input_dev, mouse, dev);
		return -ENOMEM;
	}

	usb_set_intfdata(intf, mouse);

	/* Создание потока ядра */
	play_task = kthread_create(play_handler, NULL, "thread_sound_play");
	if (!IS_ERR(play_task)) {
		wake_up_process(play_task);
		printk(KERN_INFO "+ sound_play thread was created!\n");
	} 
	else {
		printk(KERN_ERR "+ could not create the sound_play thread!\n");
		usb_free_urb(mouse->irq);
		free_devices(input_dev, mouse, dev);
		return -ENOMEM;
	}

	return 0;
}

static void usb_mouse_disconnect(struct usb_interface *intf) 
{
	struct usb_mouse *mouse = usb_get_intfdata (intf);

	/* Остановка потока ядра */
	kthread_stop(play_task);
	printk(KERN_INFO "+ sound_play thread was stoped!\n");

	usb_set_intfdata(intf, NULL);
	if (mouse) {
		usb_kill_urb(mouse->irq);
		input_unregister_device(mouse->dev);
		usb_free_urb(mouse->irq);
		usb_free_coherent(interface_to_usbdev(intf), 8, mouse->data, mouse->data_dma);
		kfree(mouse);
	}

	printk(KERN_INFO "+ usb mouse was disconnected!\n");
}

static const struct usb_device_id usb_mouse_id_table[] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
	.name		= "usbmouse",
	.probe		= usb_mouse_probe,
	.disconnect	= usb_mouse_disconnect,
	.id_table	= usb_mouse_id_table,
};

static int __init usb_mouse_init(void)
{
	int retval = usb_register(&usb_mouse_driver);
	if (retval == 0) {
		printk(KERN_INFO "+ module usb mouse driver loaded!\n");
	}

	return retval;
}

static void __exit usb_mouse_exit(void)
{
	usb_deregister(&usb_mouse_driver);
	printk(KERN_INFO "+ module usb mouse driver unloaded!\n");
}

module_init(usb_mouse_init);
module_exit(usb_mouse_exit);
