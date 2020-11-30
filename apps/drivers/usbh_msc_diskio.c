

/*================================================================
 *
 *
 *   文件名称：usbh_msc_diskio.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月30日 星期一 14时11分30秒
 *   修改日期：2020年11月30日 星期一 17时14分19秒
 *   描    述：
 *
 *================================================================*/
#include "usbh_msc_diskio.h"
#include "usbh_def.h"
#include "usbh_msc.h"
#include "diskio.h"
#include "dfs_fs.h"

#define debug(fmt, ...) rt_kprintf("[%s:%s:%d] " fmt, __FILE__, __func__, __LINE__, ## __VA_ARGS__)

extern USBH_HandleTypeDef hUsbHostFS;

typedef struct {
	/* inherit from rt_device */
	struct rt_device parent;
	char name[64];
	uint8_t lun;
} usb_msc_info_t;


#define _USE_BUFF_WO_ALIGNMENT  1
#define _USE_WRITE	1	/* 1: Enable disk_write function */
#define _USE_IOCTL	1	/* 1: Enable disk_ioctl fucntion */

/**
  * @brief  Initializes a Drive
  * @param  lun : lun id
  * @retval DSTATUS: Operation status
  */
DSTATUS USBH_initialize(BYTE lun)
{
	return RES_OK;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : lun id
  * @retval DSTATUS: Operation status
  */
DSTATUS USBH_status(BYTE lun)
{
	DRESULT res = RES_ERROR;

	if(USBH_MSC_UnitIsReady(&hUsbHostFS, lun)) {
		res = RES_OK;
	} else {
		res = RES_ERROR;
	}

	return res;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : lun id
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USBH_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res = RES_ERROR;
	MSC_LUNTypeDef info;
	USBH_StatusTypeDef  status = USBH_OK;

	if ((DWORD)buff & 3) { /* DMA Alignment issue, do single up to aligned buffer */
#if _USE_BUFF_WO_ALIGNMENT == 0

		while ((count--) && (status == USBH_OK)) {
			status = USBH_MSC_Read(&hUsbHostFS, lun, sector + count, (uint8_t *)scratch, 1);

			if(status == USBH_OK) {
				memcpy (&buff[count * _MAX_SS], scratch, _MAX_SS);
			} else {
				break;
			}
		}

#else
		return res;
#endif
	} else {
		status = USBH_MSC_Read(&hUsbHostFS, lun, sector, buff, count);
	}

	if(status == USBH_OK) {
		res = RES_OK;
	} else {
		USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info);

		switch (info.sense.asc) {
			case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
			case SCSI_ASC_MEDIUM_NOT_PRESENT:
			case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
				USBH_ErrLog ("USB Disk is not ready!");
				res = RES_NOTRDY;
				break;

			default:
				res = RES_ERROR;
				break;
		}
	}

	return res;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : lun id
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USBH_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res = RES_ERROR;
	MSC_LUNTypeDef info;
	USBH_StatusTypeDef  status = USBH_OK;

	if ((DWORD)buff & 3) { /* DMA Alignment issue, do single up to aligned buffer */
#if _USE_BUFF_WO_ALIGNMENT == 0

		while (count--) {
			memcpy (scratch, &buff[count * _MAX_SS], _MAX_SS);

			status = USBH_MSC_Write(&hUsbHostFS, lun, sector + count, (BYTE *)scratch, 1) ;

			if(status == USBH_FAIL) {
				break;
			}
		}

#else
		return res;
#endif
	} else {
		status = USBH_MSC_Write(&hUsbHostFS, lun, sector, (BYTE *)buff, count);
	}

	if(status == USBH_OK) {
		res = RES_OK;
	} else {
		USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info);

		switch (info.sense.asc) {
			case SCSI_ASC_WRITE_PROTECTED:
				USBH_ErrLog("USB Disk is Write protected!");
				res = RES_WRPRT;
				break;

			case SCSI_ASC_LOGICAL_UNIT_NOT_READY:
			case SCSI_ASC_MEDIUM_NOT_PRESENT:
			case SCSI_ASC_NOT_READY_TO_READY_CHANGE:
				USBH_ErrLog("USB Disk is not ready!");
				res = RES_NOTRDY;
				break;

			default:
				res = RES_ERROR;
				break;
		}
	}

	return res;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  lun : lun id
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USBH_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	DRESULT res = RES_ERROR;
	MSC_LUNTypeDef info;

	switch (cmd) {
		/* Make sure that no pending write process */
		case CTRL_SYNC:
			res = RES_OK;
			break;

		/* Get number of sectors on the disk (DWORD) */
		case GET_SECTOR_COUNT :
			if(USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info) == USBH_OK) {
				*(DWORD *)buff = info.capacity.block_nbr;
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}

			break;

		/* Get R/W sector size (WORD) */
		case GET_SECTOR_SIZE :
			if(USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info) == USBH_OK) {
				*(DWORD *)buff = info.capacity.block_size;
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}

			break;

		/* Get erase block size in unit of sector (DWORD) */
		case GET_BLOCK_SIZE :

			if(USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info) == USBH_OK) {
				*(DWORD *)buff = info.capacity.block_size;
				res = RES_OK;
			} else {
				res = RES_ERROR;
			}

			break;

		default:
			res = RES_PARERR;
	}

	return res;
}
#endif /* _USE_IOCTL == 1 */


static rt_err_t rt_usb_msc_init(rt_device_t dev)
{
	RT_ASSERT(dev != RT_NULL);

	usb_msc_info_t *usb_msc_info = (usb_msc_info_t *)dev->user_data;
	uint8_t lun = usb_msc_info->lun;

	return (USBH_initialize(lun) == RES_OK) ? RT_EOK : RT_ERROR;
}

static rt_err_t rt_usb_msc_open(rt_device_t dev, rt_uint16_t oflag)
{
	RT_ASSERT(dev != RT_NULL);
	return RT_EOK;
}

static rt_err_t rt_usb_msc_close(rt_device_t dev)
{
	RT_ASSERT(dev != RT_NULL);
	return RT_EOK;
}

static rt_size_t rt_usb_msc_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	RT_ASSERT(dev != RT_NULL);

	rt_size_t count = 0;
	usb_msc_info_t *usb_msc_info = (usb_msc_info_t *)dev->user_data;
	uint8_t lun = usb_msc_info->lun;

	DRESULT ret = USBH_read(lun, buffer, pos, size);

	if(ret == RES_OK) {
		count = size;
	}

	return count;
}

static rt_size_t rt_usb_msc_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	RT_ASSERT(dev != RT_NULL);

	rt_size_t count = 0;
	usb_msc_info_t *usb_msc_info = (usb_msc_info_t *)dev->user_data;
	uint8_t lun = usb_msc_info->lun;

	DRESULT ret = USBH_write(lun, buffer, pos, size);

	if(ret == RES_OK) {
		count = size;
	}

	return count;
}

static rt_err_t rt_usb_msc_control(rt_device_t dev, int cmd, void *args)
{
	RT_ASSERT(dev != RT_NULL);
	rt_err_t err = -RT_ERROR;
	usb_msc_info_t *usb_msc_info = (usb_msc_info_t *)dev->user_data;
	uint8_t lun = usb_msc_info->lun;

	switch(cmd) {
		case RT_DEVICE_CTRL_BLK_GETGEOME: {
			struct rt_device_blk_geometry *geometry = (struct rt_device_blk_geometry *)args;
			MSC_LUNTypeDef info;

			if(USBH_MSC_GetLUNInfo(&hUsbHostFS, lun, &info) == USBH_OK) {
				err = RES_OK;
				geometry->sector_count = info.capacity.block_nbr;
				geometry->bytes_per_sector = info.capacity.block_size;
				geometry->block_size = info.capacity.block_size;
			}
		}
		break;

		case RT_DEVICE_CTRL_BLK_SYNC: {
			err = RES_OK;
		}
		break;

		case RT_DEVICE_CTRL_BLK_ERASE: {
		}
		break;

		default: {
		}
		break;
	}

	return err;
}

void rt_hw_usb_msc_init()
{
	int lun;
	rt_err_t ret;

	for(lun = 0; lun < RT_DFS_ELM_DRIVES; lun++) {
		usb_msc_info_t *usb_msc_info = (usb_msc_info_t *)rt_malloc(sizeof(usb_msc_info_t));

		RT_ASSERT(usb_msc_info != RT_NULL);
		usb_msc_info->lun = lun;

		usb_msc_info->parent.type = RT_Device_Class_Block;
		usb_msc_info->parent.init = rt_usb_msc_init;
		usb_msc_info->parent.open = rt_usb_msc_open;
		usb_msc_info->parent.close = rt_usb_msc_close;
		usb_msc_info->parent.read = rt_usb_msc_read;
		usb_msc_info->parent.write = rt_usb_msc_write;
		usb_msc_info->parent.control = rt_usb_msc_control;
		/* no private */
		usb_msc_info->parent.user_data = usb_msc_info;

		snprintf(usb_msc_info->name, 64, "usb0p%d", lun);
		debug("usb_msc_info->name:%s\n", usb_msc_info->name);
		ret = rt_device_register(&usb_msc_info->parent, usb_msc_info->name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

		RT_ASSERT(ret == RT_EOK);

	}


	return;
}

void usbh_msc_mount(void)
{
	int lun;
	char name[64];
	char path[64];

	for(lun = 0; lun < 1; lun++) {
		snprintf(name, 64, "usb0p%d", lun);
		snprintf(path, 64, "/sda%d", lun);

		if (dfs_mount(name, path, "elm", 0, 0) == 0) {
			rt_kprintf("File System %s mount to %s successed!\n", name, path);
		} else {
			rt_kprintf("File System %s mount to %s failed!\n", name, path);
		}
	}
}

void usbh_msc_umount(void)
{
	int lun;
	char path[64];

	for(lun = 0; lun < 1; lun++) {
		snprintf(path, 64, "/sda%d", lun);

		if (dfs_unmount(path) == 0) {
			rt_kprintf("File System %s unmount successed!\n", path);
		} else {
			rt_kprintf("File System %s unmount failed!\n", path);
		}
	}
}
