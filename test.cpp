#include "rbslib/Commandline.h"
#include "rbslib/Network.h"

int main(int argc,char *argv)
{
	RbsLib::Command::CommandExecuter cmd;
	
	RbsLib::Network::HTTP::HTTPServer server(8081);
	server.SetGetHandle([](const RbsLib::Network::TCP::TCPConnection&connnection ,RbsLib::Network::HTTP::RequestHeader) {
		RbsLib::Network::HTTP::ResponseHeader header;
		header.headers["Content-Type"] = "text/html";
		header.headers["Content-Length"] = "5";
		connnection.Send(header.ToBuffer());
		connnection.Send(RbsLib::Buffer("Hello"));
	});
	server.LoopWait(true,10);
	return 0;
}