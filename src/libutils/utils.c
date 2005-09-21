#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <klone/klone.h>
#include <klone/os.h>
#include <klone/utils.h>
#include <klone/io.h>
#include <klone/codgzip.h>
#include <klone/emb.h>
#include <klone/mime_map.h>

/**
 *  \defgroup utils utils - Utility functions
 *  \{
 *      \par
 */

enum { LF = 0xA, CR = 0xD };

static struct html_entities_s
{
    int s_char;
    const char *entity; 
} entities[] = {
    { '"',  "&quot;" },
    { '\'', "&#39;"  }, 
    { '<',  "&lt;"   },
    { '>',  "&gt;"   },
    { 0, NULL     }
};


int u_sig_block(int sig)
{
    sigset_t sset;

    sigemptyset(&sset);
    sigaddset(&sset, sig);
    dbg_err_if(sigprocmask(SIG_BLOCK, &sset, NULL));

    return 0;
err:
    return ~0;
}

int u_sig_unblock(int sig)
{
    sigset_t sset;

    sigemptyset(&sset);
    sigaddset(&sset, sig);
    dbg_err_if(sigprocmask(SIG_UNBLOCK, &sset, NULL));

    return 0;
err:
    return ~0;
}

void u_trim(char *s)
{
    char *p;

    if(!s)
        return;

    /* trim trailing blanks */
    p = s + strlen(s) -1;
    while(s < p && isblank(*p))
        --p;
    p[1] = 0;

    /* trim leading blanks */
    p = s;
    while(*p && isblank(*p))
        ++p;

    if(p > s)
        memmove(s, p, 1 + strlen(p));
}

int u_foreach_dir_item(const char *path, unsigned int mask,
    int (*cb)(struct dirent*, const char *, void*), void* arg)
{
    struct dirent *de;
    struct stat st;
    DIR *dir = NULL;
    char buf[PATH_MAX];
    int rc;

    /* open the spool directory */
    dir = opendir(path);
    dbg_err_if(dir == NULL);

    while( (de = readdir(dir) ) != NULL)
    {
        /* skip . and .. dirs */
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        /* build fully qualified name of this item */
        dbg_err_if(u_snprintf(buf, PATH_MAX, "%s/%s", path, de->d_name));

        dbg_err_if( (rc = stat(buf, &st)) == -1);
                                                                                        /* skip if its type is not in the requested mask */
        if(((st.st_mode & S_IFMT) == mask) && cb(de, path, arg))
            break;
    }

    closedir(dir);

    return 0;
err:
    return RET_ERR_FAILURE;
}

/* hex char to int */
static short htoi(unsigned char c)
{
    c = tolower(c);

    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    else
        return 0;
}


static ssize_t u_sqlncpy_encode(char *d, const char *s, size_t slen)
{
    ssize_t wr = 0;
    int c;

    for(; slen; --slen)
    {
        c = *d++ = *s++;
        wr++;
        if(c == '\'') 
        {
            dbg_err_if(slen < 2);
            *d++ = '\'';
            wr++;
            --slen;
        } 
    }
    *d = 0;

    return ++wr;
err:
    return -1;
}

static ssize_t u_sqlncpy_decode(char *d, const char *s, size_t slen)
{
    int c, last = 0;
    ssize_t wr = 0;

    for(; slen; --slen)
    {
        c = *s++;
        if(c == '\'' && last == c) 
        {
            last = 0; 
            ; /* skip */
        } else {
            *d++ = c;
            last = c;
            wr++;
        }
    }
    *d = 0;

    return ++wr;
err:
    return -1;
}

ssize_t u_sqlncpy(char *d, const char *s, size_t slen, int flags)
{
    switch(flags)
    {
    case SQLCPY_ENCODE:
        return u_sqlncpy_encode(d, s, slen);
    case SQLCPY_DECODE:
        return u_sqlncpy_decode(d, s, slen);
    default:
        strncpy(d, s, slen);
        d[slen] = 0;
        return slen + 1;
    }

    return -1;
}

static ssize_t u_urlncpy_encode(char *d, const char *s, size_t slen)
{
    const char hexc[] = "0123456789ABCDEF";
    ssize_t wr = 0;
    int c;

    for(; slen; --slen)
    {
        c = *s++;
        if(c == ' ') {
            *d++ = '+';
            wr++;
        } else if(isalnum(c) || c == '_' || c == '-' || c == '.') {
            *d++ = c;
            wr++;
        } else {
            *d++ = '%';                                        
            *d++ = hexc[(c >> 4) & 0xF];             
            *d++ = hexc[c & 0xF];  
            wr += 3;
        }
    }
    *d = 0;

    return ++wr;
err:
    return -1;
}

static ssize_t u_urlncpy_decode(char *d, const char *s, size_t slen)
{
    short c;
    ssize_t wr = 0;

    for(; slen; --slen, ++wr)
    {
        c = *s++;
        if(c == '%')
        {
            dbg_err_if(slen < 2 || !isxdigit(s[0]) || !isxdigit(s[1]));
            c = htoi(s[0]) << 4 | htoi(s[1]);
            //dbg_err_if(c == 0);
            *d++ = (char)c;
            s += 2;
            slen -= 2;
        } else if(c == '+') {
            *d++ = ' ';
        } else {
            *d++ = c;
        }
    }
    *d = 0;

    return ++wr;
err:
    return -1;

}

/* d must be at least slen+1 size long */
ssize_t u_urlncpy(char *d, const char *s, size_t slen, int flags)
{
    switch(flags)
    {
    case URLCPY_ENCODE:
        return u_urlncpy_encode(d, s, slen);
    case URLCPY_DECODE:
        return u_urlncpy_decode(d, s, slen);
    default:
        strncpy(d, s, slen);
        d[slen] = 0; /* zero-term the string */
        return slen + 1;
    }

    return -1;
}

static ssize_t u_htmlncpy_encode(char *d, const char *s, size_t slen)
{
    struct html_entities_s *p;
    const char *map[256];
    size_t elen;
    int c;
    ssize_t wr = 0;

    /* build the map table (could be static but it wouldn't be thread-safe) */
    memset(map, 0, sizeof(map));
    for(p = entities; p->s_char; ++p)
        map[p->s_char] = p->entity;

    while(slen)
    {
        c = *s++;
        if(map[c] == NULL)
        {
            *d++ = c;   /* this char doesn't need encoding */
            wr++;
            --slen;
        } else {
            elen = strlen(map[c]);
            if(slen < elen)
                break; /* there's not enough space to fit the entity */
            strncpy(d, map[c], slen); /* append the entity */
            slen -= elen;
            d += elen;
            wr += elen;
        }
    }
    *d = 0;

    return ++wr;
err:
    return -1;
}

static ssize_t u_htmlncpy_decode(char *d, const char *s, size_t slen)
{
    struct html_entities_s *p;
    char *found, *after;

    strncpy(d, s, slen);
    d[slen] = 0;

    for(p = entities; p->s_char; ++p)
    {
        while((found = u_stristr(d, p->entity)) != NULL)
        {
            *found = p->s_char;
            after = found + strlen(p->entity);
            memmove(++found, after, 1 + strlen(after));
        }
    }

    return 1 + strlen(d);
err:
    return -1;
}

int u_htmlncpy(char *d, const char *s, size_t slen, int flags)
{
    switch(flags)
    {
    case HTMLCPY_ENCODE:
        return u_htmlncpy_encode(d, s, slen);
    case HTMLCPY_DECODE:
        return u_htmlncpy_decode(d, s, slen);
    default:
        strncpy(d, s, slen);
        d[slen] = 0; /* zero-term */
        return slen + 1;
    }

    return -1;
}

/* case insensitive strstr */
char *u_stristr(const char *string, const char *sub)
{
    const char *p;
    size_t len;

    len = strlen(sub);
    for(p = string; *p; ++p)
    {
        if(strncasecmp(p, sub, len) == 0)
            return p;
    }
    return NULL;
}

char* u_strnrchr(const char *s, char c, size_t len)
{
    register int i = len - 1;

    for(; i >= 0; --i)
        if(s[i] == c)
            return (char*)s + i; /* found */

    return NULL;
}

inline int u_isblank(int c)
{
    return c == ' ' || c == '\t';
}

inline int u_isblank_str(const char *ln)
{
    for(; *ln; ++ln)
        if(!u_isblank(*ln))
            return 0;
    return 1;
}

inline int u_isnl(int c)
{
    return c == '\n' || c == '\r';
}

int u_tmpfile_open(io_t **pio)
{
    char tmp[PATH_MAX];
    io_t *io = NULL;
    int max = 10;

    for(; max; --max) /* try just 'max' times */
    {
        if(tmpnam(tmp) != NULL)
        {
            dbg_err_if(u_file_open(tmp, O_CREAT | O_EXCL | O_RDWR, &io));

            dbg_err_if(io_set_name(io, tmp));

            *pio = io;

            return 0;
        }
    }

err:
    if(io)
        io_free(io);
    return ~0;
}

int u_emb_open(const char *file, io_t **pio)
{
    embfile_t *e = NULL;
    codec_gzip_t *gzip = NULL;;
    io_t *io;

    dbg_err_if(emb_lookup(file, (embres_t**)&e) || e->res.type != ET_FILE);

    dbg_err_if(io_mem_create(e->data, e->size, 0, &io));

    if(e->comp)
    {
        dbg_err_if(codec_gzip_create(GZIP_UNCOMPRESS, &gzip));
        dbg_err_if(io_set_codec(io, (codec_t*)gzip));
        gzip = NULL;
    }

    *pio = io;

    return 0;
err:
    if(gzip)
        codec_free((codec_t*)gzip);
    return ~0;
}

int u_file_open(const char *file, int flags, io_t **pio)
{
    int fmod = 0; /* flags modifier */
    int fd;

    #ifdef OS_WIN
    fmod = _O_BINARY;
    #endif
    
    fd = open(file, fmod | flags, 0600);
    dbg_err_if(fd < 0);

    dbg_err_if(io_fd_create(fd, IO_FD_CLOSE, pio));

    /* name the stream */
    dbg_err_if(io_set_name(*pio, file));

    return 0;
err:
    if(fd < 0)
        dbg_strerror(errno);
    else
        close(fd);
    return ~0;
}

int u_getline(io_t *io, string_t *ln)
{
    enum { BUFSZ = 1024 };
    char buf[BUFSZ];
    ssize_t len, rc;

    string_clear(ln);

    while((rc = len = io_gets(io, buf, BUFSZ)) > 0)
    {
        dbg_err_if(string_append(ln, buf, --len));
        if(!u_isnl(buf[len]))
            continue; /* line's longer the bufsz (or eof);get next line chunk */
        else
            break;
    }

err:
    return (rc <= 0 ? ~0 : 0);
}

int u_fgetline(FILE *in, string_t *ln)
{
    enum { BUFSZ = 256 };
    char buf[BUFSZ];
    size_t len;

    string_clear(ln);

    while(!ferror(in) && !feof(in) && fgets(buf, BUFSZ, in))
    {
        len = strlen(buf);
        dbg_err_if(string_append(ln, buf, len));
        if(!u_isnl(buf[len-1]))
            continue; /* line's longer the bufsz, get next line chunk */
        else
            break;
    }

    if(ferror(in))
        dbg_strerror(errno);
err:
    return (string_len(ln) ? 0 : ~0);
}

char *u_strndup(const char *s, size_t len)
{
    char *cp;

    if((cp = u_malloc(len + 1)) == NULL)
        return NULL;
    memcpy(cp, s, len);
    cp[len] = 0;
    return cp;
}

char *u_strdup(const char *s)
{
    return u_strndup(s, strlen(s));
}

int u_printf_ccstr(io_t *o, const char *buf, size_t sz)
{
    char prev, c = 0;
    int pos = 0;
    size_t i;

	for(i = 0; i < sz; ++i)
	{
		prev = c;
		c = buf[i];
		if(pos++ == 0) // first line char
			io_putc(o, '"');
		switch(c)
		{
		case CR:
			if(prev != LF) 
                io_printf(o, "\\n\"\n");
			pos = 0;
			break;
		case LF:
			if(prev != CR) 
                io_printf(o, "\\n\"\n");
			pos = 0;
			break;
		case '"':
			io_printf(o, "\\\"");
			break;
		case '\\':
			io_printf(o, "\\\\");
			break;
		default:
            if(isprint(c))
                io_putc(o, c);
            else {
                io_printf(o, "\\x%c%c", u_tochex((c >> 4) & 0x0F),
                    u_tochex(c & 0x0F));
            }
		}
	}
	if(pos)
        io_putc(o, '"');

    return 0;
}

int u_file_exists(const char *fqn)
{
    struct stat st;

    return stat(fqn, &st) == 0 && S_ISREG(st.st_mode);
}

int u_path_snprintf(char *buf, size_t sz, const char *fmt, ...)
{
    va_list ap;
    int wr, i, len;

    va_start(ap, fmt); 

    wr = vsnprintf(buf, sz, fmt, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)sz);                    

    /* remove multiple consecutive '/' */
    for(len = i = strlen(buf); i > 0; --i)
        if(buf[i] == '/' && buf[i-1] == '/')
            memmove(buf + i, buf + i + 1, len--);

    return 0;                                                 
err:                                                          
    return ~0;
}

inline char u_tochex(int n)
{
	if(n > 15)
		return '?';
	return ( n < 10 ? n + '0' : n-10 + 'a');
}

/* convert buf to hex
 * hex must be at least 2*sz
 */
void u_tohex(char *hex, const char *src, size_t sz)
{
	size_t c, i, t;
	for(i = 0, t = 0; i < sz; ++i, t += 2)
	{
		c = src[i];
        hex[t]   = u_tochex((c >> 4) & 0x0F);
        hex[t+1] = u_tochex(c & 0x0F);
	}
}

int u_md5(char *buf, size_t sz, char out[MD5_DIGEST_BUFSZ])
{
	md5_state_t md5ctx;
	md5_byte_t md5_digest[16]; /* binary digest */

	md5_init(&md5ctx);

    md5_append(&md5ctx, (md5_byte_t*)buf, sz);

	md5_finish(&md5ctx, md5_digest);

    u_tohex(out, md5_digest, 16);

    out[MD5_DIGEST_LEN] = 0;

    return 0;
}

int u_md5io(io_t *io, char out[MD5_DIGEST_BUFSZ])
{
	enum { page_sz = 4096 };
	md5_state_t md5ctx;
	md5_byte_t md5_digest[16]; /* binary digest */
	char buf[page_sz];
	size_t cnt;

    dbg_err_if(io == NULL || out == NULL);

	md5_init(&md5ctx);

    while( (cnt = io_read(io, buf, page_sz)) > 0)
		md5_append(&md5ctx, (md5_byte_t*)buf, cnt);

	md5_finish(&md5ctx, md5_digest);

    u_tohex(out, md5_digest, 16);

    out[MD5_DIGEST_LEN] = 0;

    return 0;
err:
    return ~0;
}

int u_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int wr;

    va_start(ap, format); 

    wr = vsnprintf(str, size, format, ap);

    va_end(ap);

    dbg_err_if(wr < 0 || wr >= (int)size);                    

    return 0;                                                 
err:                                                          
    return ~0;
}      

int u_signal(int sig, sig_t handler)
{
#ifdef OS_WIN
    dbg_err_if(signal(sig, handler) == SIG_ERR);
#else
    struct sigaction action;
    sigset_t all;

    sigfillset(&all); 
    action.sa_mask = all;
    action.sa_handler = handler;

    /* disable child shell jobs notification */
    action.sa_flags = sig == SIGCHLD ? SA_NOCLDSTOP : 0;      
    dbg_err_if(sigaction(sig, &action, (struct sigaction *) 0));
#endif

    return 0;
err:
    return ~0;
}                                                             

const mime_map_t* u_get_mime_map(const char *file_name)
{
    char *ext;
    mime_map_t *mm;

    if((ext = strrchr(file_name, '.')) != NULL)
    {
        ++ext; /* skip '.' */
        // FIXME binary search here
        for(mm = mime_map; mm->ext && mm->mime_type; ++mm)
        {
            if(strcasecmp(mm->ext, ext) == 0)
                return mm;
        }
    }

    /* not found */
    return mime_map; /* the first item is the default */
}

const char* u_guess_mime_type(const char *file_name)
{
    static const char * ao = "application/octect-stream";
    char *ext;
    mime_map_t *mm;

    if((ext = strrchr(file_name, '.')) != NULL)
    {
        ++ext; /* skip '.' */
        for(mm = mime_map; mm->ext && mm->mime_type; ++mm)
            if(strcmp(mm->ext, ext) == 0)
                return mm->mime_type;
    }
    return ao;
}

int u_io_unzip_copy(io_t *out, const uint8_t *data, size_t sz)
{
    codec_gzip_t *zip = NULL;
    io_t *ios = NULL;
    int rc, i;
    unsigned char c;

    /* create an io_t around the HTML block */
    dbg_err_if(io_mem_create(data, sz, 0, &ios));

    /* apply a gzip codec */
    dbg_err_if(codec_gzip_create(GZIP_UNCOMPRESS, &zip));
    dbg_err_if(io_set_codec(ios, zip));
    zip = NULL; /* io_free() will free the codec */

    /* pipe ios to out */
    dbg_err_if(io_pipe(out, ios) < 0);

    io_free(ios);

    return 0;
err:
    if(zip)
        codec_free(zip);
    if(ios)
        io_free(ios);
    return ~0;
}


/**
 *  \}
 */

   
