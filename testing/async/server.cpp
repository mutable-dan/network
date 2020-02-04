#include "sockets.h"
#include <iostream>
#include <string>
#include "string.h"

using namespace std;
using namespace gdlib;

#define MAX_SOCKET_BUFFER 64

void listener_socketCallbackHandler       ( const network::socketfd_t& a_fd, const network::callBack_t& a_type, void* const a_pData = nullptr );
void listener_socketErrorCallbackHandler  ( const int32_t a_nerrno, const char* a_pszError, void* const a_pData );


int main( int, char** )
{
   network::ServerAsync server;
   string strHost = "localhost";
   string strPort = "5200";
   const int32_t nEnventQueueSize = 5;
   const int32_t nMaxEpollEvents  = 100;
   const int32_t nEpollTimeout_ms = 1000;
        
   server.setLocalSocketProperties( network::Sockets::getDefaultServerSocketFlags() );
   server.setListenerBacklog( nEnventQueueSize );

   if( server.open( network::sockType_t::SERVER, network::protocol_t::TCP, strHost, strPort ) )
   {
      server.setMaximumPollEvents( nMaxEpollEvents );
      server.setEpollWaitTimeout( nEpollTimeout_ms );                 
      server.useStackAlloc();
      // server.useHeapAlloc();
      server.setNoDelay();

      if( false == server.nonblockingListener( listener_socketCallbackHandler, false, listener_socketErrorCallbackHandler, reinterpret_cast<void*>(&server) ) )
      {
        std::cout << "error" << std::endl;
      }
   }
   return 0;
}



void listener_socketCallbackHandler( const network::socketfd_t& a_fd, const network::callBack_t& a_type, void* const a_pData )
{
   int32_t nRecSize;
   static char ucSocketBuffer[MAX_SOCKET_BUFFER];

   network::ServerAsync* pServer = reinterpret_cast<network::ServerAsync*>(a_pData);
   
   
   switch( a_type )
   {
      case network::callBack_t::MESSAGE:
         while( (nRecSize = static_cast<int32_t>( network::Sockets::receive( a_fd, ucSocketBuffer, MAX_SOCKET_BUFFER )) ) > 0 )
         {
            // count how many messages.  since each msg is null term, count them
            char* pszCurrentMsg = ucSocketBuffer;
            for( int32_t nIndex=0; nIndex<nRecSize; ++nIndex )
            {
                if( ucSocketBuffer[nIndex] == 0 )
                {
                   auto res = pServer->send( a_fd, pszCurrentMsg, static_cast<ssize_t>(strlen( pszCurrentMsg ) + 1 ));
                   cout << "reply[" << res << "]:" << ucSocketBuffer << endl;
                   pszCurrentMsg = ucSocketBuffer + nIndex + 1;
                }
            }
         }
         break;
         
      case network::callBack_t::SESSION_CLOSE:
         std::cout << "hup:" << a_fd << std::endl;
         break;
         
      case network::callBack_t::SESION_OPEN:
         std::cout << "conn:" << a_fd << std::endl;
         break;
      default:
         std::cout << "unhandled:" << a_fd << endl;
   }
}

void listener_socketErrorCallbackHandler( const int32_t a_nerrno, const char* a_pszError, void* const )
{
   if( nullptr != a_pszError )
   {
      cerr << "socket: " << a_nerrno << ", " << a_pszError << endl;
   } else
   {
      cerr << "unknown error" << endl;
   }
}

