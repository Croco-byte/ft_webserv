#include "utils/include.hpp"

namespace Utils
{
	std::vector<std::string> split(const std::string& str, const std::string& delim)
	{
		std::vector<std::string> tokens;
		size_t prev = 0, pos = 0;
		do
		{
			pos = str.find(delim, prev);
			if (pos == std::string::npos) pos = str.length();
			std::string token = str.substr(prev, pos-prev);
			if (!token.empty())
				tokens.push_back(token);
			prev = pos + delim.length();
		} while (pos < str.length() && prev < str.length());
		return tokens;
	}

	std::vector<std::string> split_any(const std::string& str, const std::string& delim)
	{
		std::vector<std::string> tokens;
		size_t prev = 0, pos = 0;
		do
		{
			pos = str.find_first_of(delim, prev);
			if (pos == std::string::npos) pos = str.length();
			std::string token = str.substr(prev, pos-prev);
			if (!token.empty())
				tokens.push_back(token);
			prev = pos + 1;
		} while (pos < str.length() && prev < str.length());
		return tokens;
	}

	bool					ft_isspace(char c)
	{
		if ((c >= 9 && c <= 13) || c == 32)
			return (true);
		return (false);
	}

	bool					is_empty(std::string str)
	{
		for (unsigned int i= 0; i < str.length(); i++)
			if (!isspace(str[i]))
				return (false);
		return (true);
	}

	const char* whitespaces = " \t\n\r\f\v";

	std::string& rtrim(std::string& s)
	{
		const char* ws = " \t\n\r\f\v";

		s.erase(s.find_last_not_of(ws) + 1);
		return s;
	}

	std::string& ltrim(std::string& s)
	{
		const char* ws = " \t\n\r\f\v";

		s.erase(0, s.find_first_not_of(ws));
		return s;
	}

	std::string	&trim(std::string& s)
	{
		return ltrim(rtrim(s));
	}

	bool		is_alpha(char c)
	{
		return (std::isalpha(c));
	}

	bool		is_alphanum(std::string str)
	{
		for (std::string::iterator it = str.begin(); it != str.end(); it++)
			if (!std::isalnum(*it))
				return (false);
		return (true);
	}

	std::string	to_lower(std::string &str)
	{
		for (size_t i = 0; i < str.length(); i++)
			str[i] = std::tolower(str[i]);
		return (str);
	}

	std::string	to_upper(std::string &str)
	{
		for (size_t i = 0; i < str.length(); i++)
			str[i] = std::toupper(str[i]);
		return (str);
	}

	std::string base64_decode(const std::string &in)
	{
		std::string out;
		std::vector<int> T(256,-1);

		for (int i=0; i<64; i++)
			T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

		int val=0, valb=-8;
		for (size_t i = 0; i < in.length(); i++)
		{
			char c = in[i];
			if (T[c] == -1) break;
			val = (val << 6) + T[c];
			valb += 6;
			if (valb >= 0) {
				out.push_back(char((val>>valb)&0xFF));
				valb -= 8;
			}
		}
		return out;
	}

	std::string	remove_char(std::string &str, std::string c)
	{
		for (std::string::iterator it = c.begin(); it != c.end(); it++)
			str.erase(std::remove(str.begin(), str.end(), *it), str.end());
		return (str);
	}

	bool		string_to_bool(std::string on)
	{
		on = Utils::to_lower(on);
		if (on == "on")
			return (true);
		return (false);
	}

	std::string	join(std::vector<std::string> vec)
	{
		std::string	ret = "";

		for (std::vector<std::string>::iterator it = vec.begin(); it != vec.end(); it++)
		{
			ret += *it;
			if (it + 1 != vec.end())
				ret += ", ";
		}
		return (ret);
	}

	std::string join(DoubleString map)
	{
		std::string output = "";
		std::string result = "";

		for (DoubleString::iterator it = map.begin(); it != map.end(); it++)
			output += (it->first) + ":" + it->second + ", ";
			
		result = output.substr(0, output.size() - 2 );
			
		return result;
	}

	std::string	bool_to_string(bool on)
	{
		if (on)
			return ("on");
		return ("off");
	}

	std::string	colorify(std::string str)
	{
		return (str);
	}

	std::string	colorify(bool on)
	{
		if (on)
			return "\033[0;32mon\033[0m";
		else
			return "\033[0;31moff\033[0m";
	}

	size_t		getCommonValues(std::vector<std::string> a, std::vector<std::string> b)
	{
		std::vector<std::string>::iterator	it_a = a.begin();
		std::vector<std::string>::iterator	it_b = b.begin();
		size_t								ret = 0;

		while (it_a != a.end() && it_b != b.end())
		{
			if (*it_a == *it_b)
				ret++;
			it_a++;
			it_b++;
		}
		return (ret);
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
		bool		negative = false;

		if (nb  == 0)
			return (std::string("0"));
		if (nb < 0)
			negative = true;
		while (nb != 0)
		{
			str += Utils::to_string((char)('0' + (nb % 10)));
			nb /= 10;
		}
		if (negative)
			str += "-";
		return (Utils::reverse(str));
	}

	std::string					to_string(size_t a)
	{
		std::string	str;
		
		if (a  == 0)
			return (std::string("0"));
		while (a != 0)
		{
			str += Utils::to_string((char)('0' + (a % 10)));
			a /= 10;
		}
		return (Utils::reverse(str));
	}

	std::string					to_string(long nb)
	{
		std::string	str;
		bool		negative = false;

		if (nb  == 0)
			return (std::string("0"));
		if (nb < 0)
			negative = true;
		while (nb != 0)
		{
			str += Utils::to_string((char)('0' + (nb % 10)));
			nb /= 10;
		}
		if (negative)
			str += "-";
		return (Utils::reverse(str));
	}

	std::string					reverse(std::string &str)
	{
		size_t	n = str.length();
		char	tmp;

		for (size_t i = 0; i < n / 2; i++)
		{
			tmp = str[i];
			str[i] = str[n - i - 1];
			str[n - i - 1] = tmp;
		}
		return (str);
	}

	void						*memcpy(void *dst, void *src, size_t len)
	{
		unsigned char	*s1;
		unsigned char	*s2;
		size_t			i;

		if (dst == NULL || src == NULL)
			return (NULL);
		s1 = static_cast<unsigned char*>(dst);
		s2 = static_cast<unsigned char*>(src);
		i = 0;
		while (i < len)
		{
			s1[i] = s2[i];
			i++;
		}
		return (dst);
	}

	bool						is_positive_number(std::string str)
	{
		for (std::string::iterator it = str.begin(); it != str.end(); it++)
			if (*it < '0' || *it > '9')
				return (false);
		return (true);
	}

	std::string					replace(std::string src, std::string search, std::string replace)
	{
		for (size_t pos = 0; ; pos += replace.length())
		{
			pos = src.find(search, pos);
			if (pos == std::string::npos)
				break ;
			src.erase(pos, search.length());
			src.insert(pos, replace);
		}
		return (src);
	}

	int								checkEnd(const std::string& str, const std::string& end)
	{
		int	i = str.size();
		int	j = end.size();

		while (j > 0)
		{
			i--;
			j--;
			if (i < 0 || str[i] != end[j])
				return (1);
		}
		return (0);
	}

	bool						receivedLastChunk(std::string & request)
	{
		if (!checkEnd(request, "0\r\n\r\n"))
			return (true);
		return (false);
	}

	size_t						extractContentLength(std::string const & request)
	{
		size_t i = request.find("Content-Length: ");
		i += 16;

		size_t temp(i);
		size_t count(0);
		while (std::isdigit(request[temp]))
		{
			count++;
			temp++;
		}
		size_t result = std::atoi(request.substr(i, count).c_str());
		return (result);
	}

	std::vector<std::string>	divise_string(std::string str, unsigned int chunk_size)
	{
		std::vector<std::string>	vecChunks;

		for (unsigned int i = 0; i < str.length(); i+=chunk_size)
			vecChunks.push_back(str.substr(i, chunk_size));
		return (vecChunks);
	}
	
	std::string					dec_to_hex(int nb)
	{
		std::stringstream ss;
		ss<< std::hex << nb;
		return (ss.str());
	}

	int							hex_to_dec(std::string hex)
	{
		int	ret;

		std::stringstream	stream;
		stream << hex;
    	stream >> std::hex >> ret;
		return (ret);
	}
}
