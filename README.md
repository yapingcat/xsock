# xsock
````
  使用c++11 按照Reactor模式 开发的一个网络库，仅供参考
````

# build 
  use ./build.sh to build libxsock.a
 
# build example
````
  g++ -std=c++11 -DHAVE_POLL server3.cpp   -L../ -lxsock -lpthread
  ./a.out
````
# c++ example
```C++
  auto xs = std::make_shared<xsvrsock>();
  xs->opensvr("0.0.0.0",9999);
  xs->listen();
  xloop xl;
  xloop* pxl = &xl;
  std::thread t1(std::bind(&xloop::run,pxl));
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
 t1.join();

```
