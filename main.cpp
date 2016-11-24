#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <string>

#define MAX_EVENTS 100000
#define MAX_LISTEN_BLOCK 20
#define BUF_SIZE 2048

using namespace std;

class SocketServer
{
public:
	SocketServer(int port);
	~SocketServer();
	int init();
	int run();

private:
	SocketServer();
	SocketServer(const SocketServer& );
	SocketServer& operator = (const SocketServer& );

private:
	int handle_new_connect(struct epoll_event &event);
	int handle_message_in(struct epoll_event &event);
	int handle_message_out(struct epoll_event &event);
	void set_nonblock(int sockfd);

private:
	int						port_;
	int						listen_fd_;
	int						epoll_fd_;
	struct epoll_event		events_[MAX_EVENTS];
};

SocketServer::SocketServer(int port)
{
	port_ = port;
}

SocketServer::~SocketServer() 
{
	if (epoll_fd_ > 0)
	{
		close(epoll_fd_);
	}
	if (listen_fd_ > 0)
	{
		close(listen_fd_);
	}
}

void SocketServer::set_nonblock(int sockfd)
{
	int opts;
	opts = fcntl(sockfd, F_GETFL);  //读取文件状态标志
	if (opts < 0)
	{
		perror("fcntl(F_GETFL): ");
		exit(1);
	}

	if (fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)  //设置文件状态标志,非阻塞
	{
		perror("fcntl(F_SETFL): ");
		exit(1);
	}
}

int SocketServer::init()
{
	if ((listen_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("listen_fd_: ");
		exit(1);
	}
	set_nonblock(listen_fd_);	//使用非阻塞模式的监听listen_fd_

	struct sockaddr_in		  local_addr;
	bzero(&local_addr, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(port_);

	if (bind(listen_fd_, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
	{
		perror("bind: ");
		exit(1);
	}

	if (listen(listen_fd_, MAX_LISTEN_BLOCK) < 0)
	{
		perror("listen: ");
		exit(1);
	}

	epoll_fd_ = epoll_create(MAX_EVENTS);
	if (epoll_fd_ < 0)
	{
		perror("epoll_create: ");
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = listen_fd_;
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev) < 0)
	{
		perror("epoll_ctl listen_fd_: " );
		exit(EXIT_FAILURE);
	}

	return 0;
}

int SocketServer::run()
{
	for ( ; ; )
	{
		int fd_num = epoll_wait(epoll_fd_, events_, MAX_EVENTS, -1);
		if (fd_num < 0)
		{
			perror("epoll_wait: ");
			exit(EXIT_FAILURE);
		}
		int n = 0;      
		char buf[BUF_SIZE];

		for (int i = 0; i < fd_num; ++i)
		{
			if (events_[i].data.fd == listen_fd_)	//新的请求到来
			{
				int socket_new;
				struct epoll_event ev;
				while ((socket_new = accept(listen_fd_, NULL, NULL)) > 0)
				{
					if (socket_new < 0)
					{
						perror("accept socket_new: ");
						exit(EXIT_FAILURE); 
					}
					set_nonblock(socket_new);

					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = socket_new;
					if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_new, &ev) < 0)
					{
						perror("epoll_ctl add socket_new: ");
						exit(EXIT_FAILURE); 
					}
				}

				if (socket_new < 0 && errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR )
				{
					perror("accept: ");  
				}

				continue;
			}
			else if (events_[i].events & EPOLLIN)	//某个连接有消息过来
			{
				n  = 0
                while ((nread = read(fd, buf+n, BUF_SIZE-1)) > 0)
                {
                    n += nread;
                }
                if (nread == -1 && errno != EAGAIN)
                {
                    perror("read error");
                }
                buf[n] = '\0';
                printf("n = %d, buf = %s", n, buf);

                struct epoll_event ev;
                ev.data.fd = fd;
                ev.events = events_[i].events | EPOLLOUT;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0)
                {
                    perror("epoll_ctl: mod");
                }
			}
			else if (events_[i].events & EPOLLOUT)	//发消息出去
			{
				sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nHello", 5);
                int nwrite, data_size = strlen(buf);
                n = data_size;
                while (n > 0) 
                {
                    nwrite = write(fd, buf + data_size - n, n);
                    if (nwrite == -1 && errno != EAGAIN)
                    {
                        perror("write error");
                    }
                    break;
                    n -= nwrite;
                }
                close(fd); 
			}
			else
			{
				printf("event error, event=%d\n", events_[i].events);
			}
		}
	}

	close(epoll_fd_);
	epoll_fd_ = -1;
	close(listen_fd_);
	listen_fd_ = -1;
}

int SocketServer::handle_new_connect(struct epoll_event &event)
{
	int socket_new;
	struct epoll_event ev;
	while ((socket_new = accept(listen_fd_, NULL, NULL)) > 0)
	{
		if (socket_new < 0)
		{
			perror("accept socket_new: ");
			exit(EXIT_FAILURE); 
		}
		set_nonblock(socket_new);

		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = socket_new;
		if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_new, &ev) < 0)
		{
			perror("epoll_ctl add socket_new: ");
			exit(EXIT_FAILURE); 
		}
	}
}

int SocketServer::handle_message_in(struct epoll_event &event)
{

}

int SocketServer::handle_message_out(struct epoll_event &event)
{

}



int main()
{
	SocketServer my_srv(80)
	return 0;
}
