#include <particleengine.hpp>
#include <engine_abi.h>

#include <cstring>
#include <new>
#include <stdexcept>

namespace {
thread_local std::string g_lastError;

void setLastError(const char* message) noexcept {
    try {
        g_lastError = message != nullptr ? message : "Unknown native error";
    } catch (...) {
        // Diagnostics are best-effort and must never escape the C ABI.
    }
}

void clearLastError() noexcept {
    try {
        g_lastError.clear();
    } catch (...) {
    }
}
}

struct EngineHandle {
    explicit EngineHandle(uint32_t particleCount) : engine(particleCount) {}
    ParticleEngine engine;
};

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
    appInfo.pEngineName = "Cloud Particle Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;
    createInfo.enabledLayerCount = 0;

    const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        m_instance = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void ParticleEngine::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to enumerate physical devices");
    }
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-capable physical devices found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()) != VK_SUCCESS ||
        deviceCount == 0) {
        throw std::runtime_error("Failed to retrieve Vulkan physical devices");
    }

    // Prototype behavior only: compute-capable queue selection is not implemented yet.
    m_physicalDevice = devices.front();
}

void ParticleEngine::createLogicalDevice() {
    // Intentionally not implemented in this prototype. The ABI does not claim
    // that a GPU compute pipeline or a working simulation step exists yet.
}

ParticleEngine::ParticleEngine(uint32_t particleCount) {
    try {
        m_particles.resize(particleCount);
        initVulkan();
    } catch (...) {
        // A constructor that throws does not run this object's destructor.
        // Explicitly release any Vulkan resources created before the failure.
        cleanup();
        throw;
    }
}

ParticleEngine::~ParticleEngine() noexcept {
    cleanup();
}

void ParticleEngine::cleanup() noexcept {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
    m_physicalDevice = VK_NULL_HANDLE;
}

EngineStatus ParticleEngine::Step() noexcept {
    if (m_state == State::Failed) {
        return ENGINE_STATUS_INVALID_STATE;
    }

    // No simulation computation exists yet. This is a pre-execution result,
    // so the instance remains Ready and no tick is reported as completed.
    return ENGINE_STATUS_NOT_IMPLEMENTED;
}

extern "C" {

EngineStatus EngineCreate(const EngineCreateOptions* options, EngineHandle** out_handle) {
    if (out_handle == nullptr) {
        setLastError("out_handle must not be null");
        return ENGINE_STATUS_INVALID_ARGUMENT;
    }
    *out_handle = nullptr;

    if (options == nullptr || options->struct_size != sizeof(EngineCreateOptions)) {
        setLastError("options must be non-null and struct_size must match this ABI");
        return ENGINE_STATUS_INVALID_ARGUMENT;
    }

    try {
        EngineHandle* handle = new EngineHandle(options->initial_particle_count);
        *out_handle = handle;
        clearLastError();
        return ENGINE_STATUS_OK;
    } catch (const std::bad_alloc&) {
        setLastError("Not enough memory to create the engine");
        return ENGINE_STATUS_OUT_OF_MEMORY;
    } catch (const std::exception& error) {
        setLastError(error.what());
        return ENGINE_STATUS_INITIALIZATION_FAILED;
    } catch (...) {
        setLastError("Unknown exception while creating the engine");
        return ENGINE_STATUS_INTERNAL_ERROR;
    }
}

EngineStatus EngineStep(EngineHandle* handle) {
    if (handle == nullptr) {
        setLastError("handle must not be null");
        return ENGINE_STATUS_INVALID_ARGUMENT;
    }

    try {
        const EngineStatus status = handle->engine.Step();
        if (status == ENGINE_STATUS_OK) {
            clearLastError();
        } else if (status == ENGINE_STATUS_NOT_IMPLEMENTED) {
            setLastError("Simulation step is not implemented in this prototype");
        } else if (status == ENGINE_STATUS_INVALID_STATE) {
            setLastError("Engine is in a failed state");
        }
        return status;
    } catch (const std::exception& error) {
        setLastError(error.what());
        return ENGINE_STATUS_STEP_FAILED;
    } catch (...) {
        setLastError("Unknown exception while stepping the engine");
        return ENGINE_STATUS_STEP_FAILED;
    }
}

EngineStatus EngineDestroy(EngineHandle* handle) {
    if (handle == nullptr) {
        clearLastError();
        return ENGINE_STATUS_OK;
    }

    try {
        delete handle;
        clearLastError();
        return ENGINE_STATUS_OK;
    } catch (...) {
        // EngineHandle's destructor and cleanup are noexcept; this is a final
        // defensive barrier for the exported ABI.
        setLastError("Unexpected exception while destroying the engine");
        return ENGINE_STATUS_INTERNAL_ERROR;
    }
}

EngineStatus EngineGetLastError(
    char* buffer,
    uint32_t buffer_capacity,
    uint32_t* out_required_capacity) {
    if (out_required_capacity == nullptr || (buffer == nullptr && buffer_capacity != 0)) {
        return ENGINE_STATUS_INVALID_ARGUMENT;
    }

    const size_t required = g_lastError.size() + 1;
    if (required > UINT32_MAX) {
        *out_required_capacity = 0;
        return ENGINE_STATUS_INTERNAL_ERROR;
    }
    *out_required_capacity = static_cast<uint32_t>(required);

    if (buffer_capacity == 0) {
        return ENGINE_STATUS_OK;
    }

    const size_t copyLength = (required <= buffer_capacity)
        ? g_lastError.size()
        : static_cast<size_t>(buffer_capacity - 1);
    if (copyLength > 0) {
        std::memcpy(buffer, g_lastError.data(), copyLength);
    }
    buffer[copyLength] = '\0';

    return required <= buffer_capacity
        ? ENGINE_STATUS_OK
        : ENGINE_STATUS_INVALID_ARGUMENT;
}

}
