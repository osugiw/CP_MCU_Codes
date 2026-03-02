#include "mic_pdm.h"
#include "driver/i2s_common.h"
#include "driver/i2s_pdm.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_enc_reg.h"
#include "esp_audio_enc.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_audio_simple_dec.h"
#include "esp_timer.h"

i2s_chan_handle_t   rx_handle;                 /*  I2S rx channel handler */ 
static const char* DEC_TAG = "AAC DECODER";
static aac_write_ctx_t aac_write_ctx;
static aac_read_ctx_t  aac_read_ctx;
uint8_t *audio_buffer = (uint8_t *)malloc(I2S_RECV_BUFFER_SIZE);
uint8_t *fileWrite_buffer = (uint8_t *)malloc(AAC_CODEC_BUFFER_SIZE);

MIC_I2S INMP441;

/***********************************************************/
/********************** MIC_I2S Class **********************/
/**
*   @brief  Class constructor for MIC_I2S
*/
MIC_I2S::MIC_I2S(){}

/**
*   @brief  Class destructor for MIC_I2S
*   @param  None
*/
MIC_I2S::~MIC_I2S(){};

/**
*   @brief  Initialize I2S peripheral and channel
*   @param  None
*/
void MIC_I2S::init_pdm(void)
{
    esp_err_t err;

    /* Determine the I2S channel configuration and allocate both channels */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER); // dma_desc_num = 6, dma_frame_num = 240
    chan_cfg.dma_desc_num = I2S_CHANNEL_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = I2S_CHANNEL_DMA_FRAME_NUM;
    err = i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(RECORD_TAG, "Failed to create I2S channel");
        return;
    }

    /* Setting the configurations of standard mode, and initialize rx & tx channels */
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg  = I2S_PDM_RX_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
#if SOC_I2S_SUPPORTS_PDM2PCM
        .slot_cfg = I2S_PDM_RX_SLOT_PCM_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
#else
        .slot_cfg = I2S_PDM_RX_SLOT_RAW_FMT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
#endif
        .gpio_cfg = {
            .clk    = GPIO_I2S_CLK_IO,
            .din    = GPIO_I2S_DATA_IO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    /* Initialize the channels */
    err = i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_rx_cfg);
    if(err != ESP_OK)
    {
        ESP_LOGE(RECORD_TAG, "Failed to initialize PDM RX mode");
        return;
    }
    
    /* Enable the RX channel */
    err = i2s_channel_enable(rx_handle);
    if(err != ESP_OK)
    {
        ESP_LOGE(RECORD_TAG, "Failed to enable I2S channel");
        return;
    }

    
    ESP_LOGI(RECORD_TAG, "I2S PDM Microphone Initialized");
}

/**
*   @brief  Read raw I2S Data
*   @param  None
*/
uint8_t MIC_I2S::raw_i2s(void)
{
  // Get I2S data and place in data buffer
  size_t bytesIn = 0;
  float mean = 0;

  if (i2s_channel_read(rx_handle, (void*)audio_buffer, I2S_RECV_BUFFER_SIZE, &bytesIn, portMAX_DELAY) == ESP_OK)
  {
    // Read I2S data buffer
    uint8_t samples_read = bytesIn / 8;
    if (samples_read > 0) {
        
        for (uint8_t i = 0; i < samples_read; ++i) {
            mean += (audio_buffer[i]);
        }
        // Average the data reading+6666666666666666666666666666666666666666
        mean /= samples_read;
    }
  }
  return mean;
}

bool MIC_I2S::adjust_volume(int16_t *buffer, size_t samples, float scale) {
    bool clipped = false;
    for (size_t i = 0; i < samples; i++) {
        int32_t temp = (int32_t)(buffer[i] * scale);
        
        if (temp > 32767) {
            temp = 32767;
            clipped = true;
        } else if (temp < -32768) {
            temp = -32768;
            clipped = true;
        }
        
        buffer[i] = (int16_t)temp;
    }
    return clipped; // Returns true if any sample in this buffer was clamped
}

esp_err_t MIC_I2S::i2s_record_audio_aac(uint32_t rec_time, const char* fileName)
{
    // Use POSIX and C standard library functions to work with files.
    esp_err_t err = ESP_OK;
    uint32_t flash_wr_size = 0;
    size_t bytes_read = 0;
    uint32_t flash_rec_time = (CODEC_BITRATE * rec_time) / 8;

    // Register AAC Encoder
    err = esp_aac_enc_register();                     
    if(err != ESP_OK)
    {
        ESP_LOGI(RECORD_TAG, "Failed to register AAC Encoder");
        return ESP_FAIL;
    }

    // Configuration for AAC encoder
    esp_aac_enc_config_t aac_cfg = {
        .sample_rate        = I2S_SAMPLE_RATE,
        .channel            = ESP_AUDIO_MONO,
        .bits_per_sample    = I2S_BIT_SAMPLE,
        .bitrate            = CODEC_BITRATE,                       
        .adts_used          = true,             // Use ADTS header
    };
    // Audio Encoder Config
    esp_audio_enc_config_t enc_cfg = {
        .type = ESP_AUDIO_TYPE_AAC,
        .cfg = &aac_cfg,
        .cfg_sz = sizeof(aac_cfg)
    };
    // Open encoder
    esp_audio_enc_handle_t encoder = NULL;
    err = esp_audio_enc_open(&enc_cfg, &encoder);
    if(err != ESP_OK)
    {
        ESP_LOGI(RECORD_TAG, "Failed to open AAC Encoder");
        return ESP_FAIL;
    }

    // Creating a file
    int fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0);
    if (fd < 0) {
        ESP_LOGE(RECORD_TAG, "Failed to open file for writing - %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(RECORD_TAG, "Recording sound for %s", fileName);

    uint64_t start_time = esp_timer_get_time();
    uint64_t record_end_time = start_time + (rec_time * 1000000ULL);
    uint8_t audio_gain = AUDIO_GAIN;
    while (esp_timer_get_time() < record_end_time) {
        err = i2s_channel_read(rx_handle, audio_buffer, I2S_RECV_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
        bool isClipped = adjust_volume((int16_t*)audio_buffer, bytes_read / 2, audio_gain); // Example: 1.5x gain
        
        if(isClipped) {
            audio_gain -= 10; // Reduce gain slightly for the next chunk
            if (audio_gain < 10) audio_gain = 10; // Don't go below original volume
            ESP_LOGW("AUDIO", "Clipping detected! Reducing gain to %d", audio_gain);
        }
        if(err == ESP_OK && bytes_read > 0) {
            for (int offset = 0; offset + AAC_CODEC_BUFFER_SIZE <= bytes_read; offset += AAC_CODEC_BUFFER_SIZE) 
            { 
                esp_audio_enc_in_frame_t in_frame = {
                    .buffer = audio_buffer + offset,
                    .len = AAC_CODEC_BUFFER_SIZE,
                };

                esp_audio_enc_out_frame_t out_frame = {
                    .buffer = fileWrite_buffer,
                    .len = AAC_CODEC_BUFFER_SIZE,
                };

                err = esp_audio_enc_process(encoder, &in_frame, &out_frame);
                if (err == ESP_OK && out_frame.encoded_bytes > 0) {
                    size_t written = write(fd, (char*)out_frame.buffer, out_frame.encoded_bytes); // maybe the len is not the same as AAC_CODEC_BUFFER_SIZE, need to use the actual length of encoded data
                    flash_wr_size += written;
                }
            }
        }
    }

    // Clean-up resources
    esp_audio_enc_close(encoder);
    esp_audio_enc_unregister(ESP_AUDIO_TYPE_AAC);
    ESP_LOGI(RECORD_TAG,"Recording done");
    close(fd);
    ESP_LOGI(RECORD_TAG,"File written on SDCard with size %d", flash_wr_size);
    return ESP_OK;
}

esp_err_t MIC_I2S::i2s_read_aac_file(const char *fileName)
{
    std::string sdPath;
    sdPath.append("/sdcard/"); sdPath.append(fileName);

    int fd = open(sdPath.c_str(), O_RDONLY, 0);
    if (fd == NULL) {
        ESP_LOGE(DEC_TAG, "File %s not found", sdPath.c_str());
        return ESP_FAIL;
    }
    ESP_LOGI(DEC_TAG, "Decoding AAC File %s", sdPath.c_str());

    uint32_t max_out_size = AAC_CODEC_BUFFER_SIZE;
    uint32_t read_size = AAC_CODEC_BUFFER_SIZE;
    uint8_t *in_buf  = (uint8_t *)malloc(read_size);
    uint8_t *out_buf = (uint8_t *)malloc(max_out_size);
    
    int ret = esp_aac_dec_register();
    esp_audio_simple_dec_handle_t decoder = NULL;
    do{
        if (in_buf == NULL || out_buf == NULL) {
            ESP_LOGI(DEC_TAG, "No memory for decoder");
            ret = ESP_AUDIO_ERR_MEM_LACK;
            close(fd);
            break;
        }

        esp_aac_dec_cfg_t aac_cfg = {
            .no_adts_header = false,
        };
        esp_audio_simple_dec_cfg_t dec_cfg = {
            .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_AAC,
            .dec_cfg  = &aac_cfg,
            .cfg_size = sizeof(aac_cfg),
        };
        esp_audio_simple_dec_info_t dec_info = {};

        ret = esp_audio_simple_dec_open(&dec_cfg, &decoder);
        if (ret != ESP_AUDIO_ERR_OK) {
            ESP_LOGE(DEC_TAG, "Fail to open simple decoder ret %d", ret);
            close(fd);
            break;
        }
        
        int total_decoded = 0;
        uint64_t decode_time = 0;
        esp_audio_simple_dec_raw_t raw = {
            .buffer = in_buf,
        };
        uint64_t read_start = esp_timer_get_time();
        while (ret == ESP_AUDIO_ERR_OK) {
            ret = read(fd, in_buf, read_size);
            if (ret < 0) {
                close(fd);
                break;
            }
            raw.buffer = in_buf;
            raw.len = ret;
            raw.eos = (ret < read_size);
            esp_audio_simple_dec_out_t out_frame = {
                .buffer = out_buf,
                .len = max_out_size,
            };
            // ATTENTION: when input raw data unconsumed (`raw.len > 0`) do not overwrite its content
            // Or-else unexpected error may happen for data corrupt.
            while (raw.len) {
                uint64_t start = esp_timer_get_time();
                if (start > read_start + 30000000) {
                    raw.eos = true;
                    break;
                }

                ret = esp_audio_simple_dec_process(decoder, &raw, &out_frame);
                decode_time += esp_timer_get_time() - start;
                if (ret == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                    // Handle output buffer not enough case
                    uint8_t *new_buf = (uint8_t*)realloc(out_buf, out_frame.needed_size);
                    if (new_buf == NULL) {
                        break;
                    }
                    out_buf = new_buf;
                    out_frame.buffer = new_buf;
                    max_out_size = out_frame.needed_size;
                    out_frame.len = max_out_size;
                    continue;
                }
                if (ret != ESP_AUDIO_ERR_OK) {
                    ESP_LOGE(DEC_TAG, "Fail to decode data ret %d", ret);
                    close(fd);
                    break;
                }
                
                if (out_frame.decoded_size) {
                    if (total_decoded == 0) {
                        // Update audio information
                        esp_audio_simple_dec_get_info(decoder, &dec_info);
                        ESP_LOGI(DEC_TAG, "frame_rate= %"PRIi32", ch=%d, width=%d, bitrate=%"PRIi32"", 
                            dec_info.sample_rate, dec_info.channel, dec_info.bits_per_sample, dec_info.bitrate);
                    }
                    total_decoded += out_frame.decoded_size;

                    // /*  Stream the decoded data */
                    // ESP_LOGI(DEC_TAG, "Decoded AAC data (%ld bit) - length: %ld", out_frame.decoded_size, out_frame.len); // 2048 bit - length: 24576
                }
                // In case that input data contain multiple frames
                raw.len -= raw.consumed;
                raw.buffer += raw.consumed;
            }
            if (raw.eos) {
                break;
            }
        }
        if (total_decoded) {
            int sample_size = dec_info.channel * dec_info.bits_per_sample >> 3;
            float cpu_usage = (float)decode_time * sample_size * dec_info.sample_rate / total_decoded / 10000;
            ESP_LOGI(DEC_TAG, "Decode for AAC cpu: %.2f%%", cpu_usage);
        }
    } while (0);
    
    esp_audio_simple_dec_close(decoder);
    esp_audio_simple_dec_unregister_default();
    esp_audio_dec_unregister_default();
    if (in_buf) {
        free(in_buf);
    }
    if (out_buf) {
        free(out_buf);
    }
    if (ret != 0) {
        ESP_LOGE(DEC_TAG, "Fail to do simple decoder process");
    }
    close(fd);
    return ret;
}