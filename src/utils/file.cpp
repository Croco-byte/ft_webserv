#include "include.hpp"

namespace Utils
{
	bool			file_exist(std::string const & filename)
	{
		struct stat	tmp_stat;
		if (stat(filename.c_str(), &tmp_stat) != -1)
			return (true);
		return (false);
	}

	bool			isRegularFile(std::string const & filename)
	{
		struct stat tmp_stat;
		stat(filename.c_str(), &tmp_stat);
		if (S_ISREG(tmp_stat.st_mode))
			return (true);
		return (false);
	}

	bool			isDirectory(std::string const & name)
	{
		struct stat tmp_stat;
		stat(name.c_str(), &tmp_stat);
		if (S_ISDIR(tmp_stat.st_mode))
			return (true);
		return (false);
	}

	std::string		get_file_extension(std::string filename)
	{
		size_t	pos = filename.rfind('.');
		if (pos != std::string::npos)
			return (filename.substr(pos + 1));
		return ("");
	}
}
