#include "cli/previewCommands.h"

#include <spdlog/spdlog.h>
#include "integration/preview/previewRenderer.h"

namespace kaisei::cli {

void PreviewCommands::setup(CLI::App* app, core::Registry& registry) {
    // preview <image> <preset>
    auto* preview = app->add_subcommand("preview", "Preview a preset on an image");
    static std::string imagePath;
    static std::string presetName;

    preview->add_option("image", imagePath, "Path to input image")->required();
    preview->add_option("preset", presetName, "Preset name")->required();
    preview->callback([&]() { PreviewCommands::preview(registry, imagePath, presetName); });
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
