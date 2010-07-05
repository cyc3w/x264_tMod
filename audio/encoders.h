#ifndef AUDIO_ENCODERS_H_
#define AUDIO_ENCODERS_H_

#include "audio/audio.h"
#include "filters/audio/audio_filters.h"

typedef struct audio_encoder_t
{
    hnd_t (*init)( hnd_t filter_chain, const char *opts );
    audio_info_t *(*get_info)( hnd_t handle );
    audio_samples_t *(*get_next_packet)( hnd_t handle );
    void (*free_packet)( hnd_t handle, audio_samples_t *samples );
    void (*close)( hnd_t handle );
} audio_encoder_t;

extern const audio_encoder_t audio_encoder_raw;
extern const audio_encoder_t audio_encoder_mp3;

/* allowed_list[0] is the prefered encoder if encoder is "default"
 * allowed_list = NULL means any valid encoder is allowed
 * The 'none' case isn't handled by this function (will return NULL like with any other invalid encoder)
 * If the user wants 'none' to be a default, it must be tested outside of this function
 * If the user wants to allow any encoder, the default case must be tested outside of this function
 */
const audio_encoder_t *select_audio_encoder( char *encoder, char* allowed_list[] );
hnd_t audio_encoder_open( const audio_encoder_t *encoder, hnd_t filter_chain, const char *opts );

audio_info_t *audio_encoder_info( hnd_t encoder );
audio_samples_t *audio_encode_frame( hnd_t encoder );
void audio_free_frame( hnd_t encoder, audio_samples_t *frame );

void audio_encoder_close( hnd_t encoder );

#endif
