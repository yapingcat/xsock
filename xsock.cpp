#include "xsock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/select.h>

int xsocketApi::createTcpSocket()
{
	return ::socket(AF_INET,SOCK_STREAM,0);	
}

int xsocketApi::createServerSocket(const std::string& ip, uint16_t port)
{
	int server = createTcpSocket();
    setBlock(server,0);
	setReuseOption(server);
	int ret = bind(server,ip,port);
	return ret == 0 ? server: -1;
}

int xsocketApi::acceptClient(int server,std::string&ip, uint16_t& port)
{
	sockaddr_in sa;
   	socklen_t len = sizeof(sa);
	int sc = ::accept(server,(sockaddr*)&sa,&len);
    if( sc < 0)
    {
        return sc;
    }
    char addr[64]= {0};
    ::inet_ntop(AF_INET,&(sa.sin_addr),addr,63);
    ip = addr;
    port = ntohs(sa.sin_port);
    return sc;
}

int xsocketApi::acceptClient(int server)
{
	sockaddr_in sa;
   	socklen_t len = sizeof(sa);
	int sc = ::accept(server,(sockaddr*)&sa,&len);
    if( sc < 0)
    {
        return sc;
    }
    return sc;
}

int xsocketApi::listen(int sc,int backlog)
{
	return ::listen(sc,backlog);
}

int xsocketApi::bind(int sc,const std::string& ip, uint16_t port)
{
	sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET, ip == ""?"0.0.0.0":ip.c_str(),&sa.sin_addr);
    return ::bind(sc,(sockaddr*)&sa,sizeof(sa));
}

int xsocketApi::connect(int sc, const std::string& ip,uint16_t port)
{
	sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    inet_pton(AF_INET,ip.c_str(),&sa.sin_addr);
    return ::connect(sc,(sockaddr*)&sa,sizeof(sa));	
}

int xsocketApi::setReuseOption(int sc)
{
	char value = 1;
	int ret = setSocketOption(sc,SOL_SOCKET,SO_REUSEADDR,value);
	ret = setSocketOption(sc,SOL_SOCKET,SO_REUSEPORT,value);
	return ret;	
}

int xsocketApi::setBlock(int sc,int block)
{
    int flags = ::fcntl(sc,F_GETFL);
    flags = flags & ~O_NONBLOCK;
    if(!block) flags = flags & O_NONBLOCK;
	return ::fcntl(sc,F_SETFL,flags);
}

int xsocketApi::setSocketOption(int sc,int level,int option, int value)
{
	return ::setsockopt(sc,level,option, reinterpret_cast<const char*>(&value),sizeof(value));
}

int xsocketApi::getSocketOption(int sc,int level,int option, int& value)
{
	socklen_t len = sizeof(value);
	return ::getsockopt(sc,level,option, reinterpret_cast<char*>(&value),&len);
}

int xsocketApi::close(int sc)
{
	return ::close(sc);
}

int xsocketApi::getPeerName(int sc, std::string& ip, uint16_t& port)
{
    sockaddr_in sa;
    socklen_t len = sizeof(sa);
    int ret = ::getpeername(sc,(sockaddr*)&sa,&len);
    if(ret != 0)
    {
        return ret;
    }

    char addr[64] = {0};
    ::inet_ntop(AF_INET,&(sa.sin_addr),addr,63);
    ip = addr;
    port = ntohs(sa.sin_port);
    return ret;
}

pipe::~pipe()
{
	if(pipefd_[0] > 0)
		xsocketApi:close(pipefd_[0]);
	if(pipefd_[1] > 0)
		xsocketApi::close(pipefd_[1]);
}

int pipe::init()
{
	return ::pipe(pipefd_);
}

int pipe::rfd()
{
	return pipefd_[0];	
}

int pipe::wfd()
{
	return pipefd_[1];
}

int pipe::read(char* buf, int size)
{
	 return ::read(pipefd_[0],buf,size);
}

int pipe::write(const char* buf, int size)
{
 	return ::write(pipefd_[1],buf,size);
}

xsock::xsock()
	:fd_(-1)
{
	init();
}

xsock::xsock(int fd)
{
	fd_ = fd;
}

xsock::~xsock()
{
    close();
}


int xsock::connect(const std::string& ip,uint16_t port)
{
	return xsocketApi::connect(fd_,ip,port);
}

int xsock::bind(const std::string&ip,uint16_t port)
{
	return xsocketApi::bind(fd_,ip,port);
}

int xsock::listen(int backlog)
{
	return xsocketApi::listen(fd_,backlog);
}

xsock::Ptr xsock::accept(std::string& ip, uint16_t& port)
{
	int client = xsocketApi::acceptClient(fd_,ip,port);
	return std::make_shared<xsock>(client);
}

xsock::Ptr xsock::accept()
{
	int client = xsocketApi::acceptClient(fd_);
	return std::make_shared<xsock>(client);
}

int xsock::close()
{
    if(fd_ < 0)
    {
	    return 0;
    }
	int ret = xsocketApi::close(fd_);
    fd_ = ret == 0 ?invalid_xsock:fd_;
    return ret;
}

int xsock::sendBytes(const std::string& bytes,uint32_t size)
{
	return ::send(fd_,bytes.c_str(),size,0);
}

int xsock::sendBytes(const char* buf,uint32_t size)
{
	return ::send(fd_,buf,size,0);
}

int xsock::recvBytes(char* buf,uint32_t size)
{
	return ::recv(fd_,buf,size,0);
}

int xsock::fd()
{
	return fd_;
}

int xsock::sockError()
{
	int value;
	int ret = xsocketApi::getSocketOption(fd_,SOL_SOCKET,SO_ERROR,value);
	return ret == 0 ? value:ret;
}

int xsock::setReuseAddr()
{
	return  xsocketApi::setSocketOption(fd_,SOL_SOCKET,SO_REUSEADDR,1);
}

int xsock::setReusePort()
{
	return  xsocketApi::setSocketOption(fd_,SOL_SOCKET,SO_REUSEPORT,1);
}

int xsock::setBlock(int flag)
{
	int ret = xsocketApi::setBlock(fd_,flag == 1 ? flag:0);
	block_ = (flag == 1 && ret == 0)? true : false;
	return ret; 
}

bool xsock::isBlock()
{
	return block_;
}

int xsock::setSocketOption(int option,int value)
{
	return xsocketApi::setSocketOption(fd_,SOL_SOCKET,option,value);
}

int xsock::getSocketOption(int option,int &value)
{
	return xsocketApi::getSocketOption(fd_,SOL_SOCKET,option,value);
}

int xsock::getClient(std::string& ip, uint16_t& port)
{
    return xsocketApi::getPeerName(fd_,ip,port);
}

void xsock::init()
{
	assert(fd_ < 0);
	fd_ = xsocketApi::createTcpSocket();
}

int xsvrsock::opensvr(const std::string&ip, uint16_t port)
{
	if(fd_ < -1)
	{
		fd_ = xsocketApi::createServerSocket(ip,port);
		return fd_ > 0 ? 0 : -1;
	}
	else
	{
		setReuseAddr();
		setReusePort();
		int l = setBlock(0);
		assert(l == 0);
		int ret = bind(ip,port);
		return ret;
	}
}

EventHandler::EventHandler()
	:event_(0)
{

}

void EventHandler::attachHandler(xsock::Ptr handler)
{
	sock_ = handler;
}

void EventHandler::detachHandler()
{
	sock_.reset();
}

void EventHandler::enableEvent(int event)
{
	event_ |= event; 
}

void EventHandler::disableEvent(int event)
{
	event_ &= ~event;
}

int EventHandler::interest()
{
	return event_;
}

void EventHandler::handlerEvent(int event)
{
	if(event & EV_READ)
	{
		onRecv();
	}
	if(event & EV_WRITE)
	{
		onSend();
	}
	if(event & EV_ERROR)
	{
		onError();
	}
}

xsock::Ptr EventHandler::sock() const
{
	return sock_;
}

void EventHandler::onRecv()
{
	printf("on Recv\n");
	char buf[2046] = {0};
	sock_->recvBytes(buf,sizeof(buf));
}

void EventHandler::onSend()
{
	char buf[2046] = "hello world\n";
	sock_->sendBytes(buf,sizeof(buf));
}

void EventHandler::onError()
{
	errno;
}

class FuncHandler: public EventHandler
{
public:
	typedef std::shared_ptr<FuncHandler> Ptr;
public:
	FuncHandler(const XEventCB ecb):ecb_(std::move(ecb)){}
	~FuncHandler(){}

protected:
	
    void onRecv()
	{
		if(ecb_)
			ecb_(sock_,EV_READ);
	}
	
	void onSend()
	{
		if(ecb_)
			ecb_(sock_,EV_WRITE);
	}

	void onError()
	{
		if(ecb_)
			ecb_(sock_,EV_ERROR);
	}


private:
	XEventCB ecb_;
};

xloop::xloop()
	:quit_(true)
{
	wakeup_.init();
}

xloop::~xloop()
{
}

void xloop::run()
{
	quit_ = false;
	tid_ = std::this_thread::get_id();
	while(!quit_)
	{
		loop();
	}
}

void xloop::stop()
{
	quit_ = true;
	wakeup();	
}

void xloop::addEventHandler(xsock::Ptr psock,EventHandler::Ptr handler)
{
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){ eventMap_[psock] = handler;});
	}
	wakeup();
}

void xloop::delEventHandler(xsock::Ptr psock)
{
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){ eventMap_.erase(psock);});
	}
	wakeup();
}

void xloop::addEventCallBack(xsock::Ptr psock,int event,const XEventCB ecb)
{
	FuncHandler::Ptr fptr = std::make_shared<FuncHandler>(ecb);
	fptr->attachHandler(psock);
	fptr->enableEvent(event);
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){ eventMap_[psock] = fptr;});
	}
	wakeup();
}

void xloop::unregisterEvent(xsock::Ptr psock,int event)
{
	if(tid_ == std::this_thread::get_id())
	{
		if(eventMap_.find(psock) != eventMap_.end()) 
			eventMap_[psock]->disableEvent(event);
		return;
	}
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=]() { 
			if(eventMap_.find(psock) != eventMap_.end()) 
				eventMap_[psock]->disableEvent(event);
			});
	}
	wakeup();
}

void xloop::registerEvent(xsock::Ptr psock, int event)
{
	if(tid_ == std::this_thread::get_id())
	{
		if(eventMap_.find(psock) != eventMap_.end()) 
			eventMap_[psock]->enableEvent(event);
		return;
	}
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){
			if(eventMap_.find(psock) != eventMap_.end()) 
				eventMap_[psock]->enableEvent(event);
			});
	}
	wakeup();
}

void xloop::loop()
{
	std::shared_ptr<pollfd> pollArray(new pollfd[eventMap_.size() + 1](),[](pollfd* pfd){ delete[] pfd;});
	int idx = 0;
	pollArray.get()[idx].fd = wakeup_.rfd();
	pollArray.get()[idx++].events |= POLLIN;
	for(auto it : eventMap_)
	{
		if(!(it.second->interest() & EV_ALL))
		{
			continue;
		}
		if(it.second->interest() & EV_READ)
		{
			pollArray.get()[idx].fd = it.first->fd();
			pollArray.get()[idx].events |= POLLIN; 
		}
		if(it.second->interest() & EV_WRITE)
		{
			pollArray.get()[idx].fd = it.first->fd();
			pollArray.get()[idx].events |= POLLOUT; 
		}
		if(it.second->interest() & EV_ERROR)
		{
			pollArray.get()[idx].fd = it.first->fd();
			pollArray.get()[idx].events |= POLLERR;
		}
		idx++;
	}

	int rc = ::poll(pollArray.get(), idx, 10000);
	if (rc < 0)
	{
		return;
	}
	
    if (POLLIN & pollArray.get()[0].revents)
	{
		char buf[32] = {0};
		wakeup_.read(buf,32);
        rc--;
	}

	for(int i = 1; i < idx && i - 1 < rc; i++)
	{
		auto tmpfd = pollArray.get()[i].fd;
		auto sockitem = std::find_if(eventMap_.begin(),eventMap_.end(),[tmpfd](decltype(eventMap_)::value_type v) { return tmpfd == v.first->fd(); });
		if(sockitem == eventMap_.end())
		{
			continue;
		}
		int revents = 0;
		if (POLLIN & pollArray.get()[i].revents)
		{
			revents |= EV_READ; 
		}
		if (POLLOUT & pollArray.get()[i].revents)
		{
			revents |= EV_WRITE;
		}
		if(POLLERR & pollArray.get()[i].revents)
		{
			revents |= EV_ERROR;
		}
		if(revents != 0)
		{
			sockitem->second->handlerEvent(revents);
		}
	}
	
	decltype(functors_) tempfuncs;
	{
		std::lock_guard<std::mutex> guard(mtx_);
		tempfuncs = std::move(functors_);
	}
	for(auto func : tempfuncs)
	{
		func();
	}
}

void xloop::run_epoll()
{
	int maxEvents = eventMap_.size() + 1;
	std::shared_ptr<epoll_event> epollEvents(new epoll_event[maxEvents](),[](epoll_event* pevents){ delete[] pevents;});
	int idx = 0;
	
	epollEvents.get()[idx].data.fd = wakeup_.rfd();
	epollEvents.get()[idx++].events = EPOLLIN;
	for(auto &it : eventMap_)
	{
		if(!(it.second->interest() & EV_ALL))
		{
			continue;
		}
		if(it.second->interest() & EV_READ)
		{
			pollArray.get()[idx].data.fd = it.first->fd();
			pollArray.get()[idx].events |= EPOLLIN; 
		}
		if(it.second->interest() & EV_WRITE)
		{
			pollArray.get()[idx].data.fd = it.first->fd();
			pollArray.get()[idx].events |= EPOLLOUT; 
		}
		if(it.second->interest() & EV_ERROR)
		{
			pollArray.get()[idx].data.fd = it.first->fd();
			pollArray.get()[idx].events |= EPOLLERR;
		}
		idx++;
	}



}


void xloop::wakeup()
{
	wakeup_.write("wakeup man",10);
}


void xSelectLoop::loop()
{
	fd_set rset;
	fd_set wset;
	fd_set eset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_ZERO(&eset);

	FD_SET(wakeup_.rfd(),&rset);
	auto compare = [](decltype(eventMap_)::value_type& a,decltype(eventMap_)::value_type& b) { return a.first->fd() < b.first->fd(); };
	auto maxSock = std::max_element(eventMap_.begin(),eventMap_.end(),compare);
	auto maxFd = maxSock == eventMap_.end() ? wakeup_.rfd() : std::max(maxSock->first->fd(),wakeup_.rfd());
	for(auto &it : eventMap_)
	{
		if(it.second->interest() & EV_READ)
		{
			FD_SET(it.first->fd(),&rset);
		}
		if(it.second->interest() & EV_WRITE)
		{
			FD_SET(it.first->fd(),&wset);
		}
		if(it.second->interest() & EV_ERROR)
		{
			FD_SET(it.first->fd(),&eset);
		}
	}
	
	struct timeval timeout;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	int ready = ::select(maxFd+1,&rset,&wset,&eset,&timeout);
	if(ready < 0)
	{
		return ;
	}
    
    if(FD_ISSET(wakeup_.rfd(),&rset))
    {
        char buf[32] = {0};
        wakeup_.read(buf,32);
        --ready;
    }
	for(auto it = eventMap_.begin(); it != eventMap_.end() && ready-- > 0; it++)
	{
		int revents = 0;
		auto tmpfd = it->first->fd();
		if(FD_ISSET(tmpfd,&rset))
		{
			revents |= EV_READ;
		}
		if(FD_ISSET(tmpfd,&wset))
		{
			revents |= EV_WRITE;
		}
		if(FD_ISSET(tmpfd,&eset))
		{
			revents |= EV_ERROR;
		}
		if(revents == 0)
		{
			continue;
		}
		auto sockItem = std::find_if(eventMap_.begin(),eventMap_.end(),[tmpfd](decltype(eventMap_)::value_type v) { return tmpfd == v.first->fd(); });
		if(sockItem == eventMap_.end())
		{
			continue;
		}
		sockItem->second->handlerEvent(revents);
	}

	decltype(functors_) tempfuncs;
	{
		std::lock_guard<std::mutex> guard(mtx_);
		tempfuncs = std::move(functors_);
	}
	for(auto func : tempfuncs)
	{
		func();
	}
}

xEpollLoop::xEpollLoop()
{
	epollfd_ = epoll_create(10000);
	assert(epollfd_ != -1);
	auto ev = std::make_shared<epoll_event>();
	epoll_ctl(epollfd_,EPOLL_CTL_ADD,wakeup_,rfd(),ev.get());
}

xEpollLoop::~xEpollLoop()
{
	epoll_ctl(epollfd_,EPOLL_CTL_DEL,wakeup_,rfd())
	if(epollfd_ != -1)
	{
		xsocketApi::close(epollfd_);
	}
}


int xEpollLoop::toEPollEvent(int event)
{
	int flag = 0;
	if(event& EV_READ)
	{
		flag |= EPOLLIN;
	}
	if(event & EV_WRITE)
	{
		flag |= EPOLLOUT;
	}
	if(event & EV_ERROR)
	{
		flag |= EPOLLERR;
	}
	return flag;
}

void xEpollLoop::addEventHandler(xsock::Ptr psock, EventHandler::Ptr handler)
{
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){
			if(!psock || !hander)
			{
				return;
			}

			int flag = toEPollEvent(handler->interest());
			if(eventMap_.find(psock) == eventMap_.end())
			{
				auto ev = std::make_shared<epoll_event>();
				ev.data.fd = psock->fd();
				ev.events = flag;
				int r = epoll_ctl(epollfd_,EPOLL_CTL_ADD,psock->fd(),ev.get());
				if(r < 0)
				{
					return; //TODO
				}
				eventMap_[psock] = handler;
				epollEventMap_[psock->fd()] = ev;
			}	
			else
			{
				epollEventMap_[psock->fd()]->events = flag;
				int r = epoll_ctl(epollfd_,EPOLL_CTL_MOD,psock->fd(),epollEventMap_[psock->fd()].get());
				if(r < 0)
				{
					return; //TODO
				}
			});
	}
	wakeup();
}

void xEpollLoop::addEventCallBack(xsock::Ptr psock,int event,const XEventCB ecb)
{
	FuncHandler::Ptr fptr = std::make_shared<FuncHandler>(ecb);
	fptr->attachHandler(psock);
	addEventHandler(psock,fptr);
}

void xEpollLoop::unregisterEvent(xsock::Ptr psock,int event)
{
	auto modEvent = [=](){
		if(eventMap_.find(psock) == eventMap_.end())
		{
			return;
		}
		int flag = toEPollEvent(handler->interest() & ~event);
		epollEventMap_[psock->fd()]->events = flag;
		int r = epoll_ctl(epollfd_,EPOLL_CTL_MOD,psock->fd(),epollEventMap_[psock->fd()].get());
		if(r < 0)
		{
			return;
		}
		eventMap_[psock]->disableEvent(event);
	};
	if(tid_ == std::this_thread::get_id())
	{
		modEvent();
	}
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back(modEvent);
	}
	wakeup();
}

void xEpollLoop::registerEvent(xsock::Ptr psock,int event)
{
	auto modEvent = [=](){
		if(eventMap_.find(psock) == eventMap_.end())
		{
			return;
		}
		int flag = toEPollEvent(handler->interest() | event);
		epollEventMap_[psock->fd()]->events = flag;
		int r = epoll_ctl(epollfd_,EPOLL_CTL_MOD,psock->fd(),epollEventMap_[psock->fd()].get());
		if(r < 0)
		{
			return;
		}
		eventMap_[psock]->enableEvent(event);
	};
	if(tid_ == std::this_thread::get_id())
	{
		modEvent();
	}
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back(modEvent);
	}
	wakeup();
}

void xEpollLoop::delEventHandler(xsock::Ptr psock)
{
	{
		std::lock_guard<std::mutex> guard(mtx_);
		functors_.push_back([=](){ 
			if(eventMap_.find(psock) == eventMap_.end())
				return;
			
			int r = epoll_ctl(epollfd_,EPOLL_CTL_DEL,psock->fd(),epollEventMap_[psock->fd()].get());
			if(r < 0)
			{
				return;
			}
			eventMap_.erase(psock);
			epollEventMap_.erase(psock->fd());
		});
	}
	wakeup();
}

void xEpollLoop::loop()
{
	std::shared_ptr<epoll_event> events(new epoll_event[epollEventMap_.size()],[](epoll_event* ptr){delete[] ptr;})
	int nready = epoll_wait(epollfd, events.get(), epollEventMap_.size(), 10000);
	if(nready <= 0)
	{
		return; //TODO
	}

	for(int i; i < nready; i++)
	{
		if(events[i].data.fd == wakeup_.rfd())
		{
			char buf[32] = {0};
        	wakeup_.read(buf,32);
		}
		int revents = 0;
		if(events[i].events & EPOLLIN)
		{
			revents |= EV_READ;
		}
		if(events[i].events & EPOLLOUT)
		{
			revents |= EV_WRITE;
		}
		if(events[i].events & EPOLLERR)
		{
			revents |= EV_ERROR;
		}
		int tmpfd = events[i].data.fd;
		auto sockItem = std::find_if(eventMap_.begin(),eventMap_.end(),[tmpfd](decltype(eventMap_)::value_type v) { return tmpfd == v.first->fd(); });
		if(sockItem == eventMap_.end())
		{
			continue;
		}
		sockItem->second->handlerEvent(revents);
	}

	decltype(functors_) tempfuncs;
	{
		std::lock_guard<std::mutex> guard(mtx_);
		tempfuncs = std::move(functors_);
	}
	for(auto func : tempfuncs)
	{
		func();
	}
}

