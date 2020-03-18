#include "../../libco/co_routine.h"
#include "../xsock.h"




void* x_routine(void* args)
{
    xloop* xl = (xloop*)args;
    xl->run();
}

int main()
{
    co_enable_hook_sys();
    auto server = std::make_shared<xsvrsock>();
    server->opensvr("0.0.0.0",9999);
    server->listen();
    auto xl = new xloop();
    xl->addEventCallBack(server,EV_READ,[xl](xsock::Ptr psock,int event){
                if(event == EV_READ)
                {
                    auto client = psock->accept();
                    xl->addEventCallBack(client,EV_READ,[xl](decltype(client) pclient,int event) {
                                std::string clientip = "";
                                uint16_t port = 0;
                                pclient->getClient(clientip,port);
                                if(event == EV_READ)
                                {
                                    char buf[1024] = {0};
                                    int ret = pclient->recvBytes(buf,1024);
                                    if(ret <= 0)
                                    {
                                        xl->delEventHandler(pclient);
                                        xlog("client(%s:%d):close!!! %s",clientip.c_str(),port);
                                    }
                                    else
                                    {
                                        xlog("client(%s:%d):recv %s",clientip.c_str(),port,buf);
                                        if("quit" == std::string(buf))
                                        {
                                            xl->stop();
                                        }
                                    }
                                }
                        
                        });
                }
                
            });
    
        stCoRoutine_t *co = nullptr;
        co_create(&co,NULL,x_routine,xl);
        xlog("routine ....");
        co_resume(co);

        co_eventloop(co_get_epoll_ct(),0,0);

        return 0;
}
