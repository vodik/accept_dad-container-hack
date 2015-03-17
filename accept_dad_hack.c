#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <mntent.h>
#include <glob.h>
#include <sys/mount.h>

struct opt_t {
    const char *opt;
    int flag;
};

static int write_to(const char *msg, const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        warn("failed to open %s", path);
        return 1;
    }

    fputs(msg, fp);
    fclose(fp);
    return 0;
}

static int accept_dad_hack(void)
{
    int rc = 0, i;
    glob_t gl;

    if (glob("/proc/sys/net/ipv6/conf/*/accept_dad", 0, NULL, &gl) < 0)
        err(1, "failed to create glob");

    for (i = 0; i < (int)gl.gl_pathc; ++i)
        rc |= write_to("0", gl.gl_pathv[i]);

    globfree(&gl);
    return rc;
}

static int opt_to_flag(const char *opt, size_t len)
{
    static const struct opt_t opt_table[] = {
        { "rw", 0 },
        { "ro", MS_RDONLY },
        { "nosuid", MS_NOSUID },
        { "nodev", MS_NODEV },
        { "noexec", MS_NOEXEC },
        { "relatime", MS_RELATIME },
        { NULL, 0 }
    };

    const struct opt_t *n;
    for (n = opt_table; n->opt; ++n) {
        if (strncmp(opt, n->opt, len) == 0)
            return n->flag;
    }

    warnx("failed to interpret flag %.*s", (int)len, opt);
    return 0;
}

static int read_flags(const char *mnt_opts)
{
    int flags = 0;
    size_t pos = strcspn(mnt_opts, ",");

    for (; mnt_opts[pos] != '\0'; pos = strcspn(mnt_opts, ",")) {
        flags |= opt_to_flag(mnt_opts, pos);
        mnt_opts += pos + 1;
    }

    return flags | opt_to_flag(mnt_opts, pos);
}

static struct mntent *find_mnt_entry(FILE *mtab, const char *dir)
{
    static struct mntent mnt;
    struct mntent *m;
    char strings[4096];

    while ((m = getmntent_r(mtab, &mnt, strings, sizeof(strings)))) {
        if (strcmp(m->mnt_dir, dir) == 0)
            return m;
    }

    return NULL;
}

int main(void)
{
    FILE* mtab = setmntent("/etc/mtab", "r");
    struct mntent *proc = find_mnt_entry(mtab, "/proc/sys");
    endmntent(mtab);

    int flags = 0, rc = 0;

    if (proc) {
        flags = read_flags(proc->mnt_opts) & ~MS_RDONLY;

        if (mount(NULL, proc->mnt_dir, "none", MS_REMOUNT | flags, NULL) < 0)
            err(1, "failed to remount %s rw", proc->mnt_dir);
    }

    rc = accept_dad_hack();

    if (proc) {
        if (mount(NULL, proc->mnt_dir, "none", MS_REMOUNT | MS_RDONLY | flags, NULL) < 0)
            err(1, "failed to remount %s ro", proc->mnt_dir);
    }

    return rc;
}
