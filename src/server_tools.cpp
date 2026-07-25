#include "../inc/ft_irc.hpp"

bool	Server::nickname_coll(std::string& nick){
	std::vector<Client>::iterator it = this->clients.begin();
	while (it != this->clients.end()){
		if (it->nickname == nick)
			return false;
		it++;
	}
	return true;
}

bool	registration_commands(std::string& cmd, std::size_t size){
	if (size > 3)
		return false;
	return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" );
}

void	Server::cl_registration(Client& client, std::string& cmd)
{
	if (cmd == "PASS"){
		if (client.password.size())
			throw	std::runtime_error("ERR_ALREADYREGISTRED");
		else if (client.cmpnts.size() < 2)
			throw	std::runtime_error(" ERR_NEEDMOREPARAMS");
		else if (client.nickname.empty() && client.username.empty())
			throw	std::runtime_error("NICK/USER first");
		else if (client.cmpnts.begin()->_type != CMD || (client.cmpnts.begin() + 1)->_type != MIDDLE)
			throw	std::runtime_error("ERR_INVALID_COMMAND");
		client.password = cmd;
		client.registred = true;
	}
	else if (cmd == "NICK")
	{
		if (client.cmpnts.size() < 2 || (client.cmpnts.begin()->_type == PREFIX && client.cmpnts.size() != 3)
			|| client.cmpnts.end()->_type != MIDDLE)
				throw	std::runtime_error("ERR_NONICKNAMEGIVEN");
		bool	used = this->nickname_coll(client.nickname);
		if (!used && !client.nickname.empty())
			throw	std::runtime_error("ERR_NICKNAMEINUSE");
		else if (!used && client.nickname.empty())
			throw	std::runtime_error("ERR_NICKCOLLISION");
		client.nickname = client.cmpnts.end()->_data;
	}
	else if (cmd == "USER")
	{
		if (client.cmpnts.size() != 2)
			throw std::runtime_error("ERR_NEEDMOREPARAMS");
		if (client.registred)
			throw std::runtime_error("ERR_ALREADYREGISTRED");
		if (client.cmpnts.begin()->_type != CMD || client.cmpnts.end()->_type != MIDDLE)	
			throw std::runtime_error("ERR_INVALID_COMMAND");
		client.username = client.cmpnts.end()->_data;
	}
	if (!client.nickname.empty() && !client.password.empty() && !client.username.empty() && !client.registred)
			client.registred = true;
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
		}
	}
}
