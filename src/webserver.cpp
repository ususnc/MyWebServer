#include "webserver.hpp"

WebServer::WebServer()
{
    //http_conn类对象
    users = new http_conn[MAX_FD];
    m_user_timers = new timer[MAX_FD];
    for (int i{0}; i < MAX_FD; ++i) {
        m_user_timers[i].m_cb_func = [this, i]() {
            users[i].close_conn();
        };
    }

    m_root = new char[200];
    strcpy(m_root, "../root");
}

WebServer::~WebServer()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] m_user_timers;
}

void WebServer::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void WebServer::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void WebServer::log_write()
{
    // if (0 == m_close_log)
    // {
        //初始化日志
    if (1 == m_log_write)
        Log::get_instance()->init("../server_log_files/", m_close_log, 2000, 800000, 800);
    else
        Log::get_instance()->init("../server_log_files/", m_close_log, 2000, 800000, 0);
    // }
}

void WebServer::sql_pool()
{
    //初始化数据库连接池
    m_connPool = mysql_conn_pool::get_instance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 0, m_sql_num);


    //初始化数据库读取表
    users->initmysql_result();
}


void WebServer::eventListen()
{
    //网络编程基础步骤
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    //优雅关闭连接
    if (0 == m_OPT_LINGER)
    {
        struct linger tmp = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    else if (1 == m_OPT_LINGER)
    {
        struct linger tmp = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    // utils.init(TIMESLOT);

    //epoll创建内核事件表
    // epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    utils.addsig(SIGPIPE, SIG_IGN);
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

bool WebServer::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == m_LISTENTrigmode)
    {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        // timer_init(connfd, client_address);
        users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_user, m_passWord, m_databaseName);
        m_user_timers[connfd].m_expire = time(nullptr) + 3 * TIMESLOT;
        m_timer_heap.add_timer(&m_user_timers[connfd]);

    }

    else
    {
        while (1)
        {
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            // timer_init(connfd, client_address);
            users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_user, m_passWord, m_databaseName);
            m_user_timers[connfd].m_expire = time(nullptr) + 3 * TIMESLOT;
            m_timer_heap.add_timer(&m_user_timers[connfd]);
        }
        return false;
    }
    return true;
}

bool WebServer::dealwithsignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    // int sig;
    char signals[1024];
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; ++i)
        {
            switch (signals[i])
            {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void WebServer::dealwithread(int sockfd)
{
    // util_timer *timer = users_timer[sockfd].timer;

    //reactor
    if (m_actormodel == 1) 
    {
        std::promise<bool> the_promise{};
        auto the_future{the_promise.get_future()};
        m_thread_pool.submit( [sockfd, this, &the_promise](){
            if (users[sockfd].read_once())
            {
                LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
                
                users[sockfd].process();

                the_promise.set_value(true);
            }
            else
            {
                the_promise.set_value(false);
            }
        } );

        if (the_future.get()) {
            // if (timer) {
            //     adjust_timer(timer);
            // }
            m_user_timers[sockfd].m_expire = time(nullptr) + 3 * TIMESLOT;
            m_timer_heap.adjust_timer(&m_user_timers[sockfd]);
            LOG_INFO("%s", "adjust timer once");
        }
        else {
            // deal_timer(timer, sockfd);
            users[sockfd].close_conn(true);
            m_timer_heap.delete_timer(&m_user_timers[sockfd]);
        }
    }
    else
    {
        //proactor
        if (users[sockfd].read_once())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            m_thread_pool.submit([sockfd, this](){
                users[sockfd].process();
            });

            // if (timer)
            // {
            //     adjust_timer(timer);
            // }
            m_user_timers[sockfd].m_expire = time(nullptr) + 3 * TIMESLOT;
            m_timer_heap.adjust_timer(&m_user_timers[sockfd]);
            LOG_INFO("%s", "adjust timer once");

        }
        else
        {
            // deal_timer(timer, sockfd);
            users[sockfd].close_conn(true);
            m_timer_heap.delete_timer(&m_user_timers[sockfd]);
        }
    }
}

void WebServer::dealwithwrite(int sockfd)
{
    // util_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (1 == m_actormodel)
    {
        std::promise<bool> the_promise{};
        auto the_future{the_promise.get_future()};
        m_thread_pool.submit([sockfd, this, &the_promise](){
            if (users[sockfd].write())
            {
                LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

                the_promise.set_value(true); 
            }
            else
            {
                the_promise.set_value(false);
            }
        });

        if (the_future.get()) {
            // if (timer) {
            //     adjust_timer(timer);
            // }
            m_user_timers[sockfd].m_expire = time(nullptr) + 3 * TIMESLOT;
            m_timer_heap.adjust_timer(&m_user_timers[sockfd]);
            LOG_INFO("%s", "adjust timer once");
        }
        else {
            // deal_timer(timer, sockfd);
            users[sockfd].close_conn(true);
            m_timer_heap.delete_timer(&m_user_timers[sockfd]);
        }

    }
    else
    {
        //proactor
        if (users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            m_user_timers[sockfd].m_expire = time(nullptr) + 3 * TIMESLOT;
            m_timer_heap.adjust_timer(&m_user_timers[sockfd]);
            LOG_INFO("%s", "adjust timer once");
        }
        else
        {
            // deal_timer(timer, sockfd);
            users[sockfd].close_conn(true);
            m_timer_heap.delete_timer(&m_user_timers[sockfd]);
        }
    }
}

void WebServer::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;


            //处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealclinetdata();
                if (false == flag)
                    continue;
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                // util_timer *timer = users_timer[sockfd].timer;
                // deal_timer(timer, sockfd);
                users[sockfd].close_conn(true);
                m_timer_heap.delete_timer(&m_user_timers[sockfd]);
            }
            //处理信号
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if (timeout)
        {
            // utils.timer_handler();
            m_timer_heap.tick();
            alarm(TIMESLOT);

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}


