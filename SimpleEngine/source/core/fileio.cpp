#include <cassert>
#include <filesystem>
#include <fstream>
#include "core/fileio.hpp"
#include "tools/log.hpp"

using namespace SE;
using namespace std;

std::string FileIO::ReadTextFile(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return string();
    }
    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    string buffer(size, '\0');
    file.seekg(0);
    file.read(buffer.data(), size);
    return buffer;
}

bool FileIO::WriteTextFile(Directory type, const std::string& path, const std::string& content)
{
    const auto fullPath = GetPath(type, path);
    ofstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return false;
    }
    file << content;
    file.close();
    return true;
}

std::vector<char> FileIO::ReadBinaryFile(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return vector<char>();
    }
    const streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) return buffer;
    assert(false);
    return vector<char>();
}

bool FileIO::WriteBinaryFile(Directory type, const std::string& path, const std::vector<char>& content)
{
    const auto fullPath = GetPath(type, path);
    return WriteBinaryFile(fullPath, content);
}

bool SE::FileIO::WriteBinaryFile(const std::string& path, const std::vector<char>& content)
{
    ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, path);
        return false;
    }
    file.write(content.data(), content.size());
    file.close();
    return true;
}

std::string FileIO::GetPath(Directory type, const std::string& path)
{
    return Paths[type] + path;
}

bool FileIO::Exists(Directory type, const std::string& path) { return Exists(GetPath(type, path)); }

bool SE::FileIO::Exists(const std::string& path)
{
    ifstream f(path.c_str());
    auto good = f.good();
    f.close();
    return good;
}

SE::FileIO::Directory SE::FileIO::GetDirectory(const std::string& path)
{
    if (path.find(Paths[Directory::SharedAssets]) != std::string::npos)
    {
        return Directory::SharedAssets;
    }
    else if (path.find(Paths[Directory::Assets]) != std::string::npos)
    {
        return Directory::Assets;
    }
    return Directory::None;
}

bool SE::FileIO::CreateDirectories(Directory type, const std::string& directory)
{
    auto path = GetPath(type, directory);
    return std::filesystem::create_directories(path);
}

uint64_t FileIO::LastModified(Directory type, const std::string& path) 
{ 
    return LastModified(GetPath(type, path));
}

uint64_t SE::FileIO::LastModified(const std::string& path)
{
    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(path);
    return static_cast<uint64_t>(ftime.time_since_epoch().count());
}

FileIO::FileIO()
{
    Paths[Directory::Assets] = "assets/";
    Paths[Directory::SharedAssets] = "../SimpleEngine/assets/";
}

FileIO::~FileIO() = default;