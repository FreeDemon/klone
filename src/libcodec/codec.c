#include <klone/codec.h>
#include <u/memory.h>

int codec_free(codec_t *codec)
{
    if(codec)
    {
        if(codec->free)
            codec->free(codec);
        else
            u_free(codec);
    }
    return 0;
}

