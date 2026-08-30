#include "Tween.h"

namespace deki {

// ========== Interpolation Specializations ==========
// t is normalized progress in [0,1].

template <>
float Tween<float>::Interpolate(float t) const
{
    return m_StartValue + (m_EndValue - m_StartValue) * t;
}

template <>
int32_t Tween<int32_t>::Interpolate(float t) const
{
    return static_cast<int32_t>(static_cast<float>(m_StartValue) +
                                 (static_cast<float>(m_EndValue) - static_cast<float>(m_StartValue)) * t + 0.5f);
}

template <>
DekiVector2 Tween<DekiVector2>::Interpolate(float t) const
{
    return DekiVector2::Lerp(m_StartValue, m_EndValue, t);
}

template <>
deki::Color Tween<deki::Color>::Interpolate(float t) const
{
    return m_StartValue.Lerp(m_EndValue, t);
}

} // namespace deki
