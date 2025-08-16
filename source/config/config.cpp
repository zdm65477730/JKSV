#include "config/config.hpp"

#include "config/ConfigContext.hpp"
#include "logging/error.hpp"

namespace
{
    config::ConfigContext s_context{};

    constexpr std::string_view PATH_CONFIG_FOLDER = "sdmc:/config/JKSV";
    constexpr const char *PATH_CONFIG_FILE        = "sdmc:/config/JKSV/JKSV.json";
} // namespace

void config::initialize()
{
    // This is so we don't constantly construct new Paths
    const fslib::Path configDir{PATH_CONFIG_FOLDER};
    const fslib::Path configFile{PATH_CONFIG_FILE};

    const bool configDirExists = fslib::directory_exists(configDir);
    const bool configDirError  = !configDirExists && error::fslib(fslib::create_directories_recursively(configDir));
    if (!configDirExists && configDirError)
    {
        s_context.reset();
        return;
    }

    const bool configExists = fslib::file_exists(configFile);
    if (!configExists)
    {
        s_context.reset();
        return;
    }

    s_context.load();
}

void config::reset_to_default() { s_context.reset(); }

void config::save() { s_context.save(); }

uint8_t config::get_by_key(std::string_view key) { return s_context.get_by_key(key); }

void config::toggle_by_key(std::string_view key) { s_context.toggle_by_key(key); }

void config::set_by_key(std::string_view key, uint8_t value) { s_context.set_by_key(key, value); }

fslib::Path config::get_working_directory() { return s_context.get_working_directory(); }

bool config::set_working_directory(std::string_view path) { return s_context.set_working_directory(path); }

double config::get_animation_scaling() { return s_context.get_animation_scaling(); }

void config::set_animation_scaling(double newScale) { s_context.set_animation_scaling(newScale); }

void config::add_remove_favorite(uint64_t applicationID)
{
    if (s_context.is_favorite(applicationID)) { s_context.remove_favorite(applicationID); }
    else { s_context.add_favorite(applicationID); }
    s_context.save();
}

bool config::is_favorite(uint64_t applicationID) { return s_context.is_favorite(applicationID); }

void config::add_remove_blacklist(uint64_t applicationID)
{
    if (s_context.is_blacklisted(applicationID)) { s_context.remove_from_blacklist(applicationID); }
    else { s_context.add_to_blacklist(applicationID); }
    s_context.save();
}

void config::get_blacklisted_titles(std::vector<uint64_t> &listOut) { s_context.get_blacklisted_titles(listOut); }

bool config::is_blacklisted(uint64_t applicationID) { return s_context.is_blacklisted(applicationID); }

bool config::blacklist_is_empty() { return s_context.blacklist_is_empty(); }

void config::add_custom_path(uint64_t applicationID, std::string_view customPath)
{
    s_context.add_custom_path(applicationID, customPath);
}

bool config::has_custom_path(uint64_t applicationID) { return s_context.has_custom_path(applicationID); }

void config::get_custom_path(uint64_t applicationID, char *pathOut, size_t pathOutSize)
{
    s_context.get_custom_path(applicationID, pathOut, pathOutSize);
}
