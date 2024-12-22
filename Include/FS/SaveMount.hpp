#pragma once
#include <string_view>
#include <switch.h>

namespace FS
{
    // This is just the default global generic save mount device.
    static constexpr std::string_view DEFAULT_SAVE_MOUNT = "save";
    // Same as above, but the path used for root.
    static constexpr std::string_view DEFAULT_SAVE_PATH = "save:/";
    // This function mounts save data according the save info passed. FsLib's unmount should be used.
    bool MountSaveData(const FsSaveDataInfo &SaveInfo, std::string_view DeviceName);
} // namespace FS
