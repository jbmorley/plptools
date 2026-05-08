/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
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
#pragma once

#include <memory>
#include <string>

#include "connectionerror.h"
#include "Enum.h"
#include "device.h"

class rclip;
class RFSV;
class RPCS;

class DeviceEndpoint {
public:

    static std::unique_ptr<DeviceEndpoint> connect(const std::string host, int port, Enum<ConnectionError> *error);

    DeviceEndpoint(const std::string &id,
                   bool persistentId,
                   std::unique_ptr<RFSV> rfsv,
                   std::unique_ptr<RPCS> rpcs,
                   std::unique_ptr<rclip> clip);

    std::string id();

    Enum<RFSV::errs> getName(std::string &name);
    Enum<RFSV::errs> setName(const std::string &name);

    const std::unique_ptr<RFSV> rfsv_;
    const std::unique_ptr<RPCS> rpcs_;
    const std::unique_ptr<rclip> clip_;

private:
    const std::string id_;
    bool hasPersisentId_;
};
