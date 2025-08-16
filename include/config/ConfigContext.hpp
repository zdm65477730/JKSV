#pragma once
#include "fslib.hpp"

#include <json-c/json.h>
#include <map>
#include <vector>

namespace config
{
    class ConfigContext
    {
        public:
            using AppIDList = std::vector<uint64_t>;

            ConfigContext() = default;

            /// @brief Resets the config to its default state.
            void reset();

            /// @brief Saves the config to the file.
            void save();

            /// @brief Loads the config from SD.
            void load();

            /// @brief Retrieves the value of the key passed.
            uint8_t get_by_key(std::string_view key);

            /// @brief Toggles a basic setting by the key passed.
            void toggle_by_key(std::string_view key);

            /// @brief Sets the value of a key.
            void set_by_key(std::string_view key, uint8_t value);

            /// @brief Returns the current working directory for JKSV.
            fslib::Path get_working_directory() const;

            /// @brief Sets the current working directory for JKSV.
            bool set_working_directory(std::string_view workDir);

            /// @brief Returns the current UI transition scaling.
            double get_animation_scaling() const;

            /// @brief Sets the current UI transistion scaling.
            void set_animation_scaling(double scaling);

            /// @brief Adds a favorite to the list if it hasn't been already.
            void add_favorite(uint64_t applicationID);

            /// @brief Removes a favorite from the lift it it's there.
            void remove_favorite(uint64_t applicationID);

            /// @brief Returns whether or not the application ID passed is a favorite title.
            bool is_favorite(uint64_t applicationID);

            /// @brief Adds the application ID to the blacklist if it hasn't been already.
            void add_to_blacklist(uint64_t applicationID);

            /// @brief Removes the application ID passed from the blacklist.
            void remove_from_blacklist(uint64_t applicationID);

            /// @brief Gets an array of the currently blacklisted titles.
            void get_blacklisted_titles(std::vector<uint64_t> &listOut);

            /// @brief Returns whether or not the application ID passed is blacklisted.
            bool is_blacklisted(uint64_t applicationID);

            /// @brief Returns if the blacklist is empty.
            bool blacklist_is_empty() const;

            /// @brief Adds a custom output path.
            void add_custom_path(uint64_t applicationID, std::string_view path);

            /// @brief Returns whether or not the application ID has a custom output path.
            bool has_custom_path(uint64_t applicationID);

            /// @brief Gets the output path of the application ID passed.
            void get_custom_path(uint64_t applicationID, char *pathBuffer, size_t bufferSize);

        private:
            /// @brief This is where most values are stored.
            std::map<std::string, uint8_t> m_configMap{};

            /// @brief This is the main directory JKSV uses.
            fslib::Path m_workingDirectory{};

            /// @brief The scaling of transitions.
            double m_animationScaling{};

            /// @brief Vector of favorites
            ConfigContext::AppIDList m_favorites{};

            /// @brief Vector of blacklisted title ids.
            ConfigContext::AppIDList m_blacklist{};

            /// @brief Map of custom output paths for titles.
            std::map<uint64_t, std::string> m_pathMap;

            /// @brief Reads an array from a json_object into the vector passed.
            void read_array_to_vector(std::vector<uint64_t> &vector, json_object *array);

            /// @brief Saves the custom paths set by the user.
            void save_custom_paths();

            /// @brief Searches for and returns an iterator to the application ID (if found);
            ConfigContext::AppIDList::iterator find_favorite(uint64_t applicationID);

            /// @brief Searches for and returns an iterator to the application ID (if found);
            ConfigContext::AppIDList::iterator find_blacklist(uint64_t applicationID);
    };
}
