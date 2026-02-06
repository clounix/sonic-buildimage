#include "knet_dev.h"
#include "knet_pci.h"
#include "nb_drv.h"
#include "nb_chip.h"

extern clx_netif_drv_cb_t nb_pkt_driver;
extern clx_dma_drv_cb_t nb_dma_driver;
extern clx_intr_drv_cb_t nb_intr_driver;
extern clx_pci_drv_cb_t nb_pci_driver;

int
nb_init_dma_driver(uint32_t unit)
{
    uint32_t channel = 0;
    clx_dma_drv_cb_t *ptr_clx_dma_drv = clx_dma_drv(unit);

    ptr_clx_dma_drv->clx_dma_intr = (clx_dma_intr_t *)kmalloc_array(
        ptr_clx_dma_drv->channel_num, sizeof(clx_dma_intr_t), GFP_ATOMIC);
    if (!ptr_clx_dma_drv->clx_dma_intr) {
        return -ENOMEM;
    }
    for (channel = 0; channel < ptr_clx_dma_drv->channel_num; channel++) {
        if (channel < 4) {
            /* Rx channel */
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.channel = channel;
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.unit = unit;
            ptr_clx_dma_drv->clx_dma_intr[channel].dma_handler = dma_rx_tasklet_func;
        } else if (channel < 8) {
            /* Tx channel */
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.channel = channel;
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.unit = unit;
            ptr_clx_dma_drv->clx_dma_intr[channel].dma_handler = dma_tx_tasklet_func;
        } else {
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.channel = channel;
            ptr_clx_dma_drv->clx_dma_intr[channel].channel_cookie.unit = unit;
            ptr_clx_dma_drv->clx_dma_intr[channel].dma_handler = dma_general_tasklet_func;
        }
    }

    return 0;
}

void
nb_cleanup_dma_driver(uint32_t unit)
{
    clx_dma_drv_cb_t *ptr_clx_dma_drv = clx_dma_drv(unit);

    dbg_print(DBG_DEBUG, "[debug] unit:%u nb_cleanup_dma_driver\n", unit);
    kfree(ptr_clx_dma_drv->clx_dma_intr);
    dbg_print(DBG_DEBUG, "[debug] unit:%u nb_cleanup_dma_driver done.\n", unit);
}

int
nb_driver_init(uint32_t unit, clx_drv_cb_t *ptr_clx_drv)
{
    int rc = 0;

    ptr_clx_drv->dma_drv = (clx_dma_drv_cb_t *)kmalloc(sizeof(clx_dma_drv_cb_t), GFP_KERNEL);
    if (!ptr_clx_drv->dma_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for dma_drv. unit=%d\n", unit);
        return -ENOMEM;
    }
    dbg_print(DBG_DEBUG, "Allocate dma_drv. unit=%d ptr_clx_drv->dma_drv=%p\n", unit,
              ptr_clx_drv->dma_drv);
    memcpy(ptr_clx_drv->dma_drv, &nb_dma_driver, sizeof(clx_dma_drv_cb_t));

    rc = nb_init_dma_driver(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the dma driver. unit=%d\n", unit);
        return rc;
    }

    ptr_clx_drv->pkt_drv = (clx_netif_drv_cb_t *)kmalloc(sizeof(clx_netif_drv_cb_t), GFP_KERNEL);
    if (!ptr_clx_drv->pkt_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for pkt_drv. unit=%d\n", unit);
        return -ENOMEM;
    }
    dbg_print(DBG_DEBUG, "Allocate pkt_drv. unit=%d ptr_clx_drv->pkt_drv=%p\n", unit,
              ptr_clx_drv->pkt_drv);
    memcpy(ptr_clx_drv->pkt_drv, &nb_pkt_driver, sizeof(clx_netif_drv_cb_t));

    ptr_clx_drv->intr_drv = (clx_intr_drv_cb_t *)kmalloc(sizeof(clx_intr_drv_cb_t), GFP_KERNEL);
    if (!ptr_clx_drv->intr_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for intr_drv. unit=%d\n", unit);
        return -ENOMEM;
    }
    dbg_print(DBG_DEBUG, "Allocate intr_drv. unit=%d ptr_clx_drv->intr_drv=%p\n", unit,
              ptr_clx_drv->intr_drv);
    memcpy(ptr_clx_drv->intr_drv, &nb_intr_driver, sizeof(clx_intr_drv_cb_t));

    ptr_clx_drv->pci_drv = (clx_pci_drv_cb_t *)kmalloc(sizeof(clx_pci_drv_cb_t), GFP_KERNEL);
    if (!ptr_clx_drv->pci_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for pci_drv. unit=%d\n", unit);
        return -ENOMEM;
    }
    dbg_print(DBG_DEBUG, "Allocate pci_drv. unit=%d ptr_clx_drv->pci_drv=%p\n", unit,
              ptr_clx_drv->pci_drv);
    memcpy(ptr_clx_drv->pci_drv, &nb_pci_driver, sizeof(clx_pci_drv_cb_t));

    return 0;
}

int
nb_driver_deinit(uint32_t unit)
{
    clx_drv_cb_t *ptr_clx_drv = clx_misc_dev->clx_pci_dev[unit]->clx_drv;

    if (!ptr_clx_drv) {
        dbg_print(DBG_ERR, "ptr_clx_drv is NULL. already deinitialized. unit=%d\n", unit);
        return 0;
    }

    if (ptr_clx_drv->dma_drv) {
        dbg_print(DBG_DEBUG, "Free dma_drv. unit=%d ptr_clx_drv->dma_drv=%p\n", unit,
                  ptr_clx_drv->dma_drv);
        nb_cleanup_dma_driver(unit);
        kfree(ptr_clx_drv->dma_drv);
        ptr_clx_drv->dma_drv = NULL;
    }

    if (ptr_clx_drv->pkt_drv) {
        dbg_print(DBG_DEBUG, "Free pkt_drv. unit=%d ptr_clx_drv->pkt_drv=%p\n", unit,
                  ptr_clx_drv->pkt_drv);
        kfree(ptr_clx_drv->pkt_drv);
        ptr_clx_drv->pkt_drv = NULL;
    }

    if (ptr_clx_drv->intr_drv) {
        dbg_print(DBG_DEBUG, "Free intr_drv. unit=%d ptr_clx_drv->intr_drv=%p\n", unit,
                  ptr_clx_drv->intr_drv);
        kfree(ptr_clx_drv->intr_drv);
        ptr_clx_drv->intr_drv = NULL;
    }

    if (ptr_clx_drv->pci_drv) {
        dbg_print(DBG_DEBUG, "Free pci_drv. unit=%d ptr_clx_drv->pci_drv=%p\n", unit,
                  ptr_clx_drv->pci_drv);
        kfree(ptr_clx_drv->pci_drv);
        ptr_clx_drv->pci_drv = NULL;
    }

    dbg_print(DBG_DEBUG, "nb_driver_deinit. unit=%d\n", unit);
    clx_misc_dev->clx_pci_dev[unit]->clx_drv = NULL;
    return 0;
}
