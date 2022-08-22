#pragma once

#include <algorithm>

#include "solid.h"

inline bool doRayCast2D(const AABB& object, Vec2 rayOrigin, Vec2 rayDir, Vec2& contactPoint, Vec2& contactNormal, float& tNear) noexcept {
    Vec2 pos0 = object.v0() - rayOrigin;
    Vec2 pos = object.v1() - rayOrigin;

    float nearY = pos0.x / rayDir.x;
    float nearX = pos.y / rayDir.y;
    float farY = pos.x / rayDir.x;
    float farX = pos0.y / rayDir.y;

    if (nearX > farX) {
        std::swap(nearX, farX);
    }
    if (nearY > farY) {
        std::swap(nearY, farY);
    }

    if (!(nearY < farX && nearX < farY)) {
        return false;
    }

    tNear = std::max(nearX, nearY);
    float tFar = std::min(farX, farY);

    if (tNear < 0.0f || tFar < 0.0f) {
        return false;
    }

    contactPoint.x = rayOrigin.x + tNear * rayDir.x;
    contactPoint.y = rayOrigin.y + tNear * rayDir.y;

    if (nearY >= nearX) {
        contactNormal = rayDir.x < 0 ? Vec2(1.0f, 0.0f) : Vec2(-1.0f, 0.0f);
    } else {
        contactNormal = rayDir.y < 0 ? Vec2(0.0f, 1.0f) : Vec2(0.0f, -1.0f);
    }

    return true;
}
