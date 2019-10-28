/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019-2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS chips only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"

#include "mad.h"

// The mp3 read buffer size. 2106 bytes should be enough for up to 48KHz mp3s according to the sox sources. Used by libmad.
#define MAD_MP3_BUFFER_SZ                   (2106)
#define RING_BUFFER_SIZE                    (8 * 1024)

#define CODEC_DONE                          -2
#define CODEC_FAIL                          -1
#define CODEC_OK                            0

typedef struct {
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
} MadSync;

typedef struct {
    char *buffer;
    // MadSync *sync;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    int _run;
    int currentPosition;
    int _skipId3;
    // int framecnt;
    // int pcmcnt;
} MadMP3Codec;

typedef struct {
    RingbufHandle_t ring_buf;
    uint32_t filled_len;
} audio_ringbuff_t;

/**
 * @brief   Get MP3 decoder ring buffer handle
*/
audio_ringbuff_t* mp3_decoder_get_ringbuffer_handle(void);

/**
 * @brief   Init ring buffer
*/
void mp3_ringbuffer_init(void);

/**
 * @brief   Start decoder task
*/
void esp_start_mp3_decoder_task(void);

#ifdef __cplusplus
}
#endif
