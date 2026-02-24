
#pragma once

#include "main.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Basic audio information
 */
typedef struct {
    uint8_t     channel;          /*!< Audio channel */
    uint8_t     bits_per_sample;  /*!< Audio bits per sample */
    int         sample_rate;      /*!< Sample rate */
    const void *spec_info;        /*!< Specified information for certain audio codec */
    int         spec_info_size;   /*!< Specified information length */
} audio_info_t;

/**
 * @brief Input encoded data to be decoded
 * 
 */
typedef struct {
    uint8_t *data;
    int      read_size;
    int      size;
} aac_read_ctx_t;

/**
 * @brief Output of decoded data
 * 
 */
typedef struct {
    uint8_t *data;
    int      write_size;
    int      read_size;
    int      size;
    int      decode_size;
} aac_write_ctx_t;

/*  MIC Class    */
class MIC_I2S{
    public:
        /**
        *   @brief  Class constructor for MIC I2S
        *   @param  None
        *   @return None
        */
        MIC_I2S();  

        /**
        *   @brief  Class destructor for MIC I2S
        *   @param  None
        *   @return None
        */
        ~MIC_I2S(); 

        /**
        *   @brief  Initialize I2S PDM Microphone
        *   @param  none
        *   @return None
        */
        void init_pdm(void);

        /**
        *   @brief  Read raw I2S Data
        *   @param  none
        *   @return I2S data
        */
        uint8_t raw_i2s(void);

        /**
         *  @brief  Adjust the volume of audio data
         *  @param buffer The audio data buffer to be adjusted
         *  @param samples The number of audio samples in the buffer
         *  @param scale The volume scale factor (e.g., 0.5 for half volume) 
         *  @return true if any sample was clipped during adjustment, false otherwise
         */
        bool adjust_volume(int16_t *buffer, size_t samples, float scale);

        /**
        *   @brief  Save Audio Recording in AAC Format
        *   @param  rec_time    Duration Time for rcording the audio
        *   @param  fileName    Title for the audio file name
        *   @return 
        *           Status code
        */
        esp_err_t i2s_record_audio_aac(uint32_t rec_time, const char* fileName);

        /**
        *   @brief  Read a AAC file from the SD Card
        *   @param  fileName    File name 
        *   @return 
        *           Read status code
        */
        esp_err_t i2s_read_aac_file(const char *fileName);
};


#ifdef __cplusplus
}
#endif