#ifndef _KLONE_DEBUG_H_
#define _KLONE_DEBUG_H_
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <klone/utils.h>
#include "conf.h"

extern const char *DEBUG_LABEL;
extern const char *WARN_LABEL;

/** \brief output a debug message
 *
 * This macro will be called for each msg_* function to output 
 * debug text. 
 *
 * You can redefine this macro to redirect debug output.
 */
#define output_message(label, args...) \
    u_write_debug_message(label, __FILE__, __LINE__,__FUNCTION__, args) 


/** \brief output a debug message */
#define msg(label, args...) output_message(label, args)

/** \brief output a debug message */
#define msg_noargs(label, literal) output_message(label, "%s", literal)

#define msg_err(label, args...) do { msg(label, args); goto err; } while(0)

/** \brief write a debug message if \e expr not zero and enter the if-block
 *
 *   generate a debug message if \e expr not zero.
 *   \e expr text statement will be written to the log.
 *
 *   A C-style code block should follow:
 *       dbg_ifb(i == 0)
 *       {
 *           do_something();
 *           do_something();
 *       }
 */
#define msg_ifb(label, expr) if( (expr) && (msg_noargs(label, #expr) ? 1 : 1) ) 

/** \brief write a debug message if \e expr not zero.
  *
  *  gen a debug message if \e expr not zero.
  *  \e expr statement will be written . 
  *
  */
#define msg_if(label, expr) do { if( expr ) msg_noargs(label, #expr); } while(0)

/** \brief write a debug message and return \e err if \e expr not zero.  */
#define msg_return_if(label, expr, err) msg_ifb(label, expr) return err

#define msg_return_sif(label, expr, err) \
    do { msg_ifb(label, expr) { msg_strerror(label, errno); goto err; } } while(0)

/** \brief write a debug message and goto to \e gt if \e expr not zero. */ 
#define msg_goto_if(label, expr, gt) msg_ifb(label, expr) goto gt

/** \brief write a debug message and goto to \e err label if \e expr not zero. 
 *
 *   write a debug message and goto to \e err label if \e expr not zero. 
 *
 *   \e err label must be in scope.
 */
#define msg_err_if(label, expr) do { msg_ifb(label, expr) goto err; } while(0)

#define msg_err_ifm(label, expr, text) \
    do { if( (expr) ) { msg(label, text); goto err; } } while(0)

#define msg_err_sif(label, expr) \
    do { msg_ifb(label, expr) { msg_strerror(label, errno); goto err; } } while(0)

/** \brief write a debug message containing the message returned by strerror(errno) */
#ifdef HAVE_STRERROR_R
    #ifdef STRERROR_R_CHAR_P
        #define msg_strerror(label, en)                             \
            do {                                                    \
            enum { _DBG_BUFSZ = 256 };                              \
            char *p, _eb[_DBG_BUFSZ] = { 0 };                       \
            p = strerror_r(en, _eb, _DBG_BUFSZ);                    \
            if(p)                                                   \
                msg(label, "%s", p);                                \
            else                                                    \
                msg(label, "strerror_r(%d, ...) failed", en);       \
        } while(0)                                        
    #else                                                     
        #define msg_strerror(label, en)                             \
        do {                                                        \
            enum { _DBG_BUFSZ = 256 };                              \
            char _eb[_DBG_BUFSZ] = { 0 };                           \
            if(strerror_r(en, _eb, _DBG_BUFSZ) == 0)                \
                msg(label, "%s", _eb);                              \
            else                                                    \
                msg(label, "strerror_r(%d, ...) failed", en);       \
        } while(0)                                        
    #endif                                                    
#else                                                         
    #define msg_strerror(label, en)                                 \
        do {                                                        \
        char *p;                                                    \
        if((p = strerror(en)) != NULL)                              \
            msg(label, "%s", p);                                    \
        else                                                        \
            msg(label, "strerror(%d) failed", en);                  \
        } while(0)                  
#endif  

/* warn_ macros */
#define warn(args...)                   msg(WARN_LABEL, args)
#define warn_err(args...)               msg_err(WARN_LABEL, args)
#define warn_ifb(expr)                  msg_ifb(WARN_LABEL, expr)
#define warn_if(expr)                   msg_if(WARN_LABEL, expr) 
#define warn_return_if(expr, err)       msg_return_if(WARN_LABEL, expr, err)
#define warn_err_if(expr)               msg_err_if(WARN_LABEL, expr)
#define warn_err_ifm(expr, msg)         msg_err_ifm(WARN_LABEL, expr, msg)
#define warn_goto_if(expr, gt)          msg_goto_if(WARN_LABEL, expr, gt)
#define warn_strerror(errno)            msg_strerror(WARN_LABEL, errno)

/* dbg_ macros */
#ifndef NDEBUG
    #define dbg(args...)                msg(DEBUG_LABEL, args)
    #define dbg_err(args...)            msg_err(DEBUG_LABEL, args)
    #define dbg_ifb(expr)               msg_ifb(DEBUG_LABEL, expr)
    #define dbg_if(expr)                msg_if(DEBUG_LABEL, expr) 
    #define dbg_return_if(expr, err)    msg_return_if(DEBUG_LABEL, expr, err)
    #define dbg_return_sif(expr, err)   msg_return_sif(DEBUG_LABEL, expr, err)
    #define dbg_err_if(expr)            msg_err_if(DEBUG_LABEL, expr)
    #define dbg_err_sif(expr)           msg_err_sif(DEBUG_LABEL, expr)
    #define dbg_err_ifm(expr, msg)      msg_err_ifm(DEBUG_LABEL, expr, msg)
    #define dbg_goto_if(expr, gt)       msg_goto_if(DEBUG_LABEL, expr, gt)
    #define dbg_strerror(errno)         msg_strerror(DEBUG_LABEL, errno)
    /* simple debugging timing macros */
    #define TIMER_ON \
        time_t _t_beg = time(0), _t_prev = _t_beg, _t_now; int _t_step = 0
    #define TIMER_STEP                                                      \
        do {                                                                \
        _t_now = time(0);                                                   \
        dbg("  step %u: %u s (delta: %u s)",                                \
            _t_step++, _t_now - _t_beg, _t_now - _t_prev);                  \
            _t_prev = _t_now;                                               \
       } while(0)
    #define TIMER_OFF  dbg("elapsed: %u s", (time(0) - _t_beg))
#else /* disable debugging */
    #include <ctype.h>
    /* this will be used just to avoid empty-if (and similar) warnings */
    #define dbg_nop()                   isspace(0)
    #define dbg(args...)                dbg_nop()
    #define dbg_err(args...)            do { goto err; } while(0)
    #define dbg_ifb(expr)               if( (expr) )
    #define dbg_if(expr)                expr
    #define dbg_return_if(expr, err)    do { if( (expr) ) return err; } while(0)
    #define dbg_err_if(expr)            do { if( (expr)) goto err; } while(0)
    #define dbg_err_sif(expr)           do { if( (expr)) goto err; } while(0)
    #define dbg_err_ifm(expr, msg)      do { if( (expr)) goto err; } while(0)
    #define dbg_goto_if(expr, gt)       do { if((expr)) goto gt; } while(0)
    #define dbg_strerror(errno)         dbg_nop()
    #define TIMER_ON
    #define TIMER_STEP
    #define TIMER_OFF
#endif /* ifndef NDEBUG */

#endif /* _DEBUG_H_ */

