#include "sockets.h"
#include <sstream>
#include <string.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>


using namespace std;
using namespace gdlib;


/**
 * @brief ...sock d_tor
 * @ note virtual functions are not to be used
 * 
 */
network::Sockets::~Sockets()
{
   close();
   if( nullptr != m_pszHostname )
   {
      delete[] m_pszHostname;
       m_pszHostname = nullptr;
   }

}

/**
 * @brief ...set std socket options, see man setsockopt
 * 
 * @param a_fd ...
 * @param a_level ...prtocol number
 * @param a_optName ...option name
 * @param a_nValue ...option value
 * @return bool
 */
bool network::Sockets::setOption(const network::socketfd_t a_fd, const int a_level, const int a_optName, const int a_nValue )
{
   if( 0 == setsockopt( a_fd, a_level, a_optName, &a_nValue, sizeof(int) ) )
   {
      return true;
   } else
   {
      return false;
   }
}


/**
 * @brief ...get socket option see man getsocketopt
 * 
 * @param a_fd ...
 * @param a_level ...
 * @param a_optName ...
 * @param a_nValue ...
 * @param a_len ...
 * @return int
 */
int network::Sockets::getOption(const network::socketfd_t a_fd, const int a_level, const int a_optName, int& a_nValue, socklen_t& a_len )
{
   if( 0 == getsockopt( a_fd, a_level, a_optName, &a_nValue, &a_len ) )
   {
      return true;
   } else
   {
      return false;
   }
}


/**
 * @brief ...set fd to not blocking
 * 
 * @param a_fd ...descriptor
 * @return bool
 */
bool network::Sockets::makeNonBlocking( socketfd_t& a_fd )
{
   int32_t nFlags = fcntl( a_fd, F_GETFL, 0 );
   if( -1 == nFlags )
   {
      return false;
   }
   nFlags |= O_NONBLOCK;
   nFlags = fcntl( a_fd, F_SETFL, nFlags );
   return 0 == nFlags? true: false;
}

/**
 * @brief ...return ture if descriptor is non-blocking
 * 
 * @param a_fd ...fd
 * @return bool
 */
bool network::Sockets::isNonBlocking(network::socketfd_t& a_fd)
{
   int val;
   if( (val= fcntl( a_fd, F_GETFL, 0 )) >= 0 )
   {
      //bIsblk = !(val & O_NONBLOCK);
      return (val & O_NONBLOCK);
   }
   return false;
}


/**
 * @brief ...no nageling so packets are sent right away inproving larency
 * @details see https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_MRG/1.2/html/Realtime_Tuning_Guide/sect-Realtime_Tuning_Guide-Application_Tuning_and_Deployment-TCP_NODELAY_and_Small_Buffer_Writes.html
 * For this to be used effectively, applications must avoid doing small, logically related buffer writes. Because TCP_NODELAY is enabled, these small writes will make TCP send these multiple buffers as individual packets, which can result in poor overall performance.
 * I f applications have several buffers that are logically related and that should be sent as one packet it could be po*ssible to build a contiguous packet in memory and then send the logical packet to TCP, on a socket configured with TCP_NODELAY.
 * Alternatively, create an I/O vector and pass it to the kernel using writev on a socket configured with TCP_NODELAY. 
 * 
 * @return bool
 */
bool network::Sockets::setNoDelay()
{
   int nOptValue = 1;
   if( 0 == setsockopt( m_fdSocket, IPPROTO_TCP, TCP_NODELAY, &nOptValue, sizeof(int) ) )
   {
      return true;
   } else
   {
      return false;
   }
}


/**
 * @brief ...get a local socket (internal)
 * CLIENT: SOCK_STREAM, AF_INET, AI_NUMERICSERV
 * SERVER: SOCK_STREAM, AF_INET, AI_PASSIVE | AI_NUMERICSERV
 * 
 * @param a_nType ...we are using SOCK_STREAM
 * @param a_nFamily ...We are using AF_INET
 * @param a_nFlags ...See above for client and server
 * @return bool   retrun true if ok
 */
bool network::Sockets::setLocalSocketProperties( const int32_t a_nFlags, const int32_t a_nType, const int32_t a_nFamily )
{
   m_nSocketType     = a_nType;
   m_nSocketFamily   = a_nFamily;
   m_nSocketFlags    = a_nFlags;
   
   return true;
}




/**
 * @brief open a server or a client
 *    server: open local port , bind and listen
 *    client open  local port and connect to foreign port
 * 
 * @param a_nType ...CLIENT or SERVER
 * @param a_strHostname ...hostname or IP
 * @param a_strPort ...port as a string, ex "5000"
 * @param a_nProtocol ...TCP, only option
 * @return bool
 */
bool network::Sockets::open( const sockType_t a_nType, const protocol_t a_nProtocol, const string a_strHostname, const string a_strPort )
{
   if( a_strPort.length() > PORT_DIGIT_COUNT_INT32 )
   {
      // error port string more than 2^16 - 1
      return false;
   }
   strcpy( m_szPort, a_strPort.c_str() );
   m_protocol  = a_nProtocol;
   m_pszHostname = new char[a_strHostname.length() + 1];
   if( nullptr == m_pszHostname )
   {
      // log error
      return false;
   }
   strcpy( m_pszHostname, a_strHostname.c_str() );
   m_type = a_nType;
   
   
   struct addrinfo   hints;
   struct addrinfo  *pActualAddress = nullptr;
   memset( &hints, 0, sizeof( struct addrinfo ) );
   hints.ai_canonname   = nullptr;
   hints.ai_addr        = nullptr;
   hints.ai_next        = nullptr;
   hints.ai_socktype    = m_nSocketType;
   hints.ai_family      = m_nSocketFamily;

   // set for client or server -> listener or active connector
   switch( a_nType )
   {
      case sockType_t::CLIENT:
         hints.ai_flags = AI_NUMERICSERV;
         break;
         
      case sockType_t::SERVER:
         hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
         break;
         
      default:
         return false;
      
   }

   if( getaddrinfo( m_pszHostname, m_szPort, &hints, &pActualAddress ) != 0 )
   {
      // log error
      return false;
   }
   
   struct addrinfo  *pLocalAddess;
   int32_t fdLocalSock;
   // find a local socket to use
   int nOptValue = 1;
   for( pLocalAddess=pActualAddress; pLocalAddess != nullptr; pLocalAddess = pLocalAddess->ai_next )
   {
      fdLocalSock = ::socket( pLocalAddess->ai_family, pLocalAddess->ai_socktype, pLocalAddess->ai_protocol );
      if( -1 == fdLocalSock )
      {
         continue;
      }

      switch( a_nType )
      {
         case sockType_t::CLIENT:
            if( ::connect( fdLocalSock, pLocalAddess->ai_addr, pLocalAddess->ai_addrlen ) == 0 )
            {
               // we have conection to server
               m_fdSocket = fdLocalSock;
               freeaddrinfo( pActualAddress );
               pActualAddress = nullptr;
               return true;
            } else
            {
               std::cerr << "connect falilure:" <<  strerror( errno ) << std::endl;
            }
            break;

         case sockType_t::SERVER:
            setsockopt( fdLocalSock, SOL_SOCKET, SO_REUSEADDR, &nOptValue, sizeof(int) );
            if( ::bind( fdLocalSock, pLocalAddess->ai_addr, pLocalAddess->ai_addrlen ) == 0 )
            {
               // we have an active listener
               m_fdSocket = fdLocalSock;
               freeaddrinfo( pActualAddress );
               pActualAddress = nullptr;
               listen( m_fdSocket, m_nBacklog );
               return true;
            } else
            {
               std::cerr << "bind falilure:" <<  strerror( errno ) << std::endl;
            }
            break;
     
         default:  
            break;
      }
      // bind or connect failed, try next address
      ::close( fdLocalSock );
   }
   
   freeaddrinfo( pActualAddress );
   return false;
}



/**
 * @brief ...non-blocking fd: read from fd, will return bytes count read and block if no bytes avail 
 *  nonblocking
 *   level trigger = read. on data ready from epoll, call once then call epoll, if additional calls, it will return data until fd is empty, then 0
 *   edge trigger - call read getting data until 0 is returned, then process and call epoll
 * 
 *  For level trigger it is best to call receive once and then epol while edge, you must call receive untill receive returns 0 else you will loose data
 * 
 * @param a_pBuffer ...buffer to hold data
 * @param a_nBufferSize ...size of your buffer (note this is no a null term string)
 * @return ssize_t - bytes read, error -1
 *       nonblocking edge - return 0 when no bytes left on fd
 *       nonblocking level- return 0 when no bytes, better not to call repeatedly but to level epoll signal again
 */
ssize_t network::Sockets::receive( const socketfd_t& a_fd, void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   if( (nullptr == a_pBuffer) || (a_nBufferSize <= 0) )
   {
      return false;
   }
   
   
   ssize_t nBytesRead( 0 );
   ssize_t nBytesReadPerCall( 0 );
   uint8_t* pBuffer = reinterpret_cast<uint8_t*>( a_pBuffer );  // uchar
   while( nBytesRead < a_nBufferSize )
   {
      nBytesReadPerCall = ::read( a_fd, pBuffer, static_cast<size_t>(a_nBufferSize-nBytesRead) );
      switch( nBytesReadPerCall )
      {
         case -1:
            switch( errno )
            {
               case EINTR:
                  continue;
                  
               case EAGAIN:
                  return nBytesRead;
                  
               default:
                  return -1;
            }
            
          case 0:
              // reached EOF - done
              return nBytesRead;

          default:
              nBytesRead += nBytesReadPerCall;
              pBuffer = pBuffer + sizeof(uint8_t)* static_cast<decltype(sizeof(uint8_t))>( nBytesReadPerCall );
      }
   }
   
   //  buffer too small
   return nBytesRead;
}



/**
 * @brief ...read using s blocking fd
 * 
 * @param a_fd ...
 * @param a_pBuffer ...buffer to hold data
 * @param a_nBufferSize ...size of your buffer (note this is no a null term string)
 * @return ssize_t
 */
ssize_t network::Sockets::receive_blocking( const socketfd_t& a_fd, void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   if( (nullptr == a_pBuffer) || (a_nBufferSize <= 0) )
   {
      return false;
   }
   uint8_t* pBuffer = reinterpret_cast<uint8_t*>( a_pBuffer );  // uchar
   return ::read( a_fd, pBuffer, static_cast<size_t>( a_nBufferSize ) );
   
}






/**
 * @brief ...write to socket until buffer send empty, can be used with blocking or non blocking
 * with blocking the loop will only execute once
 * 
 * @param a_fs ...
 * @param a_pBuffer ...
 * @param a_nBufferSize ...
 * @return ssize_t
 * 0 < n <= a_nBufferSize     write  sucessfull, bytes written
 * -1                         write failed
 */
ssize_t network::Sockets::send( const socketfd_t& a_fd, const void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   if( (nullptr == a_pBuffer) || (a_nBufferSize <= 0) )
   {
      return -1;
   }
   
   ssize_t nBytesWrittenPerCall( 0 );
   ssize_t nBytesWritten( 0 );
   const uint8_t* pBufferPos = reinterpret_cast<const uint8_t*>( a_pBuffer );
   
   while( nBytesWritten < a_nBufferSize )
   {
      nBytesWrittenPerCall = ::write( a_fd, pBufferPos, static_cast<size_t>( a_nBufferSize-nBytesWritten ) );
      switch( nBytesWrittenPerCall )
      {
         case -1:
            switch( errno )
            {
               case EAGAIN:
               case EINTR:
                  continue;
                  
               default:
                  return -1;
            }
            
         default:
            nBytesWritten += nBytesWrittenPerCall;
            pBufferPos = pBufferPos + sizeof( uint8_t )* static_cast< decltype(sizeof(uint8_t) )>(nBytesWrittenPerCall);
      }
   }
   return nBytesWritten;
}


/**
 * @brief ...close the socket
 * 
 * @return bool
 */
bool network::Sockets::close()
{
   if( m_fdSocket > 0 )
   {
      ::close( m_fdSocket );
      m_fdSocket = 0;
   }
   return false;
}



// blocking client
//

/**
 * @brief ...connect to host a=on a port
 * 
 * @param a_strHostname ...host to connect to on port
 * @param a_strPort ...port
 * @param a_proto ... TCP
 * @return bool
 */
bool network::Client::connect( const string& a_strHostname, string a_strPort, protocol_t a_proto )
{
   return open( sockType_t::CLIENT,  a_proto, a_strHostname, a_strPort );
}

ssize_t network::Client::send( const void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   return network::Sockets::send( m_fdSocket, a_pBuffer, a_nBufferSize );
}


/**
 * @brief ...blocking receive
 * 
 * @param a_pBuffer ...
 * @param a_nBufferSize ...
 * @return ssize_t
 */
ssize_t network::Client::receive( void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   return network::Sockets::receive_blocking( m_fdSocket, a_pBuffer, a_nBufferSize );
}



// non-blocking client
//


/**
 * @brief ...receive non-blockeding reply
 * 
 * @param a_pBuffer ...
 * @param a_nBufferSize ...
 * @return ssize_t
 */
ssize_t network::ClientAsync::receive( void* a_pBuffer, const ssize_t& a_nBufferSize )
{
   return network::Sockets::receive( m_fdSocket, a_pBuffer, a_nBufferSize );
}




/**
 * @brief ...start async receiver for replies.  starts a thread with epolling to callback on events and errors
 * @example see testing/async/client.cpp
 * 
 * @param a_cbMessage ...call back on connect HUP, and data ready
 * @param a_pData ...void pointer that will be returned in call back
 * @param a_cbError ...call back on error, fn( errno, null terminaled string, a_pData )
 * @param a_bEdgeTrigger ...level or edge trigger (level default)
 *      edge  -> triggered once when data is present, must read all data on this event
 *      level -> triggeered whenever data is present, can  read some or all data per even
 * @return bool
 */
bool network::ClientAsync::startAsync( const network::socketCallback_t a_cbMessage, void* const a_pData, const network::errorCallBack_t a_cbError, const bool a_bEdgeTrigger )
{
   m_bEdgeTriggered = a_bEdgeTrigger;
   thread thd( &ClientAsync::startAsync_, this, a_cbMessage, a_cbError, a_pData );
   m_thdReceiver = std::move( thd );
   return true;
}


/**
 * @brief ...async private worker
 * 
 * @param a_onSocketEvent ...
 * @param a_error ...
 * @param a_pThis ...
 * @return bool
 */
bool network::ClientAsync::startAsync_( const network::socketCallback_t a_onSocketEvent, const errorCallBack_t a_error, void* const a_pThis )
{
   {
      lock_guard<std::mutex> lock( m_muxReady );   // used for conditional to signal async is ready
   }
   

   if( nullptr == a_onSocketEvent )
   {
      // error
      return false;
   }
   
   // alloc on stack events for all connections
   if( true == m_bUseMalloc )
   {
      m_pEvents = reinterpret_cast<epoll_event*>( malloc( static_cast<  decltype(sizeof(epoll_event))  >(m_nMaximumEpollEvents)*sizeof(epoll_event) ) );
   } else
   {
      m_pEvents = reinterpret_cast<epoll_event*>( alloca( static_cast<  decltype(sizeof(epoll_event))  >(m_nMaximumEpollEvents)*sizeof(epoll_event) ) );
   }
   if( nullptr == m_pEvents )
   {
      // try malloc if stack alloc fails, but keep track of who did the alloc, for now. fail
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), a_pThis );
      }
      return false;
   }
   
   m_fdEpoll = epoll_create1( 0 );
   if( -1 == m_fdEpoll )
   {
      // log error
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), a_pThis );
      }
      return false;
   }

   epoll_event epEventMainSocket;
   epEventMainSocket.data.fd = m_fdSocket;
   epEventMainSocket.events = EPOLLIN | EPOLLRDHUP;  // this is a socket from a connection, so listen for HUP
   if( true == m_bEdgeTriggered )
   {
      epEventMainSocket.events |= EPOLLET;
   }
   makeNonBlocking( m_fdSocket );

   

   if( -1 == epoll_ctl( m_fdEpoll, EPOLL_CTL_ADD, m_fdSocket, &epEventMainSocket ) )
   {
      // log error
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), a_pThis );  // investigate if we needno use strerror_r
      }
      return false;
   }

   int32_t fdCount;
   int64_t lIndex;
   
   // listener side ready
   bool bNotified = false;
   
   
   while( m_bAsyncRunFlag )
   {
      fdCount = epoll_wait( m_fdEpoll, m_pEvents, m_nMaximumEpollEvents, m_nEpollTimeout_ms );
      if( false == bNotified )
      {
         // notify that connection is ready
         m_cvReady.notify_one();
         bNotified = true;
         //std::cout<<pthread_self()<<"notified"<<std::endl;
      }
      
      switch( fdCount )
      {
         case 0:
            // epoll timeout
            continue;
         case -1:
            if( EINTR == errno )
            {
               continue;
            } else
            {
               if( nullptr != a_error )
               {
                  string str( "epoll error: " );
                  str.append( strerror( errno ) );
                  a_error( errno, str.c_str(), nullptr );
                  m_bAsyncRunFlag = false;
                  continue;
               }
            }
            continue;
            
         default:
            int32_t fd;
            for( lIndex=0; lIndex<fdCount; ++lIndex )
            {
               fd = m_pEvents[lIndex].data.fd;
               
               if( (m_pEvents[lIndex].events & EPOLLERR) || (m_pEvents[lIndex].events & EPOLLRDHUP) )
               {
                  // HUP: here
                  std::cerr << "hup (server dropped connection):" << strerror( errno ) << std::endl;
                  // handle connection closed by either hangup or network error
                  //m_bAsyncRunFlag = false;
                  ::close( fd );  
                  if( nullptr != a_onSocketEvent )
                  {
                     a_onSocketEvent( fd, network::callBack_t::SESSION_CLOSE, a_pThis );
                     //m_bAsyncRunFlag = false;
                  }
               } 
               if( m_pEvents[lIndex].events & EPOLLIN )
               {
                  // data is ready on a fd
                  if( nullptr != a_onSocketEvent )
                  {
                     a_onSocketEvent( fd, network::callBack_t::MESSAGE, a_pThis );
                  } else
                  {
                     a_error( 0, "no callback event:", nullptr );
                  }
               } else
               {
                  if( nullptr != a_error )
                  {
                     a_error( 0, "unhandled socket event", nullptr );
                  }
               }
            }
      }
   }

   ::close( m_fdSocket );
   ::close( m_fdEpoll );
   //sem_post( &m_semReconnect );
   //cout << " end sock startAsync" << endl;
   return true;
}



/**
 * @brief ...see TWPriceFeed for details
 * 
 * @param a_nRetryCount ...
 * @param a_nRetryWait ...
 * @param a_cbLog ...
 * @return bool
 */
bool network::ClientAsync::reconnect( const int32_t a_nRetryCount, const int32_t a_nRetryWait, logCallBack_t a_cbLog )
{
   std::cout << "m_thdReceiver:" << m_thdReceiver.native_handle() << std::endl;
   cout << "waiting for Client::startAsync to end" << endl;
   cout << "Client::startAsync ended" << endl;
   
   
   cout << "network::Client::reconnect" << endl;
   if( (a_nRetryCount == 0) || (a_nRetryWait == 0) )
   {
      if( nullptr != a_cbLog )
      {
         stringstream sstr;
         sstr << "retry disabled at least one retry var is zero. MddReconnectRetryCount:" << a_nRetryCount << ", MddReconnectWait:" << a_nRetryWait;
         a_cbLog( network::LogLevel::EERR, sstr.str().c_str() );
      }
      return false;
   }
   int32_t nRetryCount = a_nRetryCount;
   
   
   while( nRetryCount-- > 0 )
   {
      if( false == m_bAsyncRunFlag )
      {
         if( nullptr != a_cbLog )
         {
            a_cbLog( network::LogLevel::EINF, "retry, stop issued:" );
         }
         cout << "stop issued" << endl;
         break;
      }
      usleep( static_cast<__useconds_t>(a_nRetryWait)* 1000000 );
      
      if( true == open( sockType_t::CLIENT, m_protocol, m_pszHostname, m_szPort ) )  // dev-idb27.nyoffice.tradeweb.com.
      {
         if( nullptr != a_cbLog )
         {
            a_cbLog( network::LogLevel::EINF, "connection suceeded" );
         }
         
         epoll_event epEventMainSocket;
         epEventMainSocket.data.fd = m_fdSocket;
         epEventMainSocket.events = EPOLLIN | EPOLLRDHUP;  // this is a socket from a connection, so listen for HUP
         if( true == m_bEdgeTriggered )
         {
            epEventMainSocket.events |= EPOLLET;
         }
         makeNonBlocking( m_fdSocket );
         if( -1 == epoll_ctl( m_fdEpoll, EPOLL_CTL_ADD, m_fdSocket, &epEventMainSocket ) )
         {
            if( nullptr != a_cbLog )
            {
               stringstream sstr;
               sstr << "failed to add connection to poll,  no data will be received";
               a_cbLog( network::LogLevel::EERR, sstr.str().c_str() );
            }
            return false;
         }
         
         
         return true;
      } else
      {
         if( nullptr != a_cbLog )
         {
            a_cbLog( network::LogLevel::EWRN, "connection failed" );
         }
         cout <<  "retry failed" << endl;
      }
      
   }
   if( nullptr != a_cbLog )
   {
      stringstream sstr;   
      a_cbLog( network::LogLevel::EERR, "mdd reconnect failed" );
   }
   cout << "mdd reconnect failed nRetryCount:" << a_nRetryCount << ", nRetryWait:" << a_nRetryWait << endl;
   return false;
}





// blocking server
//


/**
 * @brief ...
 * 
 */
network::Server::~Server()
{
   network::Sockets::close();
}



/**
 * @brief ...return descriptor when there is a connections
 * 
 * @return falcon::core::network::socketfd_t
 */
network::socketfd_t network::Server::waitForConnection( )
{
   if( sockType_t::SERVER != m_type )
   {
      return -1;
   }
   return accept( m_fdSocket, nullptr, nullptr );
}



// non-blocking server
//

/**
 * @brief ...
 * 
 */
network::ServerAsync::~ServerAsync()
{
   network::Sockets::close();
   if( (true == m_bUseMalloc) && (m_pEvents != nullptr ) )
   {
      free( m_pEvents );
      m_pEvents = nullptr;
   }
}




/**
 * @brief ...listens and calls back on data ready connection or HUP.
 * 
 * edge trigger:  you must read data from socket while receive returns > 0, data available
 * level trigger: you must make one receive call per callback and ten return
 * 
 * @param a_socketEvent ...callback on good socket events (conection, HUP and data ready
 * @param a_bEdgeTrigger ...true if edge trigger, you must consume all data on event, else level and make mult calls
 * @param a_error ...error callback handler
 * @param a_pData ...pointer to pass back to callbacks
 *
 *      edge  -> triggered once when data is present, must read all data on this event
 *      level -> triggeered whenever data is present, can  read some or all data per even
 *
 * 
 * 
 * @return bool
 */
bool network::ServerAsync::nonblockingListener( const socketCallback_t a_socketEvent, const bool a_bEdgeTrigger, const errorCallBack_t a_error, void *a_pData )
{
   if( nullptr == a_socketEvent )
   {
      return false;
   }
   if( sockType_t::SERVER != m_type )
   {
      return false;
   }
   if( nullptr == a_error )
   {
      cerr << "Error handler not set" << endl;
   }

   // alloc on stack events for all connections
   if( true == m_bUseMalloc )
   {
      m_pEvents = reinterpret_cast<epoll_event*>( malloc( static_cast<  decltype(sizeof(epoll_event))  >(m_nMaximumEpollEvents)*sizeof(epoll_event) ) );
   } else
   {
      m_pEvents = reinterpret_cast<epoll_event*>( alloca( static_cast<  decltype(sizeof(epoll_event))  >(m_nMaximumEpollEvents)*sizeof(epoll_event) ) );
   }
   for( int32_t nIndex=0; nIndex<m_nMaximumEpollEvents; nIndex++ )
   {
      (m_pEvents+nIndex)->data.fd = 0;
   }
   if( nullptr == m_pEvents )
   {
      // try malloc if stack alloc fails, but keep track of who did the alloc, for now. fail
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), nullptr );
      }
      return false;
   }

   m_fdEpoll = epoll_create1( 0 );
   if( -1 == m_fdEpoll )
   {
      // log error
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), nullptr );
      }
      return false;
   }
   
   epoll_event epEventMainSocket;
   epEventMainSocket.data.fd = m_fdSocket;
   epEventMainSocket.events = EPOLLIN;
   // in above case we are watching READ events for fd
   // here is snippet
   // available events
   //        EPOLLIN
   //               The associated file is available for read(2) operations.
   // 
   //        EPOLLOUT
   //               The associated file is available for write(2) operations.
   // 
   //        EPOLLRDHUP (since Linux 2.6.17)
   //               Stream socket peer closed connection, or shut down writing half of  connection.
   //               (This flag is especially useful for writing simple code to detect peer shutdown
   //               when using Edge Triggered monitoring.)
   // 
   //        EPOLLPRI
   //               There is urgent data available for read(2) operations.
   // 
   //        EPOLLERR
   //               Error condition happened on the associated file descriptor.  epoll_wait(2) will
   //               always wait for this event; it is not necessary to set it in events.
   // 
   //        EPOLLHUP
   //               Hang  up happened on the associated file descriptor.  epoll_wait(2) will always
   //               wait for this event; it is not necessary to set it in events.
   // 
   //        EPOLLET
   //               Sets the Edge Triggered behavior  for  the  associated  file  descriptor.   The
   //               default  behavior for epoll is Level Triggered.  See epoll(7) for more detailed
   //               information about Edge and Level Triggered event distribution architectures.
   // 
   //        EPOLLONESHOT (since Linux 2.6.2)
   //               Sets the one-shot behavior for the associated file descriptor.  This means that
   //               after  an event is pulled out with epoll_wait(2) the associated file descriptor
   //               is internally disabled and no other events will be reported by the epoll inter-
   //               face.   The  user  must  call epoll_ctl() with EPOLL_CTL_MOD to re-arm the file
   //               descriptor with a new event mask.
   
   // add listener socket to events watched by epoll
   if( -1 == epoll_ctl( m_fdEpoll, EPOLL_CTL_ADD, m_fdSocket, &epEventMainSocket ) )
   {
      // log error
      if( nullptr != a_error )
      {
         a_error( errno, strerror( errno ), nullptr );  // investigate if we needno use strerror_r
      }
      return false;
   }
   
   // connection vars
   struct sockaddr remoteAddress;
   socklen_t       remoteAddressLength = sizeof( remoteAddress );
   
   
   ::listen( m_fdSocket, m_nBacklog );  
   int32_t fdCount;
   int64_t lIndex;
   while( m_bAsyncRunFlag )
   {
      fdCount = epoll_wait( m_fdEpoll, m_pEvents, m_nMaximumEpollEvents, m_nEpollTimeout_ms );
      
      switch( fdCount )
      {
         //cerr << "fd:" << fdCount << endl;
         case 0:
            
            continue;
         case -1:
            // epoll error
            //       EBADF  epfd is not a valid file descriptor.
            // 
            //       EFAULT The memory area pointed to by events is not accessible with write  permissions.
            // 
            //       EINTR  The call was interrupted by a signal handler before any of the requested events
            //               occurred or the timeout expired; see signal(7).
            // 
            //       EINVAL epfd is not an epoll file descriptor, or maxevents is less  than  or  equal  to
            //               zero.
            
            if( EINTR == errno )
            {
               continue;
            } else
            {
               if( nullptr != a_error )
               {
                  string str( "epoll error: " );
                  str.append( strerror( errno ) );
                  a_error( errno, str.c_str(), nullptr );
                  m_bAsyncRunFlag = false;
                  continue;
               }
            }
            continue;
            
         default:
            // process all descripters
            
            int32_t fd;
            for( lIndex=0; lIndex<fdCount; ++lIndex )
            {
               fd = m_pEvents[lIndex].data.fd;
               
               // socket error or close
               // ---------------------------
               if( (m_pEvents[lIndex].events & EPOLLERR) || (m_pEvents[lIndex].events & EPOLLRDHUP) )
               {
                  // handle connection closed by either hangup or network error
                  
                  if( nullptr != a_socketEvent )
                  {
                     a_socketEvent( fd, network::callBack_t::SESSION_CLOSE, a_pData );
                  }
                  ::close( fd );
                  //::shutdown( fd, SHUT_RDWR );
                  m_pEvents[lIndex].data.fd = 0;
                  
               // new connection
               // ---------------------------
               } else if( m_fdSocket == fd )
               {
                  // if a valid descriptor, then we have a new connection
                  //makeNonBlocking( fd );
                  socketfd_t fdRemote = accept( m_fdSocket, dynamic_cast<struct sockaddr*>( &remoteAddress ), &remoteAddressLength );
                  if( -1 == fdRemote )
                  {
                     // log failue
                     if( nullptr != a_error )
                     {
                        a_error( errno, strerror( errno ), nullptr );  // investigate if we needno use strerror_r
                     }
                     continue;
                  } else
                  {                     
                     makeNonBlocking( fdRemote );
                     epoll_event epEventNewConnection;
                     
                     epEventNewConnection.events = EPOLLIN | EPOLLRDHUP;
                     if( true == a_bEdgeTrigger )
                     {
                        epEventNewConnection.events |= EPOLLET;
                     }
                     epEventNewConnection.data.fd = fdRemote;  //!! not sure if this is neeeded
                     if( -1 == epoll_ctl( m_fdEpoll, EPOLL_CTL_ADD, fdRemote, &epEventNewConnection ) )
                     {
                        // may have to check for EAGAIN
                        string str( "error adding descriper to epoll" );
                        str.append( strerror( errno ) );
                        a_error( errno, str.c_str(), nullptr );
                        continue;
                     }
                     if( nullptr != a_socketEvent )
                     {
                        a_socketEvent( fdRemote, network::callBack_t::SESION_OPEN, a_pData );
                     }
                     // get the host name that connected
                  }
               } else
                  
               // data ready
               // ---------------------------
               if( m_pEvents[lIndex].events & EPOLLIN )
               {
                  if( nullptr != a_socketEvent )
                  {
                     a_socketEvent( fd, network::callBack_t::MESSAGE, a_pData );
                  } else
                  {
                     a_error( 0, "no socket callback set to read data", nullptr );
                  }
               }                   
               else 
               {
                  string str( "unhandled socket event:" );
                  str.append( std::to_string( static_cast<int64_t>( fd ) ) );  // gcc 4.4 does not have an overload for int
                  a_error( 0, str.c_str(), nullptr );
               }
            }
      }
      
   }
   for( int32_t nIndex=0; nIndex<m_nMaximumEpollEvents; nIndex++ )
   {
      if( (m_pEvents+nIndex)->data.fd > 0 )
      {
         if( -1 == ::shutdown( (m_pEvents+nIndex)->data.fd, SHUT_RDWR ) )
         {
            cout << "shutdown fail fd:" << (m_pEvents+nIndex)->data.fd << endl;
         } else
         {
            cout << "shutdown ok fd:" << (m_pEvents+nIndex)->data.fd << endl;
         }
      }
   }
   close();   // close listener
   ::close( m_fdEpoll );
   if( true == m_bUseMalloc )
   {
      free( m_pEvents );
      m_pEvents = nullptr;
   }
   
   
   return true;
}



