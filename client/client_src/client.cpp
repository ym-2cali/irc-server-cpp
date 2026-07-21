#include "../../inc/ft_irc.hpp"


int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return (std::cerr << "failing opening client socket\n", 1);
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6667);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)))
		return (std::cerr << "failing connecting socket\n", 1);
	while (true)
	{
		std::string s;
		std::getline(std::cin, s);
		ssize_t n  = send(sockfd, s.c_str(), s.length(), 0);
		if (n > 0)
			std::cout  << "msg sent\n";
		else if (n < 0)
			std::cerr << "ERROR SENDING MSG\n";
	}
	return 0;
}