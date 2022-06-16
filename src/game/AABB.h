#pragma once

#include "../math/vec.h"

class AABB {
    Vec2 _v0;
    Vec2 _v1;

public:
    AABB() : _v0(), _v1() {}

    AABB(Vec2 v0, Vec2 v1) : _v0(v0), _v1(v1) {}

    AABB(float x0, float x1, float y0, float y1) : _v0(x0, y0), _v1(x1, y1) {}

    const Vec2& v0() const noexcept {
        return _v0;
    }

    Vec2& v0() noexcept {
        return _v0;
    }

    const Vec2& v1() const noexcept {
        return _v1;
    }

    Vec2& v1() noexcept {
        return _v1;
    }

    Vec2 size() const noexcept {
        return _v1 - _v0;
    }
};
