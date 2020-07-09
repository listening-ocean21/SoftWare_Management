#include <fstream>
#include <iostream>
#include <vector>

class CFile {
public:
	CFile();
	static std::vector<char> readFile(const std::string& vFilename)
	{
		std::ifstream file(vFilename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
	}
};