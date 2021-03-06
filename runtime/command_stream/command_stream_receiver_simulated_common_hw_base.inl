/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "core/debug_settings/debug_settings_manager.h"
#include "core/gmm_helper/gmm.h"
#include "core/gmm_helper/gmm_helper.h"
#include "core/gmm_helper/resource_info.h"
#include "core/memory_manager/memory_manager.h"
#include "core/os_interface/os_context.h"
#include "runtime/aub/aub_helper.h"
#include "runtime/aub_mem_dump/page_table_entry_bits.h"
#include "runtime/command_stream/command_stream_receiver_simulated_common_hw.h"
#include "runtime/helpers/hardware_context_controller.h"
#include "runtime/memory_manager/address_mapper.h"

#include "third_party/aub_stream/headers/aub_manager.h"

namespace NEO {

template <typename GfxFamily>
void CommandStreamReceiverSimulatedCommonHw<GfxFamily>::initAdditionalMMIO() {
    if (DebugManager.flags.AubDumpAddMmioRegistersList.get() != "unk") {
        auto mmioList = AubHelper::getAdditionalMmioList();
        for (auto &mmioPair : mmioList) {
            stream->writeMMIO(mmioPair.first, mmioPair.second);
        }
    }
}

template <typename GfxFamily>
void CommandStreamReceiverSimulatedCommonHw<GfxFamily>::setupContext(OsContext &osContext) {
    CommandStreamReceiverHw<GfxFamily>::setupContext(osContext);

    auto engineType = osContext.getEngineType();
    uint32_t flags = 0;
    getCsTraits(engineType).setContextSaveRestoreFlags(flags);

    if (aubManager && !osContext.isLowPriority()) {
        hardwareContextController = std::make_unique<HardwareContextController>(*aubManager, osContext, flags);
    }
}

template <typename GfxFamily>
bool CommandStreamReceiverSimulatedCommonHw<GfxFamily>::getParametersForWriteMemory(GraphicsAllocation &graphicsAllocation, uint64_t &gpuAddress, void *&cpuAddress, size_t &size) const {
    cpuAddress = graphicsAllocation.getUnderlyingBuffer();
    gpuAddress = GmmHelper::decanonize(graphicsAllocation.getGpuAddress());
    size = graphicsAllocation.getUnderlyingBufferSize();
    auto gmm = graphicsAllocation.getDefaultGmm();
    if (gmm && gmm->isRenderCompressed) {
        size = gmm->gmmResourceInfo->getSizeAllocation();
    }

    if (size == 0)
        return false;

    if (cpuAddress == nullptr) {
        cpuAddress = this->getMemoryManager()->lockResource(&graphicsAllocation);
    }
    return true;
}

template <typename GfxFamily>
void CommandStreamReceiverSimulatedCommonHw<GfxFamily>::expectMemoryEqual(void *gfxAddress, const void *srcAddress, size_t length) {
    this->expectMemory(gfxAddress, srcAddress, length,
                       AubMemDump::CmdServicesMemTraceMemoryCompare::CompareOperationValues::CompareEqual);
}
template <typename GfxFamily>
void CommandStreamReceiverSimulatedCommonHw<GfxFamily>::expectMemoryNotEqual(void *gfxAddress, const void *srcAddress, size_t length) {
    this->expectMemory(gfxAddress, srcAddress, length,
                       AubMemDump::CmdServicesMemTraceMemoryCompare::CompareOperationValues::CompareNotEqual);
}

template <typename GfxFamily>
void CommandStreamReceiverSimulatedCommonHw<GfxFamily>::freeEngineInfo(AddressMapper &gttRemap) {
    alignedFree(engineInfo.pLRCA);
    gttRemap.unmap(engineInfo.pLRCA);
    engineInfo.pLRCA = nullptr;

    alignedFree(engineInfo.pGlobalHWStatusPage);
    gttRemap.unmap(engineInfo.pGlobalHWStatusPage);
    engineInfo.pGlobalHWStatusPage = nullptr;

    alignedFree(engineInfo.pRingBuffer);
    gttRemap.unmap(engineInfo.pRingBuffer);
    engineInfo.pRingBuffer = nullptr;
}

template <typename GfxFamily>
uint32_t CommandStreamReceiverSimulatedCommonHw<GfxFamily>::getDeviceIndex() const {
    return osContext->getDeviceBitfield().any() ? static_cast<uint32_t>(Math::log2(static_cast<uint32_t>(osContext->getDeviceBitfield().to_ulong()))) : 0u;
}
template <typename GfxFamily>
CommandStreamReceiverSimulatedCommonHw<GfxFamily>::CommandStreamReceiverSimulatedCommonHw(ExecutionEnvironment &executionEnvironment, uint32_t rootDeviceIndex) : CommandStreamReceiverHw<GfxFamily>(executionEnvironment, rootDeviceIndex) {}
template <typename GfxFamily>
CommandStreamReceiverSimulatedCommonHw<GfxFamily>::~CommandStreamReceiverSimulatedCommonHw() = default;
} // namespace NEO
