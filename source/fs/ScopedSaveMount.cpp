#include "fs/ScopedSaveMount.hpp"

#include "error.hpp"
#include "fslib.hpp"

fs::ScopedSaveMount::ScopedSaveMount(std::string_view mount, const FsSaveDataInfo *saveInfo, bool log)
    : m_mountPoint(mount)
    , m_log(log)
{
    if (m_log) { m_isOpen = !error::fslib(fslib::open_save_data_with_save_info(m_mountPoint, *saveInfo)); }
    else { m_isOpen = fslib::open_save_data_with_save_info(m_mountPoint, *saveInfo); }
}

fs::ScopedSaveMount::ScopedSaveMount(ScopedSaveMount &&scopedSaveMount) noexcept { *this = std::move(scopedSaveMount); }

fs::ScopedSaveMount &fs::ScopedSaveMount::operator=(ScopedSaveMount &&scopedSaveMount) noexcept
{
    m_mountPoint             = std::move(scopedSaveMount.m_mountPoint);
    m_isOpen                 = scopedSaveMount.m_isOpen;
    scopedSaveMount.m_isOpen = false;
    return *this;
}

fs::ScopedSaveMount::~ScopedSaveMount()
{
    if (m_log) { error::fslib(fslib::close_file_system(m_mountPoint)); }
    else { fslib::close_file_system(m_mountPoint); }
}

bool fs::ScopedSaveMount::is_open() const noexcept { return m_isOpen; }
