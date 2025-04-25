#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "agora_rtc_api.h"
#include "file_parser.h"
#include "file_writer.h"
#include "utility.h"
#include "pacer.h"
#include "log.h"

#define DEFAULT_CHANNEL_NAME "hello_demo"
#define DEFAULT_CERTIFACTE_FILENAME "certificate.bin"
#define DEFAULT_SEND_AUDIO_FILENAME_PCM_16K "send_audio_16k_1ch.pcm"
#define DEFAULT_SEND_AUDIO_FILENAME_PCM_08K "send_audio_8k_1ch.pcm"
#define DEFAULT_SEND_AUDIO_FILENAME "send_audio_16k_1ch.pcm"
#define DEFAULT_SEND_AUDIO_BASENAME "send_audio"
#define DEFAULT_RECV_AUDIO_BASENAME "recv_audio"
#define DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE (100000)
#define DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE (1000000)
#define DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE (500000)
#define DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS (20)
#define DEFAULT_PCM_SAMPLE_RATE (16000)
#define DEFAULT_PCM_CHANNEL_NUM (1)

typedef struct {
  // common config
  const char *p_sdk_log_dir;
  int32_t  log_level;
  const char *p_appid;
  const char *p_token;
  const char *p_channel;
  const char *p_license;
  uint32_t uid;
  const char *uname;
  uint32_t area;

  // audio related config
  audio_data_type_e audio_data_type;
  audio_codec_type_e audio_codec_type;
  const char *send_audio_file_path;
  uint32_t pcm_sample_rate;
  uint32_t pcm_channel_num;
  uint32_t pcm_duration;
  const char *capture_device;
  const char *playback_device;

  // advanced config
  bool enable_audio_mixer;
  bool receive_data_only;
  const char *local_ap;
  bool domain_limit;
  bool lan_accelerate;
  int conn_cnt;
} app_config_t;

static void app_print_usage(int argc, char **argv)
{
  LOGS("\nUsage: %s [OPTION]", argv[0]);
  // short options
  LOGS(" -h, --help                : show help info");
  LOGS(" -i, --app-id              : application id; either app-id OR token MUST be set");
  LOGS(" -t, --token               : token for authentication");
  LOGS(" -c, --channel-id          : channel name; default is 'demo'");
  LOGS(" -u, --user-id             : user id; default is 0");
  LOGS(" -U, --user-name           : user name");
  LOGS(" -l, --license             : license value MUST be set when release");
  LOGS(" -L, --log-level           : set log level. if set -1 mean disable log");
  LOGS(" -a, --audio-type          : audio data type for the input audio file; default is 100=pcm");
  LOGS("                             support: 3=PCMA 4=PCMU 5=G722 6=AACLC-8K 7=AACLC-16K 8=AACLC-48K 9=HEAAC-32K 100=PCM");
  LOGS(" -C, --audio-codec         : audio codec type; only valid when audio type is PCM; default is 1=opus");
  LOGS("                             support: 0:Disable 1=OPUS, 2=G722 3=G711A 4=G711U");
  LOGS(" -S, --send-audio-file     : send audio file path; default is './%s'", DEFAULT_SEND_AUDIO_FILENAME);
  LOGS(" -r, --pcm-sample-rate     : sample rate for the input PCM data; only valid when audio type is PCM");
  LOGS(" -n, --pcm-channel-num     : channel number for the input PCM data; only valid when audio type is PCM");
  LOGS(" -D, --pcm-duration        : sample duration; default is %d", DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS);
  LOGS(" -I, --capture-device      : audio capture device name; default is 'default'");
  LOGS(" -O, --playback-device     : audio playback device name; default is 'default'");
  LOGS(" -A, --area                : hex format with 0x header, supported area_code list:");
  LOGS("                             CN (Mainland China) : 0x00000001");
  LOGS("                             NA (North America)  : 0x00000002");
  LOGS("                             EU (Europe)         : 0x00000004");
  LOGS("                             AS (Asia)           : 0x00000008");
  LOGS("                             JP (Japan)          : 0x00000010");
  LOGS("                             IN (India)          : 0x00000020");
  LOGS("                             OC (Oceania)        : 0x00000040");
  LOGS("                             SA (South-American) : 0x00000080");
  LOGS("                             AF (Africa)         : 0x00000100");
  LOGS("                             KR (South Korea)    : 0x00000200");
  LOGS("                             OVS (Global except China): 0xFFFFFFFE");
  LOGS("                             GLOB (Global)       : 0xFFFFFFFF");
  LOGS(" -m, --audio-mixer         : enable audio mixer to mix multiple incoming audio streams");
  LOGS(" -R, --recv-only           : do not send audio data");
  LOGS(" -d, --domain-limit        : domain limit is enabled");

  // long options
  LOGS(" --lan-accelerate          : enable lan accelerate");
  LOGS(" --local-ap                : params_str = {\"ipList\": [\"ip1\", \"ip2\"], \"domainList\":[\"domain1\", \"domain2\"], \"mode\": 1}");
  LOGS("                             mode: 0: ConnectivityFirst, 1: LocalOnly");
  LOGS("\nExample:");
  LOGS("    %s --app-id xxx [--token xxx] --channel-id xxx --send-audio-file ./audio.pcm --license <your license>",
       argv[0]);
}

static void app_print_config(app_config_t *config) {
	LOGS("---------------app config show start---------------------");
	LOGS("<common config info>:");
	LOGS("  appid                   : %s", config->p_appid);
	LOGS("  channel                 : %s", config->p_channel);
	LOGS("  token                   : %s", config->p_token);
	LOGS("  uid                     : %u", config->uid);
  LOGS("  uname                   : %s", config->uname);
	LOGS("  area                    : 0x%x", config->area);
  LOGS("  log-level               : %d", config->log_level);
	LOGS("<audio config info>       -");
	LOGS("  audio_data_type         : %d", config->audio_data_type);
	LOGS("  audio_codec_type        : %d", config->audio_codec_type);
	LOGS("  pcm_sample_rate         : %u", config->pcm_sample_rate);
  LOGS("  pcm_duration            : %u", config->pcm_duration);
	LOGS("  send_audio_file_path    : %s", config->send_audio_file_path);
	LOGS("  capture_device          : %s", config->capture_device);
	LOGS("  playback_device         : %s", config->playback_device);
	LOGS("<advanced config info>    -");
	LOGS("  enable_audio_mixer      : %d", config->enable_audio_mixer);
	LOGS("  received_data_only      : %d", config->receive_data_only);
  LOGS("  lan-accelerate          : %d", config->lan_accelerate);
	LOGS("---------------app config show end-----------------------");
}

static int app_parse_args(int argc, char **argv, app_config_t *config)
{
  const char *av_short_option = "hi:t:c:u:U:a:C:S:r:n:A:gmRl:dD:L:I:O:";
  int av_option_flag = 0;
  const struct option av_long_option[] = { { "help", 0, NULL, 'h' },
                                           { "app-id", 1, NULL, 'i' },
                                           { "token", 1, NULL, 't' },
                                           { "channel-id", 1, NULL, 'c' },
                                           { "user-id", 1, NULL, 'u' },
                                           { "user-name", 1, NULL, 'U' },
                                           { "audio-type", 1, NULL, 'a' },
                                           { "audio-codec", 1, NULL, 'C' },
                                           { "send-audio-file", 1, NULL, 'S' },
                                           { "pcm-sample-rate", 1, NULL, 'r' },
                                           { "pcm-channel-num", 1, NULL, 'n' },
                                           { "pcm-duration", 1, NULL, 'D'},
                                           { "area", 0, NULL, 'A' },
                                           { "audio-mixer", 0, NULL, 'm' },
                                           { "recv-only", 0, NULL, 'R' },
                                           { "local-ap", 1, &av_option_flag, 1 },
                                           { "license", 1, NULL, 'l' },
                                           { "log-level", 1, NULL, 'L' },
                                           { "domain-limit", 0, NULL, 'd' },
                                           { "conn-cnt", 1, &av_option_flag, 2 },
                                           { "lan-accelerate", 0, &av_option_flag, 3 },
                                           { "capture-device", 1, NULL, 'I' },
                                           { "playback-device", 1, NULL, 'O' },
                                           { 0, 0, 0, 0 } };

  int ch = -1;
  int optidx = 0;
  int rval = 0;

  while (1) {
    ch = getopt_long(argc, argv, av_short_option, av_long_option, &optidx);
    if (ch == -1) {
      break;
    }

    switch (ch) {
    case 'h':
      return -1;
    case 'i':
      config->p_appid = optarg;
      break;
    case 't':
      config->p_token = optarg;
      break;
    case 'c':
      config->p_channel = optarg;
      break;
    case 'u':
      config->uid = strtoul(optarg, NULL, 10);
      break;
    case 'U':
      config->uname = optarg;
      break;
    case 'a':
      config->audio_data_type = strtol(optarg, NULL, 10);
      break;
    case 'C':
      config->audio_codec_type = strtol(optarg, NULL, 10);
      break;
    case 'S':
      config->send_audio_file_path = optarg;
      break;
    case 'r':
      config->pcm_sample_rate = atoi(optarg);
      break;
    case 'n':
      config->pcm_channel_num = atoi(optarg);
      break;
    case 'A':
      config->area = strtol(optarg, NULL, 16);
      break;
    case 'm':
      config->enable_audio_mixer = true;
      break;
    case 'R':
      config->receive_data_only = true;
      break;
    case 'l':
      config->p_license = optarg;
      break;
    case 'd':
      config->domain_limit = true;
      break;
    case 'D':
      config->pcm_duration = atoi(optarg);
      break;
    case 'L':
      config->log_level = atoi(optarg);
      break;
    case 'I':
      config->capture_device = optarg;
      break;
    case 'O':
      config->playback_device = optarg;
      break;
    case 1:
      config->local_ap = optarg;
      break;
    case 2:
      config->conn_cnt = atoi(optarg);
      break;
    case 3:
      config->lan_accelerate = true;
      break;
    default:
      return -1;
    }
  }

  // 检查必要的参数
  if (!config->p_appid && !config->p_token) {
    LOGE("app-id or token MUST be set");
    return -1;
  }

  if (!config->p_channel) {
    LOGE("channel-id MUST be set");
    return -1;
  }

  return 0;
}

static media_file_type_e video_data_type_to_file_type(video_data_type_e type)
{
  media_file_type_e file_type;
  switch (type) {
  case VIDEO_DATA_TYPE_H264:
    file_type = MEDIA_FILE_TYPE_H264;
    break;
	case VIDEO_DATA_TYPE_H265:
		file_type = MEDIA_FILE_TYPE_H265;
		break;
	case VIDEO_DATA_TYPE_GENERIC_JPEG:
		file_type = MEDIA_FILE_TYPE_JPEG;
		break;
  default:
    file_type = MEDIA_FILE_TYPE_H264;
    break;
  }
  return file_type;
}

static media_file_type_e audio_data_type_to_file_type(audio_data_type_e type)
{
  media_file_type_e file_type;
  switch (type) {
  case AUDIO_DATA_TYPE_PCM:
    file_type = MEDIA_FILE_TYPE_PCM;
    break;
  case AUDIO_DATA_TYPE_OPUS:
  case AUDIO_DATA_TYPE_OPUSFB:
    file_type = MEDIA_FILE_TYPE_OPUS;
    break;
  case AUDIO_DATA_TYPE_AACLC:
  case AUDIO_DATA_TYPE_HEAAC:
  case AUDIO_DATA_TYPE_AACLC_8K:
  case AUDIO_DATA_TYPE_AACLC_16K:
    file_type = MEDIA_FILE_TYPE_AACLC;
    break;
  case AUDIO_DATA_TYPE_G722:
    file_type = MEDIA_FILE_TYPE_G722;
    break;
  case AUDIO_DATA_TYPE_PCMA:
  case AUDIO_DATA_TYPE_PCMU:
    file_type = MEDIA_FILE_TYPE_G711;
    break;
  default:
    file_type = MEDIA_FILE_TYPE_PCM;
    break;
  }
  return file_type;
}

#endif