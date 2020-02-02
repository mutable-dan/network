#include "sockets.h"
#include <string>
#include "string.h"

using namespace std;
using namespace gdlib;

 int main( int, char** )
 {
 	network::Client sender;
 	string strHost = "localhost";
    string strPort = "4000";

 	sender.setLocalSocketProperties( network::Sockets::getDefaultClientSocketFlags() );
 	if( sender.connect( strHost, strPort ) )
 	{
 		const int32_t nBufSize=64;
 		char pszBuf[nBufSize];
 		strcpy( pszBuf, "helo" );
        const size_t  nSize  = strlen( pszBuf ) + 1;
 		cout << "send:" << pszBuf << endl;

        auto res = sender.send( reinterpret_cast<void*>( pszBuf ), static_cast<ssize_t>( nSize ) );
 		cout << "sent:" << res << endl;

 		// get reply
 		res = sender.receive( pszBuf, nBufSize );
 		cout << "resp:" << pszBuf << endl;
 	}


 	return 0;
 }
