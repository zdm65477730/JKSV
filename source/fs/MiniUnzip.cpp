#include "fs/MiniUnzip.hpp"

#include "error.hpp"
#include "logger.hpp"

fs::MiniUnzip::MiniUnzip(const fslib::Path &path) { MiniUnzip::open(path); }

fs::MiniUnzip::~MiniUnzip() { MiniUnzip::close(); }

bool fs::MiniUnzip::is_open() const { return m_isOpen; }

bool fs::MiniUnzip::open(const fslib::Path &path)
{
    MiniUnzip::close();
    m_unz = unzOpen64(path.full_path());
    if (error::is_null(m_unz) || !MiniUnzip::reset()) { return false; }
    m_isOpen = true;
    return true;
}

void fs::MiniUnzip::close()
{
    if (!m_isOpen) { return; }
    unzClose(m_unz);
    m_isOpen = false;
}

bool fs::MiniUnzip::next_file()
{
    const bool notEnd  = unzGoToNextFile(m_unz) == UNZ_OK;
    const bool getInfo = unzGetCurrentFileInfo64(m_unz, &m_fileInfo, m_filename, FS_MAX_PATH, nullptr, 0, nullptr, 0) == UNZ_OK;
    const bool opened  = unzOpenCurrentFile(m_unz) == UNZ_OK;
    return notEnd && getInfo && opened;
}

bool fs::MiniUnzip::close_current_file() { return unzCloseCurrentFile(m_unz) == UNZ_OK; }

bool fs::MiniUnzip::locate_file(std::string_view filename)
{
    if (!MiniUnzip::reset()) { return false; }

    do {
        if (m_filename == filename) { return true; }
    } while (MiniUnzip::next_file());

    MiniUnzip::reset();
    return false;
}

bool fs::MiniUnzip::reset()
{
    const bool firstFile = unzGoToFirstFile(m_unz) == UNZ_OK;
    const bool getInfo = unzGetCurrentFileInfo64(m_unz, &m_fileInfo, m_filename, FS_MAX_PATH, nullptr, 0, nullptr, 0) == UNZ_OK;
    const bool opened  = firstFile && unzOpenCurrentFile(m_unz) == UNZ_OK;
    return firstFile && getInfo && opened;
}

ssize_t fs::MiniUnzip::read(void *buffer, size_t bufferSize) { return unzReadCurrentFile(m_unz, buffer, bufferSize); }

const char *fs::MiniUnzip::get_filename() { return m_filename; }

uint64_t fs::MiniUnzip::get_compressed_size() const { return m_fileInfo.compressed_size; }

uint64_t fs::MiniUnzip::get_uncompressed_size() const { return m_fileInfo.uncompressed_size; }
