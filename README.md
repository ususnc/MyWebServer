
轻量级HTTP服务器，基于游双《Linux高性能服务器编程》和[TinyWebServer](https://github.com/qinguoyi/TinyWebServer)

## 主要改进 
* 基于std::conditional_variable重新实现了阻塞队列
* 使用std::thread和阻塞队列重新实现了线程池
* 异步日志系统改为使用新实现的阻塞队列
* 实现了基于最小堆的计时器容器，替代了原来基于有序链表的计时器容器
* 重新实现了更简洁的mysql连接池
* 重新组织了源代码，改用CMake构建
* 其他细节变动
