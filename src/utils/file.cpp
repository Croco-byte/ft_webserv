#include "utils/include.hpp"

namespace Utils
{
	bool			pathExists(std::string const & filename)
	{
		struct stat	tmp_stat;
		if (stat(filename.c_str(), &tmp_stat) != -1)
			return (true);
		return (false);
	}

	std::string		getFileContent(std::string const & filename)
	{
		std::ifstream file(filename.c_str());
		if (file.is_open() && file.good())
		{
			std::string content((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
			return (content);
		}
		return (NULL);
	}

	bool			isRegularFile(std::string const & filename)
	{
		struct stat tmp_stat;
		if (stat(filename.c_str(), &tmp_stat) == 0)
		{
			if (tmp_stat.st_mode & S_IFREG)
				return (true);
		}
		return (false);
	}

	bool			isDirectory(std::string const & name)
	{
		struct stat tmp_stat;
		if (stat(name.c_str(), &tmp_stat) == 0)
		{
			if (tmp_stat.st_mode & S_IFDIR)
				return (true);
		}
		return (false);
	}

	bool			canOpenFile(std::string const filename)
	{
		std::ifstream file(filename.c_str());
		bool ret = (file.is_open() && file.good());
		file.close();
		return (ret);
	}

	std::string		getLastModified(std::string const & path)
	{
		struct stat		tmp_stat;
		char			buffer[100];

		if (stat(path.c_str(), &tmp_stat) == 0)
		{
			strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tmp_stat.st_mtime));
			return (buffer);
		}
		return ("");
	}

	std::string		get_file_extension(std::string filename)
	{
		size_t	pos = filename.rfind('.');
		if (pos != std::string::npos)
			return (filename.substr(pos + 1));
		return ("");
	}
}
