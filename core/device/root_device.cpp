/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "core/device/root_device.h"

#include "core/command_stream/preemption.h"
#include "core/debug_settings/debug_settings_manager.h"
#include "core/device/sub_device.h"
#include "core/helpers/hw_helper.h"
#include "core/memory_manager/memory_manager.h"
#include "runtime/command_stream/command_stream_receiver.h"

namespace NEO {
extern CommandStreamReceiver *createCommandStream(ExecutionEnvironment &executionEnvironment, uint32_t rootDeviceIndex);

RootDevice::RootDevice(ExecutionEnvironment *executionEnvironment, uint32_t rootDeviceIndex) : Device(executionEnvironment), rootDeviceIndex(rootDeviceIndex) {}

RootDevice::~RootDevice() {
    for (auto subdevice : subdevices) {
        if (subdevice) {
            subdevice->decRefInternal();
        }
    }
}

uint32_t RootDevice::getNumSubDevices() const {
    return static_cast<uint32_t>(subdevices.size());
}

uint32_t RootDevice::getRootDeviceIndex() const {
    return rootDeviceIndex;
}

uint32_t RootDevice::getNumAvailableDevices() const {
    if (subdevices.empty()) {
        return 1u;
    }
    return getNumSubDevices();
}

Device *RootDevice::getDeviceById(uint32_t deviceId) const {
    UNRECOVERABLE_IF(deviceId >= getNumAvailableDevices());
    if (subdevices.empty()) {
        return const_cast<RootDevice *>(this);
    }
    return subdevices[deviceId];
};

SubDevice *RootDevice::createSubDevice(uint32_t subDeviceIndex) {
    return Device::create<SubDevice>(executionEnvironment, subDeviceIndex, *this);
}

bool RootDevice::createDeviceImpl() {
    auto numSubDevices = HwHelper::getSubDevicesCount(&getHardwareInfo());
    if (numSubDevices == 1) {
        numSubDevices = 0;
    }
    UNRECOVERABLE_IF(!subdevices.empty());
    subdevices.resize(numSubDevices, nullptr);
    for (auto i = 0u; i < numSubDevices; i++) {

        auto subDevice = createSubDevice(i);
        if (!subDevice) {
            return false;
        }
        subDevice->incRefInternal();
        subdevices[i] = subDevice;
    }
    auto status = Device::createDeviceImpl();
    if (!status) {
        return status;
    }
    return true;
}
bool RootDevice::isReleasable() {
    return false;
};
DeviceBitfield RootDevice::getDeviceBitfield() const {
    DeviceBitfield deviceBitfield{static_cast<uint32_t>(maxNBitValue(getNumAvailableDevices()))};
    return deviceBitfield;
}

bool RootDevice::createEngines() {
    if (getNumSubDevices() < 2) {
        return Device::createEngines();
    } else {
        initializeRootCommandStreamReceiver();
    }
    return true;
}

void RootDevice::initializeRootCommandStreamReceiver() {
    std::unique_ptr<CommandStreamReceiver> rootCommandStreamReceiver(createCommandStream(*executionEnvironment, rootDeviceIndex));

    auto &hwInfo = getHardwareInfo();
    auto defaultEngineType = getChosenEngineType(hwInfo);
    auto preemptionMode = PreemptionHelper::getDefaultPreemptionMode(hwInfo);

    auto osContext = getMemoryManager()->createAndRegisterOsContext(rootCommandStreamReceiver.get(), defaultEngineType,
                                                                    getDeviceBitfield(), preemptionMode, false);

    rootCommandStreamReceiver->setupContext(*osContext);
    rootCommandStreamReceiver->initializeTagAllocation();
    rootCommandStreamReceiver->createGlobalFenceAllocation();
    commandStreamReceivers.push_back(std::move(rootCommandStreamReceiver));
    engines.emplace_back(commandStreamReceivers.back().get(), osContext);
}

} // namespace NEO
