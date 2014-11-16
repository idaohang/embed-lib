/*
 * Filename: bbb_int_thread.h
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Header file for BeagleBone Black interrupt thread
 */

#ifndef EMBED_BBB_INT_THREAD_H
#define EMBED_BBB_INT_THREAD_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <vector>
#include <map>

namespace embed
{

class BBBIntThread
{
 public:
    BBBIntThread();
    ~BBBIntThread();

    typedef void (*IntHandler) (void*);

    bool start();
    void end();

    void registerListener (int32_t _fd, IntHandler _handler, void* _data);
    void unregisterListener (int32_t _fd, IntHandler _handler);
 private:
    static void* threadMain(void* _data);
    typedef struct ListenerInfoStruct
    {
        int32_t             m_fd;
        IntHandler          m_handler;
        void*               m_data;
        bool                m_firstInterrupt;
    } ListenerInfo;

    static const int32_t MAX_BUF = 64;
    static const int32_t POLL_TIMEOUT_MS = 100;

    bool                                            m_started;
    bool                                            m_exit;
    pthread_t                                       m_thread;
    // fds to call poll on
    std::vector<struct pollfd>                      m_fds;
    // What to call for a given fd with interrupt
    std::map<int32_t, std::vector<ListenerInfo> >   m_fdInfoMap;

};

}

#endif
