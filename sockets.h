#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <limits.h> 
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <iostream>
#include <exception>

namespace gdlib {
namespace network
{
   enum struct protocol_t: int32_t { TCP };
   enum struct sockType_t: int32_t { CLIENT, SERVER, UNSPEC };
   enum struct callBack_t: int32_t { MESSAGE, SESION_OPEN, SESSION_CLOSE, UNDEFINED };
   enum struct LogLevel: int32_t   { EERRALERT, EERR, EWRNALERT, EWRN, EINF, EOK };  // do not include LogFileHandler.h, too much bagage
   
   using socketfd_t = int32_t;
   //typedef void( *socketCallback_t     )( const socketfd_t& a_fd, const callBack_t& a_type, void* const a_pData );
   using socketCallback_t = void( * )( const socketfd_t& a_fd, const callBack_t& a_type, void* const a_pData );
   using errorCallBack_t  = void( * )( const int32_t a_nerrno, const char* a_pszError, void* const a_pData );
   using logCallBack_t    = void( * )( const LogLevel a_nLevel, const char* a_pszError );
   #define PORT_DIGIT_COUNT_INT32 5
   
   /**
    * @brief base class for socket libary.  use the parent classes
    * currenly only handles TCP
    * These libraies should not be derirved, esp as virtual as no virtuals are defined here
    */
   class Sockets
   {
      protected:
         int32_t     m_nPort                    = 0;                     // port to connect to if client or listn on if server
         int32_t     m_fdSocket                 = 0;                     // socket for above port
         int32_t     m_nBufferSize              = 0;                     // socket transmit buffer
         int32_t     m_nBacklog                 = 10;                    // server side size of queue for pending connections
         int32_t     m_nSocketType              = SOCK_STREAM;           // specify stream for std net
         int32_t     m_nSocketFamily            = AF_INET;               // specify inet for IPV4 or IPV6
         int32_t     m_nSocketFlags             = 0;                     // flags to specify server or client
         sockType_t  m_type                     = sockType_t::UNSPEC;    // client -> publisher. server -> subscriber
         protocol_t  m_protocol                 = protocol_t::TCP;       // spec protocol.  TCP, UDP.  Only TCP implimented
         char*       m_pszHostname              = nullptr;               // for client, hostname to connect, for server, localhost or name
         char        m_szPort[PORT_DIGIT_COUNT_INT32+1];                 // port number 1..xFFFF as a string

         
      protected:
         bool makeNonBlocking( socketfd_t& a_fd );
         bool isNonBlocking( socketfd_t& a_fd );
         
      public:
         Sockets( ) = default;
         Sockets( const Sockets& ) = delete;
         ~Sockets();

         Sockets& operator =( const Sockets& ) = delete;

         bool           setLocalSocketProperties( const int32_t a_nFlags, const int32_t a_nType = SOCK_STREAM, const int32_t a_nFamily = AF_INET );
         bool           open( const sockType_t a_nType,  const protocol_t a_nProtocol = protocol_t::TCP, const std::string a_strHostname = std::string( "localhost" ), const std::string a_strPort = std::string("5000") );
         bool           close();
         
         bool           setNoDelay(); // bypass nagle
         void           setListenerBacklog( int32_t a_nBacklog ) { m_nBacklog = a_nBacklog; }    // must be called before bind or default will be used, sets number of connection queuedon listener
         int32_t        getfd() { return m_fdSocket; }


         static bool    setOption( const socketfd_t a_fd, const int a_level, const int a_optName, const int a_nValue );
         static int     getOption( const socketfd_t a_fd, const int a_level, const int a_optName, int& a_nValue, socklen_t& a_len );
         static ssize_t send            ( const socketfd_t& a_fd, const void* a_pBuffer, const ssize_t& a_nBufferSzie );
         static ssize_t receive         ( const socketfd_t& a_fd, void* a_pBuffer, const ssize_t& a_nBufferSize );
         static ssize_t receive_blocking( const socketfd_t& a_fd, void* a_pBuffer, const ssize_t& a_nBufferSize );
         static int32_t getDefaultServerSocketFlags() { return AI_PASSIVE | AI_NUMERICSERV; }
         static int32_t getDefaultClientSocketFlags() { return AI_NUMERICSERV; }
   };
   
   
   /**
    * @brief ...blocking client 
    * @example see testing/sync/client.cpp
    * 
    */
   class Client : public Sockets
   {
      public:
         Client() = default;
         bool     connect( const std::string& a_strHostname, std::string a_strPort, protocol_t a_proto = protocol_t::TCP );
         ssize_t  send   ( const void* a_pBuffer, const ssize_t& a_nBufferSize );
         ssize_t  receive( void* a_pBuffer, const ssize_t& a_nBufferSize );
   };



   /**
    * @brief async (non-blocking) client
    * @example see testing/async/client.cpp
    * 
    */
   class ClientAsync : public Client
   {
      private:
         int32_t                       m_fdEpoll                = 0;            // server side epolling
         int32_t                       m_nMaximumEpollEvents    = 100;          // server side epolling
         int32_t                       m_nEpollTimeout_ms       = 1000;         // number of ms for epoll_wait timeout
         int32_t                       m_nPollingErrorCount     = 1;            // max number of consecuritve errors before epoll fails
         bool                          m_bAsyncRunFlag          = true;
         bool                          m_bUseMalloc             = true;
         bool                          m_bEdgeTriggered         = true;
         epoll_event*                  m_pEvents                = nullptr;
         std::thread                   m_thdReceiver            = std::thread();

         mutable std::condition_variable       m_cvReady        = std::condition_variable();                               // used to signal when the unblockedListener is ready
         mutable std::mutex                    m_muxReady       = std::mutex();
         
         // reconmnect thread params
         bool startAsync_( const socketCallback_t a_message, const errorCallBack_t a_error = nullptr, void* const a_pThis = nullptr );
       
      public:
         ClientAsync( int32_t a_nMaxEpollEvents = 100, int32_t a_nEpollTimeout_ms = 1000, int32_t a_nPollingErrorCount = 1 ) :
             m_nMaximumEpollEvents( a_nMaxEpollEvents ),
             m_nEpollTimeout_ms( a_nEpollTimeout_ms ),
             m_nPollingErrorCount( a_nPollingErrorCount )
         {}

         ClientAsync( const ClientAsync& ) = delete;
         ClientAsync& operator =( const ClientAsync& ) = delete;

         ssize_t  receive( void* a_pBuffer, const ssize_t& a_nBufferSize );
         bool     startAsync( const socketCallback_t a_message,  void* const a_pData = nullptr, const errorCallBack_t a_error = nullptr, const bool a_bEdgeTrigger = false );
         bool     reconnect( const int32_t a_nRetryCount, const int32_t a_nRetryWait, logCallBack_t a_cbLog );

         bool     unblockedListener( socketCallback_t a_message, errorCallBack_t a_error = nullptr );
         void     setMaximumPollEvents( int32_t a_nMaxCons )          { m_nMaximumEpollEvents = a_nMaxCons; }
         void     setEpollWaitTimeout( int32_t a_nEpollTimeout_ms )   { m_nEpollTimeout_ms = a_nEpollTimeout_ms; }
         void     setEpollErrorMax( int32_t a_nCount ) { m_nPollingErrorCount = a_nCount; }
         void     useStackAlloc()                      { m_bUseMalloc = false; }  // if stack space is ~1M then only about 4600 events can be stored, see setMaximumConnections
         void     useHeapAlloc()                       { m_bUseMalloc = true; }
         void     stop()                               { m_bAsyncRunFlag = false; }
         void     join()                               { m_thdReceiver.join(); }               
         void     waitready() const                    { std::unique_lock<std::mutex> lock( m_muxReady ); m_cvReady.wait( lock ); }
   };
   



   /**
    * @brief ...blocking server
    * @example see testing/sync/server.cpp
    * 
    * @details The blocking server canbe used as a simple server or onewhere each connection starts a thread to handle it
    * this will use a lot of reources if there are a lot of connections
    */
   class Server : public Sockets
   {
           
      public:
         Server() = default;
         ~Server();
         socketfd_t waitForConnection();
         ssize_t receive( const socketfd_t& a_fd, void* a_pBuffer, const ssize_t& a_nBufferSize )
         {
            return receive_blocking( a_fd, a_pBuffer, a_nBufferSize );
         }
   };





   /**
    * @brief ...async server
    * @example see testing/async/server.cpp
    * 
    * @details the non blocking server will do a callback anytime data is ready from a connection. The data handler can be done in the 
    * callback thread (if it is not time consuming) or use a pool of threads to handle the data read.
    * 
    * setMaximumPollEvents   number of events available per epoll return.  This value should be set
    *    to thee max concurrent active data connections.  If there are 10 connections and three are very active, where epoll returns three fd's
    *    with data, then you do not need more than three.  If there load changed and there were now four active, you would not loose data
    *    just will loose efficiency because another kernel calll would be needed to get data for that fd
    * 
    * setEpollWaitTimeout  ms to wait untill epoll_wait returns with no data. a low value will cause excessive cpu usage with no 
    *    data, a high value will make shutting down, slower.  This does not effect the response when data is available.  1000 ms is good
    * 
    * setEpollErrorMax  number of errors before epoll calls the error cdallback.  There should be no errors
    * 
    * useStackAlloc          use stack for epoll event queue.  Use the stack for instances where setMaximumPollEvents is not big.   What does this mean?
    *     consider that each of the events allocated from setMaximumPollEvents uses 12 bytes.... 
    * 
    * useHeapAlloc           use heap for epoll events
    * 
    * stop                   stop unblockedListener
    */
   class ServerAsync : public Server
   {
      private:
         int32_t     m_fdEpoll               = -1;        // server side epolling
         int32_t     m_nMaximumEpollEvents   = 512;       // server side epolling
         int32_t     m_nEpollTimeout_ms      = 1000;      // number of ms for epoll_wait timeout
         int32_t     m_nPollingErrorCount    = 100;       // max number of consecuritve errors before epoll fails
         bool        m_bAsyncRunFlag         = true;
         epoll_event*m_pEvents               = nullptr;
         bool        m_bUseMalloc            = true;
            
      public:
         ServerAsync() = default;
         ServerAsync( const ServerAsync& ) = delete;
         ~ServerAsync();

         ServerAsync& operator =( const ServerAsync& ) = delete;

         bool nonblockingListener( const socketCallback_t a_dataReady, const bool a_bEdgeTrigger = false, const errorCallBack_t a_error = nullptr, void *a_pData = nullptr );
         
         void setMaximumPollEvents( int32_t a_nMaxCons )       { m_nMaximumEpollEvents = a_nMaxCons; }
         void setEpollWaitTimeout( int32_t a_nEpollTimeout_ms ){ m_nEpollTimeout_ms    = a_nEpollTimeout_ms; }
         void setEpollErrorMax( int32_t a_nCount )             { m_nPollingErrorCount  = a_nCount; }
         void useStackAlloc()                                  { m_bUseMalloc          = false; }  // if stack space is ~1M then only about 4600 events can be stored, see setMaximumConnections
         void useHeapAlloc()                                   { m_bUseMalloc          = true;  }
         void stop()                                           { m_bAsyncRunFlag       = false; }
   };
   
   
   
   
}
}
