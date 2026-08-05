#include<particleengine.hpp>

void ParticleEngine::initVulkan() {
    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
}

void ParticleEngine::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Cloud Particle Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    //headless compute application so no window surface extensions
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
    std::cout << "[Vulkan] VkInstance successfully created." << std::endl;
}

void ParticleEngine::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to enumerate physical devices");
    }

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }


    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    //In production we choose the best device for our case but for now pick the first one
    m_physicalDevice = devices.at(0);
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    std::cout << "[Vulkan] using physical device: " << deviceProperties.deviceName << "\n";
}

void ParticleEngine::createLogicalDevice() {}

ParticleEngine::ParticleEngine(int particleCount) {
    std::cout << "[Engine] Initializing simulation with " << particleCount << " particles." << std::endl;
    m_particles.resize(particleCount);
    
    // Start the Vulkan setup
    initVulkan();
}

ParticleEngine::~ParticleEngine() {
    cleanup();
}

void ParticleEngine::cleanup()
{
    std::cout << "[Engine] Cleaning up Vulkan resources..." << std::endl;
    if (m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_device, nullptr);
    }
    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
    }
}

void ParticleEngine::Update(float deltaTime) {}


extern "C" {
    __declspec(dllexport) void* CreateParticleEngine(int particleCount) {
        return new ParticleEngine(particleCount);
    }

    __declspec(dllexport) void DestroyParticleEngine(void* enginePtr) {
        if (enginePtr != nullptr) {
            delete static_cast<ParticleEngine*>(enginePtr);
        }
    }

    __declspec(dllexport) void UpdateParticles(void* enginePtr, float deltaTime) {
        if (enginePtr == nullptr) {
            return;
        }
        ParticleEngine* engine = static_cast<ParticleEngine*>(enginePtr);
        try
        {
            engine->Update(deltaTime);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        

    }
}