#include "rbslib/HandleManager.h"
#include <iostream>
#include <WS2tcpip.h>
#include <format>
#include <cstring>

#include "rbslib/Network.h"


int main(int argc,char**argv)
{
	// Example usage of UniqueHandle
	if (argc != 2)
	{
		throw std::runtime_error("+client/server");
	}
	int type;
	int port;
	std::string addr;
	if (!std::strcmp(argv[1], "client"))
	{
		type = 1;
		std::cout << "请输入服务器地址和端口号(格式:ip:port):";
		std::string input;
		std::getline(std::cin, input);
		auto pos = input.find(':');
		if (pos == std::string::npos)
		{
			throw std::runtime_error("ip:port");
		}
		addr = input.substr(0, pos);
		port = std::stoi(input.substr(pos + 1));
		if (port < 0 || port > 65535)
		{
			throw std::runtime_error("port");
		}
	}
	else if (!std::strcmp(argv[1], "server"))
	{
		type = 2;
		std::cout << "请输入服务器端口号:";
		std::string input;
		std::getline(std::cin, input);
		port = std::stoi(input);
		if (port < 0 || port > 65535)
		{
			throw std::runtime_error("port");
		}
	}
	else
	{
		throw std::runtime_error("+client/server");
	}
	if (type == 2)
	{
		RbsLib::Network::UDP::UDPServer udpServer(port, addr);
		while (true)
		{
			auto data = udpServer.Recv(1024);
			std::cout << std::format("收到来自{}:{}的数据:{}", data.GetAddress(), data.GetPort(), data.GetBuffer().ToString()) << std::endl;
			RbsLib::Buffer new_buffer = data.GetBuffer();
			for (int i = 0; i < new_buffer.GetLength(); ++i)
			{
				if (std::isupper(new_buffer[i]))
				{
					new_buffer[i] = std::tolower(new_buffer[i]);
				}
				else if (std::islower(new_buffer[i]))
				{
					new_buffer[i] = std::toupper(new_buffer[i]);
				}
			}
			data.GetBuffer() = new_buffer;
			udpServer.Send(data);
		}
	}
	else
	{
		RbsLib::Network::UDP::UDPClient udpClient;
		RbsLib::Buffer buffer("Hello World");
		udpClient.Send(addr, port,buffer);
		auto data = udpClient.Recv(addr, port, 1024);
		std::cout << data.GetBuffer().ToString() << std::endl;
	}
	return 0;
}
