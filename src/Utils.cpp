#include "Utils.hpp"
#include <cctype>
#include <sstream>

std::string	to_lower(const std::string& s) {
	std::string	out(s);
	for (std::size_t i = 0; i < out.size(); i++)
		out[i] = std::tolower(static_cast<unsigned char>(out[i]));
	return out;
}

std::string	to_upper(const std::string& s) {
	std::string	out(s);
	for (std::size_t i = 0; i < out.size(); i++)
		out[i] = std::toupper(static_cast<unsigned char>(out[i]));
	return out;
}

std::vector<std::string>	split(const std::string& s, char sep) {
	std::vector<std::string>	out;
	std::size_t					start = 0;
	std::size_t					end;

	while ((end = s.find(sep, start)) != std::string::npos) {
		if (end > start)
			out.push_back(s.substr(start, end - start));
		start = end + 1;
	}
	if (start < s.size())
		out.push_back(s.substr(start));
	return out;
}

std::string	itos(long n) {
	std::ostringstream	ss;
	ss << n;
	return ss.str();
}

bool	is_channel_name(const std::string& s) {
	return !s.empty() && (s[0] == '#' || s[0] == '&');
}

bool	valid_channel_name(const std::string& s) {
	if (s.size() < 2 || s.size() > 50 || !is_channel_name(s))
		return false;
	for (std::size_t i = 1; i < s.size(); i++)
		if (s[i] == ' ' || s[i] == ',' || s[i] == ':' || s[i] == '\a')
			return false;
	return true;
}

static bool	is_special(char c) {
	return std::string("[]\\`_^{|}").find(c) != std::string::npos;
}

// RFC 2812: nickname = ( letter / special ) *8( letter / digit / special / "-" )
bool	valid_nick(const std::string& s) {
	if (s.empty() || s.size() > 9)
		return false;
	if (!std::isalpha(static_cast<unsigned char>(s[0])) && !is_special(s[0]))
		return false;
	for (std::size_t i = 1; i < s.size(); i++) {
		char	c = s[i];
		if (!std::isalnum(static_cast<unsigned char>(c)) && !is_special(c) && c != '-')
			return false;
	}
	return true;
}
