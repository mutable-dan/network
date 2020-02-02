#include "sockets.h"
#include <iostream>
#include <string>
#include "string.h"

using namespace std;
using namespace gdlib;

 int main( int, char** )
 {
 	network::Server server;
 	string strHost = "localhost";
    string strPort = "4000";
 	
 	server.setLocalSocketProperties( network::Sockets::getDefaultServerSocketFlags() );
    if( server.open( network::sockType_t::SERVER, network::protocol_t::TCP, strHost, strPort ) )
 	{
 		const ssize_t nBufSize = 1024; 
 		char   pszBuf[nBufSize];

 		network::socketfd_t fd = server.waitForConnection();
 		if( fd > 0 )
 		{
 			cout << "connection" << endl;
 			auto res = server.receive( fd, pszBuf,  nBufSize );
 			cout << "rec:" << res << ":" << pszBuf << endl;

 			cout << "send:done" << endl;
 			res = server.send( fd, "done", 5 );
 		} else
 		{
 			cout << "error" << endl;
 		}

 	}


 	return 0;
 }
