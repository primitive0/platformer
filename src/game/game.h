#pragma once

#include "world.h"

const float PHYSICS_SUBSTEP_DELTA_MAX = 0.24;

struct Game {
    World world{};
    bool moveLeft = false;
    bool moveRight = false;

    Game();

    void process(float delta);

    void process_(float delta);

    void playerJump();
};
