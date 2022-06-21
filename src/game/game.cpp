#include <cmath>

#include "game.h"

Game::Game() {
    world.objects.emplace_back(100, 900, 200, 250);
    world.player.setPos(150, 300);
//    world.player.vel() = {0.8f, 2.5f};
}

void Game::process(float delta) {
    if (delta > PHYSICS_SUBSTEP_DELTA_MAX) {
        float fSubsteps = delta / PHYSICS_SUBSTEP_DELTA_MAX; // TODO: better variable names
        float intSubsteps = std::trunc(fSubsteps);
        float leftSubsteps = fSubsteps - intSubsteps;

        for (uint32_t i = 0; i < static_cast<uint32_t>(intSubsteps); i++) {
            process_(PHYSICS_SUBSTEP_DELTA_MAX);
        }
        process_(leftSubsteps * PHYSICS_SUBSTEP_DELTA_MAX);
    } else {
        process_(delta);
    }
}

void Game::process_(float delta) {
    if (moveLeft && !moveRight) {
        world.addVelocityX(-0.011f * delta, 0.8f);
    } else if (!moveLeft && moveRight) {
        world.addVelocityX(0.011f * delta, 0.8f);
    } else if (world.player.isOnGround()) {
        world.slowDown(0.005f * delta);
    }

    world.tick(delta);
}

void Game::playerJump() {
    world.player.vel().y = 2.5f;
    world.player.setOnGround(false);
}
