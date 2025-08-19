#pragma once
#include "fslib.hpp"

#include <cstdint>
#include <json-c/json.h>
#include <map>
#include <string>
#include <vector>

namespace config
{
    class ConfigContext
    {
        public:
            ConfigContext() = default;

            /// @brief Ensures the config directory exists.
            void create_directory();

            /// @brief Resets and saves the config back to the default values.
            void reset();

            /// @brief Loads the config and custom paths files from the SD card if possible.
            bool load();

            /// @brief Saves the config to the SD if possible.
            void save();

            /// @brief Attempts to find and return the value of the key passed.
            uint8_t get_by_key(std::string_view key) const;

            /// @brief Attempts to toggle the value for the key passed. For simple 1 and 0.
            void toggle_by_key(std::string_view key);

            /// @brief Attempts to set the key passed to the value passd.
            void set_by_key(std::string_view key, uint8_t value);

            /// @brief Returns the current working directory.
            fslib::Path get_working_directory() const;

            /// @brief Sets the current work directory if the path passed is valid.
            bool set_working_directory(const fslib::Path &workDir);

            /// @brief Returns the transition scaling speed.
            double get_animation_scaling() const;

            /// @brief Sets the current animation scaling speed.
            void set_animation_scaling(double scaling);

            /// @brief Adds a favorite to the favorite titles.
            void add_favorite(uint64_t applicationID);

            /// @brief Removes a title from the favorites.
            void remove_favorite(uint64_t applicationID);

            /// @brief Returns if the application ID passed is a favorite.
            bool is_favorite(uint64_t applicationID) const;

            /// @brief Adds the application ID passed to the blacklist.
            void add_to_blacklist(uint64_t applicationID);

            /// @brief Removes the application ID passed from the blacklist.
            void remove_from_blacklist(uint64_t applicationID);

            /// @brief Writes all of the currently blacklisted titles to the vector passed.
            void get_blacklist(std::vector<uint64_t> &listOut);

            /// @brief Returns if the application ID passed is found in the blacklist.
            bool is_blacklisted(uint64_t applicationID) const;

            /// @brief Returns if the blacklist is empty.
            bool blacklist_empty() const;

            /// @brief Returns if the application ID passed has a custom output path.
            bool has_custom_path(uint64_t applicationID) const;

            /// @brief Adds a new output path.
            void add_custom_path(uint64_t applicationID, const fslib::Path &path);

            /// @brief Gets the custom output path of the applicationID passed.
            void get_custom_path(uint64_t applicationID, char *buffer, size_t bufferSize);

        private:
            /// @brief This is where the majority of the config values are.
            std::map<std::string, uint8_t> m_configMap{};

            /// @brief The main directory JKSV operates from.
            fslib::Path m_workingDirectory{};

            /// @brief This scales the transitions.
            double m_animationScaling{};

            /// @brief This holds the favorite titles.
            std::vector<uint64_t> m_favorites{};

            /// @brief This holds the blacklisted titles.
            std::vector<uint64_t> m_blacklist{};

            /// @brief This holds user defined paths.
            std::map<uint64_t, std::string> m_paths{};

            /// @brief Loads the config file from SD.
            bool load_config_file();

            /// @brief Saves the config to the SD.
            void save_config_file();

            /// @brief Reads a json array to the vector passed.
            void read_array_to_vector(std::vector<uint64_t> &vector, json_object *array);

            /// @brief Loads the custom output path file from the SD if possible.
            void load_custom_paths();

            /// @brief Saves the custom output paths to the SD card.
            void save_custom_paths();

            /// @brief Searches the vector passed for the application ID passed.
            std::vector<uint64_t>::const_iterator find_application_id(const std::vector<uint64_t> &vector,
                                                                      uint64_t applicationID) const;
    };
}
