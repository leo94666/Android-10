/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "builtins.h"
#include "first_stage_init.h"
#include "init.h"
#include "selinux.h"
#include "subcontext.h"
#include "ueventd.h"

#include <android-base/logging.h>

#if __has_feature(address_sanitizer)
#include <sanitizer/asan_interface.h>
#endif

#if __has_feature(address_sanitizer)
// Load asan.options if it exists since these are not yet in the environment.
// Always ensure detect_container_overflow=0 as there are false positives with this check.
// Always ensure abort_on_error=1 to ensure we reboot to bootloader for development builds.
extern "C" const char* __asan_default_options() {
    return "include_if_exists=/system/asan.options:detect_container_overflow=0:abort_on_error=1";
}

__attribute__((no_sanitize("address", "memory", "thread", "undefined"))) extern "C" void
__sanitizer_report_error_summary(const char* summary) {
    LOG(ERROR) << "Init (error summary): " << summary;
}

__attribute__((no_sanitize("address", "memory", "thread", "undefined"))) static void
AsanReportCallback(const char* str) {
    LOG(ERROR) << "Init: " << str;
}
#endif

using namespace android::init;

/**
 * @brief 
 * 1.参数中有ueventd，进入ueventd_main
 * 2.参数中有subcontext,进入InitLogging和SubcontextMain
 * 3.参数中有selinux_setup，进入SetupSelinux
 * 4.参数中有second_stage，进入SecondStageMain
 * 
 * main函数执行顺序：
 * 1.ueventd_main，init进程创建子进程ueventd，并将创建设备节点文件的工作托付于ueventd，ueventd通过两种方式创建设备节点文件
 * 2.FirstStageMain 启动第一阶段
 * 3.SetupSelinux 加载selinux规则，并设置selinux日志，完成selinux相关工作
 * 4.SecondStageMain 启动第二阶段
 * 
 * @param argc 参数个数
 * @param argv 参数列表，具体的参数
 * @return int 
 */
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
        //参数为selinux_setup，启动SeLinux安全策略
        if (!strcmp(argv[1], "selinux_setup")) {
            return SetupSelinux(argv);
        }
        //参数为second_stage，启动init进程第二阶段
        if (!strcmp(argv[1], "second_stage")) {
            return SecondStageMain(argc, argv);
        }
    }
    //默认启动第一阶段
    return FirstStageMain(argc, argv);
}
