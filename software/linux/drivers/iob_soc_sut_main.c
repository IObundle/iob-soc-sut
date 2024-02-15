/* iob_soc_sut_main.c: driver for iob_soc_sut
 * using device platform. No hardcoded hardware address:
 * 1. load driver: insmod iob_soc_sut.ko
 * 2. run user app: ./user/user
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include "iob_class/iob_class_utils.h"
#include "iob_soc_sut.h"

static int iob_soc_sut_probe(struct platform_device *);
static int iob_soc_sut_remove(struct platform_device *);

static ssize_t iob_soc_sut_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t iob_soc_sut_write(struct file *, const char __user *, size_t,
                               loff_t *);
static loff_t iob_soc_sut_llseek(struct file *, loff_t, int);
static int iob_soc_sut_open(struct inode *, struct file *);
static int iob_soc_sut_release(struct inode *, struct file *);

static struct iob_data iob_soc_sut_data = {0};
DEFINE_MUTEX(iob_soc_sut_mutex);

#include "iob_soc_sut_sysfs.h"

static const struct file_operations iob_soc_sut_fops = {
    .owner = THIS_MODULE,
    .write = iob_soc_sut_write,
    .read = iob_soc_sut_read,
    .llseek = iob_soc_sut_llseek,
    .open = iob_soc_sut_open,
    .release = iob_soc_sut_release,
};

static const struct of_device_id of_iob_soc_sut_match[] = {
    {.compatible = "iobundle,sut0"},
    {},
};

static struct platform_driver iob_soc_sut_driver = {
    .driver =
        {
            .name = "iob_soc_sut",
            .owner = THIS_MODULE,
            .of_match_table = of_iob_soc_sut_match,
        },
    .probe = iob_soc_sut_probe,
    .remove = iob_soc_sut_remove,
};

//
// Module init and exit functions
//
static int iob_soc_sut_probe(struct platform_device *pdev) {
  struct resource *res;
  int result = 0;

  if (iob_soc_sut_data.device != NULL) {
    pr_err("[Driver] %s: No more devices allowed!\n", IOB_SOC_SUT_DRIVER_NAME);

    return -ENODEV;
  }

  pr_info("[Driver] %s: probing.\n", IOB_SOC_SUT_DRIVER_NAME);

  // Get the I/O region base address
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!res) {
    pr_err("[Driver]: Failed to get I/O resource!\n");
    result = -ENODEV;
    goto r_get_resource;
  }

  // Request and map the I/O region
  iob_soc_sut_data.regbase = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(iob_soc_sut_data.regbase)) {
    result = PTR_ERR(iob_soc_sut_data.regbase);
    goto r_ioremmap;
  }
  iob_soc_sut_data.regsize = resource_size(res);

  // Alocate char device
  result =
      alloc_chrdev_region(&iob_soc_sut_data.devnum, 0, 1, IOB_SOC_SUT_DRIVER_NAME);
  if (result) {
    pr_err("%s: Failed to allocate device number!\n", IOB_SOC_SUT_DRIVER_NAME);
    goto r_alloc_region;
  }

  cdev_init(&iob_soc_sut_data.cdev, &iob_soc_sut_fops);

  result = cdev_add(&iob_soc_sut_data.cdev, iob_soc_sut_data.devnum, 1);
  if (result) {
    pr_err("%s: Char device registration failed!\n", IOB_SOC_SUT_DRIVER_NAME);
    goto r_cdev_add;
  }

  // Create device class // todo: make a dummy driver just to create and own the
  // class: https://stackoverflow.com/a/16365027/8228163
  if ((iob_soc_sut_data.class =
           class_create(THIS_MODULE, IOB_SOC_SUT_DRIVER_CLASS)) == NULL) {
    printk("Device class can not be created!\n");
    goto r_class;
  }

  // Create device file
  iob_soc_sut_data.device =
      device_create(iob_soc_sut_data.class, NULL, iob_soc_sut_data.devnum, NULL,
                    IOB_SOC_SUT_DRIVER_NAME);
  if (iob_soc_sut_data.device == NULL) {
    printk("Can not create device file!\n");
    goto r_device;
  }

  result = iob_soc_sut_create_device_attr_files(iob_soc_sut_data.device);
  if (result) {
    pr_err("Cannot create device attribute file......\n");
    goto r_dev_file;
  }

  dev_info(&pdev->dev, "initialized.\n");
  goto r_ok;

r_dev_file:
  iob_soc_sut_remove_device_attr_files(&iob_soc_sut_data);
r_device:
  class_destroy(iob_soc_sut_data.class);
r_class:
  cdev_del(&iob_soc_sut_data.cdev);
r_cdev_add:
  unregister_chrdev_region(iob_soc_sut_data.devnum, 1);
r_alloc_region:
  // iounmap is managed by devm
r_ioremmap:
r_get_resource:
r_ok:

  return result;
}

static int iob_soc_sut_remove(struct platform_device *pdev) {
  iob_soc_sut_remove_device_attr_files(&iob_soc_sut_data);
  class_destroy(iob_soc_sut_data.class);
  cdev_del(&iob_soc_sut_data.cdev);
  unregister_chrdev_region(iob_soc_sut_data.devnum, 1);
  // Note: no need for iounmap, since we are using devm_ioremap_resource()

  dev_info(&pdev->dev, "exiting.\n");

  return 0;
}

static int __init iob_soc_sut_init(void) {
  pr_info("[Driver] %s: initializing.\n", IOB_SOC_SUT_DRIVER_NAME);

  return platform_driver_register(&iob_soc_sut_driver);
}

static void __exit iob_soc_sut_exit(void) {
  pr_info("[Driver] %s: exiting.\n", IOB_SOC_SUT_DRIVER_NAME);
  platform_driver_unregister(&iob_soc_sut_driver);
}

//
// File operations
//

static int iob_soc_sut_open(struct inode *inode, struct file *file) {
  pr_info("[Driver] iob_soc_sut device opened\n");

  if (!mutex_trylock(&iob_soc_sut_mutex)) {
    pr_info("Another process is accessing the device\n");

    return -EBUSY;
  }

  return 0;
}

static int iob_soc_sut_release(struct inode *inode, struct file *file) {
  pr_info("[Driver] iob_soc_sut device closed\n");

  mutex_unlock(&iob_soc_sut_mutex);

  return 0;
}

static ssize_t iob_soc_sut_read(struct file *file, char __user *buf, size_t count,
                              loff_t *ppos) {
  int size = 0;
  u32 value = 0;

  /* read value from register */
  switch (*ppos) {
  case IOB_SOC_SUT_REG3_ADDR:
    value = iob_data_read_reg(iob_soc_sut_data.regbase, IOB_SOC_SUT_REG3_ADDR,
                              IOB_SOC_SUT_REG3_W);
    size = (IOB_SOC_SUT_REG3_W >> 3); // bit to bytes
    pr_info("[Driver] Read REG3!\n");
    break;
  case IOB_SOC_SUT_REG4_ADDR:
    value = iob_data_read_reg(iob_soc_sut_data.regbase, IOB_SOC_SUT_REG4_ADDR,
                              IOB_SOC_SUT_REG4_W);
    size = (IOB_SOC_SUT_REG4_W >> 3); // bit to bytes
    pr_info("[Driver] Read REG4!\n");
    break;
  case IOB_SOC_SUT_REG5_ADDR:
    value = iob_data_read_reg(iob_soc_sut_data.regbase, IOB_SOC_SUT_REG5_ADDR,
                              IOB_SOC_SUT_REG5_W);
    size = (IOB_SOC_SUT_REG5_W >> 3); // bit to bytes
    pr_info("[Driver] Read REG5!\n");
    break;
  case IOB_SOC_SUT_VERSION_ADDR:
    value = iob_data_read_reg(iob_soc_sut_data.regbase, IOB_SOC_SUT_VERSION_ADDR,
                              IOB_SOC_SUT_VERSION_W);
    size = (IOB_SOC_SUT_VERSION_W >> 3); // bit to bytes
    pr_info("[Driver] Read version!\n");
    break;
  default:
    // invalid address - no bytes read
    return 0;
  }

  // Read min between count and REG_SIZE
  if (size > count)
    size = count;

  if (copy_to_user(buf, &value, size))
    return -EFAULT;

  return count;
}

static ssize_t iob_soc_sut_write(struct file *file, const char __user *buf,
                               size_t count, loff_t *ppos) {
  int size = 0;
  u32 value = 0;

  switch (*ppos) {
  case IOB_SOC_SUT_REG1_ADDR:
    size = (IOB_SOC_SUT_REG1_W >> 3); // bit to bytes
    if (read_user_data(buf, size, &value))
      return -EFAULT;
    iob_data_write_reg(iob_soc_sut_data.regbase, value, IOB_SOC_SUT_REG1_ADDR,
                       IOB_SOC_SUT_REG1_W);
    pr_info("[Driver] REG1 iob_soc_sut: 0x%x\n", value);
    break;
  case IOB_SOC_SUT_REG2_ADDR:
    size = (IOB_SOC_SUT_REG2_W >> 3); // bit to bytes
    if (read_user_data(buf, size, &value))
      return -EFAULT;
    iob_data_write_reg(iob_soc_sut_data.regbase, value, IOB_SOC_SUT_REG2_ADDR,
                       IOB_SOC_SUT_REG2_W);
    pr_info("[Driver] REG2 iob_soc_sut: 0x%x\n", value);
    break;
  default:
    pr_info("[Driver] Invalid write address 0x%x\n", (unsigned int)*ppos);
    // invalid address - no bytes written
    return 0;
  }

  return count;
}

/* Custom lseek function
 * check: lseek(2) man page for whence modes
 */
static loff_t iob_soc_sut_llseek(struct file *filp, loff_t offset, int whence) {
  loff_t new_pos = -1;

  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;
  case SEEK_CUR:
    new_pos = filp->f_pos + offset;
    break;
  case SEEK_END:
    new_pos = (1 << IOB_SOC_SUT_SWREG_ADDR_W) + offset;
    break;
  default:
    return -EINVAL;
  }

  // Check for valid bounds
  if (new_pos < 0 || new_pos > iob_soc_sut_data.regsize) {
    return -EINVAL;
  }

  // Update file position
  filp->f_pos = new_pos;

  return new_pos;
}

module_init(iob_soc_sut_init);
module_exit(iob_soc_sut_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("IObundle");
MODULE_DESCRIPTION("IOb-Soc-SUT Drivers");
MODULE_VERSION("0.70");
