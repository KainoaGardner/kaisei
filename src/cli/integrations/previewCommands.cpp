#include "cli/integrations/previewCommands.h"

#include <spdlog/spdlog.h>
#include "integration/preview/previewRenderer.h"

namespace kaisei::cli {

void PreviewCommands::setup(CLI::App* app, core::Registry& registry) {
    // preview start <image> <preset>
    auto* start = app->add_subcommand("start", "Preview a preset on an image");
    static std::string imagePath;
    static std::string presetName;

    start->add_option("image", imagePath, "Path to input image")->required();
    start->add_option("preset", presetName, "Preset name")->required();
    start->callback([&]() { PreviewCommands::preview(registry, imagePath, presetName); });
}

void PreviewCommands::preview(core::Registry& registry, const std::string& imagePath, const std::string& presetName) {
    try {
        integration::preview::PreviewRenderer renderer(registry);
        renderer.run(imagePath, presetName);
    } catch (const std::exception& e) {
        spdlog::error("Preview failed: {}", e.what());
    }
}

} // namespace kaisei::cli
