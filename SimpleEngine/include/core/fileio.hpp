#pragma once
#include <map>
#include <string>
#include <vector>

namespace SE
{
	class FileIO
	{
	public:
		enum class Directory
		{
			SharedAssets,
			Assets,
			None
		};

		std::string ReadTextFile(Directory type, const std::string& path);

		bool WriteTextFile(Directory type, const std::string& path, const std::string& content);

		std::vector<char> ReadBinaryFile(Directory type, const std::string& path);

		bool WriteBinaryFile(Directory type, const std::string& path, const std::vector<char>& content);

		static bool WriteBinaryFile(const std::string& path, const std::vector<char>& content);

		std::string GetPath(Directory type, const std::string& path);

		bool Exists(Directory type, const std::string& path);

		static bool Exists(const std::string& path);

		uint64_t LastModified(Directory type, const std::string& path);

		static uint64_t LastModified(const std::string& path);

		Directory GetDirectory(const std::string& path);

		bool CreateDirectories(Directory type, const std::string& directory);

	private:
		friend class EngineClass;

		FileIO();

		~FileIO();

		// Roots of the paths per type, stored in a map.
		std::map<Directory, std::string> Paths;
	};

}  // namespace SE
