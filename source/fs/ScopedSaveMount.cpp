#include "fs/ScopedSaveMount.hpp"

#include "error.hpp"
#include "fslib.hpp"

fs::ScopedSaveMount::ScopedSaveMount(std::string_view mount, const FsSaveDataInfo *saveInfo)
    : m_mountPoint(mount)
{
    const bool mountError = error::fslib(fslib::open_save_data_with_save_info(m_mountPoint, *saveInfo));
    m_isOpen              = !mountError;
}

fs::ScopedSaveMount::ScopedSaveMount(ScopedSaveMount &&scopedSaveMount) { *this = std::move(scopedSaveMount); }

fs::ScopedSaveMount &fs::ScopedSaveMount::operator=(ScopedSaveMount &&scopedSaveMount)
{
    m_mountPoint             = std::move(scopedSaveMount.m_mountPoint);
    m_isOpen                 = scopedSaveMount.m_isOpen;
    scopedSaveMount.m_isOpen = false;
    return *this;
}

fs::ScopedSaveMount::~ScopedSaveMount() { error::fslib(fslib::close_file_system(m_mountPoint)); }

bool fs::ScopedSaveMount::is_open() const { return m_isOpen; }
