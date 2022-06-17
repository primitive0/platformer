#pragma once

#include "AABB.h"

class Solid {
    AABB _aabb;

public:
    Solid() : _aabb() {}

    explicit Solid(AABB aabb) : _aabb(aabb) {}

    Solid(float x0, float x1, float y0, float y1) : _aabb(x0, x1, y0, y1) {}

    const AABB& aabb() const noexcept {
        return _aabb;
    }

    AABB& aabb() noexcept {
        return _aabb;
    }

    const Vec2& v0() const noexcept {
        return _aabb.v0();
    }

    Vec2& v0() noexcept {
        return _aabb.v0();
    }

    const Vec2& v1() const noexcept {
        return _aabb.v1();
    }

    Vec2& v1() noexcept {
        return _aabb.v1();
    }

    Vec2 size() const noexcept {
        return _aabb.size();
    }
};
