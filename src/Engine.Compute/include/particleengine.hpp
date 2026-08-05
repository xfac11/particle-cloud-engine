#pragma once
#include <exception>
#include<pch.hpp>
#include<particle.hpp>

class ParticleEngine {
private:    
    std::vector<Particle> m_particles;
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    
    void initVulkan();
    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void cleanup();
public:
    ParticleEngine(int particleCount);
    virtual ~ParticleEngine();

    void Update(float deltaTime);
};

extern "C" {
    __declspec(dllexport) void* CreateParticleEngine(int particleCount);
    __declspec(dllexport) void DestroyParticleEngine(void* enginePtr);
    __declspec(dllexport) void UpdateParticles(void* enginePtr, float deltaTime);
}