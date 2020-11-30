

/*================================================================
 *   
 *   
 *   文件名称：romfs.c
 *   创 建 者：肖飞
 *   创建日期：2020年11月30日 星期一 17时01分36秒
 *   修改日期：2020年11月30日 星期一 17时13分41秒
 *   描    述：
 *
 *================================================================*/
#include <rtthread.h>
#include <dfs_romfs.h>

const static struct romfs_dirent _dummy[] =
{
};

const struct romfs_dirent _root_dirent[] =
{
    {ROMFS_DIRENT_DIR, "sda0", (rt_uint8_t *)_dummy, sizeof(_dummy) / sizeof(_dummy[0])},
    {ROMFS_DIRENT_DIR, "sda1", (rt_uint8_t *)_dummy, sizeof(_dummy) / sizeof(_dummy[0])},
};

const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_root_dirent, sizeof(_root_dirent) / sizeof(_root_dirent[0])
};
