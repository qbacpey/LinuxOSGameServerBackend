# 项目报告

项目整体技术架构如图所示：

![](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/技术架构.drawio.png)

项目整体架构前后端分离，使用长连接、有状态、双向、全双工的WebSocket技术作为应用层协议基础，并采用心跳机制同步各客户端状态。前端采用Vue、ElementUI作为UI呈现；后端采用epoll结合多线程，实现非阻塞式IO服务器。

## 功能点说明

### 房间相关

用户进入房间系统界面之后，浏览器就会和服务器建立WebSocket连接：

![image-20221227214447499](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214447499.png)

这里为了演示项目是可以在互联网上运行的，打开了两个浏览器窗口，服务器也就分别与这两个用户建立了连接了：

![image-20221227214515717](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214515717.png)

#### 创建房间

连接建立后，任何用户都可以点击这里的创建房间按钮，输入房间名称，创建一个游戏房间：

![image-20221227214647288](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214647288.png)

此时，连接到服务器的其他用户就会收到一条广播消息，告知他们有新房间被创建了：

![image-20221227214712179](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214712179.png)

#### 加入房间

具体来说，右侧这里已经出现了一个可加入房间，也就是刚才我在左侧窗口创建的房间。此时点击加入房间按钮，进入房间后，房主和房客的界面都会显示这个房间的信息：

![image-20221227214806766](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214806766.png)

#### 开启游戏

同时，房主的界面除了会显示房客的信息，开启按钮游戏也会变为可点击状态。这时房主就可以点击该按钮，向服务器发送开启游戏请求，房主和房客也会同时进入到游戏界面：

![image-20221227214904685](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214904685.png)

### 游戏状态同步

游戏本身是一个乒乓球游戏，游玩方式和世界上的第一款家用电子游戏《Pong》基本一致，两个玩家分别操控左右两块挡板，去打中间运动的球，让对方接不到球的一方得分。其中，由玩家所操作的球拍颜色是红色，对手的球拍就是白色：

![image-20221227214939300](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227214939300.png)

浏览器绘制每一帧的时候都会将玩家球拍的位置发送给服务器，再由服务器转发给对手，进而实现对局双方的状态同步

## 相关技术

由于现在市面上的浏览器前端技术生态较为成熟，相比于GUI库（如Qt、Electron、curses）而言更容易写出美观简洁的UI界面。同时由于本项目需要在互联网上联机游玩，使用前端技术实现客户端会更为简洁，而且由于项目规模较小，使用GUI带来的性能提升并不明显。因此本项目采用了Vue+ElementUI作为客户端UI框架，用户需通过浏览器游玩此项目。

### 服务器所用技术

在介绍服务器所用技术之前，我想先介绍一下服务器的启动以及运行流程：

1. 主线程执行各项初始化工作（如，配置网络、绑定并监听指定端口的连接）；
2. 主线程创建epoll实例，将指定端口的文件描述符添加到epoll的兴趣列表（**Edge Triggered**）；
3. 主线程创建`THREAD_NUM`条线程（将`epoll_fd`以及`listen_fd`作为线程运行参数），随后主线程以及其他线程都将作为请求处理线程存在；
4. 所有线程调用`epoll_wait`等待epoll事件；
5. 当任何事件被捕获时，负责处理该事件的线程将执行如下操作：
   1. 如果事件的文件描述符是`listen_fd`，代表需要建立新连接。线程会调用`accept`并将`conn_fd`添加到epoll的兴趣列表；
   2. 如果事件的文件描述符不是`listen_fd`，代表是数据传输事件。线程需要根据保存在`event.data.ptr`里的事件状态，执行不同的行为；
   3. 如果线程在读取或写入数据时，触发`EAGAIN`事件的同时还有剩余需要读写的数据（原因可能时socket暂时不可用），那么线程会将此刻请求处理状态（`WebSocketStatus`）保存到`event.data.ptr`中，以便在下次socket可用时继续进行数据读写；
   4. 最后，如果事件处理完毕，那么需要使用`epoll_ctl`将此事件再次添加到兴趣列表中；

上述最复杂的问题在于，如何正确地让WebSocket、非阻塞IO、多线程系统三者能协作起来：

1. WebSocket：无论是服务端还是客户端都可以主动发送数据，任何请求也不意味着有响应；
   1. 数据传输方式获得极大自由的同时，需要考虑的情况也变多了，比如说现在服务器不需要等客户端发送请求就进行数据的发送，而是可以主动进行数据的传输操作，要求前端先处理发送过来的数据；
2. 非阻塞IO：任何IO操作在执行时都有可能会被中断，必须要正确实现断点续传的逻辑；
   1. 多次WebSocket请求可能内核合并作为一次epoll事件发出来，同时这些请求也不一定是整数，也就是说，有可能触发epoll事件，输入中还留有一半上次未处理完全的请求被传输过来了；
3. 多线程系统：前后处理同一个IO请求的线程可能不是同一个线程。就某次请求而言，如果该请求的IO因为某些原因被阻塞了，那么就涉及到两个方面的问题：
   1. 前一线程需要将连接状态妥善保存，以便后续线程在未来能接替它未完成的工作；
   2. 后一线程需要正确恢复已保存的连接状态，以便接替前一线程的工作；

#### 非阻塞IO

使用非阻塞IO的难点在于IO可能会在任何时候被中断，因此就需要保存并恢复连接状态。由于可能同时触发读取事件和写入事件（全双工），两者任一都可能在执行时被中断。因此需要同时在`WebSocketStatus`中保存读取事件状态和写入事件状态，以便程序能恢复上一次的执行状态，`WebSocketStatus`具体定义如下：

~~~c++
struct WebSocketStatus {
    int conn_fd; // 连接的文件描述符
    
    std::string read; // 已经读取到的数据，是字符串
    std::string::size_type read_n; // 已读取的数据
    std::string::size_type total_n; // 总体需要读取的数据
    ReadStatus read_status; // 请求读取情况

    std::queue<std::string> responses; // 等待写入的请求
    std::string::size_type write_n; // 当前请求写入情况
    WriteStatus write_status; // 请求写入情况
};
~~~

程序接收到epoll事件或者想要写入数据的时候，都会检查连接对应的此结构体，以便继续执行上次未执行完毕的读取或写入操作

##### 输入事件处理

非阻塞IO的读取操作可能在任意时刻被中断，使数据的读取操作不再连续。因此需要在读取操作被中断时，保存当前数据状态，以便实现“断点续传”。

考虑到数据的读取具有连续性（报文头或报文体都必须整体读取出来再对其解析），因此程序读取报文头和报文体的时候，需要先把他们整体读取到字符串中，再调用相关函数解析这个字符串

具体来说，本项目的数据读取状态分为三种：

~~~c++
enum class ReadStatus{
    kReadingHeader, // 读取WebSocket头部时被中断
    kReadingBody, // 读取WebSocket体时被中断
    kClear // 上一请求完整读取
};
~~~

具体处理逻辑如下所示，如果是前两种情况（`kReadingHeader`，`kReadingBody`），那么需要从上次被中断的位置继续读取数据。而如果是`kClear`，那么直接读取其中内容即可：

~~~c++
if (event.events & EPOLLIN)
{
    switch (status->read_status)
    {
    case ReadStatus::kClear:
    {
        // 上次读取无数据剩余，可直接读取请求
        if ((status->read_n = ReadUntillCountOrError(status->read, MAX_HEADER)) != MAX_HEADER)
        {
            // 头部未读取完，标记状态为kReadingHeader，下次继续读取
            status->read_status = ReadStatus::kReadingHeader;
            break;
        }
        // 头部读取完毕，解析头部，获取WebSocket数据体长度
        int payload_length = ParseWebSocketHeader(status->read);
        status->read.clear();
        status->read_n = 0;
        // 读取数据体
        if ((status->read_n = ReadUntillCountOrError(status->read, payload_length)) != payload_length)
        {
            // 数据体未读取完，标记状态为kReadingBody，下次继续读取
            status->read_status = ReadStatus::kReadingBody;
            status->total_n = payload_length;
            break;
        }
        // 数据体读取完毕，处理请求
        ProressRequest(&event, status->read.c_str());
        status->read_n = 0;
        status->read.clear();
        status->read_status = ReadStatus::kClear;
        break;
    }
    case ReadStatus::kReadingHeader:
    {
        // 上次在读取头部的时候被中断了，需要继续读取头部数据
        int left_header = MAX_HEADER - status->read_n;
        int new_read_data = 0;
        if ((new_read_data = ReadUntillCountOrError(status->read, left_header)) != left_header)
        {
            // 头部未读取完，标记状态为kReadingHeader，下次继续读取
            status->read_n += new_read_data;
            status->read_status = ReadStatus::kReadingHeader;
            break;
        }
        // 头部读取完毕，解析头部，获取WebSocket数据体长度
        int payload_length = ParseWebSocketHeader(status->read);
        status->read.clear();
        status->read_n = 0;
        // 读取数据体
        if ((status->read_n = ReadUntillCountOrError(status->read, payload_length)) != payload_length)
        {
            // 数据体未读取完，标记状态为kReadingBody，下次继续读取
            status->read_status = ReadStatus::kReadingBody;
            status->total_n = payload_length;
            break;
        }
        // 数据体读取完毕，处理请求
        ProressRequest(&event, status->read.c_str());
        status->read_n = 0;
        status->read.clear();
        status->read_status = ReadStatus::kClear;
        break;
    }
    case ReadStatus::kReadingBody:
    {
        // 上次在读取数据体的时候被中断了，需要继续读取数据体
        int left_body = status->total_n - status->read_n;
        int new_read_data = 0;
        if ((new_read_data = ReadUntillCountOrError(status->read, left_body)) != left_body)
        {
            // 头部未读取完，标记状态为kReadingHeader，下次继续读取
            status->read_n += new_read_data;
            status->read_status = ReadStatus::kReadingBody;
            break;
        }
        // 数据体读取完毕，处理请求
        ProressRequest(&event, status->read.c_str());
        status->read_n = 0;
        status->read.clear();
        status->read_status = ReadStatus::kClear;
        break;
    }
    default:
    {
        perror("输入事件不合法");
        break;
    }
    }
}
~~~

上述代码其实就是根据保存在`WebSocketStatus`的上次读取状态，确定是否需要继续上次的读取操作，同时对读取结果执行合适的操作：

1. 如果是需要继续读取WebSocket头部的话，那么需要读取头部中的`payload`字段，以确定数据体长度；
2. 如果是需要继续读取WebSocket数据体的话，那么需要根据之前读取到的数据体长度，读取数据体；

数据体读取完毕之后（读取到`WebSocketStatus`的`read`字符串中），就可以调用相关函数处理该输入事件了

此外，考虑到有可能触发`EPOLLIN`事件的时候，已经有多个WebSocket请求积压在`conn_fd`中，因此外部需要套一个循环，直到程序能确定再没有任何输入为止

##### 输出事件处理

需要处理输出事件的场景实际只有一种：程序上次试图向`conn_fd`中写入数据的途中被阻塞（大多是由于写入缓存区满了，同时对端没有读取数据），导致数据写入到半途就无法继续写入了。因此和上文提到的读取操作类似，这也需要程序对写入状态进行保存。下为程序中所使用的两种写入状态：

~~~c++
enum class WriteStatus{
    kWriting, // 写入WebSocket时被中断
    kClear // 上次写入操作完成
};
~~~

程序使用了`WebSocketStatus`的`std::queue<std::string> responses`用来缓存等待输出的数据，每次输出可用的时候，程序都会尽可能将`responses`中的数据全部写入对端：

1. 如果在写入途中被中断了，那么需要将`WriteStatus`状态标记为`kWriting`，同时标记`EPOLLOUT`事件，确保下次输入可用时程序能接到通知进而可以继续写入数据；
2. 如果没有被中断，那么代表输出清空，无需标记`EPOLLOUT`事件，`WriteStatus`状态也可以标记为`kClear`；

也就是说，如果程序想要写入数据的时候，发现对应的`conn_fd`处于` kWriting`，那么就代表现在处于写入操作不可用状态，就需要把待输入数据排入`WebSocketStatus`的`std::queue<std::string> responses`中。等到输入可用时，程序会先进先出式地将`responses`队列中的数据发送到对端

总体而言，和写入相关的操作大致可以归结如下：

1. 程序写入数据全程没有受到阻碍，没有在数据写入的半途被打断。因此也就不需要保存写入状态，不需要告诉epoll：“下次写入操作可用的时候通知我”，也就是不需要标记`EPOLLOUT`事件；
2. 相对的，如果程序在写入时受到阻碍，在数据写入的半途被打断了。那么就需要保存写入状态，需要告诉epoll：“下次写入操作可用的时候通知我”。那么下次写入操作变得可用的时候，epoll就会通知程序，程序就可以继续断点续联未完成的写入操作了；
3. 只有在没有任何数据需要写入的时候（`responses`为空），才能将输入状态标记为`kClear`；

##### 事件联合处理

考虑到有可能同时触发`EPOLLIN`和`EPOLLOUT`事件，因此接受到epoll通知的时候，程序需要同时检查两种事件是否同时存在

#### epoll

epoll核心的API只有三个：`epoll_create1`, `epoll_ctl`, `epoll_wait`，其中的`epoll_ctl`是用来设置epoll兴趣列表的

执行时，首先使用`epoll_create`创建`epoll`实例：

~~~c++
int epoll_fd = epoll_create(MAX_EVENTS);
~~~

然后可以使用类似如下的代码段将指定文件描述符添加到兴趣列表中，同时可以通过设置其`ev.events`限定触发什么事件的时候，epoll才会向程序发出通知：

~~~c++
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
ev.data.fd = fd_to_monitor;

epoll_ctl(epollfd, EPOLL_CTL_ADD, fd_to_monitor, &ev);
~~~

最后就是调用`epoll_wait`捕获并等待相应事件触发之后，对该事件进行处理：

~~~c++
struct epoll_event *events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * MAX_EVENTS);

int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

if (nfds <= 0) {
    perror("epoll_wait");
    continue;
}
for (int n = 0; n < nfds; ++n) {
    consume(events[n].data.fd);
}
~~~

注意，`ev.data`是一个联合体，对其进行不同的数据赋值会出现相互覆盖的现象

##### 多线程与epoll

在线程间共享同一个epoll文件描述符是完全可行的，epoll在设计上就考虑到了这一点，本身可以在多线程系统中使用epoll。相对的，如果在每个线程中都创建一个epoll文件描述符，那么每个线程都将只是处理自己所负责的epoll实例中的事件，也就不能充分利用多线程系统带来的并发性。(因为现在每个线程只是处理自己所负责的epoll实例中的事件，而这些工作不能根据线程的繁忙程度在线程之间进行分配）。

因此程序可以只创建一个epoll实例，同时将这个实例的文件描述符作为线程参数传递给各个线程：

~~~c++
struct thread_args {
    int listen_fd;
    int epoll_fd;
};
~~~

随后就可以在各个线程中，对这同一个`epoll_fd`调用`epoll_wait`了。尽管`epoll_wait`是线程安全的，但是为了防止事件发生的时候，系统中所有的线程都被唤醒（尽管处理这一事件只需要一个线程），需要使用`EPOLLET`模式来防止出现这一问题

##### EPOLLONESHOT

除了上述提到的问题以外，还需要注意使用`EPOLLONESHOT`模式：

> EPOLLONESHOT:
>
> Sets the one-shot behavior for the associated file descriptor. This means that after an event is pulled out with epoll_wait(2) the associated file descriptor is internally disabled and no other events will be reported by the epoll interface. The user must call epoll_ctl() with EPOLL_CTL_MOD to rearm the file descriptor with a new event mask.

如果不使用`EPOLLONESHOT`，那么发生在同一个文件描述符上的两个连续事件可能会在同时被两个不同的线程所处理，导致连接状态不能被正确恢复，进而引发各种不可预知的问题

最后，如果使用了`EPOLLONESHOT`，需要在事件处理完毕之后调用`epoll_ctl`将该文件描述符重新激活

#### 多线程系统

##### 线程间通讯

本程序的线程间通讯主要借助线程安全的`epoll_wait`，通过`WebSocket`间接实现的（后一线程可以通过读取该结构体中的数据接替前一线程的工作）

##### 线程间同步

本程序除了在epoll中有线程，还有一个地方需要线程同步操作。玩家ID是一个全局变量，获取新ID之前，线程需要先获取锁再获取此变量：

~~~c++
int server::get_new_global_room_id()
{
    pthread_mutex_lock(&mutex);
    int result = ++global_room_id;
    pthread_mutex_unlock(&mutex);
    return result;
}
~~~

#### 硬件调优

首先使用`lscpu`查看虚拟机硬件情况：

<img src="https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227140836780.png" alt="image-20221227140836780" style="zoom:50%;" />

结果显示，虚拟机核心数为2，有两个On-chip的L1Cache，其余L2和L3Cache在核心间共享

##### 线程数配置

线程数配置的核心实际是让CPU-Bound task和IO-Bound task相互覆盖的同时，不至于产生过量的上下文切换开销。为此程序生成的线程数应稍大于核心数，程序最后的选择是创建4个线程。（不过，具体的取值理应使用相关的Benchmark测试获知，不过个人并不清楚一个合适的Benchmark如何设计，所以就测试了一下选了一个理论上合适的值）

##### Cache优化

执行`getconf LEVEL2_DCACHE_LINESIZE`获知机器Cache line的大小为64Byte：

![image-20221227162200659](https://github.com/qbacpey/LinuxOSGameServerBackend/blob/master/项目报告.assets/image-20221227162200659.png)

为此，程序中的所有Buffer的大小均设置为64或者64的倍数，以便程序读取数据时能恰好将其放到一个Cache line中，减少内存访问开销

#### cJSON

cJSON是一个使用C语言编写的JSON数据解析器，具有超轻便，可移植，单文件的特点，使用MIT开源协议。本系统主要使用此库来解析以及构建JSON数据体

### 前端技术

由于本课程重点并不是客户端（下简称“前端”）编程，而是服务端（下简称“后端”）编程，因此这里略去UI相关技术（Vue、ElementUI）的说明，只说明前后端通讯所需技术：

1. WebSocket；
2. 心跳同步；
3. 应用层协议；

#### WebSocket

WebSocket作为有连接的协议，通讯过程分为三步：

1. 两端握手，建立连接；
2. 连接两端通讯，任何一方都可主动发送消息，无需响应对方发送的消息；
3. 任何一端终止链接，WebSocket连接关闭；

本项目选用WebSocket作为用户层协议基础的原因有两个：

1. 本项目需要频繁在各端同步数据，每次数据收发总量小但是整体数量大，使用长连接协议能减少IO开销。如选用HTTP作为用户层协议基础，那么需要频繁开闭连接，无疑会增加巨量的IO开销（虽然HTTP也可以保持连接，允许多个请求重用单个连接，但通常会有小的超时时间来控制资源消耗。对于游戏这种高时效的应用场景来说，这一特性带来的问题是致命的）；
2. WebSocket是全双工的，任何一方可以主动发送数据，同时也不需要响应另一方的数据请求，极为便利；

##### 建立连接

用户进入房间页面时，浏览器即会调用此函数建立WebSocket连接并设置消息接收时的回调：

~~~javascript
  INIT_WEBSOCKET: (state) => {
    state.webSocket = new WebSocket('ws://127.0.0.1:8008')
    // WebSocket开启时回调
    state.webSocket.onopen = () => {
      console.log('Info: WebSocket connection opened.')
    }
    // WebSocket关闭时回调
    state.webSocket.onclose = () => {
      console.log('Info: WebSocket closed.')
    }
    // WebSocket错误时回调
    state.webSocket.onerror = function (msg) {
      console.error(msg)
    }
    // WebSocket接收到消息时回调
    state.webSocket.onmessage = (message) => {
      onMessageReceive(message, state)
    }
~~~

#### 应用层协议

由于WebSocket没有像HTTP一样的URL机制，因此需要用户自己在WebSocket数据体中添加请求标识符，这样前端和后端才能识别此WebSocket对应什么请求。

##### 前端发送消息格式

前端如果需要向后端发送消息的话，会将请求体统一格式为如下：

~~~js
const request = {
  type: RoomURL.XXX,
  body: {
    ...JSON 对象...
  }
}
~~~

上述`type`字段用于标识请求类型，备选请求如下：

~~~js
export const RoomURL = {
  CreateRoom: 0,
  JoinRoom: 1,
  BroadcastRoom: 2,
  StartGame: 3,
  Heartbeat: 4,
  Login: 5,
  UpdateRoomState: 6
}
~~~

后端接收到WebSocket时便会通过读取`type`字段的值（接收时类型为`int`），将数值与请求类型对应，进而正确识别并处理请求内容（`body`）

##### 后端发送消息格式

相比于前端，由于后端构建JSON格式数据时需要进行较为复杂的操作，因此格式不太统一，不过大致也为如下格式：

~~~js
const response = {  
    type: RoomURL.XXX,  
    yyy1: zzz1,
    ...
}
~~~

简单来说，其中不一定有`body`作为响应数据的包装，不过必然有`type`字段，以便前端可以识别WebSocket数据体

#### 心跳同步

本项目的游戏部分采用类似“心跳”机制进行数据同步，具体来说浏览器在绘制每一帧图形界面的时候，都会采用预先建立好的WebSocket连接，将自己的状态同步到服务器，再由服务器将对局数据广播到同房间其他玩家的客户端里：

1. 其中一方调用`heartbeat`将自己的位置发送到服务器端：

   ~~~js
   // anotherPlayerBody是自己的位置，JSON对象
   async heartbeat({commit}, myBody) {
     // 自己发出Heartbeat是这个URL，接收也是一样
     const request = {
       type: RoomURL.Heartbeat,
       body: myBody
     }
     await commit('SEND_JSON', request)
   }
   ~~~

2. 服务器接收并处理此请求，同时将接收到的数据发送到同局其他玩家的客户端里：

   ~~~c++
   // 向对手心跳
   void ResponseHeartbeat(int target_id, cJSON *body)
   {
       printf("target_id:%d\n", target_id);
       cJSON *response = cJSON_CreateObject();
       cJSON_AddNumberToObject(response, "type", double(server::RoomURL::kHeartbeat));
       cJSON_AddItemToObject(response, "body", body);
       if (send_msg(target_id, cJSON_PrintUnformatted(response)) == -1)
       {
           perror("ResponseHeartbeat fail!");
           exit(-1);
       }
   }
   ~~~

3. 其他玩家的客户端接收到服务端转发的心跳请求，将其状态同步到自己的组件中：

   ~~~c++
   case RoomURL.Heartbeat: {
     ...
     break
   }
   ~~~

## 应用层协议

本项目定义的应用层协议如下：

~~~js
/**
 * 服务器相应的room对象统一为：
 *     {
 *         room_id: -1,
 *         room_name: '',
 *         count: 0, // 最大为2
 *         host_id: -1,
 *         guest_id: -1,
 *         state: RoomState.Waiting/RoomState.Gaming
 *     }
 *
 * CreateRoom的执行逻辑如下：
 * 1.浏览器发送CreateRoom，请求体中只有新房间的名字
 * 2.服务端响应新房间信息，响应为如下
 *     const respond = {
 *       type: RoomURL.CreateRoom,
 *       room: {..room对象..}
 *     }
 *
 * JoinRoom的执行逻辑如下：
 * 1.房客浏览器发送JoinRoom，请求体为：
 *     const request = {
 *       type: RoomURL.JoinRoom,
 *       body: {
 *         room_id: room_id
 *       }
 *     }
 * 2.服务端响应新房间信息，响应为如下
 *     const respond = {
 *       type: RoomURL.JoinRoom,
 *       room: {..room对象..}
 *     }
 *
 * 只有房主能发送StartGame请求，请求体如下：
 *     const request = {
 *       type: RoomURL.StartGame
 *       body: {
 *         room_id: room_id
 *       }
 *     }
 * 服务器接收到对应请求之后，会将此响应体发送给房客：
 *     const request = {
 *       type: RoomURL.StartGame,
 *       room_id: 0
 *     }
 * 设置完本地状态之后，就会跳到游戏页面
 *
 * BroadcastRoom 的响应体为
 *
 *     const response = {
 *       type: RoomURL.BroadcastRoom,
 *       rooms: [{room对象} ,{room对象}, {room对象}]
 *     }
 *
 * Login请求是开启WebSocket链接的时候服务器的响应内容，响应体为：
 *     const response = {
 *       type: RoomURL.Login,
 *       host_id: 0
 *     }
 *
 * UpdateRoomState请求是房客加入游戏之后，用来更新**房主**currentRoom的函数
 * 响应如下：
 *     const response = {
 *       type: RoomURL.UpdateRoomState,
 *       room: {..room对象..}
 *     }
 *
 * Heartbeat也分为两个部分，不过无论是收还是发，JSON体格式都是这样：
 * const response = {
 *   type: RoomURL.Heartbeat,
 *   body: {
 *         room_id: 0,
 *         target_id: 0,
 *         positionY: 0
 *   }
 * }
 * 发送的时候将自己的球拍对象发过去
 * 接收的时候使用接收到的球拍对象更新对方的球拍对象
 * */
~~~

## 压力测试

使用10条线程，在建立连接，发送100次请求，得到测试结果如下：

| Label                      | 吞吐量   | 接收 KB/sec | 发送 KB/sec | 平均字节数 |
| -------------------------- | -------- | ----------- | ----------- | ---------- |
| WebSocket request-response | 49.75124 | 9.52        | 2.53        | 196.0      |

