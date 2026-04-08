/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "config.h"

#include <bufferstore.h>
#include <cli_utils.h>
#include <cmath>
#include <cstdlib>
#include <drive.h>
#include <pathutils.h>
#include <plpdirent.h>
#include <plpintl.h>
#include <rclip.h>
#include <rfsv.h>
#include <rpcs.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <tcpsocket.h>

#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

using namespace std;

static void help() {
    cout << _(
        "Usage: plpbackup [OPTIONS]... <backup|restore> <destination>\n"
        "\n"
        "Simple one-shot backup command; performs a full, non-incremental, backup of all.\n"
        "writeable drives.\n"
        "\n"
        "Supported options:\n"
        "\n"
        " -h, --help              Display this text.\n"
        " -V, --version           Print version and exit.\n"
        " -p, --port=[HOST:]PORT  Connect to port PORT on host HOST.\n"
        "                         Default for HOST is 127.0.0.1\n"
        "                         Default for PORT is "
        ) << DPORT << "\n\n";
}

static void usage() {
    cerr << _("Try `plpbackup --help' for more information") << endl;
}

static struct option opts[] = {
    {"help",     no_argument,       nullptr, 'h'},
    {"version",  no_argument,       nullptr, 'V'},
    {"port",     required_argument, nullptr, 'p'},
    {NULL,       0,                 nullptr,  0 }
};

void backupHeader() {
    cout << _("plpbackup ") << VERSION << endl;
}

struct ProgressCallbackContext {

    std::string path_;
    off_t totalSize_;
    off_t completedSize_;

};

struct HostDirectoryEntry {
    std::string path;
    bool isDirectory;
    off_t size;
};

int dir(const std::string &path, const bool recursive, std::vector<HostDirectoryEntry> &_files) {

    // List the top-level directory.
    std::vector<std::string> children;
    DIR *directory = opendir(path.c_str());
    if (!directory) {
        return errno;
    }
    struct dirent *entry;
    while ((entry = readdir(directory)) != nullptr) {
        if (entry->d_name[0] == '.') {  // TODO: Do we want to ignore all hidden files?
            continue;
        }
        children.push_back(pathutils::appending_component(path, entry->d_name, pathutils::kHostSeparator));
    }
    closedir(directory);

    // Check each file, listing the inner directories if we're recursive.
    std::vector<HostDirectoryEntry> files;
    for (const auto &file : children) {
        struct stat st;
        if (stat(file.c_str(), &st) != 0) {
            return errno;
        }
        HostDirectoryEntry directoryEntry({file, S_ISDIR(st.st_mode), st.st_size});
        files.push_back(directoryEntry);
        if (!recursive) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            std::vector<HostDirectoryEntry> childFiles;
            int result = dir(file, recursive, childFiles);
            if (result != 0) {
                return result;
            }
            files.insert(files.end(), childFiles.begin(), childFiles.end());
        }
    }

    _files = files;

    return 0;
}

void log_progress(const std::string &path, float_t completedSize, float_t totalSize) {
    float_t progress =  completedSize / totalSize;
    cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << path << std::flush;
}

int backup(RFSV *rfsv, std::string backupPath) {

    // Create the destination directory.
    if (mkdir(backupPath.c_str(), 0755) != 0) {
        cout << "Backup directory '" << backupPath << "' exists." << endl;
        return EXIT_FAILURE;
    }

    // List the available drives, excluding ROM drives.
    cout << "Listing drives..." << endl;
    std::vector<Drive> drives;
    if (rfsv->drives(drives) != RFSV::E_PSI_GEN_NONE) {
        cout << "Failed to list drives." << endl;
        return EXIT_FAILURE;
    }
    drives.erase(
        std::remove_if(
            drives.begin(),
            drives.end(),
            [](const Drive &drive) {
                return (drive.getMediaType() == MediaType::kROM);
            }
        ),
        drives.end()
    );

    // Recursively list all the files.
    std::vector<PlpDirent> files;
    for (const auto &drive : drives) {
        cout << "Listing " << drive.getPath() << "..." << endl;
        std::vector<PlpDirent> driveFiles;
        if (rfsv->dir(drive.getPath(), true, driveFiles) != RFSV::E_PSI_GEN_NONE) {
            cout << "Failed to list files." << endl;
            return EXIT_FAILURE;
        }
        files.insert(files.end(), driveFiles.begin(), driveFiles.end());
    }

    // Create directories for all drives we're backing up.
    for (const auto &drive : drives) {
        std::string letter;
        letter += drive.getDriveLetter();
        std::string drivePath = pathutils::appending_component(backupPath, letter, pathutils::kHostSeparator);
        if (mkdir(drivePath.c_str(), 0755) != 0) {
            cout << "Failed to create directory '" << drivePath << "'." << endl;
            return EXIT_FAILURE;
        }
    }

    // Work out how much we're backing up and print the summary.
    off_t totalSize = std::accumulate(files.begin(), files.end(), 0ULL,
        [](off_t sum, const PlpDirent& file) {
            return sum + file.getSize();
        });
    cout << "Backing up " << files.size() << " files (" << totalSize << " bytes)..." << endl;

    // Copy the files.
    off_t completedSize = 0;
    for (const PlpDirent &file : files) {

        // Determine the destination path.
        std::vector<std::string> components = pathutils::split(file.getPath(), pathutils::kEPOCSeparator);
        components[0] = components[0][0];  // Remove the colon from the drive letter.
        std::string destinationPath = pathutils::appending_components(backupPath, components, pathutils::kHostSeparator);

        if (file.isDirectory()) {
            // Create directory.
            if (mkdir(destinationPath.c_str(), 0755) != 0) {
                cout << "Failed to create directory '" << destinationPath << "'." << endl;
                return EXIT_FAILURE;
            }
        } else {
            // Copy file.
            log_progress(file.getPath(), static_cast<float_t>(completedSize), static_cast<float_t>(totalSize));
            ProgressCallbackContext context = ProgressCallbackContext{file.getPath(), totalSize, completedSize};
            RFSV::errs result = rfsv->copyFromPsion(
                file.getPath().c_str(),
                destinationPath.c_str(),
                static_cast<void *>(&context),
                [](void *context, uint32_t size) {
                    ProgressCallbackContext *progressContext = static_cast<ProgressCallbackContext *>(context);
                    log_progress(
                        progressContext->path_,
                        progressContext->completedSize_ + size,
                        progressContext->totalSize_);
                    return 1;
                });
            if (result != RFSV::E_PSI_GEN_NONE) {
                cout << "Failed to copy file '" << file.getPath() << "'." << endl;
                return EXIT_FAILURE;
            }
            completedSize += file.getSize();
        }
        log_progress(file.getPath(), static_cast<float_t>(completedSize), static_cast<float_t>(totalSize));
    }

    cout << "\n";
}

struct BackupEntry {
    std::string localPath;
    std::string remotePath;
    bool isDirectory;
    off_t size;
};

int restore(RFSV *rfsv, const std::string &backupPath) {
    cout << "Restoring..." << endl;
    int result = 0;

    std::vector<std::string> driveLetters;
    std::vector<HostDirectoryEntry> drivePaths;
    if ((result = dir(backupPath, false, drivePaths)) != 0) {
        cout << "Failed to list drives to restore with error '" << strerror(result) << "'." << endl;
        return EXIT_FAILURE;
    }
    for (const auto &drivePath : drivePaths) {
        std::string driveLetter = drivePath.path.substr(backupPath.length() + 1);
        driveLetters.push_back(driveLetter);
    }

    std::vector<BackupEntry> backupEntries;
    for (const auto &driveLetter : driveLetters) {
        auto drivePath = pathutils::appending_component(backupPath, driveLetter, pathutils::kHostSeparator);
        cout << driveLetter << endl;
        std::vector<HostDirectoryEntry> files;
        if ((result = dir(drivePath, true, files)) != 0) {
            cout << "Failed to list directory with error '" << strerror(result) << "'." << endl;
            return EXIT_FAILURE;
        }
        for (const auto &file : files) {
            std::string relativePath = file.path.substr(drivePath.length() + 1);
            auto relativePathComponents = pathutils::split(relativePath, pathutils::kHostSeparator);
            std::string remotePath = pathutils::appending_components(driveLetter + ":", relativePathComponents, pathutils::kEPOCSeparator);
            backupEntries.push_back(BackupEntry({file.path, remotePath, file.isDirectory, file.size}));
        }
    }

    // Work out how much we're restoring up and print the summary.
    off_t totalSize = std::accumulate(backupEntries.begin(), backupEntries.end(), 0,
        [](off_t sum, const BackupEntry& entry) {
            return sum + entry.size;
        });
    cout << "Restoring up " << backupEntries.size() << " files (" << totalSize << " bytes)..." << endl;

    off_t completedSize = 0;
    for (const auto &backupEntry : backupEntries) {

        Enum<RFSV::errs> result;
        if (backupEntry.isDirectory) {
            result = rfsv->mkdir(backupEntry.remotePath.c_str());
            if (result != RFSV::E_PSI_GEN_NONE && result != RFSV::E_PSI_FILE_EXIST) {
                cout << "\nFailed to create directory '" << backupEntry.remotePath << "' with error '" << result << "'." << endl;
                return EXIT_FAILURE;
            }
        } else {
            ProgressCallbackContext context = ProgressCallbackContext{backupEntry.remotePath, totalSize, completedSize};
            result = rfsv->copyToPsion(
                backupEntry.localPath.c_str(),
               backupEntry.remotePath.c_str(),
               static_cast<void *>(&context),
               [](void *context, uint32_t size) {
                   ProgressCallbackContext *progressContext = static_cast<ProgressCallbackContext *>(context);
                   log_progress(
                       progressContext->path_,
                       progressContext->completedSize_ + size,
                       progressContext->totalSize_);
                   return 1;
                       });
            if (result != RFSV::E_PSI_GEN_NONE) {
                cout << "\nFailed to copy file '" << backupEntry.remotePath << "' with error '" << result << "'." << endl;
                return EXIT_FAILURE;
            }
            completedSize += backupEntry.size;
        }
        log_progress(backupEntry.remotePath, static_cast<float_t>(completedSize), static_cast<float_t>(totalSize));

    }
}

int main(int argc, char **argv) {

    string host = "127.0.0.1";
    int sockNum = cli_utils::lookup_default_port();

    setlocale(LC_ALL, "");
    textdomain(PACKAGE);

    while (1) {
        int c = getopt_long(argc, argv, "hVp:", opts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case '?':
                usage();
                return EXIT_FAILURE;
            case 'V':
                cout << _("plpftp Version ") << VERSION << endl;
                return EXIT_SUCCESS;
            case 'h':
                help();
                return EXIT_SUCCESS;
            case 'p':
                if (!cli_utils::parse_port(optarg, &host, &sockNum)) {
                    cout << _("Invalid port definition.") << endl;
                    return 1;
                }
                break;
        }
    }
    if (argc - optind != 2) {
        usage();
        return EXIT_FAILURE;
    }
    std::string command = argv[optind];
    if (command != "backup" && command != "restore") {
        usage();
        return EXIT_FAILURE;
    }
    std::string backupPath = argv[optind + 1];
    backupPath = pathutils::resolve_path(backupPath, pathutils::get_cwd(), pathutils::kHostSeparator);

    backupHeader();

    RFSV *rfsv = nullptr;
    if ((rfsv = RFSV::connect(host, sockNum)) == nullptr) {
        cout << _("Could not connect to ncpd.") << endl;
        return EXIT_FAILURE;
    }

    int result = EXIT_SUCCESS;
    if (command == "backup") {
        result = backup(rfsv, backupPath);
    } else if (command == "restore") {
        result = restore(rfsv, backupPath);
    }

    delete rfsv;

    return result;
}
