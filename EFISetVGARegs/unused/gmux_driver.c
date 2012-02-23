struct gmux_priv {
	acpi_handle gmux_handle;

	struct backlight_device *gmux_backlight_device;
	struct backlight_properties props;
};

static struct gmux_priv gmux_priv;

DECLARE_COMPLETION(powerchange_done);

#define PORT_VER_MAJOR 0x704
#define PORT_VER_MINOR 0x705
#define PORT_VER_RELEASE 0x706

#define PORT_SWITCH_DISPLAY 0x710
#define PORT_SWITCH_UNK 0x728
#define PORT_SWITCH_DDC 0x740

#define PORT_DISCRETE_POWER 0x750

#define PORT_BACKLIGHT 0x774
#define MAX_BRIGHTNESS 0x1af40
#define BRIGHTNESS_STEPS 20

#define PORT_INTERRUPT_ENABLE 0x714
#define PORT_INTERRUPT_STATUS 0x716

#define INTERRUPT_ENABLE 0xFF
#define INTERRUPT_DISABLE 0x0

#define INTERRUPT_STATUS_ACTIVE 0
/* display switch complete */
#define INTERRUPT_STATUS_DISPLAY 1
/* dedicated card powered up/down */
#define INTERRUPT_STATUS_POWER 4
/* unknown, set after boot */
#define INTERRUPT_STATUS_UNK 5

static int gmux_switchto(enum vga_switcheroo_client_id id)
{
	if (id == VGA_SWITCHEROO_IGD) {
		outb(1, PORT_SWITCH_UNK);
		outb(2, PORT_SWITCH_DISPLAY);
		outb(2, PORT_SWITCH_DDC);
	} else {
		outb(2, PORT_SWITCH_UNK);
		outb(3, PORT_SWITCH_DISPLAY);
		outb(3, PORT_SWITCH_DDC);
	}

	return 0;
}

//static int gmux_switchddc(enum vga_switcheroo_client_id id)
//{
//	if (id == VGA_SWITCHEROO_IGD) {
//		outb(1, PORT_SWITCH_UNK);
//		outb(2, PORT_SWITCH_DDC);
//	} else {
//		outb(2, PORT_SWITCH_UNK);
//		outb(3, PORT_SWITCH_DDC);
//	}
//
//	return 0;
//}

static int gmux_set_discrete_state(enum vga_switcheroo_state state)
{
	/* TODO: locking for completions needed? */
	init_completion(&powerchange_done);

	if (state == VGA_SWITCHEROO_ON) {
		outb(1, PORT_DISCRETE_POWER);
		outb(3, PORT_DISCRETE_POWER);	
		printk("gmux: discrete powered up\n");
	} else {
		outb(0, PORT_DISCRETE_POWER);	
		printk("gmux: discrete powered down\n");
	}

	/* TODO: add timeout */
	printk("vga_switcheroo: before completion\n");
	wait_for_completion(&powerchange_done);
	printk("vga_switcheroo: after completion\n");

	return 0;
}


static int gmux_set_power_state(enum vga_switcheroo_client_id id,
				enum vga_switcheroo_state state)
{
	if (id == VGA_SWITCHEROO_IGD)
		return 0;

	return gmux_set_discrete_state(state);
}

static int gmux_init(void)
{
	return 0;
}

static int gmux_get_client_id(struct pci_dev *pdev)
{
	if (pdev->vendor == 0x8086) /* TODO: better detection */
		return VGA_SWITCHEROO_IGD;
	else
		return VGA_SWITCHEROO_DIS;
}

static struct vga_switcheroo_handler gmux_handler = {
	.switchto = gmux_switchto,
//	.switchddc = gmux_switchddc,
	.power_state = gmux_set_power_state,
	.init = gmux_init,
	.get_client_id = gmux_get_client_id,
};

static void disable_interrupts(void)
{
	outb(INTERRUPT_DISABLE, PORT_INTERRUPT_ENABLE);
}

static void enable_interrupts(void)
{
	outb(INTERRUPT_ENABLE, PORT_INTERRUPT_ENABLE);
}

static int interrupt_get_status(void)
{
	return inb(PORT_INTERRUPT_STATUS);
}

static void interrupt_activate_status(void)
{
	int old_status;
	int new_status;
	
	/* to reactivate interrupts write back current status */
	old_status = inb(PORT_INTERRUPT_STATUS);
	outb(old_status, PORT_INTERRUPT_STATUS);
	new_status = inb(PORT_INTERRUPT_STATUS);
	
	/* status = 0 indicates active interrupts */
	if (new_status)
		printk("gmux: error: activate_status, old_status %d new_status %d\n", old_status, new_status);
}

static u32 gpe_handler(acpi_handle gpe_device, u32 gpe_number, void *context)
{
	int status;

	status = interrupt_get_status();
	disable_interrupts();
	printk("gmux: gpe handler called: status %d\n", status);

	interrupt_activate_status();
	enable_interrupts();
	
	if (status == INTERRUPT_STATUS_POWER)
		complete(&powerchange_done);

	return 0;
}

static bool gmux_detect(void)
{
	int ver_major, ver_minor, ver_release;
	
	ver_major = inb(PORT_VER_MAJOR);
	ver_minor = inb(PORT_VER_MINOR);
	ver_release = inb(PORT_VER_RELEASE);
	
	if (!(ver_major == 0x1 && ver_minor == 0x9)) {
		printk(KERN_INFO "gmux: detected unknown gmux HW version: %x.%x.%x\n", ver_major, ver_minor, ver_release);
		return false;
	}
	else {
		printk(KERN_INFO "gmux: detected gmux HW version: %x.%x.%x\n", ver_major, ver_minor, ver_release);
		return true;
	}
}

//static int set_brightness(struct backlight_device *bd)
//{
//	int brightness = bd->props.brightness;
//
//	if (bd->props.state & BL_CORE_FBBLANK)
//		brightness = 0;
//	else if (bd->props.state & BL_CORE_SUSPENDED)
//		brightness = 0;
//
//	outl((MAX_BRIGHTNESS / BRIGHTNESS_STEPS) * brightness, PORT_BACKLIGHT);
//	
//	printk("gmux: set brightness %d\n", brightness);
//	return 0;
//}
//
//static int get_brightness(struct backlight_device *bd)
//{
//	printk("gmux: get brightness\n");
//	return inl(PORT_BACKLIGHT) / (MAX_BRIGHTNESS / BRIGHTNESS_STEPS);
//}
//
//const struct backlight_ops backlight_ops = {
//	.options        = BL_CORE_SUSPENDRESUME,
//	.get_brightness	= get_brightness,
//	.update_status	= set_brightness
//};
//
//int init_backlight(void)
//{
//	memset(&gmux_priv.props, 0, sizeof(struct backlight_properties));
//	gmux_priv.props.max_brightness = BRIGHTNESS_STEPS;
//	gmux_priv.gmux_backlight_device = backlight_device_register("gmux", NULL,
//							 NULL,
//							 &backlight_ops,
//							 &gmux_priv.props);
//							 
//	if (IS_ERR(gmux_priv.gmux_backlight_device)) {
//		return PTR_ERR(gmux_priv.gmux_backlight_device);
//	}
//	
//	gmux_priv.gmux_backlight_device->props.brightness =
//		backlight_ops.get_brightness(gmux_priv.gmux_backlight_device);
//	backlight_update_status(gmux_priv.gmux_backlight_device);
//	
//	return 0;
//}

static int __devinit gmux_pnp_probe(struct pnp_dev *dev, const struct pnp_device_id *dev_id)
{
	acpi_status status;

	/* TODO: get ioport region and gpe num from acpi */
//	if (! request_region(0x700, 0xff, "gmux")) {
	if (! request_region(0x700, 0x51, "gmux")) {
		printk("gmux: request ioport region failed\n");
		goto err;
	}
	
	
	/* TODO: add more checks if it's really a gmux */
	
	if (! gmux_detect())
		goto err;
	
	status = acpi_install_gpe_handler(NULL, 0x16, ACPI_GPE_LEVEL_TRIGGERED, &gpe_handler, &gmux_priv.gmux_handle);
	if (ACPI_FAILURE(status)) {
		printk("gmux: install gpe handler failed: %s\n", acpi_format_exception(status));
		goto err_detect;
	}
	
//	if (init_backlight())
//		goto err_backlight;
		
	status = acpi_enable_gpe(NULL, 0x16);
	if (ACPI_FAILURE(status)) {
		printk("gmux: enable gpe failed: %s\n", acpi_format_exception(status));
		goto err_enable_gpe;
	}
	
	enable_interrupts();
	
	if (vga_switcheroo_register_handler(&gmux_handler))
		goto err_switcheroo;

	printk("gmux: loaded successfully\n");
	return 0;
	
err_switcheroo:
	acpi_disable_gpe(NULL, 0x16);
err_enable_gpe:
//	backlight_device_unregister(gmux_priv.gmux_backlight_device);
//err_backlight:
	acpi_remove_gpe_handler(NULL, 0x16, &gpe_handler);
err_detect:
//	release_region(0x700, 0xff);
	release_region(0x700, 0x51);
err:
	return -ENODEV;
}

static void gmux_pnp_remove(struct pnp_dev * dev)
{
	vga_switcheroo_unregister_handler();
	
//	backlight_device_unregister(gmux_priv.gmux_backlight_device);
	
	disable_interrupts();

	acpi_remove_gpe_handler(NULL, 0x16, &gpe_handler);

	acpi_disable_gpe(NULL, 0x16);
	
//	release_region(0x700, 0xff);
	release_region(0x700, 0x51);
	
	printk("gmux: unloaded\n");
}

static const struct pnp_device_id pnp_dev_table[] = {
	{ "APP000b", 0 },
	{ "", 0 }
};

static struct pnp_driver gmux_pnp_driver = {
	.name		= "gmux",
	.id_table	= pnp_dev_table,
	.probe		= gmux_pnp_probe,
	.remove		= gmux_pnp_remove,
};

static int __init init_gmux(void)
{
	return pnp_register_driver(&gmux_pnp_driver);
}


static void unload_gmux(void)
{
	pnp_unregister_driver(&gmux_pnp_driver);
}

module_init(init_gmux);
module_exit(unload_gmux);

MODULE_AUTHOR("Andreas Heider");
MODULE_DESCRIPTION("Support for Macbook Pro graphics switching");
MODULE_LICENSE("GPL and additional rights");
