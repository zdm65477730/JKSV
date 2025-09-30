#pragma once

namespace cmdargs
{
    /// @brief Stores the pointers passed to main for later usage without making JKSV a mess.
    void store(int argc, const char *argv[]);

    /// @brief Gets the argument at index.
    const char *get(int index);
}
