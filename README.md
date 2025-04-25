# Audio RTSA 示例

这是一个基于声网RTSA SDK的音频通信示例程序，支持通过USB麦克风采集音频并通过3.5mm音频输出播放。

## 功能特点

- 支持USB麦克风音频输入
- 支持3.5mm音频输出
- 支持实时音频通信
- 支持音频混音
- 支持网络自适应

## 编译要求

- CMake 2.4或更高版本
- ALSA库
- 声网RTSA SDK

## 编译步骤

1. 安装依赖：
```bash
sudo apt-get install libasound2-dev
```

2. 编译程序：
```bash
chmod +x build.sh
./build.sh
```

## 运行程序

1. 查看音频设备：
```bash
arecord -l  # 查看录音设备
aplay -l    # 查看播放设备
```

2. 运行程序：
```bash
cd build/out
./audio_rtsa -I "hw:0,0" -O "hw:1,0" -i YOUR_APPID -c YOUR_CHANNEL_NAME
```

## 参数说明

- `-i`：声网App ID
- `-c`：频道名称
- `-I` 或 `--capture-device`：指定录音设备名称
- `-O` 或 `--playback-device`：指定播放设备名称
- `-t`：Token（可选）
- `-l`：License（可选）
- `-u`：用户ID（可选）
- `-n`：用户名（可选）

## 注意事项

1. 确保音频输入输出设备已正确连接
2. 确保网络连接稳定
3. 确保有足够的系统资源
4. 如果遇到权限问题，可能需要以root权限运行

## 常见问题

1. 无法打开音频设备
   - 检查设备是否正确连接
   - 检查设备权限
   - 检查ALSA配置

2. 音频质量差
   - 检查网络状况
   - 调整音频参数
   - 检查设备质量

3. 程序崩溃
   - 检查日志输出
   - 检查系统资源
   - 检查设备状态

## 技术支持

如有问题，请联系声网技术支持。 