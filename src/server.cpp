#include "../inc/ft_irc.hpp"

int	main(){
	Server	_IRC(6667);
	try {
		_IRC.init_server();
		_IRC.Server_ON();
	}
	catch (const std::exception& e){
		std::cerr << e.what() << std::endl;
	}
	_IRC.close_server();
	return	0;
}