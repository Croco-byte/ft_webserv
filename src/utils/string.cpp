#include "include.hpp"

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
		}
		while (pos < str.length() && prev < str.length());
		return tokens;
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
		std::vector<std::string>	tmp;
		std::string					ret;

		tmp = Utils::split(str, c);
		for (std::vector<std::string>::iterator it = tmp.begin(); it != tmp.end(); it++)
			ret.append(*it);
		return (ret);
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
}