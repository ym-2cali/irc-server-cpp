#include "../inc/ft_irc.hpp"

static bool	special(char c)
{
	std::string specials = "[]\\^_{|}`";
	for (std::size_t i = 0; i < specials.size(); i++)
	{
		if (c == specials[i])
			return true;
	}
	return (false);
}

bool	valid_username(std::string& user)
{
	if(user.empty())
		return false;
	for(std::size_t i = 0; i < user.size(); i++){
		if (user[i] == ' ' || user[i] == '@' || user[i] == '\r' || user[i] == '\n')
			return false;
	}
	return true;
}

bool	valid_nick(std::string& nick)
{
	std::size_t	i = 0;
	if (nick.empty() || nick.length() > 9 || !special(nick[i]) || !std::isalpha(nick[i]))
		return false;
	while (++i < nick.size())
	{
		if (!std::isalpha(nick[i] && !special(nick[i]) && !std::isdigit(nick[i]) && nick[i] != '-'))
			return false;
	}
	return true;
}

bool	Server::nickname_coll(std::string& nick){
	std::vector<Client>::iterator it = this->clients.begin();
	while (it != this->clients.end()){
		if (!it->nickname.empty() && it->nickname == nick)
			return false;
		it++;
	}
	return true;
}

bool	registration_commands(std::string& cmd, std::size_t size){
	if (size > 4)
		return false;
	return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" );
}

void	Server::cl_registration(Client& client, std::string& cmd)
{
	if (cmd == "PASS"){
		if (client.password.size())
			client.out_buf =  "ERR_ALREADYREGISTRED\r\n";
		else if (client.cmpnts.size() < 2)
			client.out_buf = "ERR_NEEDMOREPARAMS\r\n";
		else if (client.nickname.empty() && client.username.empty())
			client.out_buf = "NICK/USER first\r\n";
		else if (client.cmpnts.begin()->_type != CMD || (client.cmpnts.begin() + 1)->_type != MIDDLE)
			client.out_buf = "ERR_INVALID_COMMAND\r\n";
		else
			client.password = client.cmpnts.back()._data;
	}
	else if (cmd == "NICK")
	{
		if (client.cmpnts.size() < 2 || (client.cmpnts.begin()->_type == PREFIX && client.cmpnts.size() != 3)
			|| client.cmpnts.back()._type != MIDDLE)
				client.out_buf = "ERR_NONICKNAMEGIVEN\r\n" ;
		else{
			bool	used = this->nickname_coll(client.nickname);
			if (!used && !client.nickname.empty())
				client.out_buf = "ERR_NICKNAMEINUSE\r\n";
			else if (!used && client.nickname.empty())
				client.out_buf = "ERR_NICKCOLLISION\r\n";
			if (valid_nick(client.cmpnts.back()._data))
				client.nickname = client.cmpnts.back()._data;
			else
				client.out_buf = "ERR_INVALIDNICK\r\n";
		}
	}
	else if (cmd == "USER")
	{
		if (client.registred)
			client.out_buf = "ERR_ALREADYREGISTRED\r\n";
		else if (client.cmpnts.size() != 2)
			client.out_buf = "ERR_NEEDMOREPARAMS\r\n";
		else if (client.cmpnts.begin()->_type != CMD || client.cmpnts.back()._type != MIDDLE)	
			client.out_buf = "ERR_INVALID_COMMAND\r\n";
		else{
			if (valid_username(client.cmpnts.back()._data))
				client.username = client.cmpnts.back()._data;
			else
				client.out_buf = "ERR_INVALIDUSER\r\n";
		}
	}
	if (!client.nickname.empty() && !client.password.empty() && !client.username.empty() && !client.registred)
	{
		client.out_buf = "RPL_WELCOME\r\n";
		client.registred = true;
	}
}

void	Server::handle_commands()
{// this function which fills the output_buffer which will be send to the client
	for (std::size_t i = 0; i < this->clients.size(); i++)
	{
		if (this->clients[i].cmpnts.size())
		{
			std::vector<t_msg_cmpnts>::iterator	it = this->clients[i].cmpnts.begin();
			if (it->_type == PREFIX)
				it++;
			if (registration_commands(it->_data, it->_data.size()))
				this->cl_registration(this->clients[i], it->_data);
			if (!this->send_msg(this->clients[i]))
				std::cerr << "send() error\n";
		}
	}
}

bool	Server::send_msg(Client& client){
	if (!client.out_buf.empty()){
		ssize_t n = send(client._fd, client.out_buf.c_str(), client.out_buf.size(), 0);
		if (n == -1)
			return false;
	}
	client.out_buf.clear();
	return true;
}