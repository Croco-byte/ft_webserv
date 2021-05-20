#include <unistd.h>
#include <iostream>

int main()
{
	uint32_t		a = 0b11111111111111111111111111111111;
	unsigned int	tab[4];

	char ipAddr[16];

	tab[0] = (a & 0xff000000) >> 24;
	tab[1] = (a & 0x00ff0000) >> 16;
	tab[2] = (a & 0x0000ff00) >> 8;
	tab[3] = (a & 0x000000ff);

	std::cout << a << std::endl;
	for (int i = 0; i < 4; i++)
		std::cout << tab[i] << std::endl;
}