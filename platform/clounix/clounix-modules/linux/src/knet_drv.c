#include <linux/pci.h>
#include <knet_dev.h>
#include <knet_pci.h>

#include <nb_drv.h>
#include <kg_drv.h>

clx_netif_drv_cb_t default_pkt_driver;
clx_dma_drv_cb_t default_dma_driver;
clx_intr_drv_cb_t default_intr_driver;
clx_pci_drv_cb_t default_pci_driver = {
    .dma_bit_mask = 48,
    .mmio_bar = 2,
};

static clx_drv_cb_t default_driver = {
    .pci_drv = &default_pci_driver,
    .pkt_drv = &default_pkt_driver,
    .intr_drv = &default_intr_driver,
    .dma_drv = &default_dma_driver,
};

int
clx_pkt_drv_init(uint32_t unit)
{
    uint32_t array_elem_num = 0;

    array_elem_num = clx_netif_drv(unit)->unit_num * clx_netif_drv(unit)->dies_per_unit *
        clx_netif_drv(unit)->slices_per_die * clx_netif_drv(unit)->ports_per_slice;

    clx_netif_drv(unit)->ptr_port_map_db =
        (uint32_t *)kmalloc_array(array_elem_num, sizeof(uint32_t), GFP_ATOMIC);
    if (!clx_netif_drv(unit)->ptr_port_map_db) {
        return -ENOMEM;
    }

    memset(clx_netif_drv(unit)->ptr_port_map_db, (uint32_t)-1, array_elem_num * sizeof(uint32_t));

    return 0;
}

void
clx_pkt_drv_deinit(uint32_t unit)
{
    if (!clx_netif_drv(unit)->ptr_port_map_db) {
        dbg_print(DBG_WARN, "ptr_port_map_db pointer is NULL. unit=%u\n", unit);
        return;
    }

    kfree(clx_netif_drv(unit)->ptr_port_map_db);

    return;
}

int
clx_drv_init(uint32_t unit, struct pci_dev *dev)
{
    int rc = 0;
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);

    pci_dev_data->clx_drv = (clx_drv_cb_t *)kmalloc(sizeof(clx_drv_cb_t), GFP_KERNEL);
    if (!pci_dev_data->clx_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for clx_drv. unit=%u\n", unit);
        return -ENOMEM;
    }

    memset(pci_dev_data->clx_drv, 0, sizeof(clx_drv_cb_t));
    if (CLX_DEVICE_IS_NAMCHABARWA(pci_dev_data->device_id)) {
        nb_driver_init(unit, pci_dev_data->clx_drv);
    } else if (CLX_DEVICE_IS_KAWAGARBO(pci_dev_data->device_id)) {
        kg_driver_init(unit, pci_dev_data->clx_drv);
    } else {
        dbg_print(DBG_WARN, "Unknown chip, use default config. unit=%u\n", unit);
        memcpy(pci_dev_data->clx_drv, &default_driver, sizeof(clx_drv_cb_t));
        return 0;
    }

    rc = clx_pkt_drv_init(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the pkt driver. unit=%u\n", unit);
        return rc;
    }

    return 0;
}

int
clx_drv_cleanup(uint32_t unit, struct pci_dev *dev)
{
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);

    clx_pkt_drv_deinit(unit);

    if (CLX_DEVICE_IS_NAMCHABARWA(pci_dev_data->device_id)) {
        nb_driver_deinit(unit);
    } else if (CLX_DEVICE_IS_KAWAGARBO(pci_dev_data->device_id)) {
        kg_driver_deinit(unit);
    } else {
        dbg_print(DBG_WARN, "Unknown chip. unit=%u\n", unit);
        return 0;
    }

    kfree(pci_dev_data->clx_drv);
    pci_dev_data->clx_drv = NULL;

    return 0;
}
