#include "FS/SaveMount.hpp"
#include "FsLib.hpp"

bool FS::MountSaveData(const FsSaveDataInfo &SaveInfo, std::string_view DeviceName)
{
    switch (SaveInfo.save_data_type)
    {
        case FsSaveDataType_System:
        {
            // This should work because JKSV's internal ID for the system user is 0.
            return FsLib::OpenSystemSaveFileSystem(DeviceName, SaveInfo.system_save_data_id, SaveInfo.uid);
        }
        break;

        case FsSaveDataType_Account:
        {
            return FsLib::OpenAccountSaveFileSystem(DeviceName, SaveInfo.application_id, SaveInfo.uid);
        }
        break;

        case FsSaveDataType_Bcat:
        {
            return FsLib::OpenBCATSaveFileSystem(DeviceName, SaveInfo.application_id);
        }
        break;

        case FsSaveDataType_Device:
        {
            return FsLib::OpenDeviceSaveFileSystem(DeviceName, SaveInfo.application_id);
        }
        break;

        case FsSaveDataType_Temporary:
        {
            return FsLib::OpenTemporarySaveFileSystem(DeviceName);
        }
        break;

        case FsSaveDataType_Cache:
        {
            return FsLib::OpenCacheSaveFileSystem(DeviceName, SaveInfo.application_id, SaveInfo.save_data_index);
        }
        break;

        case FsSaveDataType_SystemBcat:
        {
            return FsLib::OpenSystemBCATSaveFileSystem(DeviceName, SaveInfo.system_save_data_id);
        }
        break;
    }
    return false;
}
