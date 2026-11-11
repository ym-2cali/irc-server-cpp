#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>

std::string					to_lower(const std::string& s);
std::string					to_upper(const std::string& s);
std::vector<std::string>	split(const std::string& s, char sep);
std::string					itos(long n);

bool	is_channel_name(const std::string& s);
bool	valid_channel_name(const std::string& s);
bool	valid_nick(const std::string& s);

#endif
