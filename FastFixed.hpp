#pragma once

template <size_t N, size_t K>
class Fixed;

template <size_t N>
struct SelectFastIntType {
    using Type = std::conditional_t<
        (N <= 8), int_fast8_t,
        std::conditional_t<
            (N <= 16), int_fast16_t,
            std::conditional_t<
                (N <= 32), int_fast32_t,
                std::conditional_t<
                    (N <= 64), int_fast64_t,
                    void
                >
            >
        >
    >;
    static_assert(!std::is_void_v<Type>, "no suitable type found for the given size of fast fixed number");
};

template <size_t N, size_t K>
class FastFixed {
    using Type = typename SelectFastIntType<N>::type;
    Type v_;

public:
    constexpr FastFixed(int value) : v_(value << K) {}
    constexpr FastFixed(float value) : v_(static_cast<Type>(value * static_cast<Type>(1ULL << K))) {}
    constexpr FastFixed(double value) : v_(static_cast<Type>(value * static_cast<Type>(1ULL << K))) {}
    template <size_t N2, size_t K2>
    constexpr FastFixed(const FastFixed<N2, K2>& other) {
        if constexpr (K2 > K) {
            v_ = static_cast<Type>(other.raw_value() >> (K2 - K));
        } else {
            v_ = static_cast<Type>(other.raw_value() << (K - K2));
        }
    }

    template <size_t N2, size_t K2>
    constexpr FastFixed(const Fixed<N2, K2>& other) {
        if constexpr (K2 > K) {
            v_ = static_cast<Type>(other.raw_value() >> (K2 - K));
        } else {
            v_ = static_cast<Type>(other.raw_value() << (K - K2));
        }
    }

    constexpr FastFixed() : v_(0) {}

    constexpr static FastFixed from_raw(Type raw_value) {
        FastFixed result;
        result.v_ = raw_value;
        return result;
    }

    static size_t getK() {
        return K;
    }

    template <size_t N1, size_t K1>
    auto operator<(const FastFixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ < other1.v;
    };

    template <size_t N1, size_t K1>
    auto operator<=(const FastFixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ <= other1.v;
    };

    template <size_t N1, size_t K1>
    auto operator<=(const Fixed<N1, K1>& other) const {
        FastFixed other1 = static_cast<Fixed<N, K>>(other);
        return v_ <= other1.v_;
    };

    template <size_t N1, size_t K1>
    auto operator>(const FastFixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ > other1.v;
    };

    template <size_t N1, size_t K1>
    auto operator>=(const FastFixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ >= other1.v;
    };

    template <size_t N1, size_t K1>
    bool operator==(const FastFixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ == other1.v;
    };

    template <size_t N1, size_t K1>
    bool operator==(const Fixed<N1, K1>& other) const {
        auto other1 = static_cast<FastFixed>(other);
        return v_ == other1.v;
    };

    template <size_t N1, size_t K1>
    friend FastFixed operator+(FastFixed a, FastFixed<N1, K1> b) {
        return FastFixed::from_raw(a.v_ + static_cast<FastFixed<N, K>>(b).v_);
    }

    template <size_t N1, size_t K1>
    friend FastFixed operator-(FastFixed a, FastFixed<N1, K1> b) {
        return FastFixed::from_raw(a.v_ - static_cast<FastFixed<N, K>>(b).v_);
    }

    template <size_t N1, size_t K1>
    friend FastFixed operator*(FastFixed a, FastFixed<N1, K1> b) {
        using DoubleLengthType = std::conditional_t<N <= 32, int64_t, __int128>;
        return FastFixed::from_raw(static_cast<Type>((static_cast<DoubleLengthType>(a.v_) * static_cast<FastFixed>(b).v_) >> K));
    }

    template <size_t N1, size_t K1>
    friend FastFixed operator/(FastFixed a, FastFixed<N1, K1> b) {
        using DoubleLengthType = std::conditional_t<N <= 32, int64_t, __int128>;
        return FastFixed::from_raw(static_cast<Type>((static_cast<DoubleLengthType>(a.v_) << K) / static_cast<FastFixed<N, K>>(b).v_));
    }

    template <size_t N1, size_t K1>
    FastFixed& operator+=(FastFixed<N1, K1> other) {
        v_ += static_cast<FastFixed>(other).v_;
        return *this;
    }

    template <size_t N1, size_t K1>
    FastFixed& operator-=(FastFixed<N1, K1> other) {
        v_ -= static_cast<FastFixed>(other).v_;
        return *this;
    }

    template <size_t N1, size_t K1>
    FastFixed& operator*=(FastFixed<N1, K1> other) {
        *this = *this * static_cast<FastFixed>(other);
        return *this;
    }

    template <size_t N1, size_t K1>
    FastFixed& operator/=(FastFixed<N1, K1> other) {
        *this = *this / static_cast<FastFixed>(other);
        return *this;
    }

    FastFixed operator-() const {
        return FastFixed::from_raw(-v_);
    }

    template <size_t N1, size_t K1>
    friend FastFixed abs(FastFixed<N1, K1> x) {
        return x.v < 0 ? FastFixed::from_raw(-static_cast<FastFixed>(x).v_) : x;
    }

    friend std::ostream& operator<<(std::ostream& out, FastFixed x) {
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

    template <size_t N1, size_t K1>
    friend FastFixed min(FastFixed& a, FastFixed<N1, K1>& b) {
        auto x = static_cast<FastFixed>(b);
        if (a.v_ < x.v) {
            return a;
        }
        return x;
    }

    template <size_t N1, size_t K1>
    friend FastFixed max(FastFixed& a, FastFixed<N1, K1>& b) {
        auto x = static_cast<FastFixed>(b);
        if (a.v_ < x.v) {
            return b;
        }
        return x;
    }
};
