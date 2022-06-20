#pragma once

class Vec2 {
public:
    float x, y;

    constexpr Vec2(float x, float y) : x(x), y(y) {}

    constexpr Vec2();

    constexpr Vec2(const Vec2&) = default;

    Vec2 operator+(const Vec2& rhs) const noexcept {
        return {this->x + rhs.x, this->y + rhs.y};
    }

    Vec2 operator-() const noexcept {
        return {-this->x, -this->y};
    }

    Vec2 operator-(const Vec2& rhs) const noexcept {
        return *this + (-rhs);
    }

    Vec2 operator*(const float scalar) const noexcept {
        return {this->x * scalar, this->y * scalar};
    }

    Vec2 operator/(const float scalar) const noexcept {
        return {this->x / scalar, this->y / scalar};
    }

    bool operator==(const Vec2& rhs) const noexcept {
        return this->x == rhs.x && this->y == rhs.y;
    }

    Vec2 onlyX() const noexcept {
        return {x, 0.0f};
    }

    Vec2 onlyY() const noexcept {
        return {0.0f, y};
    }
};

inline constexpr Vec2 VEC2_ZEROED{0.0f, 0.0f};

constexpr Vec2::Vec2() : Vec2(VEC2_ZEROED) {}

class Vec3 {
public:
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}

    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3(const Vec3&) = default;
};
