#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

class Client {
	public:
		Client(int fd, const std::string& host);
		~Client();

		int			fd;
		std::string	host;
		std::string	nick;
		std::string	user;
		std::string	realname;
		bool		pass_ok;
		bool		registered;
		bool		closing;
		std::string	inbuf;
		std::string	outbuf;

		// nick!user@host, used as the source of messages relayed to others
		std::string	prefix() const;
		// nick, or "*" before one is set (used as reply target)
		std::string	name() const;
		// queue a line for sending; CRLF is appended
		void		send(const std::string& line);

	private:
		Client(const Client& other);
		Client& operator=(const Client& other);
};

#endif
