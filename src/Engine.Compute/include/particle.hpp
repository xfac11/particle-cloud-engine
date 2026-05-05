#pragma once
struct Particle {
    float x, y, z; // Position
    float vx, vy, vz; // Velocity
    float mass; // Mass
};

extern "C" {
    __declspec(dllexport) void UpdateParticle(Particle* particle, float deltaTime);
}