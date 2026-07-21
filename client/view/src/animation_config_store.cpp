#include "client/view/include/animation_config_store.hpp"

#include <cctype>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <utility>

#include "client/view/include/asset_paths.hpp"

namespace kfc::view {
namespace {

// Keys read from the config.json "graphics" section. The "physics" keys
// (speed_m_per_sec, next_state_when_finished) are intentionally not read: they
// are Business Logic concerns, so the view never parses them.
const char* const framesPerSecKey = "frames_per_sec";
const char* const isLoopKey = "is_loop";
const char* const trueLiteral = "true";
const char* const spritesSubdir = "sprites";
const char* const spriteExtension = ".png";

// Used when a config is missing or malformed, so a bad asset degrades to a
// playable (if wrong) animation rather than crashing the render loop.
const int fallbackFramesPerSec = 6;
const bool fallbackIsLoop = true;

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Position just past the ':' that follows "key" in text, or npos if the key is
// absent. A single helper so both field readers locate their value the same way.
std::size_t valueStart(const std::string& text, const std::string& key) {
    const std::size_t keyPos = text.find('"' + key + '"');
    if (keyPos == std::string::npos) {
        return std::string::npos;
    }
    const std::size_t colon = text.find(':', keyPos);
    if (colon == std::string::npos) {
        return std::string::npos;
    }
    std::size_t i = colon + 1;
    while (i < text.size() &&
           std::isspace(static_cast<unsigned char>(text[i]))) {
        ++i;
    }
    return i;
}

std::optional<int> readInt(const std::string& text, const std::string& key) {
    const std::size_t start = valueStart(text, key);
    if (start == std::string::npos) {
        return std::nullopt;
    }
    int value = 0;
    const char* first = text.data() + start;
    const char* last = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(first, last, value);
    if (parsed.ec != std::errc()) {
        return std::nullopt;
    }
    return value;
}

std::optional<bool> readBool(const std::string& text, const std::string& key) {
    const std::size_t start = valueStart(text, key);
    if (start == std::string::npos) {
        return std::nullopt;
    }
    return text.compare(start, std::string(trueLiteral).size(), trueLiteral) == 0;
}

int countSpriteFrames(const std::string& stateDirPath) {
    const std::filesystem::path spritesDir =
        std::filesystem::path(stateDirPath) / spritesSubdir;
    std::error_code ec;
    int count = 0;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(spritesDir, ec)) {
        if (entry.is_regular_file() &&
            entry.path().extension() == spriteExtension) {
            ++count;
        }
    }
    return count;
}

}  // namespace

AnimationConfigStore::AnimationConfigStore(std::string assetsRoot)
    : assetsRoot_(std::move(assetsRoot)) {}

AnimationConfig AnimationConfigStore::configFor(model::Kind kind,
                                                model::Color color,
                                                model::State state) {
    const std::string key =
        pieceCode(kind, color) + '/' + stateFolder(state);
    auto cached = cache_.find(key);
    if (cached != cache_.end()) {
        return cached->second;
    }
    const AnimationConfig config = load(kind, color, state);
    cache_.emplace(key, config);
    return config;
}

AnimationConfig AnimationConfigStore::load(model::Kind kind, model::Color color,
                                           model::State state) const {
    const std::string text =
        readFile(configPath(assetsRoot_, kind, color, state));
    const int framesPerSec =
        readInt(text, framesPerSecKey).value_or(fallbackFramesPerSec);
    const bool isLoop = readBool(text, isLoopKey).value_or(fallbackIsLoop);
    const int frameCount =
        countSpriteFrames(stateDir(assetsRoot_, kind, color, state));
    return AnimationConfig{framesPerSec, isLoop, frameCount};
}

}  // namespace kfc::view
