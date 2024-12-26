#pragma once

template <size_t N, size_t K>
class FastFixed;

template <size_t N>
struct SelectIntType {
    using Type = std::conditional_t<
        (N == 8), int8_t,
        std::conditional_t<
            (N == 16), int16_t,
            std::conditional_t<
                (N == 32), int32_t,
                std::conditional_t<
                    (N == 64), int64_t,
                    void
                >
            >
        >
    >;
    static_assert(!std::is_void_v<Type>, "No suitable type found for the given size of fixed number.");
};

template <size_t N, size_t K>
class Fixed {
    using Type = typename SelectIntType<N>::type;
    Type v_;

public:
    constexpr Fixed(int value) : v_(value << K) {}
    constexpr Fixed(float value) : v_(static_cast<Type>(value * static_cast<Type>(1ULL << K))) {}
    constexpr Fixed(double value) : v_(static_cast<Type>(value * static_cast<Type>(1ULL << K))) {}
    template <size_t N2, size_t K2>
    constexpr Fixed(const Fixed<N2, K2>& other) {
        if constexpr (K2 > K) {
            v_ = static_cast<Type>(other.raw_value() >> (K2 - K));
        } else {
            v_ = static_cast<Type>(other.raw_value() << (K - K2));
        }
    }

    template <size_t N2, size_t K2>
    constexpr Fixed(const FastFixed<N2, K2>& other) {
        if constexpr (K2 > K) {
            v_ = static_cast<Type>(other.raw_value() >> (K2 - K));
        } else {
            v_ = static_cast<Type>(other.raw_value() << (K - K2));
        }
    }
    constexpr Fixed() : v_(0) {}

    constexpr static Fixed from_raw(Type
     raw_value) {
        Fixed result;
        result.v_ = raw_value;
        return result;
    }

    static size_t getK() {
        return K;
    }

    template <size_t OtherN, size_t OtherK>
    auto operator<(const Fixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ < other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    auto operator<=(const Fixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ <= other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    auto operator<=(const FastFixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ <= other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    auto operator>(const Fixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ > other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    auto operator>=(const Fixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ >= other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    bool operator==(const Fixed<OtherN, OtherK>& other) const {
        auto other1 = static_cast<Fixed>(other);
        return v_ == other1.v;
    };

    template <size_t OtherN, size_t OtherK>
    bool operator==(const FastFixed<OtherN, OtherK>& other) const {
        Fixed other1 = static_cast<FastFixed<N, K>>(other);
        return v_ == other1.v_;
    };

    template <size_t OtherN, size_t OtherK>
    friend Fixed operator+(Fixed a, Fixed<OtherN, OtherK> b) {
        return Fixed::from_raw(a.v_ + static_cast<Fixed<N, K>>(b).v_);
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed operator-(Fixed a, Fixed<OtherN, OtherK> b) {
        return Fixed::from_raw(a.v_ - static_cast<Fixed<N, K>>(b).v_);
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed operator*(Fixed a, Fixed<OtherN, OtherK> b) {
        using DoubleLengthType = std::conditional_t<N <= 32, int64_t, __int128>;
        return Fixed::from_raw(static_cast<Type>((static_cast<DoubleLengthType>(a.v_) * static_cast<Fixed<N, K>>(b).v_) >> K));
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed operator/(Fixed a, Fixed<OtherN, OtherK> b) {
        using DoubleLengthType = std::conditional_t<N <= 32, int64_t, __int128>;
        return Fixed::from_raw(static_cast<Type>((static_cast<DoubleLengthType>(a.v_) << K) / static_cast<Fixed<N, K>>(b).v_));
    }

    template <size_t OtherN, size_t OtherK>
    Fixed& operator+=(Fixed<OtherN, OtherK> other) {
        v_ += static_cast<Fixed<N, K>>(other).v_;
        return *this;
    }

    template <size_t OtherN, size_t OtherK>
    Fixed& operator-=(Fixed<OtherN, OtherK> other) {
        v_ -= static_cast<Fixed<N, K>>(other).v_;
        return *this;
    }

    template <size_t OtherN, size_t OtherK>
    Fixed& operator*=(Fixed<OtherN, OtherK> other) {
        *this = *this * static_cast<Fixed<N, K>>(other);
        return *this;
    }

    template <size_t OtherN, size_t OtherK>
    Fixed& operator/=(Fixed<OtherN, OtherK> other) {
        *this = *this / static_cast<Fixed<N, K>>(other);
        return *this;
    }

    Fixed operator-() const {
        return Fixed::from_raw(-v_);
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed abs(Fixed<OtherN, OtherK> x) {
        return x.v < 0 ? Fixed::from_raw(-static_cast<Fixed<N, K>>(x).v_) : x;
    }

    friend std::ostream& operator<<(std::ostream& out, Fixed x) {
        return out << static_cast<double>(x.v_) / static_cast<Type>(1ULL << K);
    }

    explicit operator float() const {
        return static_cast<float>(v_) / static_cast<Type>(1ULL << K);
    }

    explicit operator double() const {
        return static_cast<double>(v_) / static_cast<Type>(1ULL << K);
    }

    constexpr Type raw_value() const {
        return v_;
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed min(Fixed& a, Fixed<OtherN, OtherK>& b) {
        auto x = static_cast<Fixed>(b);
        if (a.v_ < x.v) {
            return a;
        }
        return x;
    }

    template <size_t OtherN, size_t OtherK>
    friend Fixed max(Fixed& a, Fixed<OtherN, OtherK>& b) {
        auto x = static_cast<Fixed>(b);
        if (a.v_ < x.v) {
            return x;
        }
        return a;
    }
};
