/*
    Copyright (c) 2009-2011 250bpm s.r.o.
    Copyright (c) 2011 Botond Ballo
    Copyright (c) 2007-2009 iMatix Corporation

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to
    deal in the Software without restriction, including without limitation the
    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#ifndef __ZMQ_HPP_INCLUDED__
#define __ZMQ_HPP_INCLUDED__

#include <zmq.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <exception>

//  Detect whether the compiler supports C++11 rvalue references.
#if (defined(__GNUC__) && (__GNUC__ > 4 || \
      (__GNUC__ == 4 && __GNUC_MINOR__ > 2)) && \
      defined(__GXX_EXPERIMENTAL_CXX0X__))
    #define ZMQ_HAS_RVALUE_REFS
    #define ZMQ_DELETED_FUNCTION = delete
#elif defined(__clang__)
    #if __has_feature(cxx_rvalue_references)
        #define ZMQ_HAS_RVALUE_REFS
    #endif

    #if __has_feature(cxx_deleted_functions)
        #define ZMQ_DELETED_FUNCTION = delete
    #else
        #define ZMQ_DELETED_FUNCTION
    #endif
#elif defined(_MSC_VER) && (_MSC_VER >= 1600)
    #define ZMQ_HAS_RVALUE_REFS
    #define ZMQ_DELETED_FUNCTION
#else
    #define ZMQ_DELETED_FUNCTION
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 3, 0)
#define ZMQ_NEW_MONITOR_EVENT_LAYOUT
#endif

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(4, 1, 0)
#define ZMQ_HAS_PROXY_STEERABLE
/*  Socket event data  */
typedef struct {
    uint16_t event;  // id of the event as bitfield
    int32_t  value ; // value is either error code, fd or reconnect interval
} zmq_event_t;
#endif

// In order to prevent unused variable warnings when building in non-debug
// mode use this macro to make assertions.
#ifndef NDEBUG
#   define ZMQ_ASSERT(expression) assert(expression)
#else
#   define ZMQ_ASSERT(expression) (void)(expression)
#endif

namespace zmq
{

    typedef zmq_free_fn free_fn;
    typedef zmq_pollitem_t pollitem_t;

    class error_t : public std::exception
    {
    public:

        error_t () : errnum (zmq_errno ()) {}

        virtual const char *what () const throw ()
        {
            return zmq_strerror (errnum);
        }

        int num () const
        {
            return errnum;
        }

    private:

        int errnum;
    };

    inline int poll (zmq_pollitem_t *items_, int nitems_, long timeout_ = -1)
    {
        int rc = zmq_poll (items_, nitems_, timeout_);
        if (rc < 0)
            throw error_t ();
        return rc;
    }

    inline void proxy (void *frontend, void *backend, void *capture)
    {
        int rc = zmq_proxy (frontend, backend, capture);
        if (rc != 0)
            throw error_t ();
    }
    
#ifdef ZMQ_HAS_PROXY_STEERABLE
    inline void proxy_steerable (void *frontend, void *backend, void *capture, void *control)
    {
        int rc = zmq_proxy_steerable (frontend, backend, capture, control);
        if (rc != 0)
            throw error_t ();
    }
#endif
    
    inline void version (int *major_, int *minor_, int *patch_)
    {
        zmq_version (major_, minor_, patch_);
    }

    class message_t
    {
        friend class socket_t;

    public:

        inline message_t ()
        {
            int rc = zmq_msg_init (&msg);
            if (rc != 0)
                throw error_t ();
        }

        inline explicit message_t (size_t size_)
        {
            int rc = zmq_msg_init_size (&msg, size_);
            if (rc != 0)
                throw error_t ();
        }

        inline message_t (void *data_, size_t size_, free_fn *ffn_,
            void *hint_ = NULL)
        {
            int rc = zmq_msg_init_data (&msg, data_, size_, ffn_, hint_);
            if (rc != 0)
                throw error_t ();
        }

#ifdef ZMQ_HAS_RVALUE_REFS
        inline message_t (message_t &&rhs) : msg (rhs.msg)
        {
            int rc = zmq_msg_init (&rhs.msg);
            if (rc != 0)
                throw error_t ();
        }

        inline message_t &operator = (message_t &&rhs)
        {
            std::swap (msg, rhs.msg);
            return *this;
        }
#endif

        inline ~message_t ()
        {
            int rc = zmq_msg_close (&msg);
            ZMQ_ASSERT (rc == 0);
        }

        inline void rebuild ()
        {
            int rc = zmq_msg_close (&msg);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init (&msg);
            if (rc != 0)
                throw error_t ();
        }

        inline void rebuild (size_t size_)
        {
            int rc = zmq_msg_close (&msg);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init_size (&msg, size_);
            if (rc != 0)
                throw error_t ();
        }

        inline void rebuild (void *data_, size_t size_, free_fn *ffn_,
            void *hint_ = NULL)
        {
            int rc = zmq_msg_close (&msg);
            if (rc != 0)
                throw error_t ();
            rc = zmq_msg_init_data (&msg, data_, size_, ffn_, hint_);
            if (rc != 0)
                throw error_t ();
        }

        inline void move (message_t *msg_)
        {
            int rc = zmq_msg_move (&msg, &(msg_->msg));
            if (rc != 0)
                throw error_t ();
        }

        inline void copy (message_t *msg_)
        {
            int rc = zmq_msg_copy (&msg, &(msg_->msg));
            if (rc != 0)
                throw error_t ();
        }

        inline bool more ()
        {
            int rc = zmq_msg_more (&msg);
            return rc != 0;
        }

        inline void *data ()
        {
            return zmq_msg_data (&msg);
        }

        inline const void* data () const
        {
            return zmq_msg_data (const_cast<zmq_msg_t*>(&msg));
        }

        inline size_t size () const
        {
            return zmq_msg_size (const_cast<zmq_msg_t*>(&msg));
        }

    private:

        //  The underlying message
        zmq_msg_t msg;

        //  Disable implicit message copying, so that users won't use shared
        //  messages (less efficient) without being aware of the fact.
        message_t (const message_t&);
        void operator = (const message_t&);
    };

    class context_t
    {
        friend class socket_t;

    public:
        inline context_t ()
        {
            ptr = zmq_ctx_new ();
            if (ptr == NULL)
                throw error_t ();
        }


        inline explicit context_t (int io_threads_, int max_sockets_ = ZMQ_MAX_SOCKETS_DFLT)
        {
            ptr = zmq_ctx_new ();
            if (ptr == NULL)
                throw error_t ();

            int rc = zmq_ctx_set (ptr, ZMQ_IO_THREADS, io_threads_);
            ZMQ_ASSERT (rc == 0);

            rc = zmq_ctx_set (ptr, ZMQ_MAX_SOCKETS, max_sockets_);
            ZMQ_ASSERT (rc == 0);
        }

#ifdef ZMQ_HAS_RVALUE_REFS
        inline context_t (context_t &&rhs) : ptr (rhs.ptr)
        {
            rhs.ptr = NULL;
        }
        inline context_t &operator = (context_t &&rhs)
        {
            std::swap (ptr, rhs.ptr);
            return *this;
        }
#endif

        inline ~context_t ()
        {
            close();
        }

        inline void close()
        {
            if (ptr == NULL)
                return;
            int rc = zmq_ctx_destroy (ptr);
            ZMQ_ASSERT (rc == 0);
            ptr = NULL;
        }

        //  Be careful with this, it's probably only useful for
        //  using the C api together with an existing C++ api.
        //  Normally you should never need to use this.
        inline operator void* ()
        {
            return ptr;
        }

    private:

        void *ptr;

        context_t (const context_t&);
        void operator = (const context_t&);
    };

    class socket_t
    {
        friend class monitor_t;
    public:

        inline socket_t (context_t &context_, int type_)
        {
            ctxptr = context_.ptr;
            ptr = zmq_socket (context_.ptr, type_);
            if (ptr == NULL)
                throw error_t ();
        }

#ifdef ZMQ_HAS_RVALUE_REFS
        inline socket_t(socket_t&& rhs) : ptr(rhs.ptr)
        {
            rhs.ptr = NULL;
        }
        inline socket_t& operator=(socket_t&& rhs)
        {
            std::swap(ptr, rhs.ptr);
            return *this;
        }
#endif

        inline ~socket_t ()
        {
            close();
        }

        inline operator void* ()
        {
            return ptr;
        }

        inline void close()
        {
            if(ptr == NULL)
                // already closed
                return ;
            int rc = zmq_close (ptr);
            ZMQ_ASSERT (rc == 0);
            ptr = 0 ;
        }

        inline void setsockopt (int option_, const void *optval_,
            size_t optvallen_)
        {
            int rc = zmq_setsockopt (ptr, option_, optval_, optvallen_);
            if (rc != 0)
                throw error_t ();
        }

        inline void getsockopt (int option_, void *optval_,
            size_t *optvallen_)
        {
            int rc = zmq_getsockopt (ptr, option_, optval_, optvallen_);
            if (rc != 0)
                throw error_t ();
        }
        
        inline void bind (const char *addr_)
        {
            int rc = zmq_bind (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline void unbind (const char *addr_)
        {
            int rc = zmq_unbind (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline void connect (const char *addr_)
        {
            int rc = zmq_connect (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline void disconnect (const char *addr_)
        {
            int rc = zmq_disconnect (ptr, addr_);
            if (rc != 0)
                throw error_t ();
        }

        inline bool connected()
        {
            return(ptr != NULL);
        }
        
        inline size_t send (const void *buf_, size_t len_, int flags_ = 0)
        {
            int nbytes = zmq_send (ptr, buf_, len_, flags_);
            if (nbytes >= 0)
                return (size_t) nbytes;
            if (zmq_errno () == EAGAIN)
                return 0;
            throw error_t ();
        }

        inline bool send (message_t &msg_, int flags_ = 0)
        {
            int nbytes = zmq_msg_send (&(msg_.msg), ptr, flags_);
            if (nbytes >= 0)
                return true;
            if (zmq_errno () == EAGAIN)
                return false;
            throw error_t ();
        }

        inline size_t recv (void *buf_, size_t len_, int flags_ = 0)
        {
            int nbytes = zmq_recv (ptr, buf_, len_, flags_);
            if (nbytes >= 0)
                return (size_t) nbytes;
            if (zmq_errno () == EAGAIN)
                return 0;
            throw error_t ();
        }

        inline bool recv (message_t *msg_, int flags_ = 0)
        {
            int nbytes = zmq_msg_recv (&(msg_->msg), ptr, flags_);
            if (nbytes >= 0)
                return true;
            if (zmq_errno () == EAGAIN)
                return false;
            throw error_t ();
        }
        
    private:
        void *ptr;
        void *ctxptr;

        socket_t (const socket_t&) ZMQ_DELETED_FUNCTION;
        void operator = (const socket_t&) ZMQ_DELETED_FUNCTION;
    };

    class monitor_t
    {
    public:
        monitor_t() : socketPtr(NULL) {}
        virtual ~monitor_t() {}

        void monitor(socket_t &socket, const char *addr_, int events = ZMQ_EVENT_ALL)
        {
            int rc = zmq_socket_monitor(socket.ptr, addr_, events);
            if (rc != 0)
                throw error_t ();

            socketPtr = socket.ptr;
            void *s = zmq_socket (socket.ctxptr, ZMQ_PAIR);
            assert (s);

            rc = zmq_connect (s, addr_);
            assert (rc == 0);
            
            on_monitor_started();
            
            while (true) {
                zmq_msg_t eventMsg;
                zmq_msg_init (&eventMsg);
                rc = zmq_recvmsg (s, &eventMsg, 0);
                if (rc == -1 && zmq_errno() == ETERM)
                    break;
                assert (rc != -1);
#if ZMQ_VERSION_MAJOR >= 4
                const char* data = static_cast<const char*>(zmq_msg_data(&eventMsg));
                zmq_event_t msgEvent;
                msgEvent.event = *(uint16_t*)data; data += sizeof(uint16_t);
                msgEvent.value = *(int32_t*)data;
                zmq_event_t* event = &msgEvent;
#else
                zmq_event_t* event = static_cast<zmq_event_t*>(zmq_msg_data(&eventMsg));
#endif
                
#ifdef ZMQ_NEW_MONITOR_EVENT_LAYOUT
                zmq_msg_t addrMsg;
                zmq_msg_init (&addrMsg);
                rc = zmq_recvmsg (s, &addrMsg, 0);
                if (rc == -1 && zmq_errno() == ETERM)
                    break;
                assert (rc != -1);
                const char* str = static_cast<const char*>(zmq_msg_data (&addrMsg));
                std::string address(str, str + zmq_msg_size(&addrMsg));
                zmq_msg_close (&addrMsg);
#else
                // Bit of a hack, but all events in the zmq_event_t union have the same layout so this will work for all event types.
                std::string address = event->data.connected.addr;
#endif

#ifdef ZMQ_EVENT_MONITOR_STOPPED
                if (event->event == ZMQ_EVENT_MONITOR_STOPPED)
                    break;
#endif

                switch (event->event) {
                case ZMQ_EVENT_CONNECTED:
                    on_event_connected(*event, address.c_str());
                    break;
                case ZMQ_EVENT_CONNECT_DELAYED:
                    on_event_connect_delayed(*event, address.c_str());
                    break;
                case ZMQ_EVENT_CONNECT_RETRIED:
                    on_event_connect_retried(*event, address.c_str());
                    break;
                case ZMQ_EVENT_LISTENING:
                    on_event_listening(*event, address.c_str());
                    break;
                case ZMQ_EVENT_BIND_FAILED:
                    on_event_bind_failed(*event, address.c_str());
                    break;
                case ZMQ_EVENT_ACCEPTED:
                    on_event_accepted(*event, address.c_str());
                    break;
                case ZMQ_EVENT_ACCEPT_FAILED:
                    on_event_accept_failed(*event, address.c_str());
                    break;
                case ZMQ_EVENT_CLOSED:
                    on_event_closed(*event, address.c_str());
                    break;
                case ZMQ_EVENT_CLOSE_FAILED:
                    on_event_close_failed(*event, address.c_str());
                    break;
                case ZMQ_EVENT_DISCONNECTED:
                    on_event_disconnected(*event, address.c_str());
                    break;
                default:
                    on_event_unknown(*event, address.c_str());
                    break;
                }
                zmq_msg_close (&eventMsg);
            }
            zmq_close (s);
            socketPtr = NULL;
        }

#ifdef ZMQ_EVENT_MONITOR_STOPPED
        void abort()
        {
            if (socketPtr)
                zmq_socket_monitor(socketPtr, NULL, 0);
        }
#endif
        virtual void on_monitor_started() {}
        virtual void on_event_connected(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_connect_delayed(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_connect_retried(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_listening(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_bind_failed(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_accepted(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_accept_failed(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_closed(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_close_failed(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_disconnected(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
        virtual void on_event_unknown(const zmq_event_t &event_, const char* addr_) { (void)event_; (void)addr_; }
    private:
        void* socketPtr;
    };
}

#endif
