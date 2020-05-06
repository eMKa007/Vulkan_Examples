#include "Simulation.h"

int main(int argc, char* argv[]) 
{
    try
    {
        std::unique_ptr<Simulation> app = std::make_unique<Simulation>(1024, 768, "Shadow Mapping - Vulkan");
        app->run();
    } 
    catch ( const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}