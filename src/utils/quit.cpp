#include "core/Webserv.hpp"

namespace Utils
{
	void	quit(std::string error)
	{
		std::cout << error << std::endl;
		exit (EXIT_FAILURE);
	}
}
