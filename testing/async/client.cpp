#include "sockets.h"
#include <string>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

using namespace std;
using namespace gdlib;

#define MAX_SOCKET_BUFFER  64
static int32_t g_nReplyies = 0;

void dataCallbackHandler  ( const network::socketfd_t& a_fd, const network::callBack_t& a_type, void* const a_pData = nullptr );
void errorCallbackHandler ( const int32_t a_nerrno, const char* a_pszError, void* const a_pData );

int main( int, char** )
{
   network::ClientAsync sender;
   string strHost = "localhost";
   string strPort = "5200";

   sender.setLocalSocketProperties( network::Sockets::getDefaultClientSocketFlags() );

    if( sender.open( network::sockType_t::CLIENT, network::protocol_t::TCP, strHost, strPort ) )
{

   sender.useStackAlloc();
   // sender.useHeapAlloc();
   sender.setNoDelay();

   sender.startAsync( dataCallbackHandler, nullptr, errorCallbackHandler, true );  // this param is param for callbacks void* param
   sender.waitready();


   const int32_t nBufSize=MAX_SOCKET_BUFFER;
   char pszBuf[nBufSize];
   strcpy( pszBuf, "helo" );
   size_t  nSize  = strlen( pszBuf ) + 1;
   auto res = sender.send( static_cast<void*>(pszBuf), static_cast<ssize_t>(nSize*sizeof(char)) );
   cout << "send 1:" << res << endl;
      
   strcat( pszBuf, "." );
   nSize  = strlen( pszBuf ) + 1;
   res = sender.send( static_cast<void*>(pszBuf), static_cast<ssize_t>(nSize*sizeof(char)) );
   cout << "send 2:" << res << endl;
      
   strcat( pszBuf, "." );
   nSize  = strlen( pszBuf ) + 1;
   res = sender.send( static_cast<void*>(pszBuf), static_cast<ssize_t>(nSize*sizeof(char)) );
   cout << "send 3:" << res << endl;

   __sync_synchronize( );
   while( g_nReplyies < 3 )
   {
   sleep( 1 );
   cout << "g_nReplyies:" << g_nReplyies << endl;
   }
   __sync_synchronize( );

   cout << "done" << endl;
   sender.stop();
   sender.join();
   }
   return 0;
}



void dataCallbackHandler( const network::socketfd_t& a_fd, const network::callBack_t& a_type, void* const  )
{
   static char ucSocketBuffer[MAX_SOCKET_BUFFER];

   try
   {
     switch( a_type )
     {
        case network::callBack_t::MESSAGE:
           {
            int32_t nRecSize;
                while( (nRecSize = static_cast<int32_t>(network::Sockets::receive( a_fd, ucSocketBuffer, MAX_SOCKET_BUFFER )) > 0 ) )
              {
               char* pszCurrentMsg = ucSocketBuffer;
                  for( int32_t nIndex=0; nIndex<nRecSize; ++nIndex )
                  {
                     if( ucSocketBuffer[nIndex] == 0 )
                     {
                        cout << "rec[" << nRecSize << "]:" << pszCurrentMsg << endl;
                        pszCurrentMsg = ucSocketBuffer + nIndex + 1;
                        __sync_fetch_and_add( &g_nReplyies, 1 );
                     }
                  }
              }
          }
           break;
           
           case network::callBack_t::SESSION_CLOSE:
              std::cout << "session closed by remote:" << a_fd << std::endl;
              break;
              
           case network::callBack_t::SESION_OPEN:
              std::cout << "session opened:" << a_fd << std::endl;
              break;
              
           default:
              std::cerr << "******* unhandled" << std::endl;
     }  
   } catch( exception& e )
   {
     cerr << e.what() << endl;
   }
}



void errorCallbackHandler( const int32_t a_nerrno, const char* a_pszError, void* const a_pData )
{
   if( (nullptr == a_pszError) || (nullptr == a_pData) )
   {
      cerr << "unknerror" << endl;     
   } else
   {
      cerr << "error:" << a_nerrno << ":" << a_pszError << endl;
      
   }
}

   
   
