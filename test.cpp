#include <iostream>

int main(int ac, char **av, char **env)
{
	int i = 0;
	std::cout << "Request file " << av[1] << std::endl;
	for (; env[i] != NULL; i++)
		std::cout << env[i] << std::endl;
	std::cout << "Total: " << i << " meta variables" << std::endl;
}