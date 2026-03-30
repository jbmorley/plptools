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
#include "plpdirent.h"

#include <cli_utils.h>
#include <cmath>
#include <cstdlib>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <rclip.h>
#include <plpintl.h>
#include <stdint.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <tcpsocket.h>
#include <bufferstore.h>
#include <semaphore.h>
#include <path.h>

#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#include <stdlib.h>
#include <stdio.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>

using namespace std;

static void help() {
    cout << _(
        "Usage: plpbackup [OPTIONS]... [FTPCOMMAND]\n"
        "\n"
        "Simple one-shot backup command; performs a full, non-incremental, backup.\n"
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

struct QualifiedDirectoryEntry {

    std::string parentPath_;
    PlpDirent directoryEntry_;

    std::string path() const {
        if (directoryEntry_.isDirectory()) {
            return parentPath_ + directoryEntry_.getName() + "\\";
        } else {
            return parentPath_ + directoryEntry_.getName();
        }
    }

};

struct ProgressCallbackContext {

    std::string path_;
    size_t totalSize_;
    size_t completedSize_;

};

std::vector<char> map_devices(uint32_t deviceBits) {
    std::vector<char> result;
    for (int i = 0; i < 26; i++) {
        if (deviceBits & (1 << i)) {
            result.push_back('A' + i);
        }
    }
    return result;
}

std::vector<QualifiedDirectoryEntry> dir(RFSV *rfsv, const std::string &path, bool recursive) {
    std::vector<QualifiedDirectoryEntry> files;
    PlpDir entries;
    rfsv->dir(path.c_str(), entries);  // TODO: Check errors.
    for (PlpDirent entry: entries) {
        files.push_back(QualifiedDirectoryEntry{path, entry});
    }
    if (!recursive) {
        return files;
    }
    std::vector<QualifiedDirectoryEntry> result;
    for(const QualifiedDirectoryEntry &file : files) {
        result.push_back(file);
        if (!file.directoryEntry_.isDirectory()) {
            continue;
        }
        std::vector<QualifiedDirectoryEntry> contents = dir(rfsv, file.path(), recursive);
        result.insert(result.end(), contents.begin(), contents.end());
    }
    return result;
}

Enum<RFSV::errs> listDrives(RFSV *rfsv, std::vector<PlpDrive> &_drives) {
    Enum<RFSV::errs> result;

    // Get the supported drive letters.
    uint32_t deviceBits = 0;
    result = rfsv->devlist(deviceBits);
    if (result != RFSV::E_PSI_GEN_NONE) {
        return result;
    }
    auto devices = map_devices(deviceBits);

    // Iterate over the devices and get the info for the available drives.
    std::vector<PlpDrive> drives;
    for (const auto &device : devices) {
        PlpDrive driveInfo;
        result = rfsv->devinfo(device, driveInfo);
        if (result == RFSV::E_PSI_FILE_NOTREADY) {
            // Ignore drives that aren't available.
            continue;
        }
        if (result != RFSV::E_PSI_GEN_NONE) {
            return result;
        }
        drives.push_back(driveInfo);
    }
    _drives = drives;
    return RFSV::E_PSI_GEN_NONE;
}

int main(int argc, char **argv) {

    TCPSocket *skt;
    string host = "127.0.0.1";
    int sockNum = cli_utils::lookup_default_port();

    setlocale (LC_ALL, "");
    textdomain(PACKAGE);

    while (1) {
        int c = getopt_long(argc, argv, "hVp:", opts, NULL);
        if (c == -1)
            break;
        switch (c) {
            case '?':
                usage();
                return -1;
            case 'V':
                cout << _("plpftp Version ") << VERSION << endl;
                return 0;
            case 'h':
                help();
                return 0;
            case 'p':
                if (!cli_utils::parse_port(optarg, &host, &sockNum)) {
                    cout << _("Invalid port definition.") << endl;
                    return 1;
                }
                break;
        }
    }
    if (optind == argc) {
        backupHeader();
    }

    // Create the destination directory.
    std::string backupPath = Path::appending_component(Path::get_cwd(), "backup");
    if (mkdir(backupPath.c_str(), 0755) != 0) {
        cout << "Backup directory '" << backupPath << "' exists." << endl;
        return EXIT_FAILURE;
    }

    skt = new TCPSocket();
    if (!skt->connect(host.c_str(), sockNum)) {
        cout << _("Could not connect to ncpd.") << endl;
        return EXIT_FAILURE;
    }
    rfsvfactory *rf = new rfsvfactory(skt);
    RFSV *fs = rf->create(false);

    cout << "Listing drives..." << endl;
    std::vector<PlpDrive> drives;
    if (listDrives(fs, drives) != RFSV::E_PSI_GEN_NONE) {
        cout << "Failed to list drives." << endl;
        return EXIT_FAILURE;
    }

    std::vector<QualifiedDirectoryEntry> files;
    for (const auto &drive : drives) {
        cout << "Listing " << drive.getPath() << "..." << endl;
        if (static_cast<MediaType>(drive.getMediaType()) == MediaType::kROM) {
            cout << "Skipping ROM drive..." << endl;
            continue;
        }
        auto driveFiles = dir(fs, drive.getPath(), true);
        files.insert(files.end(), driveFiles.begin(), driveFiles.end());
    }

    // Create directories for all drives we're backing up.
    for (const auto &drive : drives) {
        // TODO: Do this filtering earlier.
        if (static_cast<MediaType>(drive.getMediaType()) == MediaType::kROM) {
            continue;
        }
        std::string letter;
        letter += drive.getDriveLetter();
        std::string drivePath = Path::appending_component(backupPath, letter);
        if (mkdir(drivePath.c_str(), 0755) != 0) {
            cout << "Failed to create directory '" << drivePath << "'." << endl;
            return EXIT_FAILURE;
        }
    }

    // Work out how much we're backing up and print the summary.
    auto totalSize = std::accumulate(files.begin(), files.end(), 0ULL,
        [](unsigned long long sum, const QualifiedDirectoryEntry& file) {
            return sum + file.directoryEntry_.getSize();
        });
    cout << "Backing up " << files.size() << " files (" << totalSize << " bytes)..." << endl;

    // Copy the files.
    size_t completedSize = 0;
    for (const QualifiedDirectoryEntry &file : files) {

        // Determine the destination path.
        std::vector<std::string> components = Path::split(file.path(), '\\');
        auto drive = components[0][0];
        components[0] = drive;
        // components.erase(components.begin());  // Drop the drive.
        std::string destinationPath = Path::appending_components(backupPath, components);

        if (file.directoryEntry_.isDirectory()) {
            if (mkdir(destinationPath.c_str(), 0755) != 0) {
                cout << "Failed to create directory '" << destinationPath << "'." << endl;
                return EXIT_FAILURE;
            }
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.path() << std::flush;
            }
        } else {
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.path() << std::flush;
            }
            ProgressCallbackContext context = ProgressCallbackContext{file.path(), totalSize, completedSize};
            fs->copyFromPsion(  // TODO: Check for errors.
                file.path().c_str(),
                destinationPath.c_str(),
                (void *)&context,
                [](void *context, uint32_t size) {
                    ProgressCallbackContext *progressContext = static_cast<ProgressCallbackContext *>(context);
                    float_t progress =  (float_t)(progressContext->completedSize_ + size) / (float_t)progressContext->totalSize_;
                    cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << progressContext->path_ << std::flush;
                    return 1;
                });
            completedSize += file.directoryEntry_.getSize();
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.path() << std::flush;
            }
        }
    }

    delete rf;
    return 0;
}
