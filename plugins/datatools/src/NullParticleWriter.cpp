/*
 * NullParticleWriter.h
 *
 * Copyright (C) 2016 by MegaMol Team
 * Alle Rechte vorbehalten.
 */
#include "NullParticleWriter.h"
#include "geometry_calls/MultiParticleDataCall.h"
#include "mmcore/utility/log/Log.h"
#include "stdafx.h"
#include <cstdint>
#include <thread>

using namespace megamol;
using namespace megamol::datatools;

NullParticleWriter::NullParticleWriter(void)
        : core::AbstractDataWriter()
        , dataSlot("data", "The slot requesting the data to be written") {

    this->dataSlot.SetCompatibleCall<geocalls::MultiParticleDataCallDescription>();
    this->MakeSlotAvailable(&this->dataSlot);
}

NullParticleWriter::~NullParticleWriter(void) {
    this->Release();
}

bool NullParticleWriter::create(void) {
    return true;
}

void NullParticleWriter::release(void) {}

bool NullParticleWriter::run(void) {
    using geocalls::MultiParticleDataCall;
    using megamol::core::utility::log::Log;

    MultiParticleDataCall* mpdc = this->dataSlot.CallAs<MultiParticleDataCall>();
    if (mpdc == nullptr) {
        Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR, "No data source connected. Abort.");
        return false;
    }

    // fetch meta data
    uint32_t frameCnt = 1;
    if ((*mpdc)(1)) {
        frameCnt = mpdc->FrameCount();
        if (frameCnt == 0) {
            Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR, "Data source counts zero frames. Abort.");
            mpdc->Unlock();
            return false;
        }
    }

    mpdc->Unlock();

    // fetch all frames
    for (uint32_t i = 0; i < frameCnt; i++) {

        int missCnt = -9;
        do {
            mpdc->SetFrameID(i, true);
            if (!(*mpdc)(0)) {
                Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR, "Cannot get data frame %u. Abort.\n", i);
                return false;
            }

            if (mpdc->FrameID() != i) {
                if ((missCnt % 10) == 0) {
                    Log::DefaultLog.WriteMsg(
                        Log::LEVEL_WARN, "Frame %u returned on request for frame %u\n", mpdc->FrameID(), i);
                }
                ++missCnt;

                mpdc->Unlock();

                std::this_thread::sleep_for(std::chrono::milliseconds(1 + std::max<int>(missCnt, 0) * 100));
            }

            mpdc->Unlock();

        } while (mpdc->FrameID() != i);
    }

    Log::DefaultLog.WriteMsg(Log::LEVEL_INFO, "All frames touched");

    return true;
}

bool NullParticleWriter::getCapabilities(core::DataWriterCtrlCall& call) {
    call.SetAbortable(false);
    return true;
}
