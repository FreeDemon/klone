#include "conf.h"
#ifdef HAVE_LIBOPENSSL
#include <unistd.h>
#include <klone/io.h>
#include <klone/ioprv.h>
#include <u/libu.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


typedef struct io_ssl_s
{
    struct io_s io; /* must be the first item */
    SSL *ssl;
    int fd;
    int flags;
} io_ssl_t;

static ssize_t io_ssl_read(io_ssl_t *io, char *buf, size_t size);
static ssize_t io_ssl_write(io_ssl_t *io, const char *buf, size_t size);
static int io_ssl_term(io_ssl_t *io);

static ssize_t io_ssl_read(io_ssl_t *io_ssl, char *buf, size_t size)
{
    ssize_t c;

again:
    c = SSL_read(io_ssl->ssl, buf, size);
    if(c < 0 && (errno == EINTR || errno == EAGAIN))
        goto again; 

    dbg_err_if(c < 0); 

    return c;
err:
    return -1;
}

static ssize_t io_ssl_write(io_ssl_t *io_ssl, const char *buf, size_t size)
{
    ssize_t c;

again:
    c = SSL_write(io_ssl->ssl, buf, size);
    if(c < 0 && (errno == EINTR || errno == EAGAIN))
        goto again; 

    dbg_err_if(c < 0); 

    return c;
err:
    return -1;
}

static int io_ssl_term(io_ssl_t *io_ssl)
{
    dbg_err_if(io_ssl == NULL);

    SSL_free(io_ssl->ssl);
    if(io_ssl->flags & IO_FD_CLOSE)
    {
        close(io_ssl->fd);
        io_ssl->fd = -1;
    }

    return 0;
err:
    return -1;
}

int io_ssl_create(int fd, int flags, SSL_CTX *ssl_ctx, io_t **pio)
{
    io_ssl_t *io_ssl = NULL;

    dbg_err_if(ssl_ctx == NULL);

    dbg_err_if(io_create(io_ssl_t, (io_t**)&io_ssl));

    io_ssl->fd = fd;
    io_ssl->flags = flags;

    io_ssl->ssl = SSL_new(ssl_ctx);
    dbg_err_if(io_ssl->ssl == NULL);

    /* assign a working descriptor to the SSL stream */
    dbg_err_if(SSL_set_fd(io_ssl->ssl, fd) == 0);

    io_ssl->io.read     = (io_read_op) io_ssl_read;
    io_ssl->io.write    = (io_write_op) io_ssl_write;
    io_ssl->io.term     = (io_term_op) io_ssl_term; 
    io_ssl->io.size     = NULL;
    
    /* start a SSL connection */
    dbg_err_if(SSL_accept(io_ssl->ssl) <= 0);

    *pio = (io_t*)io_ssl;

    return 0;
err:
    if(io_ssl && io_ssl->ssl)
    {
        ERR_print_errors_fp( stderr );
        SSL_set_shutdown(io_ssl->ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
    }
    if(io_ssl)
        io_free(io_ssl);
    return ~0;
}

#endif

