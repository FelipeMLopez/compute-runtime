/*
 * Copyright (C) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "core/helpers/array_count.h"
#include "runtime/command_stream/aub_command_stream_receiver_hw.h"
#include "runtime/command_stream/aub_command_stream_receiver_hw_bdw_plus.inl"
#include "runtime/helpers/base_object.h"

namespace NEO {

typedef SKLFamily Family;
static auto gfxCore = IGFX_GEN9_CORE;

template <>
void populateFactoryTable<AUBCommandStreamReceiverHw<Family>>() {
    extern AubCommandStreamReceiverCreateFunc aubCommandStreamReceiverFactory[IGFX_MAX_CORE];
    UNRECOVERABLE_IF(!isInRange(gfxCore, aubCommandStreamReceiverFactory));
    aubCommandStreamReceiverFactory[gfxCore] = AUBCommandStreamReceiverHw<Family>::create;
}

template class AUBCommandStreamReceiverHw<Family>;
} // namespace NEO
