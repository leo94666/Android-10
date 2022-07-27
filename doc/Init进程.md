what why how


# Init进程

注意事项

- 基于Android 10源码

## 什么是Init进程？

Android系统是基于Linux系统开发的，和Linux系统一样，Android的Linux内核启动后，Linux内核会创建第一个用户级进程**init**，


Android可以通过以下方式来查看Android内的**init**进程

```shell
adb shell ps | grep init
```

![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/b5a61734191b4407a65fa9ab6f9b3992~tplv-k3u1fbpfcp-watermark.image?)

- **Init**进程的特点

> - Init进程是所有用户空间的鼻祖 , PID = 1 
> - Init进程是Android启动过程中在Linux系统中用户空间的第一个进程
> - init进程是由内核启动的用户级进程


-----

## Init进程是怎么工作的

init进程的入口main函数不再在init.cpp,而是main.cpp,内容如下

```c++
int main(int argc, char** argv) {
#if __has_feature(address_sanitizer)
    __asan_set_error_report_callback(AsanReportCallback);
#endif
    //当argv[0]的内容为ueventd时，执行ueventd_main，ueventd主要是负责设备节点的创建、权限设定等一些列工作
    if (!strcmp(basename(argv[0]), "ueventd")) {
        return ueventd_main(argc, argv);
    }
    //当传入的参数个数大于1时，执行下面几个操作
    if (argc > 1) {
        //参数为subcontext，初始化日志系统
        if (!strcmp(argv[1], "subcontext")) {
            android::base::InitLogging(argv, &android::base::KernelLogger);
            const BuiltinFunctionMap function_map;

            return SubcontextMain(argc, argv, &function_map);
        }
        //参数为selinux_setup，启动SeLinux安全策略，创建增强型Linux
        if (!strcmp(argv[1], "selinux_setup")) {
            return SetupSelinux(argv);
        }
        //参数为second_stage，启动init进程第二阶段
        if (!strcmp(argv[1], "second_stage")) {
            return SecondStageMain(argc, argv);
        }
    }
    //默认启动第一阶段，挂载相关文件系统
    return FirstStageMain(argc, argv);
}
```
**main.cpp** 

- 1.FirstStateMain
- 2.SetupSelinux
- 3.SecondStageMain

**main.cpp** 根据传入的参数进入不同的操作流程, 流程如下

- 当argv[0]的内容为ueventd时，执行ueventd_main，ueventd主要是负责设备节点的创建、权限设定等一些列工作
- 当传入的参数 > 1的时候
  - 参数为subcontext，即初始化Android日志系统，并执行SubcontexMain主函数
  - 参数为selinux_setup, 即执行SetupSelinux()主函数, 创建增强型Linux
  - 参数为second_state, 即进入SecondStateMain主函数，**重点**
- 默认启动FirstStateMain主函数


接下来主要介绍 **FirstStateMain** 和 **SecondStageMain**

### FirstStateMain

该部分的主要工作是挂载相关文件系统，主要源码如下

```c++
int FirstStageMain(int argc, char** argv) {
    //将各种信号量，如SIGABRT、SIGBUG等行为设置为SA_RESTART,一旦监听到这些信号即执行重启函数
    if (REBOOT_BOOTLOADER_ON_PANIC) {
        InstallRebootSignalHandlers();
    }
    boot_clock::time_point start_time = boot_clock::now();
    std::vector<std::pair<std::string, int>> errors;
#define CHECKCALL(x) \
    if (x != 0) errors.emplace_back(#x " failed", errno);

    // Clear the umask.
    //清空文件权限
    umask(0);
    CHECKCALL(clearenv());
    CHECKCALL(setenv("PATH", _PATH_DEFPATH, 1));
    // Get the basic filesystem setup we need put together in the initramdisk
    // on / and then we'll let the rc file figure out the rest.
    //在RAM内存上获取基本的文件系统，剩余的被rc文件所用
    CHECKCALL(mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755"));
    CHECKCALL(mkdir("/dev/pts", 0755));
    CHECKCALL(mkdir("/dev/socket", 0755));
    CHECKCALL(mount("devpts", "/dev/pts", "devpts", 0, NULL));
#define MAKE_STR(x) __STRING(x)
    CHECKCALL(mount("proc", "/proc", "proc", 0, "hidepid=2,gid=" MAKE_STR(AID_READPROC)));
#undef MAKE_STR
    // Don't expose the raw commandline to unprivileged processes.
    //非特权应用不能使用Android cmdline
    CHECKCALL(chmod("/proc/cmdline", 0440));
    gid_t groups[] = {AID_READPROC};
    CHECKCALL(setgroups(arraysize(groups), groups));
    CHECKCALL(mount("sysfs", "/sys", "sysfs", 0, NULL));
    CHECKCALL(mount("selinuxfs", "/sys/fs/selinux", "selinuxfs", 0, NULL));

    CHECKCALL(mknod("/dev/kmsg", S_IFCHR | 0600, makedev(1, 11)));

    if constexpr (WORLD_WRITABLE_KMSG) {
        CHECKCALL(mknod("/dev/kmsg_debug", S_IFCHR | 0622, makedev(1, 11)));
    }

    CHECKCALL(mknod("/dev/random", S_IFCHR | 0666, makedev(1, 8)));
    CHECKCALL(mknod("/dev/urandom", S_IFCHR | 0666, makedev(1, 9)));

    // This is needed for log wrapper, which gets called before ueventd runs.
    //这对于日志包装是必须的，他在ueventd运行前被调用
    CHECKCALL(mknod("/dev/ptmx", S_IFCHR | 0666, makedev(5, 2)));
    CHECKCALL(mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3)));

    // These below mounts are done in first stage init so that first stage mount can mount
    // subdirectories of /mnt/{vendor,product}/.  Other mounts, not required by first stage mount,
    // should be done in rc files.
    // Mount staging areas for devices managed by vold
    // See storage config details at http://source.android.com/devices/storage/
    //
    //在第一阶段挂在tmpfs、mnt/vendor、mount/product分区。其他的分区不需要在第一阶段加载，
    //只需要在第二阶段通过rc文件解析来加载。
    CHECKCALL(mount("tmpfs", "/mnt", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=1000"));
    // /mnt/vendor is used to mount vendor-specific partitions that can not be
    // part of the vendor partition, e.g. because they are mounted read-write.
    //创建可供读写的vendor目录
    CHECKCALL(mkdir("/mnt/vendor", 0755));
    // /mnt/product is used to mount product-specific partitions that can not be
    // part of the product partition, e.g. because they are mounted read-write.
    CHECKCALL(mkdir("/mnt/product", 0755));

    // /apex is used to mount APEXes
    // 挂载APEX，这在Android 10.0中特殊引入，用来解决碎片化问题，类似一种组件方式，对Treble的增强，
    // 不写谷歌特殊更新不需要完整升级整个系统版本，只需要像升级APK一样，进行APEX组件升级
    CHECKCALL(mount("tmpfs", "/apex", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=0"));

    // /debug_ramdisk is used to preserve additional files from the debug ramdisk
    CHECKCALL(mount("tmpfs", "/debug_ramdisk", "tmpfs", MS_NOEXEC | MS_NOSUID | MS_NODEV,
                    "mode=0755,uid=0,gid=0"));
#undef CHECKCALL

    //把标准输入、标准输出和标准错误重定向到空设备文件"/dev/null"

    SetStdioToDevNull(argv);
    // Now that tmpfs is mounted on /dev and we have /dev/kmsg, we can actually
    // talk to the outside world...
    //在/dev目录下挂载好 tmpfs 以及 kmsg 
    //这样就可以初始化 /kernel Log 系统，供用户打印log
    InitKernelLogging(argv);

    if (!errors.empty()) {
        for (const auto& [error_string, error_errno] : errors) {
            LOG(ERROR) << error_string << " " << strerror(error_errno);
        }
        LOG(FATAL) << "Init encountered errors starting first stage, aborting";
    }

    LOG(INFO) << "init first stage started!";

    auto old_root_dir = std::unique_ptr<DIR, decltype(&closedir)>{opendir("/"), closedir};
    if (!old_root_dir) {
        PLOG(ERROR) << "Could not opendir(\"/\"), not freeing ramdisk";
    }

    struct stat old_root_info;
    if (stat("/", &old_root_info) != 0) {
        PLOG(ERROR) << "Could not stat(\"/\"), not freeing ramdisk";
        old_root_dir.reset();
    }

    if (ForceNormalBoot()) {
        mkdir("/first_stage_ramdisk", 0755);
        // SwitchRoot() must be called with a mount point as the target, so we bind mount the
        // target directory to itself here.
        if (mount("/first_stage_ramdisk", "/first_stage_ramdisk", nullptr, MS_BIND, nullptr) != 0) {
            LOG(FATAL) << "Could not bind mount /first_stage_ramdisk to itself";
        }
        SwitchRoot("/first_stage_ramdisk");
    }

    // If this file is present, the second-stage init will use a userdebug sepolicy
    // and load adb_debug.prop to allow adb root, if the device is unlocked.
    if (access("/force_debuggable", F_OK) == 0) {
        std::error_code ec;  // to invoke the overloaded copy_file() that won't throw.
        if (!fs::copy_file("/adb_debug.prop", kDebugRamdiskProp, ec) ||
            !fs::copy_file("/userdebug_plat_sepolicy.cil", kDebugRamdiskSEPolicy, ec)) {
            LOG(ERROR) << "Failed to setup debug ramdisk";
        } else {
            // setenv for second-stage init to read above kDebugRamdisk* files.
            setenv("INIT_FORCE_DEBUGGABLE", "true", 1);
        }
    }

    /* 初始化一些必须的分区
     *主要作用是去解析/proc/device-tree/firmware/android/fstab,
     * 然后得到"/system", "/vendor", "/odm"三个目录的挂载信息
     */
    if (!DoFirstStageMount()) {
        LOG(FATAL) << "Failed to mount required partitions early ...";
    }

    struct stat new_root_info;
    if (stat("/", &new_root_info) != 0) {
        PLOG(ERROR) << "Could not stat(\"/\"), not freeing ramdisk";
        old_root_dir.reset();
    }

    if (old_root_dir && old_root_info.st_dev != new_root_info.st_dev) {
        FreeRamdisk(old_root_dir.get(), old_root_info.st_dev);
    }

    SetInitAvbVersionInRecovery();

    static constexpr uint32_t kNanosecondsPerMillisecond = 1e6;
    uint64_t start_ms = start_time.time_since_epoch().count() / kNanosecondsPerMillisecond;
    setenv("INIT_STARTED_AT", std::to_string(start_ms).c_str(), 1);

    //启动init进程，传入参数selinux_steup
    // 执行命令： /system/bin/init selinux_setup
    const char* path = "/system/bin/init";
    const char* args[] = {path, "selinux_setup", nullptr};
    execv(path, const_cast<char**>(args));

    // execv() only returns if an error happened, in which case we
    // panic and never fall through this conditional.
    PLOG(FATAL) << "execv(\"" << path << "\") failed";
    return 1;
}

```


从上边源码可以看出来，FirstStateMain主要操作有以下几类

- 设置信号量监听
- 通过mount挂载对应的文件系统，mkdir创建对应的文件目录，并配置相应的访问权限
  > 挂载的文件类型主要有以下几类
  > - **tmpfs**
  >   一种虚拟内存文件系统，它会将所有的文件存储在虚拟内存中。由于tmpfs是驻留在RAM的，因此它的内容是不持久的。断电后，tmpfs >的内容就消失了，这也是被称作tmpfs的根本原因
  > - **devpts**
  > 为伪终端提供了一个标准接口，它的标准挂接点是/dev/pts。只要pty的主复合设备/dev/ptmx被打开，就会在/dev/pts下动态的创建一个新的pty设备文件。
  > - **proc**
  > 也是一个虚拟文件系统，它可以看作是内核内部数据结构的接口，通过它我们可以获得系统的信息，同时也能够在运行时修改特定的内核参数。
  > - **sysfs**
  > 与proc文件系统类似，也是一个不占有任何磁盘空间的虚拟文件系统。它通常被挂接在/sys目录下。


- 初始化内核日志系统，此时Android还没有自己的系统日志，采用Kernel的log系统
  > 挂载/dev/kmsg
  > 
  > CHECKCALL(mknod("/dev/kmsg", S_IFCHR | 0600, makedev(1, 11)));
  > 
  > 可通过cat /dev/kmsg 获取内核log

- 最后通过execv方法执行进程"/system/bin/init"，并传递selinux_linux参数，进入下一阶段


### SetupSelinux

该部分主要是提高linux的安全性，进一步约束访问的权限，源码如下

```c++
// This function initializes SELinux then execs init to run in the init SELinux context.
int SetupSelinux(char** argv) {
    InitKernelLogging(argv);

    if (REBOOT_BOOTLOADER_ON_PANIC) {
        InstallRebootSignalHandlers();
    }
  
    // Set up SELinux, loading the SELinux policy.
    SelinuxSetupKernelLogging();
    SelinuxInitialize();
 
    // We're in the kernel domain and want to transition to the init domain.  File systems that
    // store SELabels in their xattrs, such as ext4 do not need an explicit restorecon here,
    // but other file systems do.  In particular, this is needed for ramdisks such as the
    // recovery image for A/B devices.
    if (selinux_android_restorecon("/system/bin/init", 0) == -1) {
        PLOG(FATAL) << "restorecon failed of /system/bin/init failed";
    }

    // 进入下一步
    const char* path = "/system/bin/init";
    const char* args[] = {path, "second_stage", nullptr};
    execv(path, const_cast<char**>(args));
  
    // execv() only returns if an error happened, in which case we
    // panic and never return from this function.
    PLOG(FATAL) << "execv(\"" << path << "\") failed";

    return 1;
} 
```
从上边源码可以看出来，SetupSelinux主要操作有以下几类

- 初始化内核日志系统
- 设置信号量监听
- 启动SeLinux，加载SELinux策略
- 最后通过execv方法执行进程"/system/bin/init"，并传递second_stage参数，进入下一阶段



### SecondStageMain

该部分进入了init进程重头戏部分

init启动入口是在它的SecondStageMain方法

```c++
int SecondStageMain(int argc, char** argv) {
    if (REBOOT_BOOTLOADER_ON_PANIC) {
        InstallRebootSignalHandlers();
    }
    
    ...
    ...
    
    // 初始化属性服务
    property_init();

    // If arguments are passed both on the command line and in DT,
    // properties set in DT always have priority over the command-line ones.
    process_kernel_dt();
    process_kernel_cmdline();

    // Propagate the kernel variables to internal variables
    // used by init as well as the current required properties.
    export_kernel_boot_props();

    // Make the time that init started available for bootstat to log.
    property_set("ro.boottime.init", getenv("INIT_STARTED_AT"));
    property_set("ro.boottime.init.selinux", getenv("INIT_SELINUX_TOOK"));

    // Set libavb version for Framework-only OTA match in Treble build.
    const char* avb_version = getenv("INIT_AVB_VERSION");
    if (avb_version) property_set("ro.boot.avb_version", avb_version);

    // See if need to load debug props to allow adb root, when the device is unlocked.
    const char* force_debuggable_env = getenv("INIT_FORCE_DEBUGGABLE");
    if (force_debuggable_env && AvbHandle::IsDeviceUnlocked()) {
        load_debug_prop = "true"s == force_debuggable_env;
    }

    // Clean up our environment.
    unsetenv("INIT_STARTED_AT");
    unsetenv("INIT_SELINUX_TOOK");
    unsetenv("INIT_AVB_VERSION");
    unsetenv("INIT_FORCE_DEBUGGABLE");

    // Now set up SELinux for second stage.
    SelinuxSetupKernelLogging();
    SelabelInitialize();
    SelinuxRestoreContext();

    Epoll epoll;
    if (auto result = epoll.Open(); !result) {
        PLOG(FATAL) << result.error();
    }

    // 初始化single句柄
    InstallSignalFdHandler(&epoll);

    property_load_boot_defaults(load_debug_prop);
    UmountDebugRamdisk();
    fs_mgr_vendor_overlay_mount_all();
    export_oem_lock_status();
    // 开启属性服务
    StartPropertyService(&epoll);
    MountHandler mount_handler(&epoll);
    set_usb_controller();

    const BuiltinFunctionMap function_map;
    Action::set_function_map(&function_map);

    if (!SetupMountNamespaces()) {
        PLOG(FATAL) << "SetupMountNamespaces failed";
    }

    subcontexts = InitializeSubcontexts();

    ActionManager& am = ActionManager::GetInstance();
    ServiceList& sm = ServiceList::GetInstance();

    // 解析init.rc等相关文件
    LoadBootScripts(am, sm);

    // Turning this on and letting the INFO logging be discarded adds 0.2s to
    // Nexus 9 boot time, so it's disabled by default.
    if (false) DumpState();

    // Make the GSI status available before scripts start running.
    if (android::gsi::IsGsiRunning()) {
        property_set("ro.gsid.image_running", "1");
    } else {
        property_set("ro.gsid.image_running", "0");
    }

    am.QueueBuiltinAction(SetupCgroupsAction, "SetupCgroups");

    am.QueueEventTrigger("early-init");

    // Queue an action that waits for coldboot done so we know ueventd has set up all of /dev...
    am.QueueBuiltinAction(wait_for_coldboot_done_action, "wait_for_coldboot_done");
    // ... so that we can start queuing up actions that require stuff from /dev.
    am.QueueBuiltinAction(MixHwrngIntoLinuxRngAction, "MixHwrngIntoLinuxRng");
    am.QueueBuiltinAction(SetMmapRndBitsAction, "SetMmapRndBits");
    am.QueueBuiltinAction(SetKptrRestrictAction, "SetKptrRestrict");
    Keychords keychords;
    am.QueueBuiltinAction(
        [&epoll, &keychords](const BuiltinArguments& args) -> Result<Success> {
            for (const auto& svc : ServiceList::GetInstance()) {
                keychords.Register(svc->keycodes());
            }
            keychords.Start(&epoll, HandleKeychord);
            return Success();
        },
        "KeychordInit");
    am.QueueBuiltinAction(console_init_action, "console_init");

    // Trigger all the boot actions to get us started.
    am.QueueEventTrigger("init");

    // Starting the BoringSSL self test, for NIAP certification compliance.
    am.QueueBuiltinAction(StartBoringSslSelfTest, "StartBoringSslSelfTest");

    // Repeat mix_hwrng_into_linux_rng in case /dev/hw_random or /dev/random
    // wasn't ready immediately after wait_for_coldboot_done
    am.QueueBuiltinAction(MixHwrngIntoLinuxRngAction, "MixHwrngIntoLinuxRng");

    // Initialize binder before bringing up other system services
    am.QueueBuiltinAction(InitBinder, "InitBinder");

    // Don't mount filesystems or start core system services in charger mode.
    std::string bootmode = GetProperty("ro.bootmode", "");
    if (bootmode == "charger") {
        am.QueueEventTrigger("charger");
    } else {
        am.QueueEventTrigger("late-init");
    }

    // Run all property triggers based on current state of the properties.
    am.QueueBuiltinAction(queue_property_triggers_action, "queue_property_triggers");

    while (true) {
        // By default, sleep until something happens.
        auto epoll_timeout = std::optional<std::chrono::milliseconds>{};

        if (do_shutdown && !shutting_down) {
            do_shutdown = false;
            if (HandlePowerctlMessage(shutdown_command)) {
                shutting_down = true;
            }
        }

        if (!(waiting_for_prop || Service::is_exec_service_running())) {
            am.ExecuteOneCommand();
        }
        if (!(waiting_for_prop || Service::is_exec_service_running())) {
            if (!shutting_down) {
                auto next_process_action_time = HandleProcessActions();

                // If there's a process that needs restarting, wake up in time for that.
                if (next_process_action_time) {
                    epoll_timeout = std::chrono::ceil<std::chrono::milliseconds>(
                            *next_process_action_time - boot_clock::now());
                    if (*epoll_timeout < 0ms) epoll_timeout = 0ms;
                }
            }

            // If there's more work to do, wake up again immediately.
            if (am.HasMoreCommands()) epoll_timeout = 0ms;
        }

        if (auto result = epoll.Wait(epoll_timeout); !result) {
            LOG(ERROR) << result.error();
        }
    }

    return 0;
} 
```


SecondStageMain主要实现在init.cpp源码中，如下

```c++

```


在SecondStageMain中主要分为以下几个步骤

- 初始化属性服务
  > property_init();

- 进行SELinux第二阶段并恢复一些文件安全上下文
  > SelinuxSetupKernelLogging();
  >
  > SelabelInitialize();
  >
  > SelinuxRestoreContext();
  
- 初始化single句柄
  > InstallSignalFdHandler(&epoll);

- 开启属性服务
  > StartPropertyService(&epoll);

- 解析.rc文件
  > ActionManager& am = ActionManager::GetInstance();
  > ServiceList& sm = ServiceList::GetInstance();
  > LoadBootScripts(am, sm);



#### 初始化属性服务

system/core/init/property_service.cpp

```c++
void property_init() {
    mkdir("/dev/__properties__", S_IRWXU | S_IXGRP | S_IXOTH);
    CreateSerializedPropertyInfo();
    if (__system_property_area_init()) {
        LOG(FATAL) << "Failed to initialize property area";
    }
    if (!property_info_area.LoadDefaultPath()) {
        LOG(FATAL) << "Failed to load serialized property info file";
    }
} 
```
该部分主要方法是**__system_property_area_init()**,用来创建跨进程内存，主要进行了以下操作

- 通过open打开dev/properities共享内存文件，大小为128KB
- 通过mmap将内存映射到init进程
- 将该内存的首地址保存在全局变量__system_property_area__，后续的增加或者修改属性都基于该变量来计算位置


#### SELinux
SELinux(Security-Enhanced Linux) 一种加强型linux，是 Linux历史上最杰出的新安全子系统。
#### 初始化single句柄

```c++
static void InstallSignalFdHandler(Epoll* epoll) {
    // Applying SA_NOCLDSTOP to a defaulted SIGCHLD handler prevents the signalfd from receiving
    // SIGCHLD when a child process stops or continues (b/77867680#comment9).
    const struct sigaction act { .sa_handler = SIG_DFL, .sa_flags = SA_NOCLDSTOP };
    sigaction(SIGCHLD, &act, nullptr);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    if (!IsRebootCapable()) {
        // If init does not have the CAP_SYS_BOOT capability, it is running in a container.
        // In that case, receiving SIGTERM will cause the system to shut down.
        sigaddset(&mask, SIGTERM);
    }

    if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
        PLOG(FATAL) << "failed to block signals";
    }

    // Register a handler to unblock signals in the child processes.
    const int result = pthread_atfork(nullptr, nullptr, &UnblockSignals);
    if (result != 0) {
        LOG(FATAL) << "Failed to register a fork handler: " << strerror(result);
    }

    signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (signal_fd == -1) {
        PLOG(FATAL) << "failed to create signalfd";
    }

    if (auto result = epoll->RegisterHandler(signal_fd, HandleSignalFd); !result) {
        LOG(FATAL) << result.error();
    }
}
```
处理信号处理器

每个进程在处理其他进程发送的signal信号时都需要先注册，当进程的运行状态改变或终止时会产生某种single信号，其父进程都会接收到并处理它。
init进程是所有用户进程的父进程，当其子进程终止时产生signal信号，这样init进程就可以进行处理。

这块的主要目的是防止子进程变成**僵尸进程**


#### 开启属性服务

system/core/init/property_service.cpp

```c++

```

该部分主要进行以下操作

- 创建非阻塞式的Socket，并返回property_set_fd文件描述符
- 使用listen()函数去监听property_set_fd，此时Socket即成为属性服务端，并且它最多同时可为8个试图设置属性的用户提供服务。
- 使用epoll注册，当property_set_fd中有数据到来时，init进程将调用handle_property_set_fd()函数进行处理

#### 解析.rc文件


```c++
static void LoadBootScripts(ActionManager& action_manager, ServiceList& service_list) {
    //创建解析器
    Parser parser = CreateParser(action_manager, service_list);
    //获取缓存
    std::string bootscript = GetProperty("ro.boot.init_rc", "");
    
    if (bootscript.empty()) {
        //寻找init文件并解析
        parser.ParseConfig("/init.rc");
        if (!parser.ParseConfig("/system/etc/init")) {
            late_import_paths.emplace_back("/system/etc/init");
        }
        if (!parser.ParseConfig("/product/etc/init")) {
            late_import_paths.emplace_back("/product/etc/init");
        }
        if (!parser.ParseConfig("/product_services/etc/init")) {
            late_import_paths.emplace_back("/product_services/etc/init");
        }
        if (!parser.ParseConfig("/odm/etc/init")) {
            late_import_paths.emplace_back("/odm/etc/init");
        }
        if (!parser.ParseConfig("/vendor/etc/init")) {
            late_import_paths.emplace_back("/vendor/etc/init");
        }
    } else {
        //解析init相关配置
        parser.ParseConfig(bootscript);
    }
}
```

该部分经过检索init文件，并通过ParseConfig来解析init.rc配置文件，从而启动Zygote等相关进程

>.rc文件语法是以行尾单位，以空格间隔的语法，以#开始代表注释行。
> rc文件主要包含Action、Service、Command、Options、Import，
> 其中对于Action和Service的名称都是唯一的，对于重复的命名视为无效。
> init.rc中的Action、Service语句都有相应的类来解析，即ActionParser、ServiceParser。

## Init进程总结

- 1.创建与挂载启动所需要的文件系统
- 2.初始化属性服务
- 3.创建single，来监听子进程，防止僵尸进程的产生
- 4.开启属性服务
- 5.解析.rc文件并启动相关进程，如Zygote进程、system_manager进程、SurfaceFlinger进程、MediaServer进程等等



