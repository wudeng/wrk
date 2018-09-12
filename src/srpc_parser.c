#include "srpc_parser.h"
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define CURRENT_STATE() p_state
#define UPDATE_STATE(V) p_state = (enum state) (V);
# define NEW_MESSAGE() s_start_req

#ifdef __GNUC__
# define LIKELY(X) __builtin_expect(!!(X), 1)
# define UNLIKELY(X) __builtin_expect(!!(X), 0)
#else
# define LIKELY(X) (X)
# define UNLIKELY(X) (X)
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define SET_ERRNO(e)                                                 \
do {                                                                 \
  parser->srpc_errno = (e);                                          \
} while(0)

/* Run the notify callback FOR, returning ER if it fails */
#define CALLBACK_NOTIFY_(FOR, ER)                                    \
do {                                                                 \
  assert(SRPC_PARSER_ERRNO(parser) == SPE_OK);                       \
  if (LIKELY(settings->on_##FOR)) {                                  \
    parser->state = CURRENT_STATE();                                 \
    if (UNLIKELY(0 != settings->on_##FOR(parser))) {                 \
      SET_ERRNO(SPE_CB_##FOR);                                       \
    }                                                                \
    UPDATE_STATE(parser->state);                                     \
                                                                     \
    /* We either errored above or got paused; get out */             \
    if (UNLIKELY(SRPC_PARSER_ERRNO(parser) != SPE_OK)) {             \
      return (ER);                                                   \
    }                                                                \
  }                                                                  \
} while (0)

/* Run the notify callback FOR and consume the current byte */
#define CALLBACK_NOTIFY(FOR)            CALLBACK_NOTIFY_(FOR, p - data + 1)

#define RETURN(V)                                                    \
do {                                                                 \
  parser->state = CURRENT_STATE();                                   \
  return (V);                                                        \
} while (0);
#define REEXECUTE()                                                  \
  goto reexecute;                                                    \

enum state {
    s_dead = 1,
    s_start_req,
    s_start_res,
    s_message_len,
    s_message_identity,
    s_message_done
};

size_t
srpc_parser_execute (srpc_parser *parser,
                            const srpc_parser_settings *settings,
                            const uint8_t *data,
                            size_t len) {
    const uint8_t *p = (uint8_t *)data;
    uint8_t ch;
    enum state p_state = (enum state) parser->state;

    if (len == 0) {
        switch (CURRENT_STATE()) {
            case s_start_req:
            case s_start_res:
                return 0;
            default:
                return 1;
        }
    }

    for (p=data; p != data + len; p++) {
        ch = *p;
reexecute:
        switch(CURRENT_STATE()) {
            case s_start_req:
            case s_start_res:
            {
                parser->content_length = ch;
                UPDATE_STATE(s_message_len);
                break;
            }

            case s_message_len:
            {
                parser->content_length = parser->content_length << 8 | ch;
                UPDATE_STATE(s_message_identity);
                break;
            }

            case s_message_identity:
            {
                uint16_t to_read = MIN(parser->content_length, (uint16_t) ((data + len) - p));
                parser->content_length -= to_read;
                p += to_read - 1;
                if (parser->content_length == 0) {
                    UPDATE_STATE(s_message_done);
                    REEXECUTE();
                }
                break;
            }

            case s_message_done:
            {
                UPDATE_STATE(NEW_MESSAGE());
                CALLBACK_NOTIFY(message_complete);
                break;
            }

            default:
            {
                break;
            }
        }
    }
    RETURN(len)
}

const char *
srpc_errno_description(enum srpc_errno err) {
  assert(((size_t) err) < ARRAY_SIZE(srpc_strerror_tab));
  return srpc_strerror_tab[err].description;
}

int
srpc_body_is_final(const struct srpc_parser *parser) {
    return parser->state == s_message_done;
}

void
srpc_parser_init (srpc_parser *parser, enum srpc_parser_type t)
{
  void *data = parser->data; /* preserve application data */
  memset(parser, 0, sizeof(*parser));
  parser->data = data;
  parser->type = t;
  parser->state = (t == SRPC_REQUEST ? s_start_req : s_start_res);
  parser->srpc_errno = SPE_OK;
}
