#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* pico.toml 格式（极简）:
   [package]
   name = "myapp"
   version = "0.1.0"

   [dependencies]
   utils = "ZerexaNet/pico-utils@v1.0.0"
*/

static void ensure_modules_dir(void) {
    system("mkdir -p pico_modules");
}

static int pkg_install(const char *pkg) {
    /* pkg 格式: owner/repo 或 owner/repo@tag */
    char repo[256], tag[64];
    tag[0] = '\0';
    const char *at = strchr(pkg, '@');
    if (at) {
        int rlen = (int)(at - pkg);
        if (rlen >= (int)sizeof(repo)) { fprintf(stderr, "包名过长\n"); return 1; }
        strncpy(repo, pkg, rlen); repo[rlen] = '\0';
        strncpy(tag, at + 1, sizeof(tag) - 1);
    } else {
        strncpy(repo, pkg, sizeof(repo) - 1);
    }

    /* 取仓库名作为本地目录名 */
    const char *slash = strrchr(repo, '/');
    const char *name  = slash ? slash + 1 : repo;

    ensure_modules_dir();

    char cmd[512];
    if (tag[0]) {
        snprintf(cmd, sizeof(cmd),
            "git clone --depth=1 --branch %s https://github.com/%s pico_modules/%s 2>&1",
            tag, repo, name);
    } else {
        snprintf(cmd, sizeof(cmd),
            "git clone --depth=1 https://github.com/%s pico_modules/%s 2>&1",
            repo, name);
    }

    printf("安装 %s → pico_modules/%s\n", pkg, name);
    int ret = system(cmd);
    if (ret == 0) printf("安装成功：pico_modules/%s\n", name);
    else          fprintf(stderr, "安装失败（git 返回 %d）\n", ret);
    return ret;
}

static int pkg_remove(const char *name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf pico_modules/%s", name);
    printf("移除 pico_modules/%s\n", name);
    return system(cmd);
}

static void pkg_list(void) {
    printf("已安装的包（pico_modules/）：\n");
    system("ls pico_modules/ 2>/dev/null || echo '  （空）'");
}

int pkg_main(int argc, char **argv) {
    if (argc < 1) { fprintf(stderr, "用法: pico install <owner/repo[@tag]>\n"); return 1; }
    const char *sub = argv[0];
    if (strcmp(sub, "install") == 0 || strcmp(sub, "add") == 0) {
        if (argc < 2) { fprintf(stderr, "用法: pico install <owner/repo[@tag]>\n"); return 1; }
        return pkg_install(argv[1]);
    }
    if (strcmp(sub, "remove") == 0 || strcmp(sub, "rm") == 0) {
        if (argc < 2) { fprintf(stderr, "用法: pico remove <包名>\n"); return 1; }
        return pkg_remove(argv[1]);
    }
    if (strcmp(sub, "list") == 0 || strcmp(sub, "ls") == 0) {
        pkg_list(); return 0;
    }
    fprintf(stderr, "未知子命令: %s\n用法: pico install|remove|list\n", sub);
    return 1;
}
