// File overview: Generates an exponentially growing series of numbers; InstanceList uses it as a retry back-off when
// committing a new instance from its staging folder.

#pragma once

template <typename T>
inline void clamp(T& current, T min, T max)
{
    if (current < min) {
        current = min;
    } else if (current > max) {
        current = max;
    }
}

// List of numbers from min to max. Next is exponent times bigger than previous.

class ExponentialSeries {
   public:
    ExponentialSeries(unsigned min, unsigned max, unsigned exponent = 2)
    {
        m_current = m_min = min;
        m_max = max;
        m_exponent = exponent;
    }
    void reset() { m_current = m_min; }
    unsigned operator()()
    {
        unsigned retval = m_current;
        m_current *= m_exponent;
        clamp(m_current, m_min, m_max);
        return retval;
    }
    unsigned m_current;
    unsigned m_min;
    unsigned m_max;
    unsigned m_exponent;
};
