#pragma once

#include <type_traits>

#include "AbstractParam.h"
#include "vislib/math/Vector.h"

namespace megamol::core::param {

template<typename T>
struct is_vislib_vector_f
        : std::integral_constant<bool, std::is_same_v<std::decay_t<T>, vislib::math::Vector<float, 2>> ||
                                           std::is_same_v<std::decay_t<T>, vislib::math::Vector<float, 3>> ||
                                           std::is_same_v<std::decay_t<T>, vislib::math::Vector<float, 4>>> {};

template<typename T>
inline constexpr bool is_vislib_vector_f_v = is_vislib_vector_f<T>::value;

template<typename T>
struct enable_cond : std::integral_constant<bool, std::is_arithmetic_v<T> || is_vislib_vector_f_v<T>> {};

template<typename T>
inline constexpr bool enable_cond_v = enable_cond<T>::value;

template<typename T, AbstractParamPresentation::ParamType TYPE>
class GenericParam : public AbstractParam {
public:
    using Value_T = T;

    GenericParam(T const& initVal) {
        initParam(initVal);
    }

    template<typename U = T>
    GenericParam(T const& initVal, T const& minVal, std::enable_if_t<enable_cond_v<U>, void>* = nullptr)
            : GenericParam(initVal, minVal, std::numeric_limits<T>::max()) {}

    template<typename U = T>
    GenericParam(
        T const& initVal, T const& minVal, T const& maxVal, std::enable_if_t<enable_cond_v<U>, void>* = nullptr)
            : minVal(minVal)
            , maxVal(maxVal) {
        initParam(initVal);
    }

    virtual ~GenericParam() = default;

    /**
     * Sets the value of the parameter and optionally sets the dirty flag
     * of the owning parameter slot.
     *
     * @param v the new value for the parameter
     * @param setDirty If 'true' the dirty flag of the owning parameter
     *                 slot is set and the update callback might be called.
     */
    void SetValue(T v, bool setDirty = true) {
        if constexpr (enable_cond_v<T>) {
            if (v < this->minVal)
                v = this->minVal;
            else if (v > this->maxVal)
                v = this->maxVal;
        }
        if (this->val != v) {
            this->val = v;
            this->indicateChange();
            if (setDirty)
                this->setDirty();
        }
    }

    /**
     * Gets the value of the parameter
     *
     * @return The value of the parameter
     */
    T const& Value(void) const {
        return this->val;
    }

    /**
     * Needed for RemoteControl - Manuel Gräber
     * Gets the minimum value of the parameter
     *
     * @return The minimum value of the parameter
     */
    template<typename U = T>
    std::enable_if_t<enable_cond_v<U>, T const&> MinValue() const {
        return minVal;
    }

    /**
     * Needed for RemoteControl - Manuel Gräber
     * Gets the maximum value of the parameter
     *
     * @return The maximum value of the parameter
     */
    template<typename U = T>
    std::enable_if_t<enable_cond_v<U>, T const&> MaxValue() const {
        return maxVal;
    }

private:
    void initParam(T const& v) {
        initMinMax();
        InitPresentation(TYPE);
        SetValue(v);
    }

    void initMinMax() {
        if constexpr (std::is_arithmetic_v<T>) {
            minVal = std::numeric_limits<T>::lowest();
            maxVal = std::numeric_limits<T>::max();
        } else if constexpr (is_vislib_vector_f_v<T>) {
            minVal = T(std::numeric_limits<typename T::ValueT>::lowest());
            maxVal = T(std::numeric_limits<typename T::ValueT>::max());
        }
    }

    T val;

    T minVal;

    T maxVal;
};
} // namespace megamol::core::param
