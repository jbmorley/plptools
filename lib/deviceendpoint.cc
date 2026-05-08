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
#include "config.h"

#include "deviceendpoint.h"

#include <memory>
#include <string>

#include "connectionerror.h"
#include "device.h"
#include "deviceconfiguration.h"
#include "drive.h"
#include "plpdirent.h"
#include "rclip.h"
#include "rfsv.h"
#include "rpcs.h"
#include "uuid.h"
#include "bufferarray.h"

std::unique_ptr<DeviceEndpoint> DeviceEndpoint::connect(const std::string host,
                                                        int port,
                                                        Enum<ConnectionError> *error) {
    Enum<ConnectionError> internalError = ConnectionError::FACERR_NONE;
    auto rfsv = std::unique_ptr<RFSV>(RFSV::connect(host, port, &internalError));
    if (!rfsv) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    // Get the device configuration.
    Enum<RFSV::errs> result;
    auto deviceConfiguration = device::read_configuration(*rfsv, result);
    std::string id = deviceConfiguration ? deviceConfiguration->id() : uuid::uuid4();
    bool persistentId = static_cast<bool>(deviceConfiguration);

    auto rpcs = std::unique_ptr<RPCS>(RPCS::connect(host, port, &internalError));
    if (!rpcs) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    auto clip = std::unique_ptr<rclip>(rclip::connect(host, port, &internalError));
    if (!clip) {
        if (error) {
            *error = internalError;
        }
        return nullptr;
    }

    return std::unique_ptr<DeviceEndpoint>(
        new DeviceEndpoint(id, persistentId, std::move(rfsv), std::move(rpcs), std::move(clip)));
}

Result<DeviceEndpoint, Enum<ConnectionError>> DeviceEndpoint::connect(const std::string host, int port) {
    Enum<ConnectionError> error;
    auto deviceEndpoint = DeviceEndpoint::connect(host, port, &error);
    if (!deviceEndpoint) {
        return Result<DeviceEndpoint, Enum<ConnectionError>>::failure(error);
    }
    return Result<DeviceEndpoint, Enum<ConnectionError>>::success(std::move(deviceEndpoint));
}

DeviceEndpoint::DeviceEndpoint(const std::string &id,
                               bool persistentId,
                               std::unique_ptr<RFSV> rfsv,
                               std::unique_ptr<RPCS> rpcs,
                               std::unique_ptr<rclip> clip)
: rfsv_(std::move(rfsv))
, rpcs_(std::move(rpcs))
, clip_(std::move(clip))
, id_(id)
, hasPersistentId_(persistentId) {}

std::string DeviceEndpoint::id() const {
    return id_;
}

bool DeviceEndpoint::hasPersistentId() const {
    return hasPersistentId_;
}

Enum<RFSV::errs> DeviceEndpoint::getName(std::string &name) const {
    Enum<RFSV::errs> error = RFSV::E_PSI_GEN_NONE;
    auto deviceConfiguration = device::read_configuration(*rfsv_, error);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return error;
    }
    name = deviceConfiguration->name();
    return RFSV::E_PSI_GEN_NONE;
}

Result<std::string, Enum<RFSV::errs>> DeviceEndpoint::getName() const {
    Enum<RFSV::errs> error = RFSV::E_PSI_GEN_NONE;
    auto deviceConfiguration = device::read_configuration(*rfsv_, error);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return Result<std::string, Enum<RFSV::errs>>::failure(error);
    }
    return Result<std::string, Enum<RFSV::errs>>::success(deviceConfiguration->name());
}

Enum<RFSV::errs> DeviceEndpoint::setName(const std::string &name) {
    auto deviceConfiguration = std::make_unique<DeviceConfiguration>(id_, name);
    auto result = device::write_configuration(*rfsv_, *deviceConfiguration);
    if (result == RFSV::E_PSI_GEN_NONE) {
        hasPersistentId_ = true;
    }
    return result;
}

Result<uint32_t, Enum<RFSV::errs>> DeviceEndpoint::directoryCount(const std::string &path) {
    return Result<uint32_t, Enum<RFSV::errs>>::check(RFSV::E_PSI_GEN_NONE, [&](uint32_t &count) {
        return rfsv_->dircount(path.c_str(), count);
    });
}

Result<std::vector<Drive>, Enum<RFSV::errs>> DeviceEndpoint::drives() {
    return Result<std::vector<Drive>, Enum<RFSV::errs>>::check(RFSV::E_PSI_GEN_NONE, [&](std::vector<Drive> &drives) {
        return rfsv_->drives(drives);
    });
}

Result<std::vector<PlpDirent>, Enum<RFSV::errs>> DeviceEndpoint::dir(const std::string &path) {
    PlpDir dirent;
    auto error = rfsv_->dir(path.c_str(), dirent);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return Result<std::vector<PlpDirent>, Enum<RFSV::errs>>::failure(error);
    }
    std::vector<PlpDirent> result(dirent.begin(), dirent.end());
    return Result<std::vector<PlpDirent>, Enum<RFSV::errs>>::success(result);
}

Result<std::vector<std::string>, Enum<RFSV::errs>> DeviceEndpoint::ownerInfo() {
    BufferArray buffer;
    auto error = rpcs_->getOwnerInfo(buffer);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return Result<std::vector<std::string>, Enum<RFSV::errs>>::failure(error);
    }
    std::vector<std::string> result;
    while (!buffer.empty()) {
        result.push_back(buffer.pop().getString());
    }
    return Result<std::vector<std::string>, Enum<RFSV::errs>>::success(result);
}
