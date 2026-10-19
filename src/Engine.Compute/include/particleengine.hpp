#pragma once

#include <particle.hpp>
#include <pch.hpp>
#include <cstdint>

class ParticleEngine {
private:
    enum class State {
        Ready,
        Failed
    };

    std::vector<Particle> m_particles;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    State m_state = State::Ready;

    void initVulkan();
    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void cleanup() noexcept;

public:
    explicit ParticleEngine(uint32_t particleCount);
    ~ParticleEngine() noexcept;

    ParticleEngine(const ParticleEngine&) = delete;
    ParticleEngine& operator=(const ParticleEngine&) = delete;

    // One logical fixed tick. Scheduler wall-clock delta is intentionally absent.
    void Step();
};
