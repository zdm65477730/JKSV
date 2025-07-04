#pragma once
#include <switch.h>

// This solves a lot of problems.
namespace data
{
    static constexpr AccountUid BLANK_ACCOUNT_ID = {0};
} // namespace data


/// @brief Allows comparison of AccountUids since devkitpro decided a struct with two uint64_t's is better than u128
/// @param accountIDA First account to compare.
/// @param accountIDB Second account to compare.
/// @return True if both account IDs match.
static inline bool operator==(AccountUid accountIDA, AccountUid accountIDB)
{
    return (accountIDA.uid[0] == accountIDB.uid[0]) && (accountIDA.uid[1] == accountIDB.uid[1]);
}

/// @brief Allows comparison of an AccountUid and a number.
/// @param accountIDA AccountUid to compare.
/// @param accountIDB Number to compare.
/// @return True if they match. False if they don't.
/// @note I'm not 100% sure which uint64_t in the AccountUid struct comes first. I don't know if it's [0][1] or [1][0]. To do: Figure that out.
static inline bool operator==(AccountUid accountIDA, u128 accountIDB)
{
    return accountIDA.uid[0] == (accountIDB >> 64 & 0xFFFFFFFFFFFFFFFF) &&
           accountIDA.uid[1] == (accountIDB & 0xFFFFFFFFFFFFFFFF);
}
