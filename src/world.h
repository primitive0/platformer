#pragma once

#include <algorithm>
#include <vector>

#include "game/AABB.h"
#include "game/player.h"

class Solid {
    AABB _aabb;

public:
    Solid() : _aabb() {}

    explicit Solid(AABB aabb) : _aabb(aabb) {}

    Solid(float x0, float x1, float y0, float y1) : _aabb(x0, x1, y0, y1) {}

    const AABB& aabb() const {
        return _aabb;
    }

    AABB& aabb() {
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

class World {
public:
    Player player;
    std::vector<Solid> objects{};

    World() {
        player.setPos(150, 350);
    }

    void tick(float delta) {
        if (!player.isOnGround()) {
            addVelocityY(-0.005f * delta);
        }

        for (auto i = objects.cbegin(); i < objects.cend(); i++) {
            if (player.vel() == Vec2(0.0f, 0.0f)) {
                continue;
            }

            auto playerAABB = player.aabb();
            Vec2 rayOrigin = (playerAABB.v0() + playerAABB.v1()) * 0.5f;
            Vec2 rayDirection = {player.vel().x * delta, player.vel().y * delta};

            Vec2 objectSize = playerAABB.size();

            Solid expanded = *i;
            expanded.v0().x -= objectSize.x / 2;
            expanded.v1().x += objectSize.x / 2;
            expanded.v0().y -= objectSize.y / 2;
            expanded.v1().y += objectSize.y / 2;

            Vec2 contactPoint, contactNormal;
            float t;
            if (World::rayCast(expanded, rayOrigin, rayDirection, contactPoint, contactNormal, t) && t <= 1.0f) {
                player.setOnGround(true); // TODO: do resolving
                player.setVel(Vec2(0.0f, 0.0f));
            }
        }

        auto dVel = player.vel() * delta;
        if (!player.isOnGround()) {
            player.pos() = player.pos() + dVel.onlyY();
        }

        player.pos() = player.pos() + dVel.onlyX();
    }

    static bool rayCast(const Solid& object, Vec2 rayOrigin, Vec2 rayDir, Vec2& contactPoint, Vec2& contactNormal, float& tNear) {
        Vec2 _pos0 = object.aabb().v0();
        Vec2 _pos = object.aabb().v1();
        Vec2 pos0 = {_pos0.x - rayOrigin.x, _pos0.y - rayOrigin.y};
        Vec2 pos = {_pos.x - rayOrigin.x, _pos.y - rayOrigin.y};

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

        if (tFar < 0.0f) {
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

    void addVelocityX(float val, float max) {
        auto& vel = player.vel();
        vel.x = std::clamp(vel.x + val, -max, max);
    }

    void slowDown(float val) {
        auto& vel = player.vel();
        if (vel.x < 0) {
            vel.x = std::min(vel.x + val, 0.0f);
        } else {
            vel.x = std::max(vel.x - val, 0.0f);
        }
    }

    void addVelocityY(float val) {
        const float VELOCITY_MAX = 5.0f;
        auto& vel = player.vel();
        vel.y = std::clamp(vel.y + val, -VELOCITY_MAX, VELOCITY_MAX);
    }
};
