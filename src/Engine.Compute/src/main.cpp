#include<particleengine.hpp>

int main() {
    try {
        ParticleEngine particleEngine(100);

    } 
    catch (const std::exception& e) {
        std::cerr << "Vulkan Error: " << e.what() << std::endl;
    }
    return 0;
}
