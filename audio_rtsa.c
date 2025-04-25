/*************************************************************
 * File  :  audio_rtsa.c
 * Module:  Agora SD-RTN SDK RTC C API demo application.
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include "app_config.h"

// 音频设备结构体
typedef struct {
    snd_pcm_t *capture_handle;    // 录音设备句柄
    snd_pcm_t *playback_handle;   // 播放设备句柄
    snd_pcm_hw_params_t *hw_params;  // 硬件参数
    unsigned int sample_rate;      // 采样率
    unsigned int channels;         // 通道数
    snd_pcm_format_t format;      // 音频格式
} audio_device_t;

// 应用程序结构体
typedef struct {
    app_config_t config;
    audio_device_t audio_dev;
    connection_id_t conn_id;
    bool b_stop_flag;
    bool b_connected_flag;
} app_t;

static app_t g_app = {
    .config = {
        // common config
        .p_sdk_log_dir              = "io.agora.rtc_sdk",
        .log_level                  = RTC_LOG_INFO,
        .p_appid                    = "",
        .p_channel                  = DEFAULT_CHANNEL_NAME,
        .p_token                    = "",
        .p_license                  = "",
        .uid                        = 0,
        .uname                      = NULL,
        .area                       = AREA_CODE_GLOB,

        // audio related config
        .audio_data_type            = AUDIO_DATA_TYPE_PCM,
        .audio_codec_type           = AUDIO_CODEC_TYPE_OPUS,
        .send_audio_file_path       = NULL,

        // pcm related config
        .pcm_sample_rate            = DEFAULT_PCM_SAMPLE_RATE,
        .pcm_channel_num            = DEFAULT_PCM_CHANNEL_NUM,
        .pcm_duration               = DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS,

        // advanced config
        .enable_audio_mixer         = false,
        .receive_data_only          = false,
        .local_ap                   = "",
        .domain_limit               = false,
        .lan_accelerate             = false,
    },

    .b_stop_flag            = false,
    .b_connected_flag       = false,
};

// 信号处理函数
static void app_signal_handler(int sig) {
    switch (sig) {
    case SIGINT:
        g_app.b_stop_flag = true;
        break;
    default:
        LOGW("no handler, sig=%d", sig);
    }
}

// 初始化音频设备
static int init_audio_device(audio_device_t *dev, const char *capture_device, 
                           const char *playback_device) {
    int err;
    snd_pcm_uframes_t buffer_size = 1024;
    snd_pcm_uframes_t period_size = 256;
    
    // 初始化录音设备
    err = snd_pcm_open(&dev->capture_handle, capture_device, 
                      SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
        LOGE("无法打开录音设备: %s", snd_strerror(err));
        return -1;
    }
    
    // 初始化播放设备
    err = snd_pcm_open(&dev->playback_handle, playback_device, 
                      SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        LOGE("无法打开播放设备: %s", snd_strerror(err));
        snd_pcm_close(dev->capture_handle);
        return -1;
    }
    
    // 设置硬件参数
    snd_pcm_hw_params_alloca(&dev->hw_params);
    err = snd_pcm_hw_params_any(dev->capture_handle, dev->hw_params);
    if (err < 0) {
        LOGE("无法初始化硬件参数: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置访问模式
    err = snd_pcm_hw_params_set_access(dev->capture_handle, dev->hw_params,
                                      SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        LOGE("无法设置访问模式: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置采样格式
    dev->format = SND_PCM_FORMAT_S16_LE;  // 16位有符号整数，小端
    err = snd_pcm_hw_params_set_format(dev->capture_handle, dev->hw_params,
                                      dev->format);
    if (err < 0) {
        LOGE("无法设置采样格式: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置采样率
    dev->sample_rate = 16000;  // 16kHz
    err = snd_pcm_hw_params_set_rate_near(dev->capture_handle, dev->hw_params,
                                         &dev->sample_rate, 0);
    if (err < 0) {
        LOGE("无法设置采样率: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置通道数
    dev->channels = 1;  // 单声道
    err = snd_pcm_hw_params_set_channels(dev->capture_handle, dev->hw_params,
                                        dev->channels);
    if (err < 0) {
        LOGE("无法设置通道数: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置缓冲区大小
    err = snd_pcm_hw_params_set_buffer_size_near(dev->capture_handle, dev->hw_params, &buffer_size);
    if (err < 0) {
        LOGE("无法设置缓冲区大小: %s", snd_strerror(err));
        goto error;
    }
    
    // 设置周期大小
    err = snd_pcm_hw_params_set_period_size_near(dev->capture_handle, dev->hw_params, &period_size, 0);
    if (err < 0) {
        LOGE("无法设置周期大小: %s", snd_strerror(err));
        goto error;
    }
    
    // 应用参数
    err = snd_pcm_hw_params(dev->capture_handle, dev->hw_params);
    if (err < 0) {
        LOGE("无法应用硬件参数: %s", snd_strerror(err));
        goto error;
    }
    
    // 对播放设备应用相同的参数
    err = snd_pcm_hw_params(dev->playback_handle, dev->hw_params);
    if (err < 0) {
        LOGE("无法应用播放设备参数: %s", snd_strerror(err));
        goto error;
    }
    
    return 0;
    
error:
    snd_pcm_close(dev->capture_handle);
    snd_pcm_close(dev->playback_handle);
    return -1;
}

// 清理音频设备
static void cleanup_audio_device(audio_device_t *dev) {
    if (dev->capture_handle) {
        snd_pcm_close(dev->capture_handle);
    }
    if (dev->playback_handle) {
        snd_pcm_close(dev->playback_handle);
    }
}

// 发送音频数据
static int app_send_audio(void) {
    app_config_t *config = &g_app.config;
    audio_frame_info_t info = { 0 };
    info.data_type = config->audio_data_type;
    
    // 计算每帧的字节数
    int bytes_per_frame = 2 * config->pcm_channel_num;  // 16位采样，每个采样2字节
    int frames_per_packet = config->pcm_sample_rate * config->pcm_duration / 1000;  // 每包帧数
    int buffer_size = frames_per_packet * bytes_per_frame;
    
    // 从USB麦克风读取音频数据
    char buffer[buffer_size];
    snd_pcm_sframes_t frames = snd_pcm_readi(g_app.audio_dev.capture_handle, 
                                           buffer, frames_per_packet);
    if (frames < 0) {
        LOGE("读取音频数据失败: %s", snd_strerror(frames));
        return -1;
    }
    
    // 确保读取到足够的帧数
    if (frames < frames_per_packet) {
        LOGW("读取的帧数不足: %ld < %d", (long)frames, frames_per_packet);
        return -1;
    }
    
    // 发送音频数据
    int rval = agora_rtc_send_audio_data(g_app.conn_id, buffer, 
                                        frames * bytes_per_frame, &info);
    if (rval < 0) {
        LOGE("发送音频数据失败: %s", agora_rtc_err_2_str(rval));
        return -1;
    }
    
    return 0;
}

// 事件处理函数
static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed) {
    g_app.b_connected_flag = true;
    connection_info_t conn_info = { 0 };
    agora_rtc_get_connection_info(g_app.conn_id, &conn_info);
    LOGI("[conn-%u] Join the channel %s successfully, uid %u elapsed %d ms", conn_id,
         conn_info.channel_name, uid, elapsed);
}

static void __on_reconnecting(connection_id_t conn_id) {
    g_app.b_connected_flag = false;
    LOGW("[conn-%u] connection timeout, reconnecting", conn_id);
}

static void __on_connection_lost(connection_id_t conn_id) {
    g_app.b_connected_flag = false;
    LOGW("[conn-%u] Lost connection from the channel", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms) {
    g_app.b_connected_flag = true;
    LOGI("[conn-%u] Rejoin the channel successfully, uid %u elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_error(connection_id_t conn_id, int code, const char *msg) {
    if (code == ERR_INVALID_APP_ID) {
        LOGE("Invalid App ID. Please double check. Error msg \"%s\"", msg);
    } else if (code == ERR_INVALID_CHANNEL_NAME) {
        LOGE("Invalid channel name for conn_id %u. Please double check. Error msg \"%s\"", conn_id,
             msg);
    } else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
        LOGE("Invalid token. Please double check. Error msg \"%s\"", msg);
    } else {
        LOGW("Error %d is captured. Error msg \"%s\"", code, msg);
    }

    g_app.b_stop_flag = true;
}

static void __on_audio_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts,
                           const void *data, size_t len, const audio_frame_info_t *info_ptr) {
    // 计算每帧的字节数
    int bytes_per_frame = 2 * g_app.config.pcm_channel_num;  // 16位采样，每个采样2字节
    int frames = len / bytes_per_frame;
    
    // 将接收到的音频数据写入3.5mm音频输出
    snd_pcm_sframes_t written = snd_pcm_writei(g_app.audio_dev.playback_handle, 
                                            data, frames);
    if (written < 0) {
        LOGE("写入音频数据失败: %s", snd_strerror(written));
        return;
    }
    
    // 确保所有数据都被写入
    if (written < frames) {
        LOGW("部分音频数据未被写入: %ld < %d", (long)written, frames);
    }
}

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler, app_config_t *config) {
    event_handler->on_join_channel_success = __on_join_channel_success;
    event_handler->on_reconnecting = __on_reconnecting;
    event_handler->on_connection_lost = __on_connection_lost;
    event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
    event_handler->on_error = __on_error;
    event_handler->on_audio_data = __on_audio_data;
}

int main(int argc, char **argv) {
    app_config_t *config = &g_app.config;
    int rval = 0;
    char priv_params[512] = { 0 };

    LOGI("Welcome to RTSA SDK v%s", agora_rtc_get_version());

    // 0. 解析命令行参数
    rval = app_parse_args(argc, argv, config);
    if (rval < 0) {
        app_print_usage(argc, argv);
        return -1;
    }

    app_print_config(config);

    // 1. 初始化音频设备
    const char *capture_device = config->capture_device ? config->capture_device : "default";
    const char *playback_device = config->playback_device ? config->playback_device : "default";
    if (init_audio_device(&g_app.audio_dev, capture_device, playback_device) < 0) {
        LOGE("初始化音频设备失败");
        return -1;
    }

    // 2. 初始化声网RTC SDK
    int appid_len = strlen(config->p_appid);
    void *p_appid = (void *)(appid_len == 0 ? NULL : config->p_appid);

    agora_rtc_event_handler_t event_handler = { 0 };
    app_init_event_handler(&event_handler, config);

    rtc_service_option_t service_opt = { 0 };
    service_opt.area_code = config->area;
    service_opt.log_cfg.log_path = config->p_sdk_log_dir;
    service_opt.log_cfg.log_disable = config->log_level == -1;
    service_opt.log_cfg.log_level = config->log_level;
    service_opt.use_string_uid = config->uname ? true : false;
    snprintf(service_opt.license_value, sizeof(service_opt.license_value), "%s", config->p_license);

    rval = agora_rtc_init(p_appid, &event_handler, &service_opt);
    if (rval < 0) {
        LOGE("Failed to initialize Agora sdk, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    // 3. 创建连接
    rval = agora_rtc_create_connection(&g_app.conn_id);
    if (rval < 0) {
        LOGE("Failed to create connection, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    // 4. 设置连接配置
    rval = agora_rtc_set_bwe_param(g_app.conn_id, DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE,
                                  DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE,
                                  DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE);
    if (rval != 0) {
        LOGE("Failed set bwe param, reason: %s", agora_rtc_err_2_str(rval));
        return -1;
    }

    // 5. 加入频道
    int token_len = strlen(config->p_token);
    void *p_token = (void *)(token_len == 0 ? NULL : config->p_token);

    rtc_channel_options_t channel_options = { 0 };
    memset(&channel_options, 0, sizeof(channel_options));
    channel_options.auto_subscribe_audio = true;
    channel_options.enable_audio_mixer = config->enable_audio_mixer;
    channel_options.enable_lan_accelerate = config->lan_accelerate;

    channel_options.audio_codec_opt.pcm_duration = config->pcm_duration;
    channel_options.audio_codec_opt.audio_codec_type = config->audio_codec_type;
    channel_options.audio_codec_opt.pcm_sample_rate = config->pcm_sample_rate;
    channel_options.audio_codec_opt.pcm_channel_num = config->pcm_channel_num;

    if (!config->uname) {
        rval = agora_rtc_join_channel(g_app.conn_id, config->p_channel, config->uid, p_token, &channel_options);
    } else {
        rval = agora_rtc_join_channel_with_user_account(g_app.conn_id, config->p_channel, config->uname, p_token,
                                                       &channel_options);
    }
    if (rval < 0) {
        LOGE("Failed to join channel \"%s\", reason: %s", config->p_channel, agora_rtc_err_2_str(rval));
        return -1;
    }

    // 6. 等待成功加入频道
    while (!g_app.b_connected_flag && !g_app.b_stop_flag) {
        usleep(100 * 1000);
    }

    // 7. 主循环：发送和接收音频数据
    while (!g_app.b_stop_flag) {
        if (g_app.b_connected_flag) {
            app_send_audio();
        }
        usleep(1000);  // 避免CPU占用过高
    }

    // 8. 离开频道
    agora_rtc_leave_channel(g_app.conn_id);

    // 9. 销毁连接
    agora_rtc_destroy_connection(g_app.conn_id);

    // 10. 结束RTC SDK
    agora_rtc_fini();

    // 11. 清理音频设备
    cleanup_audio_device(&g_app.audio_dev);

    return 0;
} 