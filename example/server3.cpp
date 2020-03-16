#include "../xsock.h"
#include <errno.h>

int main()
{
	auto xs = std::make_shared<xsvrsock>();
	int ret = xs->opensvr("0.0.0.0",9999);
	if(ret != 0)
	{
		printf("bind failed socket error %d,%d\n",xs->sockError(),errno);
		return 0;
	}
	xs->listen();
	xloop xl;
	xloop* pxl = &xl;
	xl.addEventCallBack(xs,EV_READ,[pxl](xsock::Ptr psock,int event){
				std::string ip = "";
				uint16_t port = 0;
				auto sc = psock->accept(ip, port);
				printf("new connection coming (%s:%d)\n",ip.c_str(),port);
				pxl->addEventCallBack(sc,EV_READ,[pxl](decltype(sc) psock,int event){
						char buf[64] = "";
						int len = psock->recvBytes(buf,64);
						if(len == 0 )
						{
							printf("close connection; %d\n",psock->sockError());
							//pxl->delEventHandler(psock);
							psock->close();
							return ;
						}
						else if(len < 0)
						{
							pxl->delEventHandler(psock);
							printf("error(%d %d)\n",psock->sockError(),errno);
							return;
						}
						printf("recv:%s,%d\n",buf,len);
						return;		
				});
		});
	xl.run();
}
