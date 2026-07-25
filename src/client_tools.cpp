#include "../inc/ft_irc.hpp"

Client::Client(int fd):_fd(fd){
	this->registred = false;
	this->nickname = "";
	this->username = "";
	this->password = "";
}


void	Client::set_cmpnts(std::string& msg){
	std::size_t c_pos = 0;
	std::size_t s_pos = 0;

	if (this->cmpnts.size())
		this->cmpnts.clear();
	while (c_pos < msg.size())
	{
		s_pos = msg.find(' ',  c_pos);
		t_msg_cmpnts	cmpnt;
		if (this->cmpnts.size() && msg[c_pos] && msg[c_pos] == ':')
		{
			cmpnt._data = msg.substr(c_pos, msg.size());
			cmpnt._type = TRAILING;
			break ;
		}
		else
			cmpnt._data = msg.substr(c_pos, s_pos - c_pos);
		this->cmpnts.push_back(cmpnt);
		if (s_pos == std::string::npos)
			break;
		c_pos = s_pos + 1;
	}
	this->tokenizer();
}

bool	is_middle(std::string& param){
	if (param.empty() || param[0] == ':')
		return false;
	for (std::size_t i = 0; i < param.size(); i++)
	{
		if (std::isspace(param[i]) || param[i] == '\r' || param[i] == '\n')
			return false;
	}
	return true;
}

bool	is_trailing(std::string& param){
	for (std::size_t i = 0; i < param.size(); i++)
	{
		if (param[i] == '\r' || param[i] == '\n')
			return false;
	}
	return true;
}

bool	Client::tokenizer(){
	if (!cmpnts.size())
		return false;
	std::size_t i = 0;
	if (this->cmpnts[i]._data[i] == ':')
	{
		this->cmpnts[i]._data = this->cmpnts[i]._data.substr(i + 1, this->cmpnts[i]._data.size());
		this->cmpnts[i++]._type = PREFIX;
	}
	while ( i < this->cmpnts.size())
	{
		if (i == 0 || (i == 1 && this->cmpnts[0]._type == PREFIX))
			this->cmpnts[i]._type = CMD;
		else if (this->cmpnts[i]._type != TRAILING){
			if (is_middle(this->cmpnts[i]._data))
				this->cmpnts[i]._type = MIDDLE;
			else
				this->cmpnts[i]._type = PARAMS;
		}
		i++;
	}
	return true;
}

bool	Client::get_msg(){
	std::size_t  _crlf_pos = this->buf.find("\r\n", 0);
	if (_crlf_pos == std::string::npos || _crlf_pos > 510)
		return false;
	std::string	msg = this->buf.substr(0, _crlf_pos);
	this->buf.erase(0, _crlf_pos + 2);
	this->set_cmpnts(msg);
	return true;
}


