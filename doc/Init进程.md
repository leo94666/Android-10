what why how

## Init进程

Android系统是基于Linux系统开发的，和Linux系统一样，Android的Linux内核启动后，Linux内核会创建第一个用户级进程**init**，


所以Android启动将由Linux Kernel启动并创建init进程。Init进程是所有用户空间的鼻祖。

Init进程是Android启动过程中在Linux系统中用户空间的第一个进程。
init启动入口是在它的SecondStageMain方法中。但调用init的SecondStageMain方法是通过main.cpp中的main方法进行的。

```shell
adb shell ps | grep init
```

```cmake

```

![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b5a61734191b4407a65fa9ab6f9b3992~tplv-k3u1fbpfcp-watermark.image?)

在init进程启动的过程中，会相继启动servicemanager(binder服务管理者)、Zygote进程(java进程)。而Zygote又会创建system_server进程以及app进程。



app进程是由Zygote进程通过fork创建出来的。

