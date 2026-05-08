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

#include <memory>
#include <string>
#include <system_error>

#include "deviceendpoint.h"

#include "device.h"
#include "deviceconfiguration.h"
#include "rclip.h"
#include "rfsv.h"
#include "rpcs.h"
#include "uuid.h"

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

    return std::make_unique<DeviceEndpoint>(id, persistentId, std::move(rfsv), std::move(rpcs), std::move(clip));
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
, hasPersisentId_(persistentId) {}

std::string DeviceEndpoint::id() {
    return id_;
}

Enum<RFSV::errs> DeviceEndpoint::getName(std::string &name) {
    Enum<RFSV::errs> error = RFSV::E_PSI_GEN_NONE;
    auto deviceConfiguration = device::read_configuration(*rfsv_, error);
    if (error != RFSV::E_PSI_GEN_NONE) {
        return error;
    }
    name = deviceConfiguration->name();
    return RFSV::E_PSI_GEN_NONE;
}

Enum<RFSV::errs> DeviceEndpoint::setName(const std::string &name) {
    auto deviceConfiguration = std::make_unique<DeviceConfiguration>(id_, name);
    auto result = device::write_configuration(*rfsv_, *deviceConfiguration);
    if (result == RFSV::E_PSI_GEN_NONE) {
        hasPersisentId_ = true;
    }
    return result;
}
