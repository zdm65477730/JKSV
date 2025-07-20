#include "fs/MiniUnzip.hpp"

#include "error.hpp"

fs::MiniUnzip::MiniUnzip(const fslib::Path &path) { MiniUnzip::open(path); }

fs::MiniUnzip::~MiniUnzip() { MiniUnzip::close(); }

bool fs::MiniUnzip::is_open() const { return m_isOpen; }

bool fs::MiniUnzip::open(const fslib::Path &path)
{
    MiniUnzip::close();
    m_unz = unzOpen64(path.full_path());
    if (error::is_null(m_unz)) { return false; }
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
    const bool end = unzGoToNextFile(m_unz) == UNZ_END_OF_LIST_OF_FILE;
    const bool readInfo =
        !end && unzGetCurrentFileInfo64(m_unz, &m_fileInfo, m_filename, FS_MAX_PATH, nullptr, 0, nullptr, 0) == UNZ_OK;
    return !end && readInfo;
}

ssize_t fs::MiniUnzip::read(void *buffer, size_t bufferSize) { return unzReadCurrentFile(buffer, buffer, bufferSize); }

uint64_t fs::MiniUnzip::get_compressed_size() const { return m_fileInfo.compressed_size; }

uint64_t fs::MiniUnzip::get_uncompressed_size() const { return m_fileInfo.uncompressed_size; }
