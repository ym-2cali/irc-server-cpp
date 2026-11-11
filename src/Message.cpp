#include "Message.hpp"
#include "Utils.hpp"

Message	Message::parse(const std::string& line) {
	Message		msg;
	std::size_t	len = line.size();
	std::size_t	pos = 0;
	std::size_t	end;

	while (pos < len && line[pos] == ' ')
		pos++;
	if (pos < len && line[pos] == ':') {
		end = line.find(' ', pos);
		if (end == std::string::npos)
			return msg;
		msg.prefix = line.substr(pos + 1, end - pos - 1);
		pos = end;
	}
	while (pos < len && line[pos] == ' ')
		pos++;
	end = line.find(' ', pos);
	if (end == std::string::npos)
		end = len;
	msg.command = to_upper(line.substr(pos, end - pos));
	pos = end;
	while (pos < len) {
		while (pos < len && line[pos] == ' ')
			pos++;
		if (pos >= len)
			break;
		if (line[pos] == ':') {
			msg.params.push_back(line.substr(pos + 1));
			break;
		}
		end = line.find(' ', pos);
		if (end == std::string::npos)
			end = len;
		msg.params.push_back(line.substr(pos, end - pos));
		pos = end;
	}
	return msg;
}
