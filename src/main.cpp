// #include "../inc/ft_irc.hpp"

// int main(){
// 	int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
// 	if (sockfd < 0)
// 		return (std::cerr << "failing opening socket\n", 1);
// 	int opt = 1;
// 	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// 	// setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
// 	struct sockaddr_in addr;
// 	std::memset(&addr, 0, sizeof(addr));
// 	addr.sin_family = AF_INET;
// 	addr.sin_port = htons(6667);
// 	addr.sin_addr.s_addr = htonl(INADDR_ANY);
// 	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)))
// 		return (std::cerr << "failing binding socket\n", 1);
// 	if (listen(sockfd, SOMAXCONN))
// 		return (std::cerr << "socket could not listen\n", 1);
// 	int i = 0;
// 	std::vector<struct pollfd>	fds;
// 	while(true)
// 	{
// 		int	c_fd = accept(sockfd, NULL, NULL);
// 		if (c_fd > 0)
// 		{
// 			std::cout << "<----------client " << ++i << " connected---------->\n";
// 			struct pollfd __fd;
// 			std::memset(&__fd, 0, sizeof(struct pollfd));
// 			__fd.fd = c_fd;
// 			__fd.events = POLLIN;
// 			fds.push_back(__fd);
// 		}
// 		else
// 		{
// 			std::cout << "<----------LISTENING---------->\n";
// 			int ready = poll(&fds[0], fds.size(), -1);
// 			if (ready > 0)
// 			{
// 				std::vector<struct pollfd>::iterator it = fds.begin();
// 				while (it != fds.end())
// 				{
// 					if (it->revents & POLLIN)
// 					{
// 						char buf[1024];
// 					  	ssize_t n;
// 						while ((n = recv(it->fd, buf, sizeof(buf) - 1, 0)) > 0) {
// 					    	buf[n] = '\0';
// 					      	std::string s(buf, n);
// 					      	std::cout << s << std::endl;
// 					  	}
// 					}
// 					it++;
// 				}
// 			}
// 			else if (ready < 0)
// 			{
// 				std::cerr << "ERROR_poll()";
// 				break ;
// 			}
// 			sleep(1);
// 			continue ;
// 		}
		
// 		// handle clients.
// 		sleep(1);
// 	}
// 	close(sockfd);
// 	return 0;
// }