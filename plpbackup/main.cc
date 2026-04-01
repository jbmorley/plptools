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
#include <path.h>
#include <plpdirent.h>
#include <plpintl.h>
#include <rclip.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <tcpsocket.h>

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
    size_t totalSize_;
    size_t completedSize_;

};

int main(int argc, char **argv) {

    TCPSocket *skt;
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
    std::string backupPath = Path::appending_component(Path::get_cwd(), "backup", Path::kHostSeparator);
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
    RFSV *rfsv = rf->create(false);

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
        std::string drivePath = Path::appending_component(backupPath, letter, Path::kHostSeparator);
        if (mkdir(drivePath.c_str(), 0755) != 0) {
            cout << "Failed to create directory '" << drivePath << "'." << endl;
            return EXIT_FAILURE;
        }
    }

    // Work out how much we're backing up and print the summary.
    auto totalSize = std::accumulate(files.begin(), files.end(), 0ULL,
        [](unsigned long long sum, const PlpDirent& file) {
            return sum + file.getSize();
        });
    cout << "Backing up " << files.size() << " files (" << totalSize << " bytes)..." << endl;

    // Copy the files.
    size_t completedSize = 0;
    for (const PlpDirent &file : files) {

        // Determine the destination path.
        std::vector<std::string> components = Path::split(file.getPath(), Path::kEPOCSeparator);
        components[0] = components[0][0];  // Remove the colon from the drive letter.
        std::string destinationPath = Path::appending_components(backupPath, components, Path::kHostSeparator);

        if (file.isDirectory()) {
            if (mkdir(destinationPath.c_str(), 0755) != 0) {
                cout << "Failed to create directory '" << destinationPath << "'." << endl;
                return EXIT_FAILURE;
            }
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.getPath() << std::flush;
            }
        } else {
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.getPath() << std::flush;
            }
            ProgressCallbackContext context = ProgressCallbackContext{file.getPath(), totalSize, completedSize};
            rfsv->copyFromPsion(  // TODO: Check for errors.
                file.getPath().c_str(),
                destinationPath.c_str(),
                (void *)&context,
                [](void *context, uint32_t size) {
                    ProgressCallbackContext *progressContext = static_cast<ProgressCallbackContext *>(context);
                    float_t progress =  (float_t)(progressContext->completedSize_ + size) / (float_t)progressContext->totalSize_;
                    cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << progressContext->path_ << std::flush;
                    return 1;
                });
            completedSize += file.getSize();
            {
                float_t progress =  (float_t)completedSize / (float_t)totalSize;
                cout << "\r\033[2K" << "[" << std::setw(3) << std::right << static_cast<int>(progress * 100) << "%] " << file.getPath() << std::flush;
            }
        }
    }

    cout << "\n";

    delete rfsv;
    delete rf;

    return EXIT_SUCCESS;
}
