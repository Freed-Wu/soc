/**
 * @file axi_dma.c
 * @date Saturday, November 14, 2015 at 11:20:00 AM EST
 * @author Brandon Perez (bmperez)
 * @author Jared Choi (jaewonch)
 *
 * This file contains the top level functions for AXI DMA module.
 *
 * @bug No known bugs.
 **/

// Kernel dependencies
#include <linux/module.h>          // Module init and exit macros
#include <linux/moduleparam.h>     // Module param macro
#include <linux/platform_device.h> // Platform device definitions
#include <linux/slab.h>            // Allocation functions
#include <linux/stat.h>            // Module parameter permission values

// new
#include <linux/of_dma.h>

// Local dependencies
#include "axitangxi_dev.h" // Internal definitions

/*----------------------------------------------------------------------------
 * Module Parameters
 *----------------------------------------------------------------------------*/

// The name to use for the character device. This is "axidma" by default.
static char *chrdev_name = DEVICE_NAME;
module_param(chrdev_name, charp, S_IRUGO);

// The minor number to use for the character device. 0 by default.
static int minor_num = MINOR_NUMBER;
module_param(minor_num, int, S_IRUGO);

/*----------------------------------------------------------------------------
 * Platform Device Functions
 *----------------------------------------------------------------------------*/

int axitangxi_dma_init(struct platform_device *pdev,
                       struct axitangxi_device *dev) {
  u64 dma_mask = DMA_BIT_MASK(8 * sizeof(dma_addr_t));
  int rc = dma_set_coherent_mask(&dev->pdev->dev, dma_mask);
  if (rc < 0)
    axitangxi_err("Unable to set the DMA coherent mask.\n");

  return rc;
}

static int axitangxi_probe(struct platform_device *pdev) {
  // Allocate a AXI DMA device structure to hold metadata about the DMA
  struct axitangxi_device *axitangxi_dev =
      kmalloc(sizeof(*axitangxi_dev), GFP_KERNEL);
  if (axitangxi_dev == NULL) {
    axitangxi_err("Unable to allocate the AXI DMA device structure.\n");
    return -ENOMEM;
  }
  axitangxi_dev->pdev = pdev;

  // Initialize the DMA interface
  int rc = axitangxi_dma_init(pdev, axitangxi_dev);
  if (rc < 0) {
    goto free_axitangxi_dev;
  }

  // Assign the character device name, minor number, and number of devices
  axitangxi_dev->chrdev_name = chrdev_name;
  axitangxi_dev->minor_num = minor_num;
  axitangxi_dev->num_devices = NUM_DEVICES;

  // Initialize the character device for the module.
  rc = axitangxi_chrdev_init(axitangxi_dev);
  if (rc < 0) {
    goto free_axitangxi_dev;
  }

  // Set the private data in the device to the AXI DMA device structure
  dev_set_drvdata(&pdev->dev, axitangxi_dev);
  return 0;

free_axitangxi_dev:
  kfree(axitangxi_dev);
  return -ENOSYS;
}

static int axitangxi_remove(struct platform_device *pdev) {
  // Get the AXI DMA device structure from the device's private data
  struct axitangxi_device *axitangxi_dev = dev_get_drvdata(&pdev->dev);

  // Cleanup the character device structures
  axitangxi_chrdev_exit(axitangxi_dev);

  // Free the device structure
  kfree(axitangxi_dev);
  return 0;
}

static const struct of_device_id axitangxi_compatible_of_ids[] = {
    {.compatible = "xlnx,axi-tangxi"}, {}};

static struct platform_driver axitangxi_driver = {
    .driver =
        {
            .name = "axi-tangxi",
            .owner = THIS_MODULE,
            .of_match_table = axitangxi_compatible_of_ids,
        },
    .probe = axitangxi_probe,
    .remove = axitangxi_remove,
};

/*----------------------------------------------------------------------------
 * Module Initialization and Exit
 *----------------------------------------------------------------------------*/
module_platform_driver(axitangxi_driver);

/* 驱动描述信息 */
MODULE_AUTHOR("Wyn");
MODULE_ALIAS("axitangxi device");
MODULE_DESCRIPTION("AXI TANGXI driver");
MODULE_VERSION("v3.0");
MODULE_LICENSE("GPL");
