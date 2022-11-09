/** 22T3 COMP3601 Design Project A
 * File name: main.c
 * Description: Example main file for using the audio_i2s driver for your Zynq audio driver.
 *
 * Distributed under the MIT license.
 * Copyright (c) 2022 Elton Shih
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

#include "audio_i2s.h"
// #include "wav.h"


#define TRANSFER_RUNS 3750

#define NUM_CHANNELS 2
#define BPS 24
#define SAMPLE_RATE 48000
#define RECORD_DURATION 10
FILE *fptr;

void bin(uint8_t n) {
    uint8_t i;
    // for (i = 1 << 7; i > 0; i = i >> 1)
    //     (n & i) ? printf("1") : printf("0");
    for (i = 0; i < 8; i++) // LSB first
        (n & (1 << i)) ? printf("1") : printf("0");
}

void parsemem(void* virtual_address, int word_count) {
    
    uint32_t *p = (uint32_t *)virtual_address;
    char *b = (char*)virtual_address;
    int offset;

    uint32_t sample_count = 0;
    uint32_t sample_value = 0;
    for (offset = 0; offset < word_count; offset++) {
        sample_value = p[offset] & ((1<<18)-1);
        sample_count = p[offset] >> 18;
        fprintf(fptr, "%d\n", (int) sample_value);

    }

}

void create_wav(char *filename, uint32_t num_samples, uint16_t num_channels, uint32_t *data, uint32_t fs, uint16_t bit_depth) {
    FILE *wav_file;
    unsigned int bytes_per_sample = bit_depth / 8;
    uint32_t byte_rate = fs * num_channels * bytes_per_sample;
    uint32_t sub_chunk_1_size = 16;
    uint16_t pcm_format = 1;
    uint16_t block_align = num_channels * bytes_per_sample;
    uint32_t sub_chunk_2_size = num_samples * num_channels * bytes_per_sample;
    uint32_t chunk_size = 36 + sub_chunk_2_size;

    wav_file = fopen(filename, "w+");
    assert(wav_file);

    fwrite("RIFF", 1, 4, wav_file);
    fwrite(&chunk_size, sizeof(uint32_t), 1, wav_file);
    fwrite("WAVE", 1, 4, wav_file);

    fwrite("fmt ", 1, 4, wav_file);
    fwrite(&sub_chunk_1_size, sizeof(uint32_t), 1, wav_file);
    fwrite(&pcm_format, sizeof(uint16_t), 1, wav_file);
    fwrite(&num_channels, sizeof(uint16_t), 1, wav_file);
    fwrite(&fs, sizeof(uint32_t), 1, wav_file);
    fwrite(&byte_rate, sizeof(uint32_t), 1, wav_file);
    fwrite(&block_align, sizeof(uint16_t), 1, wav_file);
    fwrite(&bit_depth, sizeof(uint16_t), 1, wav_file);

    fwrite("data", sizeof(char), 4, wav_file);
    fwrite(&sub_chunk_2_size, sizeof(uint32_t), 1, wav_file);

    for (uint32_t i = 0; i < num_samples; i++) {
        fwrite(&data[i], bytes_per_sample, 1, wav_file);
    }


    fclose(wav_file);
}

int main() {
    printf("Entered main\n");
    fptr = fopen("sample.txt", "w");

    uint32_t frames[TRANSFER_RUNS][TRANSFER_LEN] = {0};
    uint32_t count = 0;

    audio_i2s_t my_config;
    if (audio_i2s_init(&my_config) < 0) {
        printf("Error initializing audio_i2s\n");
        return -1;
    }

    printf("mmapped address: %p\n", my_config.v_baseaddr);
    printf("Before writing to CR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_CR));
    audio_i2s_set_reg(&my_config, AUDIO_I2S_CR, 0x1);
    printf("After writing to CR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_CR));
    printf("SR: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_SR));
    printf("Key: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_KEY));
    printf("Before writing to gain: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_GAIN));
    audio_i2s_set_reg(&my_config, AUDIO_I2S_GAIN, 0x1);
    printf("After writing to gain: %08x\n", audio_i2s_get_reg(&my_config, AUDIO_I2S_GAIN));
    

    printf("Initialized audio_i2s\n");
    printf("Starting audio_i2s_recv\n");

    for (int i = 0; i < TRANSFER_RUNS; i++) {
        int32_t *samples = audio_i2s_recv(&my_config);
        memcpy(frames[i], samples, TRANSFER_LEN*sizeof(uint32_t));
        parsemem(frames[i], TRANSFER_LEN);
    }


    audio_i2s_release(&my_config);
    fclose(fptr);
    return 0;
}
