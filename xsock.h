#ifndef X_SOCK_H
#define X_SOCK_H

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <stdio.h>

#define xlog(...)  do { fprintf(stdout,__VA_ARGS__); fprintf(stdout,"\n");fflush(stdout);}while(0);
#define xloge(...)  do { fprintf(stderr,__VA_ARGS__); fprintf(stdout,"\n");fflush(stderr);}while(0);

class xsocketApi
{
public:
	static int createTcpSocket();
	static int createServerSocket(const std::string& ip, uint16_t port);
	static int acceptClient(int server,std::string& ip, uint16_t& port);
	static int acceptClient(int server);
	static int listen(int sc,int backlog);
	static int bind(int sc,const std::string& ip, uint16_t port);
	static int connect(int sc,const std::string& ip,uint16_t port);
	static int setReuseOption(int sc);
	static int setBlock(int sc,int block);
	static int setSocketOption(int sc,int level,int option,int value);
	static int getSocketOption(int sc,int level,int option,int& value);
	static int close(int sc);
    static int getPeerName(int sc, std::string& ip,uint16_t& port);
};

enum
{
	EV_READ = 0x01,
	EV_WRITE = 0x02,
	EV_ERROR = 0x04,
	EV_ALL = 0x07,
};

class pipe
{
public:
	pipe() = default;
	~pipe();
    pipe(const pipe&) = delete;
    pipe& operator=(const pipe&) = delete;
    
public:
	int init();
	int rfd();
	int wfd();
	int read(char* buf, int size);
	int write(const char* buf, int size);
private:
    int pipefd_[2] = {0};
};

class xsock
{
public:
	typedef std::shared_ptr<xsock> Ptr;
    enum { invalid_xsock = -1};

public:
	xsock();
	xsock(int fd);
	virtual ~xsock();
	xsock(const xsock&) = delete;
	xsock& operator=(const xsock&) = delete;

public:
	int connect(const std::string& ip,uint16_t port);
	int bind(const std::string&ip,uint16_t port);
	int listen(int backlog = 64);
	xsock::Ptr accept(std::string& ip, uint16_t& port);
	xsock::Ptr accept();
	int close();
	int sendBytes(const std::string& bytes,uint32_t size);
	int sendBytes(const char* bytes,uint32_t size);
	int recvBytes(char* buf,uint32_t size);
	int fd();	
	int sockError();
	int setReuseAddr();
	int setReusePort();
	int setBlock(int flag);
	bool isBlock();
	int setSocketOption(int option,int value);
	int getSocketOption(int option,int &value);
    int getClient(std::string& ip,uint16_t& port);

private:
	void init();

protected:
	int fd_;
private:
	bool block_;
};

class xsvrsock: public xsock
{
public:
	typedef std::shared_ptr<xsvrsock> Ptr;
public:
	xsvrsock() = default;
	~xsvrsock() = default;
	
public:
	int opensvr(const std::string&ip, uint16_t port);
};

class EventHandler
{
public:
	typedef std::shared_ptr<EventHandler> Ptr;
public:
	EventHandler();
	virtual ~EventHandler() = default;
public:
	void attachHandler(xsock::Ptr handler);
	void detachHandler();
	void enableEvent(int event);
	void disableEvent(int event);
	int interest();
	void handlerEvent(int event);
	xsock::Ptr sock() const;

protected:
	virtual void onRecv();
	virtual void onSend();
	virtual void onError();
protected:
	int event_;
	xsock::Ptr sock_;	
};

typedef std::function<void(xsock::Ptr,int)> XEventCB;
class xloop
{
public:
	xloop();
	~xloop();
	
	xloop(const xloop&) = delete;
	xloop& operator=(const xloop&) = delete;

public:
	void run();
	void stop();
 	void addEventHandler(xsock::Ptr psock, EventHandler::Ptr handler);	
	void addEventCallBack(xsock::Ptr psock,int event,const XEventCB ecb);
	void unregisterEvent(xsock::Ptr psock,int event);
	void registerEvent(xsock::Ptr psock,int event);
	void delEventHandler(xsock::Ptr psock);

private:
	void run_poll();
	void run_select();
	void run_epoll();
	void wakeup();
private:

	std::unordered_map<xsock::Ptr,EventHandler::Ptr> eventMap_;
	std::vector<std::function<void()>> functors_;
	std::mutex mtx_;
	std::thread::id tid_;
	bool quit_;
	pipe wakeup_;
};



#endif
