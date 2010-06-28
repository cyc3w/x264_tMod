#include "audio/encoders.h"
#include "filters/audio/internal.h"

#include "lame/lame.h"
#include <assert.h>

typedef struct enc_lame_t {
    audio_info_t *info;
    audio_info_t *af_info;
    hnd_t filter_chain;

    lame_global_flags *lame;
    int64_t last_sample;
    uint8_t *buffer;
    size_t bufsize;
    audio_samples_t in;
} enc_lame_t;

static hnd_t init( hnd_t filter_chain, const char *opt_str )
{
    assert( filter_chain );
    enc_lame_t *h = calloc( 1, sizeof( enc_lame_t ) );
    audio_hnd_t *chain = h->filter_chain = filter_chain;
    h->af_info = af_get_info(chain);
    h->info = malloc( sizeof( audio_info_t ) );
    memcpy( h->info, h->af_info, sizeof( audio_info_t ) );

    if( h->info->chansize != 2 )
    {
        fprintf( stderr, "lame [error]: lame only supports 16 bits per channel.\n" );
        fprintf( stderr, "lame [error]: FIXME: auto-insert converter when it is implemented.\n");
        goto error;
    }

    char *optlist[] = { "bitrate", "vbr", "quality", NULL };
    char **opts     = split_options( opt_str, optlist );
    assert( opts );

    char *cbr = get_option( "bitrate", opts );
    char *vbr = get_option( "vbr"    , opts );
    char *qua = get_option( "quality", opts );

    free_string_array( opts );

    assert( ( cbr && !vbr ) || ( !cbr && vbr ) );

    h->info->codec_name     = "mp3";
    h->info->extradata      = NULL;
    h->info->extradata_size = 0;

    h->lame = lame_init();
    lame_set_in_samplerate( h->lame, h->info->samplerate );
    lame_set_num_channels( h->lame, h->info->channels );
    lame_set_quality( h->lame, 0 );

    if( cbr )
    {
        lame_set_VBR( h->lame, vbr_off );
        lame_set_brate( h->lame, atoi( cbr ) );
    }
    if( vbr )
    {
        lame_set_VBR( h->lame, vbr_default );
        lame_set_VBR_quality( h->lame, atof( vbr ) );
    }
    if( qua )
        lame_set_quality( h->lame, atoi( qua ) );

    lame_init_params( h->lame );

    h->info->framelen = lame_get_framesize( h->lame );
    h->info->framesize = h->info->framelen * h->info->samplesize;

    h->bufsize = 125 * h->info->framelen / 100 + 7200;

    return h;

error2:
    lame_close( h->lame );
error:
    free( h->info );
    free( h );
    return NULL;
}

static audio_info_t *get_info( hnd_t handle )
{
    assert( handle );
    enc_lame_t *h = handle;

    return h->info;
}

static audio_samples_t *get_next_packet( hnd_t handle )
{
    enc_lame_t *h = handle;

    audio_samples_t *out = calloc( 1, sizeof( audio_samples_t ) );
    out->data = malloc( h->bufsize );

    while( !out->len )
    {
        if( h->in.flags & AUDIO_FLAG_EOF )
        {
            out->len = lame_encode_flush( h->lame, out->data, h->bufsize );
            if( !out->len )
                goto error;
            break;
        }

        int res = af_get_samples( &h->in, h->filter_chain, h->last_sample, h->last_sample + h->info->framelen );
        if( res == AUDIO_ERROR )
            goto error;
        h->last_sample += h->info->framelen;

        out->len = lame_encode_buffer_interleaved( h->lame, (short*) h->in.data,
                                                   h->in.len / h->af_info->samplesize,
                                                   out->data, h->bufsize );
        af_free_samples( &h->in );
    }

    return out;

error:
    free( out );
    return NULL;
}

static void free_packet( hnd_t handle, audio_samples_t *packet )
{
    af_free_samples( packet );
    free( packet );
}

static void close( hnd_t handle )
{
    enc_lame_t *h = handle;

    lame_close( h->lame );
    free( h->info );
    free( h );
}

const audio_encoder_t audio_encoder_lame = {
    .init = init,
    .get_info = get_info,
    .get_next_packet = get_next_packet,
    .free_packet = free_packet,
    .close = close
};

