# esp-baidu tts demo
读者根据此 `demo`, 可实现文字转语音功能, 并通过 [ESP32_LyraT](https://www.espressif.com/zh-hans/products/hardware/esp32-lyrat) 播放出来.

# 准备工作
根据百度智能云上[百度语音文档](https://cloud.baidu.com/doc/SPEECH/s/9jwvy5kt6), 获取 `demo` 需要的 `API Key` 和 `Secret Key`.

# 开始 demo
1. 根据 [esp-idf](https://docs.espressif.com/projects/esp-idf/en/v3.3/index.html)编程指南进行环境搭建, 能编译运行 `hello world`.
> 本 `demo` 使用 `esp-idf tag v3.3`

2. 下载本 `demo`, 并导出 `IDF_PATH`
```
git clone https://gitlab.espressif.cn:6688/demo/tts_baidu
cd tts_baidu/examples/baidu-tts
export IDF_PATH=$YOUR_IDF_PATH
```

3. 配置 `WiFi` 和百度 `Key`
- `make menuconfig` -> `Audio Example Configuration` -> `WiFi SSID`
- `make menuconfig` -> `Audio Example Configuration` -> `WiFi Password`
- `make menuconfig` -> `Audio Example Configuration` -> `API Key`
- `make menuconfig` -> `Audio Example Configuration` -> `Secret Key`

4. 烧写运行
```
make flash monitor
```

正常 `log` 如下:
```
...
I (4841) event: sta ip: 192.168.3.87, mask: 255.255.255.0, gw: 192.168.3.1
I (4841) main: Connected to AP, begin audio example
I (4841) mp3d: Create Ring Buffer
I (6741) http: HTTP GET Status = 200
I (6741) http: TTS init done
I (6751) http: TTS URL:http://tsn.baidu.com/text2audio?ctp=1&lan=zh&cuid=1234567C&tok=24.6c583dc0e541bef39b0076c11f05b150.2592000.1574922145.282335-17488148&tex=9 月 24 日，ESP-BLE-MESH 通过了蓝牙技术联盟（Bluetooth SIG）全功能支持的认证！&per=0&spd=5&pit=5&vol=5&aue=3
I (8221) http: state:4, recv len:58, heap:206380
I (8231) http: state:4, recv len:512, heap:208040
...
I (8231) mp3d: start mp3 decoder task
I (8241) mp3d: MAD: Decoder start
I (8241) mp3d: Codec Processing
I (8251) mp3d: Rate 16000
I (8251) I2S: APLL: Req RATE: 16000, real rate: 15999.986, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 4095996.500, SCLK: 511999.562500, diva: 1, divb: 0
I (8271) http: state:4, recv len:512, heap:174996
...
I (12431) http: state:4, recv len:490, heap:179892
I (13391) http: state:5, recv len:0, heap:180084
I (13391) http: receive TTS done
I (13391) http: HTTP POST Status = 200
I (13401) http: TTS deinit done
I (13651) http: HTTP GET Status = 200
I (13661) http: TTS init done
I (13661) http: TTS URL:http://tsn.baidu.com/text2audio?ctp=1&lan=zh&cuid=1234567C&tok=24.873731f094e2eda984e7953699e2d12a.2592000.1574922152.282335-17488148&tex=我和我的祖国，一刻也不能分割.&per=0&spd=5&pit=5&vol=5&aue=3
I (14061) http: state:4, recv len:59, heap:172816
...
I (16491) http: state:4, recv len:512, heap:177788
I (16491) http: state:4, recv len:269, heap:179512
I (16491) http: state:5, recv len:0, heap:179512
I (16501) http: receive TTS done
I (16501) http: HTTP POST Status = 200
I (16511) http: TTS deinit done
I (16921) http: HTTP GET Status = 200
I (16931) http: TTS init done
I (16931) http: TTS URL:http://tsn.baidu.com/text2audio?ctp=1&lan=zh&cuid=1234567C&tok=24.5ab14ca5b1134538dd49e4d003d646b0.2592000.1574922155.282335-17488148&tex=在那山的那边海的那边，有一群蓝精灵，他们活泼又聪明，他们调皮又灵敏，    他们自由自在生活在，那绿色的大森林，他们善良勇敢相互关心，    欧,可爱的蓝精灵，欧,可爱的蓝精灵，他们齐心协力开动脑筋，    斗败了格格巫，他们唱歌跳舞快乐又欢欣，mm qq，在那山的那边海的那边，有一群蓝精灵，他们活泼又聪明&per=0&spd=5&pit=5&vol=5&aue=3
I (17961) http: state:4, recv len:58, heap:172244
I (17961) http: state:4, recv len:512, heap:172244
...
I (47651) http: state:4, recv len:512, heap:177880
I (48641) http: state:4, recv len:218, heap:176856
I (48641) http: state:5, recv len:0, heap:176856
I (48641) http: receive TTS done
I (48651) http: HTTP POST Status = 200
I (48661) http: TTS deinit done
W (51661) mp3d: not enough FIFO len:1966,2088
I (52701) mp3d: MAD: Decoder done
I (52701) mp3d: decoder process end
I (52701) mp3d: Closed
I (52701) mp3d: delete mp3 decoder task
I (58911) http: HTTP GET Status = 200
I (58911) http: TTS init done
I (58911) http: TTS URL:http://tsn.baidu.com/text2audio?ctp=1&lan=zh&cuid=1234567C&tok=24.54d2eeeede9dbcaf7f0b2690712f3e65.2592000.1574922197.282335-17488148&tex=自古皆贵中华，贱夷狄，朕独爱之如一&per=0&spd=5&pit=5&vol=5&aue=3
I (59251) http: state:4, recv len:59, heap:206252
...
I (59261) http: state:4, recv len:512, heap:205388
I (59261) mp3d: start mp3 decoder task
I (59271) mp3d: MAD: Decoder start
I (59271) http: state:4, recv len:512, heap:175104
I (59271) mp3d: Codec Processing
I (59291) http: state:4, recv len:512, heap:172816
...
I (59401) http: state:4, recv len:149, heap:178508
I (59401) http: state:5, recv len:0, heap:178508
I (59401) http: receive TTS done
I (59411) http: HTTP POST Status = 200
I (59411) http: TTS deinit done
I (59411) main: Test TTS End
W (62401) mp3d: not enough FIFO len:1230,2088
I (63371) mp3d: MAD: Decoder done
I (63371) mp3d: decoder process end
I (63371) mp3d: Closed
I (63371) mp3d: delete mp3 decoder task
```

# 代码说明
1. 读者若对音频没有要求, 则可以直接参考 `app_main()` 中做法:
```c
    char* tts1 = "9 月 24 日，ESP-BLE-MESH 通过了蓝牙技术联盟（Bluetooth SIG）全功能支持的认证！";
    tts_download(tts1);
```

2. 如果需要配置音频, 则可以修改 `baidu_get_tts_default_config()` 实现:
```c
    int per = 0;    // {0, 1, 2, 3, 4, 5, 103, 106, 110, 111}, speech from different people
    int spd = 5;    // [0, 9], speech speed
    int pit = 5;    // [0, 9], speech tone
    int vol = 5;    // [0, 9], speech volume
    int aue = 3;    // {3.mp3, 4.pcm-16k, 5.pcm-8k, 6.wav} // speech format
    
    snprintf(sp_tts_config->cuid, sizeof(sp_tts_config->cuid), "1234567C");
    sp_tts_config->per = per;
    sp_tts_config->spd = spd;
    sp_tts_config->pit = pit;
    sp_tts_config->vol = vol;
    sp_tts_config->aue = aue;
    
    const char formats[4][4] = {"mp3", "pcm", "pcm", "wav"};
    snprintf(sp_tts_config->format, sizeof(sp_tts_config->format), formats[aue - 3]);
```
