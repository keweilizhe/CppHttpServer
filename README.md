# CppHttpServer

主要涉及epoll的使用: 

http://blog.csdn.net/ctthuangcheng/article/details/9332655 

http://blog.csdn.net/ctthuangcheng/article/details/9716715

http的 get post 协议 :

## epoll接口

- int epoll_create(int size);  

> 创建一个epoll的句柄，size用来告诉内核这个监听的数目一共有多大。这个参数不同于select()中的第一个参数，给出最大监听的fd+1的值。需要注意的是，当创建好epoll句柄后，它就是会占用一个fd值，在linux下如果查看/proc/进程id/fd/，是能够看到这个fd的，所以在使用完epoll后，必须调用close()关闭，否则可能导致fd被耗尽。

- int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);  

> epoll的事件注册函数，它不同与select()是在监听事件时告诉内核要监听什么类型的事件，而是在这里先注册要监听的事件类型。第一个参数是epoll_create()的返回值，第二个参数表示动作，用三个宏来表示：
- EPOLL_CTL_ADD：注册新的fd到epfd中；
- EPOLL_CTL_MOD：修改已经注册的fd的监听事件；
- EPOLL_CTL_DEL：从epfd中删除一个fd；
