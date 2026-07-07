#include "Application.hpp"
#include "ProgramOptions.hpp"

int main(int argc, char* argv[])
{
    auto options = parseArguments(argc, argv);

    Application app(options);
    return app.run();
}