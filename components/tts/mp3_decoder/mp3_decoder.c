
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

static const char* TAG = "MP3_DECODER";

extern audio_ringbuff_t audio_ringbuf;

#define I2S_NUM         (0)
#define ADD_DEL_BUFFPERSAMP (1)

//Reformat the 16-bit mono sample to a format we can send to I2S.
static int sampToI2s(short s)
{
    //We can send a 32-bit sample to the I2S subsystem and the DAC will neatly split it up in 2
    //16-bit analog values, one for left and one for right.

    //Duplicate 16-bit sample to both the L and R channel
    int samp = s;
    samp = (samp) & 0xffff;
    samp = (samp << 16) | samp;
    return samp;
}
#if 0
//Calculate the number of samples that we add or delete. Added samples means a slightly lower
//playback rate, deleted samples means we increase playout speed a bit. This returns an
//8.24 fixed-point number
int recalcAddDelSamp(int oldVal)
{
    int ret;
    long prevUdr = 0;
    static int cnt;
    int i;
    static int minFifoFill = 0;
    uint32_t fill_len, total_len, underrun_len;

    spi_ram_fifo_get_fill(spi_ram_fifo_download, (uint32_t*)&i);

    if (i < minFifoFill) {
        minFifoFill = i;
    }

    //Do the rest of the calculations plusminus every 100mS (assuming a sample rate of 44KHz)
    cnt++;

    if (cnt < 1500) {
        return oldVal;
    }

    cnt = 0;

    spi_ram_fifo_get_total(spi_ram_fifo_download, &total_len);

    if (total_len < 10 * 1024) {
        //The FIFO is very small. We can't do calculations on how much it's filled on average, so another
        //algorithm is called for.
        int tgt = 1600; //we want an average of this amount of bytes as the average minimum buffer fill
        //Calculate underruns this cycle
        spi_ram_fifo_get_underrun(spi_ram_fifo_download, &underrun_len);
        int udr = underrun_len - prevUdr;

        //If we have underruns, the minimum buffer fill has been lower than 0.
        if (udr != 0) {
            minFifoFill = -1;
        }

        //If we're below our target decrease playback speed, and vice-versa.
        ret = oldVal + ((minFifoFill - tgt));
        prevUdr += udr;
        minFifoFill = 9999;
    } else {
        //We have a larger FIFO; we can adjust according to the FIFO fill rate.
        spi_ram_fifo_get_total(spi_ram_fifo_download, &total_len);
        int tgt = total_len / 2;
        spi_ram_fifo_get_fill(spi_ram_fifo_download, &fill_len);
        ret = (fill_len - tgt) * ADD_DEL_BUFFPERSAMP;
    }

    return ret;
}
#endif

//This routine is called by the NXP modifications of libmad. It passes us (for the mono synth)
//32 16-bit samples.
void render_sample_block(short* short_sample_buff, int no_samples)
{
    //Signed 16.16 fixed point number: the amount of samples we need to add or delete
    //in every 32-sample
    static int sampAddDel = 0;
    //Remainder of sampAddDel cumulatives
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

        //Dependent on the amount of buffer we have too much or too little, we're going to add or remove
        //samples. This basically does error diffusion on the sample added or removed.
        if (sampErr > (1 << 24)) {
            sampErr -= (1 << 24);
            //...and don't output an i2s sample
        } else if (sampErr < -(1 << 24)) {
            sampErr += (1 << 24);
            //..and output 2 samples instead of one.
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
        } else {
            //Just output the sample.
            data[count++] = (samp & 0x00FF0000) >> 16;    // Left: LSB
            data[count++] = (samp & 0xFF000000) >> 24;    // Left: MSB
            data[count++] = (samp & 0x000000FF);          // Right: LSB
            data[count++] = (samp & 0x0000FF00) >> 8;     // Right: MSB
        }
    }

    i2s_write(I2S_NUM, data, count, &i2s_bytes_write, portMAX_DELAY);
    free(data);
}

//Called by the NXP modificationss of libmad. Sets the needed output sample rate.
static uint32_t oldRate = 0;
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
    int n, i;
    int rem;
    size_t recv_len = 0;
    //Shift remaining contents of buf to the front
    rem = stream->bufend - stream->next_frame;
    memmove(readBuf, stream->next_frame, rem);

    while (rem < MAD_MP3_BUFFER_SZ) {
        n = (MAD_MP3_BUFFER_SZ - rem);   //Calculate amount of bytes we need to fill buffer.
        //spi_ram_fifo_get_fill(spi_ram_fifo_download, (uint32_t*)&i);
        //printf("spi i:%d\n", i);
        i = audio_ringbuf.filled_len;

        if (i < n) {
            n = i;                 //If the fifo can give us less, only take that amount
        }

        if (n == 0) {                    //Can't take anything?
            i2s_zero_dma_buffer(I2S_NUM);
            return MAD_FLOW_STOP;
        } else {
            //Read some bytes from the FIFO to re-fill the buffer.
            //spi_ram_fifo_read(spi_ram_fifo_download, (uint8_t*)&readBuf[rem], n, portMAX_DELAY);
            //spi_ram_fifo_read(spi_ram_fifo_download, (uint8_t *)&readBuf[rem], n);
            uint8_t* recv_data = (uint8_t*)xRingbufferReceiveUpTo(audio_ringbuf.ring_buf, &recv_len, portMAX_DELAY, n);
            if(recv_data == NULL || recv_len != n) {
                printf("Receive ringbuffer error, len:%d,%d\n", recv_len, n);
            }
            memcpy((uint8_t*)&readBuf[rem], recv_data, recv_len);
            vRingbufferReturnItem(audio_ringbuf.ring_buf, (void*)recv_data);
            audio_ringbuf.filled_len -= recv_len;
            rem += n;
        }
    }

    //Okay, let MAD decode the buffer.
    mad_stream_buffer(stream, (unsigned char const*)readBuf, MAD_MP3_BUFFER_SZ);

    return MAD_FLOW_CONTINUE;
}
//Routine to print out an error
static enum mad_flow error(void* data, struct mad_stream* stream, struct mad_frame* frame)
{
    ESP_LOGD(TAG, "dec err 0x%04x (%s)\n", stream->error, mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}

MadMP3Codec *MadMP3Open(void)
{
    assert(audio_ringbuf.ring_buf != NULL && (audio_ringbuf.filled_len != 0));
    MadMP3Codec* mp3_decoder = malloc(sizeof(MadMP3Codec));

    if (mp3_decoder == NULL) {
        printf("MAD: malloc(mp3_decoder) failed\n");
        return NULL;
    }

    mp3_decoder->buffer = malloc(MAD_MP3_BUFFER_SZ);

    if (mp3_decoder->buffer == NULL) {
        printf("MAD: malloc(mp3 input buffer) failed\n");
        return NULL;
    }

    printf("MAD: Decoder start.\n");
    mad_stream_init(&(mp3_decoder->stream));
    mad_frame_init(&(mp3_decoder->frame));
    mad_synth_init(&(mp3_decoder->synth));

    return mp3_decoder;
}

int MadMP3Process(MadMP3Codec* decoder)
{
    int r;
    if (input(decoder) == MAD_FLOW_STOP) { //calls mad_stream_buffer internally
        ESP_LOGI(TAG, "MAD: Decoder done\n");
        return CODEC_DONE;
    }

    while (1) {
        r = mad_frame_decode(&(decoder->frame), &(decoder->stream));

        if (r == -1) {
            if (!MAD_RECOVERABLE((&(decoder->stream))->error)) {
                //We're most likely out of buffer and need to call input() again
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

    //mad_decoder_finish(codec->decoder);
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
        printf("codecProcessing\n");

        while (1) {
            res = MadMP3Process(p);

            if (res != 0 && res != -1) {
                printf("decoder process error");
                break;
            } else if (res == -1) {
                printf("decoder breaked\r\n");
                break;
            }
        }
    } else {
        printf("Error open decoder");
    }

    MadMP3Close(p);
    printf("mp3 decoder task delete\r\n");
    vTaskDelete(NULL);
}

void esp_start_mp3_decoder_task(void)
{
    xTaskCreate(esp_mp3_decoder_process_task, "mp3d", 8192, NULL, 5, NULL);
}
