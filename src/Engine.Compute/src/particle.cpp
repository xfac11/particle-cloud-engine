#include "particle.hpp"

void UpdateParticle(Particle *particle, float deltaTime)
{
    particle->x += 1.0f;
    particle->y += 2.0f;
    particle->z += 3.0f;
}