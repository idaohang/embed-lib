/*
 * Filename: bbb_int_thread.cpp
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Implementation file for BeagleBone Black interrupt thread
 */

#include "bbb_int_thread.h"

using namespace embed;

BBBIntThread::BBBIntThread() :
    m_started (false),
    m_exit (false),
    m_thread(),
    m_fds(),
    m_fdInfoMap(),
    m_mutexFds()
{
}

BBBIntThread::~BBBIntThread()
{
    m_fds.clear();
    m_fdInfoMap.clear();
}

bool BBBIntThread::start()
{
    if (m_started && !m_exit)
        return true;
    else if (m_started && m_exit)
        return false;

    if (pthread_mutex_init(&m_mutexFds, NULL) != 0)
    {
        fprintf(stderr, "BBB::IntThread::start pthread_mutex_init error\n");
        return false;
    }

    if (pthread_create(&m_thread, NULL, threadMain, this) != 0)
    {
        fprintf(stderr, "BBBIntThread::start pthread_create error\n");
        return false;
    }

    struct sched_param params;
    params.sched_priority = sched_get_priority_max(SCHED_RR);
    if (pthread_setschedparam(m_thread, SCHED_RR, &params) != 0)
    {
        fprintf(stderr, "BBBIntThread::start pthread_setschedparam error\n");
        return true;
    }

    m_started = true;
    m_exit = false;

    return true;
}

void BBBIntThread::end()
{
    if (!m_started || m_exit)
        return;

    m_exit = true;
    pthread_join(m_thread, NULL);

    pthread_mutex_destroy(&m_mutexFds);

    m_started = false;
    m_exit = false;
}

void BBBIntThread::registerListener (int32_t _fd, IntHandler _handler, void* _data)
{
    // This function may be inefficient, but we do not expect it to happen often
    // and the data structures are not expected to be large

    ListenerInfo info;
    info.m_fd = _fd;
    info.m_handler = _handler;
    info.m_data = _data;

    // std::vectors are not thread safe, so need to lock
    // if we will be adding or removing while thread is running
    // (should not really affect performance much)
    if (m_started)
        pthread_mutex_lock (&m_mutexFds);

    // Check if a pollfd struct already exists for this fd
    bool exists = false;
    std::vector<struct pollfd>::const_iterator it;
    for (it = m_fds.begin(); it != m_fds.end(); it++)
    {
        if (it->fd == _fd)
        {
            exists = true;
            break;
        }
    }

    // Create it and add it to the vector if it does not exist
    if (!exists)
    {
        struct pollfd fd;
        fd.fd = _fd;
        fd.events = POLLPRI;
        m_fds.push_back(fd);
    }

    // Add interrupt info to map of fds to infos
    m_fdInfoMap[_fd].push_back(info);

    if (m_started)
        pthread_mutex_unlock (&m_mutexFds);
}

void BBBIntThread::unregisterListener (int32_t _fd, IntHandler _handler)
{
    // This function may be inefficient, but we do not expect it to happen often
    // and the data structures are not expected to be large

    // std::vectors are not thread safe, so need to lock
    // if we will be adding or removing while thread is running
    // (should not really affect performance much)
    if (m_started)
        pthread_mutex_lock (&m_mutexFds);

    // Find the interrupt info for this handler in the list
    // of handlers for this fd
    std::vector<ListenerInfo>::iterator it;
    for (it = m_fdInfoMap[_fd].begin(); it != m_fdInfoMap[_fd].end(); it++)
    {
        if (it->m_handler == _handler)
            break;
    }
    if (it != m_fdInfoMap[_fd].end())
        m_fdInfoMap[_fd].erase(it);

    // Erase the fd from the fds list if there are no other listeners
    if (m_fdInfoMap[_fd].size() == 0)
    {
        std::vector<struct pollfd>::iterator toDel;
        for (toDel = m_fds.begin(); toDel != m_fds.end(); toDel++)
        {
            if (toDel->fd == _fd)
                break;
        }
        if (toDel != m_fds.end())
            m_fds.erase(toDel);
    }

    if (m_started)
        pthread_mutex_unlock (&m_mutexFds);
}

void* BBBIntThread::threadMain(void* _data)
{
    BBBIntThread* _this = static_cast<BBBIntThread*>(_data);

    char buf[MAX_BUF];
    while(1)
    {
        int32_t rc = poll(&(_this->m_fds[0]), _this->m_fds.size(), POLL_TIMEOUT_MS);

        if (rc < 0)
        {
            fprintf(stderr, "BBBIntThread::threadMain poll error\n");
            return NULL;
        }

        // std::vectors are not thread safe, so need to lock
        // if we will be adding or removing while thread is running
        // (should not really affect performance much)
        pthread_mutex_lock (&_this->m_mutexFds);

        // Iterate over fds to check for events
        std::vector<struct pollfd>::const_iterator it;
        for (it = _this->m_fds.begin(); it != _this->m_fds.end(); it++)
        {
            // Check for interrupt
            if (it->revents & POLLPRI)
            {
                // Acknowledge interrupt
                read(it->fd, buf, MAX_BUF);

                // Notify listeners
                int32_t fd = it->fd;
                std::vector<ListenerInfo>::iterator lit;
                for (lit = _this->m_fdInfoMap[fd].begin(); lit != _this->m_fdInfoMap[fd].end(); lit++)
                    lit->m_handler(lit->m_data);
            }
        }

        pthread_mutex_unlock (&_this->m_mutexFds);

        if (_this->m_exit)
            break;
    }

    return NULL;
}
