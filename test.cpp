#include <unistd.h>
#include <iostream>

std::string					reverse(std::string &str)
{
	size_t	n = str.length();
	char	tmp;

	for (int i = 0; i < n / 2; i++)
	{
		tmp = str[i];
		str[i] = str[n - i - 1];
		str[n - i - 1] = tmp;
	}
	return (str);
}

std::string					to_string(char c)
	{
		std::string	str = "x";
		str[0] = c;
		return (str);
	}

std::string					to_string(int nb)
	{
		std::string	str;

		if (nb < 0)
			str += "-";
		while (nb != 0)
		{
			str += to_string((char)('0' + (nb % 10)));
			nb /= 10;
		}
		return (reverse(str));
	}

	std::string					to_string(size_t a)
	{
		std::string	str;
		
		while (a != 0)
		{
			str += to_string((char)('0' + (a % 10)));
			a /= 10;
		}
		return (reverse(str));
	}

int main()
{
	int a = 42;
	size_t b = 21;
	char c = 'y';
	std::cout << to_string(a) << to_string(b) << to_string(c) << std::endl;
	return (0);
}