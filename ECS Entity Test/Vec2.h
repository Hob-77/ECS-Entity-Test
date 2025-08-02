#pragma once
#include <cmath>
#include <algorithm>

struct Vec2 {
    float x;
    float y;

    // Constructors
    constexpr Vec2() noexcept : x(0.0f), y(0.0f) {}
    constexpr Vec2(float x, float y) noexcept : x(x), y(y) {}

    // Unary operators
    constexpr Vec2 operator-() const noexcept { return Vec2(-x, -y); }

    // Arithimetic operators
    constexpr Vec2 operator+(const Vec2& rhs) const noexcept
    {
        return Vec2(x + rhs.x, y + rhs.y);
    }

    constexpr Vec2 operator-(const Vec2& rhs) const noexcept
    {
        return Vec2(x - rhs.x, y - rhs.y);
    }

    constexpr Vec2 operator*(float scalar) const noexcept
    {
        return Vec2(x * scalar, y * scalar);
    }

    constexpr Vec2 operator/(float scalar) const noexcept
    {
        return Vec2(x / scalar, y / scalar);
    }

    // Compound assignment
    Vec2& operator+=(const Vec2& rhs) noexcept
    {
        x += rhs.x; y += rhs.y; return *this;
    }

    Vec2& operator-=(const Vec2& rhs) noexcept
    {
        x -= rhs.x; y -= rhs.y; return *this;
    }

    Vec2& operator*=(float scalar) noexcept
    {
        x *= scalar; y *= scalar; return *this;
    }

    Vec2& operator/=(float scalar) noexcept
    {
        x /= scalar; y /= scalar; return *this;
    }

    // Comparison
    constexpr bool operator==(const Vec2& rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y;
    }

    constexpr bool operator!=(const Vec2& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    // Math utilities
    float Length() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }
    constexpr float LengthSquared() const noexcept
    {
        return x * x + y * y;
    }
    Vec2 Normalized() const noexcept
    {
        float len = Length();
        return (len > 0.0f) ? Vec2(x / len, y / len) : Vec2(0.0f, 0.0f);
    }
    constexpr float Dot(const Vec2& rhs) const noexcept
    {
        return x * rhs.x + y * rhs.y;
    }
    constexpr float Cross(const Vec2& rhs) const noexcept
    {
        return x * rhs.y - y * rhs.x;
    }

    // Useful additions
    constexpr Vec2 Perpendicular() const noexcept
    {
        return Vec2(-y, x);
    }
    Vec2 Abs() const noexcept
    {
        return Vec2(std::abs(x), std::abs(y));
    }

    // Static utilities
    static float Distance(const Vec2& a, const Vec2& b) noexcept
    {
        return (b - a).Length();
    }
    static constexpr float DistanceSquared(const Vec2& a, const Vec2& b) noexcept
    {
        return (b - a).LengthSquared();
    }

    // Constants
    static constexpr Vec2 Zero() noexcept { return Vec2(0.0f, 0.0f); }
    static constexpr Vec2 One() noexcept { return Vec2(1.0f, 1.0f); }
    static constexpr Vec2 Up() noexcept { return Vec2(0.0f, -1.0f); }
    static constexpr Vec2 Down() noexcept { return Vec2(0.0f, 1.0f); }
    static constexpr Vec2 Left() noexcept { return Vec2(-1.0f, 0.0f); }
    static constexpr Vec2 Right() noexcept { return Vec2(1.0f, 0.0f); }

};

constexpr Vec2 operator*(float scalar, const Vec2& vec) noexcept
{
    return vec * scalar;
}