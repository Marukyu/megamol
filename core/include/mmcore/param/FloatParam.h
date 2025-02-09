/*
 * FloatParam.h
 *
 * Copyright (C) 2021 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#pragma once

#include <sstream>

#include "GenericParam.h"
#include "mmcore/api/MegaMolCore.std.h"


namespace megamol::core::param {

class MEGAMOLCORE_API FloatParam : public GenericParam<float, AbstractParamPresentation::ParamType::FLOAT> {
public:
    FloatParam(float initVal) : Super(initVal) {}

    FloatParam(float initVal, float minVal) : Super(initVal, minVal) {}

    FloatParam(float initVal, float minVal, float maxVal) : Super(initVal, minVal, maxVal) {}

    virtual ~FloatParam() = default;

    std::string Definition() const override {
        std::ostringstream outDef;
        outDef << "MMFLOT";
        outDef.write(reinterpret_cast<char const*>(&MinValue()), sizeof(MinValue()));
        outDef.write(reinterpret_cast<char const*>(&MaxValue()), sizeof(MaxValue()));

        return outDef.str();
    }

    bool ParseValue(std::string const& v) override {
        try {
            this->SetValue((float)vislib::TCharTraits::ParseDouble(v.c_str()));
            return true;
        } catch (...) {}
        return false;
    }

    std::string ValueString() const override {
        return std::to_string(Value());
    }

private:
    using Super = GenericParam<float, AbstractParamPresentation::ParamType::FLOAT>;
};

} // namespace megamol::core::param
