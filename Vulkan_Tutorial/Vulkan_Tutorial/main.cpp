#include "libs.h"

int main() 
{
    try
    {
        std::unique_ptr<TutorialApp> app = std::make_unique<TutorialApp>(800, 600, "VulkanWindow");
        app->run();
    } 
    catch ( const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}