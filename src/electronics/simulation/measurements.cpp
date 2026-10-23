#include "measurements.h"
#include <cmath>

// BEGIN class Measurements
double Measurements::update(double v)
{
    switch (m_mode) {
        case AVG: {
            if (m_samples.isFull())
                m_cache -= m_samples.first();
            m_samples.append(v);
            m_cache += v;
            return m_cache/m_samples.size();
        }
        case RMS: {
            if (m_samples.isFull()) {
                const double v0 = m_samples.first();
                m_cache -= v0 * v0;
            }
            m_samples.append(v);
            m_cache += v * v;
            return std::sqrt(m_cache/m_samples.size());
        }
        case MAX: {
            if (!m_samples.isEmpty() && v <= m_samples.last())
                return m_samples.last();
            m_samples.append(v);
            return v;
        }
        case MIN: {
            if (!m_samples.isEmpty() && v >= m_samples.last())
                return m_samples.last();
            m_samples.append(v);
            return v;
        }
    }
    Q_UNREACHABLE_RETURN(v); // LCOV_EXCL_LINE
}

// END class Measurements

