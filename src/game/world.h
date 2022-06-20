#pragma once

#include <algorithm>
#include <vector>

#include "game/AABB.h"
#include "game/player.h"
#include "game/raycast.h"
#include "game/solid.h"

class World {
public:
    Player player;
    std::vector<Solid> objects{};
    std::vector<Vec2> posLog{};

    World() {
        player.setPos(150, 350);
    }

    void tick(float delta) {
        if (!player.isOnGround()) {
            addVelocityY(-0.005f * delta);
        }

        if (player.vel() == Vec2(0.0f, 0.0f)) {
            return;
        }

        Vec2 vel = player.vel();

        for (auto i = objects.cbegin(); i < objects.cend(); i++) {
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
            if (doRayCast2D(expanded, rayOrigin, rayDirection, contactPoint, contactNormal, t) && t <= 1.0f) {
                player.vel().y = 0.0f;
                player.setOnGround(true);
                if (contactNormal.x != 0.0f) {
                    vel.x *= t;
                } else {
                    vel.y *= t;
                }
            }
        }

        player.pos() = player.pos() + vel * delta;
        posLog.push_back(player.pos());
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
