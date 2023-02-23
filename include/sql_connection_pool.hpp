#pragma once

#include <mysql/mysql.h>
#include <string>
#include "block_queue.hpp"
#include "log.hpp"


class mysql_conn_pool {
  public:
    static mysql_conn_pool* get_instance();

  public:
    MYSQL* get_connection();
    void release_connection(MYSQL* conn);
    void init(std::string host, std::string user, std::string password,
              std::string dbname, int port, int num_conn);

  private:
    mysql_conn_pool() = default;
    ~mysql_conn_pool();
  
  private:
    block_queue<MYSQL*> m_conn_queue{};
    std::string m_host{};
    int m_port{};
    std::string m_user{};
    std::string m_password{};
    std::string m_database{};
    // int m_close_log{};
};

class sql_conn {
  public:
    sql_conn(MYSQL*& sql_con) {
        sql_con = mysql_conn_pool::get_instance()->get_connection();
        m_sql_con = sql_con;
    }

    ~sql_conn() {
        mysql_conn_pool::get_instance()->release_connection(m_sql_con);
    }

  private:
    MYSQL* m_sql_con{};
};