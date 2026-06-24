#include "cli/cli.h"
#include <spdlog/spdlog.h>

int main(int argc, char** argv) {
    spdlog::set_level(spdlog::level::info);

    kaisei::cli::CLIApp app;
    return app.run(argc, argv);
}
