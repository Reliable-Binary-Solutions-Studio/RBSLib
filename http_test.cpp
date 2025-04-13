#include "rbslib/Network.h"
#include <iostream>
#include <format>
int main()
{
	RbsLib::Network::HTTP::HTTPServer server(8080);
	server.SetGetHandle([](const RbsLib::Network::TCP::TCPConnection& connection, RbsLib::Network::HTTP::RequestHeader&) {
		RbsLib::Network::HTTP::ResponseHeader header;
		std::string content = "<body>ÖÇÄÜ2201ÑîÅôÐù</body>";
		header.headers["Content-Length"] = std::format("{}", content.length());
		header.headers["Content-Type"] = "text/html";
		connection.Send(header.ToBuffer());
		connection.Send(RbsLib::Buffer(content));
		});
	server.LoopWait();
}