/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
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
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"

#include "esp_log.h"
#include "esp_system.h"

#include "driver/i2s.h"
#include "mp3_decoder.h"
#include "mad.h"

#define I2S_NUM             (0)
#define ADD_DEL_BUFFPERSAMP (1)

static uint32_t oldRate;
static bool s_mp3_task_started = false;
static audio_ringbuff_t* p_audio_ringbuf;

static const char* TAG = "mp3d";

static void esp_set_mp3_decoder_task_flag(bool flag)
{
    s_mp3_task_started = flag;
}

static bool esp_get_mp3_decoder_task_flag(void)
{
    return s_mp3_task_started;
}

audio_ringbuff_t* mp3_decoder_get_ringbuffer_handle(void)
{
    return p_audio_ringbuf;
}

void mp3_ringbuffer_init(void)
{
    if (p_audio_ringbuf == NULL) {
        ESP_LOGI(TAG, "Create Ring Buffer");
        p_audio_ringbuf = (audio_ringbuff_t *)malloc(sizeof(audio_ringbuff_t));
        p_audio_ringbuf->ring_buf = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
        p_audio_ringbuf->filled_len = 0;
    }
}

// Reformat the 16-bit mono sample to a format we can send to I2S.
static int sampToI2s(short s)
{
    // We can send a 32-bit sample to the I2S subsystem and the DAC will neatly split it up in 2
    // 16-bit analog values, one for left and one for right.

    // Duplicate 16-bit sample to both the L and R channel
    int samp = s;
    samp = (samp) & 0xffff;
    samp = (samp << 16) | samp;
    return samp;
}

// This routine is called by the NXP modifications of libmad. It passes us (for the mono synth)
// 32 16-bit samples.
void render_sample_block(short* short_sample_buff, int no_samples)
{
    // Signed 16.16 fixed point number: the amount of samples we need to add or delete
    // in every 32-sample
    static int sampAddDel = 0;
    // Remainder of sampAddDel cumulatives
    static int sampErr = 0;
    int i;
    int samp;
/*
#ifdef ADD_DEL_SAMPLES
    sampAddDel = recalcAddDelSamp(sampAddDel);
#endif
*/
    sampErr += sampAddDel;
    size_t i2s_bytes_write = 0;
    uint8_t* data = (uint8_t*)malloc(sizeof(uint8_t) * no_samples * 8);
    int count = 0;

    for (i = 0; i < no_samples; i++) {
        samp = sampToI2s(short_sample_buff[i]);

        // Dependent on the amount of buffer we have too much or too little, we're going to add or remove
        // samples. This basically does error diffusion on the sample added or removed.
        if (sampErr > (1 << 24)) {
            sampErr -= (1 << 24);
            // ...and don't output an i2s sample
        } else if (sampErr < -(1 << 24)) {
            sampErr += (1 << 24);
            // ..and output 2 samples instead of one.
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
        } else {
            // Just output the sample.
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
        }
    }

    i2s_write(I2S_NUM, data, count, &i2s_bytes_write, portMAX_DELAY);
    free(data);
}

// Called by the NXP modificationss of libmad. Sets the needed output sample rate.
void set_dac_sample_rate(int rate)
{
    if (rate == oldRate) {
        return;
    }

    oldRate = rate;
    ESP_LOGI(TAG, "Rate %d", rate);

    i2s_set_sample_rates(I2S_NUM, rate);
}

static enum mad_flow input(MadMP3Codec* decoder)
{
    struct mad_stream* stream = &(decoder->stream);
    char* readBuf = decoder->buffer;
    int n, fifo_have_data_len;
    int rem;
    size_t recv_len = 0;
    audio_ringbuff_t* audio_ringbuf_handle = mp3_decoder_get_ringbuffer_handle();

    // Shift remaining contents of buf to the front
    rem = stream->bufend - stream->next_frame;
    memmove(readBuf, stream->next_frame, rem);

    while (rem < MAD_MP3_BUFFER_SZ) {
        n = (MAD_MP3_BUFFER_SZ - rem);                      // Calculate amount of bytes we need to fill buffer.
        fifo_have_data_len = audio_ringbuf_handle->filled_len;

        if (fifo_have_data_len < n) {
            n = fifo_have_data_len;                         // If the fifo can give us less, only take that amount
        }

        if (n == 0) {                                       // Can't take anything?
            i2s_zero_dma_buffer(I2S_NUM);
            return MAD_FLOW_STOP;
        } else {
            // Read some bytes from the FIFO to re-fill the buffer.
            uint8_t* recv_data = (uint8_t *)xRingbufferReceiveUpTo(audio_ringbuf_handle->ring_buf, &recv_len, portMAX_DELAY, n);
            if(recv_data == NULL || recv_len != n) {
                ESP_LOGW(TAG, "not enough FIFO len:%d,%d", recv_len, n);
            }

            memcpy((uint8_t*)&readBuf[rem], recv_data, recv_len);
            vRingbufferReturnItem(audio_ringbuf_handle->ring_buf, (void*)recv_data);
            audio_ringbuf_handle->filled_len -= recv_len;
            rem += n;
        }
    }

    // Okay, let MAD decode the buffer
    mad_stream_buffer(stream, (unsigned char const*)readBuf, MAD_MP3_BUFFER_SZ);
    return MAD_FLOW_CONTINUE;
}

// Routine to print out an error
static enum mad_flow error(void* data, struct mad_stream* stream, struct mad_frame* frame)
{
    ESP_LOGD(TAG, "dec err 0x%04x (%s)\n", stream->error, mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}

MadMP3Codec *MadMP3Open(void)
{
    audio_ringbuff_t* audio_ringbuf_handle = mp3_decoder_get_ringbuffer_handle();
    assert(audio_ringbuf_handle != NULL);
    MadMP3Codec* mp3_decoder = malloc(sizeof(MadMP3Codec));

    if (mp3_decoder == NULL) {
        ESP_LOGE(TAG, "MAD: malloc(mp3_decoder) failed");
        return NULL;
    }

    mp3_decoder->buffer = malloc(MAD_MP3_BUFFER_SZ + 1);

    if (mp3_decoder->buffer == NULL) {
        ESP_LOGE(TAG, "MAD: malloc(mp3 input buffer) failed");
        return NULL;
    }

    ESP_LOGI(TAG, "MAD: Decoder start");
    mad_stream_init(&(mp3_decoder->stream));
    mad_frame_init(&(mp3_decoder->frame));
    mad_synth_init(&(mp3_decoder->synth));

    return mp3_decoder;
}

int MadMP3Process(MadMP3Codec* decoder)
{
    int r;
    if (input(decoder) == MAD_FLOW_STOP) {    // calls mad_stream_buffer internally
        ESP_LOGI(TAG, "MAD: Decoder done");
        return CODEC_DONE;
    }

    while (1) {
        r = mad_frame_decode(&(decoder->frame), &(decoder->stream));

        if (r == -1) {
            if (!MAD_RECOVERABLE((&(decoder->stream))->error)) {
                // We're most likely out of buffer and need to call input() again
                break;
            }

            error(NULL, &(decoder->stream), &(decoder->frame));
            continue;
        }

        mad_synth_frame(&(decoder->synth), &(decoder->frame));
    }

    return CODEC_OK;
}

int MadMP3Close(MadMP3Codec* codec)
{
    ESP_LOGI(TAG, "Closed");

    if (codec == NULL) {
        return CODEC_OK;
    }

    // mad_decoder_finish(codec->decoder);
    free(codec->buffer);
    free(codec);
    codec = NULL;
    return CODEC_OK;
}

void esp_mp3_decoder_process_task(void *pvParameters)
{
    int res = 0;
    MadMP3Codec * p = NULL;

    p = MadMP3Open();

    if (p != NULL) {
        ESP_LOGI(TAG, "Codec Processing");

        while (1) {
            res = MadMP3Process(p);

            if (res != 0 && res != -1) {
                ESP_LOGI(TAG, "decoder process end");
                break;
            } else if (res == -1) {
                ESP_LOGI(TAG, "decoder breaked");
                break;
            }
        }
    } else {
        ESP_LOGE(TAG, "Error open decoder");
    }

    MadMP3Close(p);
    ESP_LOGI(TAG, "delete mp3 decoder task");
    esp_set_mp3_decoder_task_flag(false);
    vTaskDelete(NULL);
}

void esp_start_mp3_decoder_task(void)
{
    if (!esp_get_mp3_decoder_task_flag()){
        ESP_LOGI(TAG, "start mp3 decoder task");
        esp_set_mp3_decoder_task_flag(true);
        xTaskCreate(esp_mp3_decoder_process_task, "mp3d", 8192*2, NULL, 5, NULL);
    }
}
