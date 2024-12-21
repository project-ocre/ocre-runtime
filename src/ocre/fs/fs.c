/**
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ocre/ocre.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(filesystem, OCRE_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/flash_map.h>
#include "fs.h"

#define MAX_PATH_LEN LFS_NAME_MAX

#ifdef CONFIG_SHELL
#define print(shell, level, fmt, ...)                                                                                  \
    do {                                                                                                               \
        shell_fprintf(shell, level, fmt, ##__VA_ARGS__);                                                               \
    } while (false)
#else
#define print(shell, level, fmt, ...)                                                                                  \
    do {                                                                                                               \
        printk(fmt, ##__VA_ARGS__);                                                                                    \
    } while (false)
#endif

static int lsdir(const char *path) {
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, path);
    if (res) {
        LOG_ERR("Error opening dir %s [%d]\n", path, res);
        return res;
    }

    for (;;) {
        // Verify fs_readdir()
        res = fs_readdir(&dirp, &entry);

        // entry.name[0] == 0 means end-of-dir
        if (res || entry.name[0] == 0) {
            if (res < 0) {
                LOG_ERR("Error reading dir [%d]\n", res);
            }
            break;
        }
    }

    // Verify fs_closedir()
    fs_closedir(&dirp);

    return res;
}

static int littlefs_flash_erase(unsigned int id) {
    const struct flash_area *pfa;
    int rc;

    rc = flash_area_open(id, &pfa);
    if (rc < 0) {
        LOG_ERR("FAIL: unable to find flash area %u: %d\n", id, rc);
        return rc;
    }

    LOG_PRINTK("Area %u at 0x%x on %s for %u bytes\n", id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
               (unsigned int)pfa->fa_size);

    rc = flash_area_erase(pfa, 0, pfa->fa_size);

    if (rc < 0) {
        LOG_ERR("Failed to erase flash: %d", rc);
    } else {
        LOG_INF("Successfully erased flash");
    }

    flash_area_close(pfa);

    return rc;
}

#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else  /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(user_data);
static struct fs_mount_t lfs_storage_mnt = {
        .type = FS_LITTLEFS,
        .fs_data = &user_data,
        .storage_dev = (void *)FIXED_PARTITION_ID(user_data_partition),
        .mnt_point = FS_MOUNT_POINT,
};
#endif /* PARTITION_NODE */

struct fs_mount_t *mp =
#if DT_NODE_EXISTS(PARTITION_NODE)
        &FS_FSTAB_ENTRY(PARTITION_NODE)
#else
        &lfs_storage_mnt
#endif
        ;

static int littlefs_mount(struct fs_mount_t *mp) {
    int rc = 0;

    /* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) || !(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
    rc = fs_mount(mp);
    if (rc < 0) {
        LOG_ERR("FAIL: mount id %" PRIuPTR " at %s: %d\n", (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
        return rc;
    }
    LOG_INF("%s mount: %d\n", mp->mnt_point, rc);
#else
    LOG_INF("%s automounted\n", mp->mnt_point);
#endif

    return rc;
}

#ifdef CONFIG_APP_LITTLEFS_STORAGE_BLK_SDMMC
struct fs_littlefs lfsfs;
static struct fs_mount_t __mp = {
        .type = FS_LITTLEFS,
        .fs_data = &lfsfs,
        .flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

struct fs_mount_t *mp = &__mp;

static int littlefs_mount(struct fs_mount_t *mp) {
    static const char *disk_mount_pt = "/" CONFIG_SDMMC_VOLUME_NAME ":";
    static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;

    mp->storage_dev = (void *)disk_pdrv;
    mp->mnt_point = disk_mount_pt;

    return fs_mount(mp);
}
#endif /* CONFIG_APP_LITTLEFS_STORAGE_BLK_SDMMC */

void ocre_app_storage_init() {
    struct fs_dirent entry;
    struct fs_statvfs sbuf;
    int rc;

    rc = littlefs_mount(mp);
    if (rc < 0) {
        return;
    }

    rc = fs_statvfs(mp->mnt_point, &sbuf);
    if (rc < 0) {
        LOG_ERR("FAILR statvfs: %d", rc);
        return;
    }

    LOG_DBG("%s: bsize = %lu ; frsize = %lu ;"
            " blocks = %lu ; bfree = %lu",
            mp->mnt_point, sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);

    rc = lsdir(mp->mnt_point);
    if (rc < 0) {
        LOG_ERR("FAIL: lsdir %s: %d", mp->mnt_point, rc);
    }

    if (fs_stat(OCRE_BASE_PATH, &entry) == -ENOENT) {
        fs_mkdir(OCRE_BASE_PATH);
    }

    if (fs_stat(APP_RESOURCE_PATH, &entry) == -ENOENT) {
        fs_mkdir(APP_RESOURCE_PATH);
    }
}

static int cmd_flash_format(const struct shell *shell, size_t argc, char *argv[]) {
    int rc;
    fs_unmount(mp);

    // FIXME: if erasing the whole chip, could cause problems
    // https://github.com/zephyrproject-rtos/zephyr/issues/56442
    rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);

    if (rc < 0) {
        print(shell, SHELL_WARNING, "Format failed: %d\n", rc);
    } else {
        print(shell, SHELL_NORMAL, "Format succeeded\n");
    }

    if (rc == 0) {
        LOG_INF("Mounting...");
        rc = littlefs_mount(mp);
    }

    return rc;
}

SHELL_STATIC_SUBCMD_SET_CREATE(flash_commands,
                               SHELL_CMD(format, NULL, "Format the flash storage device (all user data will be lost)",
                                         cmd_flash_format),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(user_storage, &flash_commands, "User storage management", NULL);
