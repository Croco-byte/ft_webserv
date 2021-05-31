#ifndef INCLUDE_HPP
# define INCLUDE_HPP

# include <unistd.h>
# include <iostream>
# include <fstream>
# include <sys/socket.h>
# include <cstdlib>
# include <fcntl.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <ctime>
# include <limits.h>
# include <sys/un.h>
# include <netinet/in.h>
# include <stdio.h>
# include <stdlib.h>
# include <vector>
# include <map>
# include <list>
# include <algorithm>
# include <utility>
# include <sys/time.h>
# include <sys/stat.h>
# include <signal.h>
# include "Console.hpp"

# define RED								"\033[0;31m"
# define GREEN								"\033[0;32m"
# define CYAN								"\033[0;36m"
# define NC									"\033[0m"

# define closesocket(param)					close(param)
# define INVALID_SOCKET						-1
# define SOCKET_ERROR						-1

# define ENDL								"\r\n"

# define ON									true
# define OFF								false

# define UNUSED(X)							(void)(X)

typedef int									t_socket;
typedef struct sockaddr_in					t_sockaddr_in;
typedef struct sockaddr						t_sockaddr;

typedef	std::map<std::string, std::string>	DoubleString;

namespace Utils
{
	void						quit(std::string error);

	std::vector<std::string>	split(const std::string& str, const std::string& delim);
	std::vector<std::string>	split_any(const std::string& str, const std::string& delim);
	bool						ft_isspace(char c);
	bool						is_empty(std::string str);
	std::string					&trim(std::string &s);
	std::string					to_lower(std::string &str);
	std::string					to_upper(std::string &str);
	std::string					get_current_time();
	std::string 				base64_decode(const std::string &in);
	std::string					remove_char(std::string &str, std::string c);
	bool						is_alphanum(std::string str);
	bool						is_positive_number(std::string str);
	bool						string_to_bool(std::string on);
	std::string					bool_to_string(bool on);
	std::string					join(std::vector<std::string> vec);
	size_t						getCommonValues(std::vector<std::string> a, std::vector<std::string> b);
	std::string					to_string(int nb);
	std::string					to_string(char c);
	std::string					to_string(size_t a);
	std::string					to_string(long a);
	std::string					reverse(std::string &str);
	void						*memcpy(void *dst, void *src, size_t len);
	std::string					replace(std::string src, std::string search, std::string replace);

	std::string					colorify(std::string str);
	std::string					colorify(bool on);

	bool						pathExists(std::string const & filename);
	bool						isRegularFile(std::string const & filename);
	bool						isDirectory(std::string const & name);
	bool						canOpenFile(std::string const filename);
	std::string					getFileContent(std::string const & filename);
	std::string					getLastModified(std::string const & path);
	std::string					get_file_extension(std::string filename);

	template <typename T1, typename T2>
	bool		find_in_vector(std::vector<T1> vec, T2 elem)
	{
		for (size_t i = 0; i < vec.size(); i++)
			if (vec[i] == elem)
				return (true);
		return (false);
	}
}

#endif
