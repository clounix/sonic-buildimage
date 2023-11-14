//**********************************************************************
//**********************************************************************
//**                                                                  **
//**        (C)Copyright 1985-2019, American Megatrends, Inc.         **
//**                                                                  **
//**                       All Rights Reserved.                       **
//**                                                                  **
//**      5555 Oakbrook Parkway, Suite 200, Norcross, GA 30093        **
//**                                                                  **
//**                       Phone: (770)-246-8600                      **
//**                                                                  **
//**********************************************************************
//**********************************************************************

//**********************************************************************
//<AMI_FHDR_START>
//
// Name: amifldrv.c
//
// Description: AMI Linux Generic Driver source file
//
//<AMI_FHDR_END>
//**********************************************************************

#include "amifldrv.h"

static int	Major;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static int		device_open_count = 0;
#endif

static int amifldrv_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	MOD_INC_USE_COUNT;
#else
	if (device_open_count) return -EBUSY;

	device_open_count++;
	try_module_get(THIS_MODULE);
#endif
	return 0;
}

static int amifldrv_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	MOD_DEC_USE_COUNT;
#else
	device_open_count--;
	module_put(THIS_MODULE);
#endif
	return 0;
}

#if defined(HAVE_UNLOCKED_IOCTL)
static long amifldrv_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int amifldrv_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	switch(cmd) {
		case CMD_ALLOC :
			return amimemdrv_alloc((void *)arg);

		case CMD_FREE :
			return amimemdrv_free((void *)arg);

		case CMD_IOWRITE_BYTE :
			return amiio_write8((void *)arg);

		case CMD_IOWRITE_WORD :
			return amiio_write16((void *)arg);

		case CMD_IOWRITE_DWORD :
			return amiio_write32((void *)arg);

		case CMD_IOREAD_BYTE :
			return amiio_read8((void *)arg);

		case CMD_IOREAD_WORD :
			return amiio_read16((void *)arg);

		case CMD_IOREAD_DWORD :
			return amiio_read32((void *)arg);

		case CMD_LOCK_KB:
		//	disable_irq(1);
			return 0;

		case CMD_UNLOCK_KB:
		//	enable_irq(1);
			return 0;

		case CMD_GET_DRIVER_INFO :
			return amidrv_getinfo((void *)arg);
	}

	return -ENOTTY;
}

static int amifldrv_mmap(struct file *file, struct vm_area_struct *vma)
{
	int				status;
	unsigned long	size = vma->vm_end - vma->vm_start;

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) return -EINVAL;
	vma->vm_flags |= VM_LOCKED;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	status = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, PAGE_SHARED);
#else
	status = remap_page_range(vma, vma->vm_start, vma->vm_pgoff << PAGE_SHIFT, size, PAGE_SHARED);
#endif

	return status ? -ENXIO : 0;
}

static struct file_operations amifldrv_fops =
{
	owner			: THIS_MODULE,
	open			: amifldrv_open,
	release			: amifldrv_release,
#if defined(HAVE_UNLOCKED_IOCTL)
	unlocked_ioctl	: amifldrv_unlocked_ioctl,
#else
	ioctl			: amifldrv_ioctl,
#endif
	mmap			: amifldrv_mmap
};

static int amifldrv_init_module(void)
{
	Major = register_chrdev(0, AMIFLDRV_NAME, &amifldrv_fops);
	if (Major < 0) return (-EIO);

	// initialize physical memory module
	amimemdrv_init();

	return 0;
}

static void amifldrv_cleanup_module(void)
{
	// release allocated resources
	amimemdrv_release();

	unregister_chrdev(Major, AMIFLDRV_NAME);
}

module_init(amifldrv_init_module);
module_exit(amifldrv_cleanup_module);

//**********************************************************************
//**********************************************************************
//**                                                                  **
//**        (C)Copyright 1985-2019, American Megatrends, Inc.         **
//**                                                                  **
//**                       All Rights Reserved.                       **
//**                                                                  **
//**      5555 Oakbrook Parkway, Suite 200, Norcross, GA 30093        **
//**                                                                  **
//**                       Phone: (770)-246-8600                      **
//**                                                                  **
//**********************************************************************
//**********************************************************************
