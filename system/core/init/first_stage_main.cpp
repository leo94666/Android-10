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

#include "first_stage_init.h"
/**
 * @brief 
 * liyang
 * init进程第一阶段主要是挂载分区，创建设备节点和一些关键目录，初始化日志输出系统，启用SeLinux安全策略
 * 第一阶段主要完成以下内容:
 * 1.挂载文件系统目录并挂载相关的文件系统
 * 2.屏蔽标准的输入输出/初始化内核log系统
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char** argv) {
    return android::init::FirstStageMain(argc, argv);
}
