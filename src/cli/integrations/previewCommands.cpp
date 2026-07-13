#include "cli/integrations/previewCommands.h"

#include <spdlog/spdlog.h>
#include "integration/preview/previewRenderer.h"

namespace kaisei::cli {

void PreviewCommands::setup(CLI::App* app, core::Registry& registry) {
    // preview start [image]
    auto* start = app->add_subcommand("start", "Preview the selected preset on an image");
    static std::string imagePath;

    start->add_option("image", imagePath, "Path to input image (defaults to examples/images/nature.jpg)");
    start->callback([&]() { PreviewCommands::start(registry, imagePath); });
}

void PreviewCommands::start(core::Registry& registry, const std::string& imagePath) {
    std::string finalPreset = registry.getCurrentPreset();
    if (finalPreset.empty()) {
        spdlog::error("No current preset is selected. Use 'kaisei preset select <name>' first.");
        return;
    }

    std::string finalImage = imagePath;
    if (finalImage.empty()) {
        finalImage = "examples/images/nature.jpg";
    }

    PreviewCommands::preview(registry, finalImage, finalPreset);
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
