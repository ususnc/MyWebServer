#include "sql_connection_pool.hpp"

void mysql_conn_pool::init(std::string host, std::string user, std::string password,
              std::string dbname, int port, int num_conn) {
    m_host = std::move(host);
    m_user = std::move(user);
    m_password = std::move(password);
    m_database = std::move(dbname);
    m_port = port;

    for (int i{0}; i < num_conn; ++i) {
        MYSQL* con{nullptr};
		con = mysql_init(con);

		if (con == nullptr)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		con = mysql_real_connect(con, m_host.c_str(), m_user.c_str(), m_password.c_str(), 
                                 m_database.c_str(), m_port, nullptr, 0);

		if (con == nullptr)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}

        m_conn_queue.push(con);
    }
}

mysql_conn_pool::~mysql_conn_pool() {
    m_conn_queue.des_all_elem(mysql_close);
    mysql_library_end();
    
}

mysql_conn_pool* mysql_conn_pool::get_instance() {
    static mysql_conn_pool pool{};
    return &pool;
}

MYSQL* mysql_conn_pool::get_connection() {
    return m_conn_queue.pop();
}

void mysql_conn_pool::release_connection(MYSQL* conn) {
    if (conn != nullptr) {
        m_conn_queue.push(conn);
    }
}