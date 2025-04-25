#!/bin/bash

# 创建构建目录
mkdir -p build
cd build

# 运行CMake
cmake ..

# 编译
make -j4

# 复制必要的文件到输出目录
mkdir -p out
cp ../../../agora_sdk/lib/libagora-rtc-sdk.so out/
cp ../../../agora_sdk/certificate.bin out/

echo "编译完成，可执行文件在 build/out 目录下" 