#include "Client.hpp"

Client::Client(int fd, const std::string& host)
	: fd(fd), host(host), pass_ok(false), registered(false), closing(false) {}

Client::~Client() {}

std::string	Client::prefix() const {
	return nick + "!" + (user.empty() ? "*" : user) + "@" + host;
}

std::string	Client::name() const {
	return nick.empty() ? "*" : nick;
}

void	Client::send(const std::string& line) {
	outbuf += line + "\r\n";
}
