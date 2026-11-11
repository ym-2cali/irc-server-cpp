#ifndef MESSAGE_HPP
# define MESSAGE_HPP

# include <string>
# include <vector>

// One parsed IRC line: [:prefix] COMMAND [params...] [:trailing]
struct Message {
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;

	static Message	parse(const std::string& line);
};

#endif
