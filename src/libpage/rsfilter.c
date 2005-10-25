#include <time.h>
#include <klone/response.h>
#include <klone/utils.h>
#include <klone/io.h>
#include <klone/codec.h>
#include <klone/http.h>
#include <klone/response.h>
#include <klone/rsfilter.h>
#include <u/libu.h>

/* this filter prints the HTTP header before any body part of the web page. 
 * the first RFBUFSZ bytes (at most) of the response will be buffered to 
 * postpone the header printing (the header can be modified until filter 
 * flush)
 */

enum { 
    RFS_BUFFERING,
    RFS_FLUSHING
};

struct response_filter_s
{
    codec_t codec;          /* must be the first item in the struct */
    struct response_s *rs;  /* the response object                  */
    int state;              /* filter state                         */
    char buf[RFBUFSZ], *ptr;
    size_t off;
    io_t *iob;
};

static int rf_init_iob(response_filter_t *rf)
{
    char *h;
    size_t hsz, htell;

    hsz = response_get_max_header_size(rf->rs) + rf->off;

    h = (char *)u_zalloc(hsz);
    dbg_err_if(h == NULL);

    dbg_err_if(io_mem_create(h, hsz, 0, &rf->iob));

    /* write the header to the memory io_t */
    response_print_header_to_io(rf->rs, rf->iob);

    if(response_get_method(rf->rs) != HM_HEAD)
    {
        /* append the rf->buf to the iob */
        dbg_err_if(io_write(rf->iob, rf->buf, rf->off) < 0);
    }
    dbg_err_if(io_flush(rf->iob));

    htell = io_tell(rf->iob);

    dbg_if(io_free(rf->iob));
    rf->iob = NULL;

    /* create another in-memory io to read from it */
    dbg_err_if(io_mem_create(h, htell, IO_MEM_FREE_BUF, &rf->iob));

    return 0;
err:
    return ~0;
}

static int rf_flush(response_filter_t *rf, char *dst, size_t *dcount)
{
    ssize_t c;

    if(rf->state == RFS_BUFFERING)
    {
        rf->state = RFS_FLUSHING;

        /* create a in-memory io_t and fill it with header and rf->buf */
        dbg_err_if(rf_init_iob(rf));
    }

    if(rf->iob)
    {
        dbg_err_if((c = io_read(rf->iob, dst, *dcount)) < 0);
        if(c == 0)
        { /* eof */
            io_free(rf->iob);
            rf->iob = NULL;
        } else {
            *dcount = c;
            return CODEC_FLUSH_CHUNK;
        }
    }

    return CODEC_FLUSH_COMPLETE;
err:
    return -1;
}

static ssize_t rf_transform(response_filter_t *rf, 
        char *dst, size_t *dcount, 
        const char *src, size_t src_sz)
{
    size_t max;
    ssize_t c;

    /* if this's a HEAD request don't print the body of the page */
    if(response_get_method(rf->rs) == HM_HEAD)
    {
        *dcount = 0;    /* zero output byte written */
        return src_sz;  /* all input bytes consumed */
    }

    if(rf->state == RFS_BUFFERING)
    {
        if(rf->off + src_sz < RFBUFSZ)
        {
            memcpy(rf->buf + rf->off, src, src_sz);
            rf->off += src_sz;
            *dcount = 0;    /* zero output byte written */
            return src_sz;  /* src_sz input byte consumed */
        } else {
            /* the buffer is full, print the header and flush the buffer */
            rf->state = RFS_FLUSHING;

            /* create a in-memory io_t and fill it with header and rf->buf */
            dbg_err_if(rf_init_iob(rf));
        }
    }

    if(rf->iob)
    {
        dbg_err_if((c = io_read(rf->iob, dst, *dcount)) < 0);
        if(c == 0)
        { /* eof */
            io_free(rf->iob);
            rf->iob = NULL;
        } else {
            *dcount = c;
            return 0;
        }
    }

    /* copyout the next data block */
    max = MIN(*dcount, src_sz);
    memcpy(dst, src, max);
    *dcount = max;
    return max;
err:
    return -1;
}

static int rf_free(response_filter_t *rf)
{
    if(rf->iob)
        io_free(rf->iob);

    u_free(rf);

    return 0;
}

int response_filter_create(response_t *rs, response_filter_t **prf)
{
    response_filter_t *rf = NULL;

    rf = u_zalloc(sizeof(response_filter_t));
    dbg_err_if(rf == NULL);

    rf->rs = rs;
    rf->codec.transform = rf_transform;
    rf->codec.flush = rf_flush;
    rf->codec.free = rf_free;
    rf->ptr = rf->buf;
    rf->iob = NULL;

    *prf = rf;

    return 0;
err:
    if(rf)
        u_free(rf);
    return ~0;
}
