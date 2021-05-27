#include "utils/include.hpp"

namespace Utils
{
	std::string	get_current_time()
	{
		struct timeval tv;
		std::string    ret;
		char           str[64];
		time_t         time;
		struct tm      *current = NULL;

		gettimeofday(&tv, NULL);
		time = tv.tv_sec;
		current = localtime(&time);
		strftime(str, sizeof str, "%a, %d %b %G %T GMT", current);
		ret.assign(str);
		return (ret);
	}
}
