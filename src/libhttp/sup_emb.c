#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <klone/supplier.h>
#include <klone/queue.h>
#include <klone/io.h>
#include <klone/debug.h>
#include <klone/page.h>
#include <klone/utils.h>
#include <klone/emb.h>
#include <klone/codgzip.h>

static int supemb_is_valid_uri(const char* uri, size_t len, time_t *mtime)
{
    embres_t *e;
    char filename[PATH_MAX] = { 0 };

    dbg_err_if(len >= PATH_MAX);

    strncpy(filename, uri, len);

    if(emb_lookup(filename, &e) == 0)
    {   /* resource found */

        if(e->type == ET_FILE)
            *mtime = ((embfile_t*)e)->mtime;
        else
            *mtime = 0; /* dynamic pages cannot be cached */

        return 1;
    }

err:
    return 0; /* not found */
}

static int supemb_serve_static(request_t *rq, response_t *rs, embfile_t *e)
{
    codec_gzip_t *gzip = NULL;
    int sai = 0; /* send as is */

    dbg("mime type: %s (%scompressed)", e->mime_type, (e->comp ? "" : "NOT "));

    /* set content-type, last-modified and content-length*/
    dbg_err_if(response_set_content_type(rs, e->mime_type));
    dbg_err_if(response_set_last_modified(rs, e->mtime));
    dbg_err_if(response_set_content_length(rs, e->file_size));

    /* if the client can accept deflated content then don't uncompress the 
       resource but send as it is */
    if(e->comp && (sai = request_is_encoding_accepted(rq, "deflate")) != 0)
    {   /* we can send compressed responses */
        dbg_err_if(response_set_content_encoding(rs, "deflate"));
        dbg_err_if(response_set_content_length(rs, e->size));
    } 

    if(request_get_method(rq) == HM_HEAD)
        return 0; /* just the header is requested */

    /* if needed apply a gzip codec to uncompress content data */
    if(e->comp && !sai)
    {
        dbg_err_if(codec_gzip_create(GZIP_UNCOMPRESS, &gzip));
        dbg_err_if(io_set_codec(response_io(rs), (codec_t*)gzip));
        gzip = NULL; /* io_t owns it after io_set_codec */
    } 

    /* print out page content (the header will be autoprinted by the 
       response io filter */
    dbg_err_if(!io_write(response_io(rs), e->data, e->size));

    /* free the codec (if it has been set) */
    dbg_if(io_set_codec(response_io(rs), NULL)); 

    return 0;
err:
    if(gzip)
        codec_free((codec_t*)gzip);
    return ~0;
}

static int supemb_serve_dynamic(request_t *rq, response_t *rs, embpage_t *e)
{
    session_t *ss = NULL;
    config_t *c = NULL, *config = NULL;
    http_t *http = NULL;

    /* get the config object */
    dbg_err_if((http = request_get_http(rq)) == NULL || 
               (config = http_get_config(http)) == NULL);

    /* create/get the session */
    dbg_err_if(config_get_subkey(config, "session", &c));
    dbg_err_if(session_create(c, rq, rs, &ss));

    /* set some default values */
    dbg_err_if(response_set_content_type(rs, "text/html"));

    e->run(rq, rs, ss);

    /* flush the output buffer */
    io_flush(response_io(rs));

    /* save and destroy the session */
    session_free(ss);

    return 0;
err:
    io_flush(response_io(rs));
    if(ss)
        session_free(ss);
    return ~0;
}

static int supemb_serve(request_t *rq, response_t *rs)
{
    page_t *pg = NULL;
    char *file_name;
    int rc = 0;
    embres_t *e;

    file_name = request_get_resolved_filename(rq);
    dbg_ifb(file_name == NULL || emb_lookup(file_name, &e))
    {
        response_set_status(rs, HTTP_STATUS_NOT_FOUND); 
        return 0;
    }

    dbg("serving %s", e->filename);

    switch(e->type)
    {
    case ET_FILE:
        dbg_err_if(supemb_serve_static(rq, rs, (embfile_t*)e));
        break;
    case ET_PAGE:
        dbg_err_if(supemb_serve_dynamic(rq, rs, (embpage_t*)e));
        break;
    default:
        dbg_err_if("unknown res type");
    }

    return 0;
err:
    return ~0;
}

static int supemb_init()
{
    return 0;
}

static void supemb_term()
{
    return;
}

supplier_t sup_emb = {
    "embedded content supplier",
    supemb_init,
    supemb_term,
    supemb_is_valid_uri,
    supemb_serve
};

