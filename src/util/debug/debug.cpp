/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pwd.h>
#include <unistd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/stat.h>

#include "util/logger.h"
#include "debug.h"

namespace linglong {

void dumpIDMap()
{
    logDbg() << "dumpIDMap start -----------";
    std::ifstream uidMap("/proc/self/uid_map");
    for (std::string line; getline(uidMap, line);) {
        logDbg() << "uid_map of pid:" << getpid() << line;
    }

    std::ifstream gidMap("/proc/self/gid_map");
    for (std::string line; getline(gidMap, line);) {
        logDbg() << "gid_map of pid:" << getpid() << line;
    }

    auto setgroupsPath = util::format("/proc/self/setgroups");
    std::ifstream setgroupsFileRead(setgroupsPath);
    std::string line;
    std::getline(setgroupsFileRead, line);
    logDbg() << "setgroups of pid:" << getpid() << line;
    logDbg() << "dumpIDMap end -----------";
}

void dumpUidGidGroup()
{
    logDbg() << "dumpUidGidGroup start -----------";
    //    __uid_t uid = getuid(); // you can change this to be the uid that you want
    //
    //    struct passwd *pw = getpwuid(uid);
    //    if (pw == NULL) {
    //        perror("getpwuid error: ");
    //    }
    //
    //    int ngroups = 0;
    //
    //    // this call is just to get the correct ngroups
    //    getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
    //    __gid_t groups[ngroups];
    //
    //    // here we actually get the groups
    //    getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
    //
    //    // example to print the groups name
    //    for (int i = 0; i < ngroups; i++) {
    //        struct group *gr = getgrgid(groups[i]);
    //        if (gr == NULL) {
    //            perror("getgrgid error: ");
    //        }
    //        printf("%s\n", gr->gr_name);
    //    }

    logDbg() << "getuid" << getuid() << "geteuid" << geteuid();
    logDbg() << "getgid" << getgid() << "getegid" << getegid();
    const int groupSize = getgroups(0, NULL);
    __gid_t list[groupSize + 1];
    getgroups(groupSize, list);

    std::string groupListStr;
    for (int i = 0; i < groupSize; ++i) {
        groupListStr += util::format("%d ", list[i]);
    }
    logDbg() << "getgroups size " << groupSize << ", list:" << groupListStr;
    logDbg() << "dumpUidGidGroup end -----------";
}

void dumpFilesystem(const std::string &path)
{
    logDbg() << "dumpFilesystem begin -----------";
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(path.c_str())) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
            logDbg() << path + "/" + ent->d_name;
        }
        closedir(dir);
    } else {
        /* could not open directory */
        logErr() << linglong::util::errnoString() << errno;
        return;
    }
    logDbg() << "dumpFilesystem end -----------";
}

void dumpFileInfo(const std::string &path)
{
    struct stat destStat {
    };
    lstat(path.c_str(), &destStat);
    logDbg() << path << destStat.st_uid << destStat.st_gid << ((destStat.st_mode & S_IFMT) == S_IFDIR);
}
} // namespace linglong