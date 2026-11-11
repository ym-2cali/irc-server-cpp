#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

volatile sig_atomic_t	g_running = 1;

static void	handle_signal(int sig) {
	(void)sig;
	g_running = 0;
}

static int	parse_port(const std::string& s) {
	if (s.empty() || s.size() > 5)
		return -1;
	for (std::size_t i = 0; i < s.size(); i++)
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return -1;
	int	port = std::atoi(s.c_str());
	return (port >= 1 && port <= 65535) ? port : -1;
}

int	main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}
	int			port = parse_port(argv[1]);
	std::string	password = argv[2];
	if (port < 0) {
		std::cerr << "Error: invalid port '" << argv[1] << "'" << std::endl;
		return 1;
	}
	if (password.empty() || password.find_first_of(" \r\n") != std::string::npos) {
		std::cerr << "Error: password must be non-empty and contain no spaces" << std::endl;
		return 1;
	}
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGPIPE, SIG_IGN);
	try {
		Server	server(port, password);
		server.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	std::cout << "\nServer stopped." << std::endl;
	return 0;
}
