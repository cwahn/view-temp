#ifndef FIXED_POINT_HPP_
#define FIXED_POINT_HPP_

template <typename Base, int fractional_bits>
class FixedPoint
{
public:
    using ValueType = Base;

    FixedPoint() : value(0) {}
    explicit FixedPoint(ValueType val) : value(val << fractional_bits) {}
    explicit FixedPoint(double val) : value(static_cast<ValueType>(val * (1 << fractional_bits))) {}

    // Arithmetic operations
    FixedPoint operator+(const FixedPoint &other) const
    {
        return FixedPoint(value + other.value);
    }

    FixedPoint operator-(const FixedPoint &other) const
    {
        return FixedPoint(value - other.value);
    }

    FixedPoint operator*(const FixedPoint &other) const
    {
        return FixedPoint((value * other.value) >> fractional_bits);
    }

    FixedPoint operator/(const FixedPoint &other) const
    {
        return FixedPoint((value << fractional_bits) / other.value);
    }

    // Conversion to double
    operator double() const
    {
        return static_cast<double>(value) / (1 << fractional_bits);
    }

    template <typename T>
    explicit operator T() const
    {
        return static_cast<T>(static_cast<double>(*this));
    }

private:
    ValueType value;
};

using f64_20 = FixedPoint<int64_t, 20>;

#endif