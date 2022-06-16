#pragma once

#include "AABB.h"

class Player {
    Vec2 _pos;
    Vec2 _vel;
    bool onGround = false;

public:
    static inline constexpr Vec2 SIZE = {100.0f, 100.0f};
    static inline constexpr Vec2 HALF_SIZE = {50.0f, 50.0f};

    Player() : _pos(), _vel() {}

    // AABB

    AABB aabb() const {
        return {-HALF_SIZE + _pos, HALF_SIZE + _pos};
    }

    // POS

    const Vec2& pos() const {
        return _pos;
    }

    Vec2& pos() {
        return _pos;
    }

    void setPos(Vec2 pos) {
        _pos = pos;
    }

    void setPos(float x, float y) {
        setPos({x, y});
    }

    // GROUND

    bool isOnGround() const {
        return onGround;
    }

    void setOnGround(bool val) {
        onGround = val;
    }

    // VEL

    const Vec2& vel() const {
        return _vel;
    }

    Vec2& vel() {
        return _vel;
    }

    void setVel(Vec2 vel) {
        _vel = vel;
    }
};
