#ifndef srpc_parser_h
#define srpc_parser_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#if defined(_WIN32) && !defined(__MINGW32__) && \
  (!defined(_MSC_VER) || _MSC_VER<1600) && !defined(__WINE__)
#include <BaseTsd.h>
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

/* Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run
 * faster
 */
#ifndef HTTP_PARSER_STRICT
# define HTTP_PARSER_STRICT 1
#endif

/* Maximium header size allowed. If the macro is not defined
 * before including this header then the default is used. To
 * change the maximum header size, define the macro in the build
 * environment (e.g. -DHTTP_MAX_HEADER_SIZE=<value>). To remove
 * the effective limit on the size of the header, define the macro
 * to a very large number (e.g. -DHTTP_MAX_HEADER_SIZE=0x7fffffff)
 */
#ifndef HTTP_MAX_HEADER_SIZE
# define HTTP_MAX_HEADER_SIZE (80*1024)
#endif

typedef struct srpc_parser srpc_parser;
typedef struct srpc_parser_settings srpc_parser_settings;


/* Callbacks should return non-zero to indicate an error. The parser will
 * then halt execution.
 *
 * The one exception is on_headers_complete. In a HTTP_RESPONSE parser
 * returning '1' from on_headers_complete will tell the parser that it
 * should not expect a body. This is used when receiving a response to a
 * HEAD request which may contain 'Content-Length' or 'Transfer-Encoding:
 * chunked' headers that indicate the presence of a body.
 *
 * Returning `2` from on_headers_complete will tell parser that it should not
 * expect neither a body nor any futher responses on this connection. This is
 * useful for handling responses to a CONNECT request which may not contain
 * `Upgrade` or `Connection: upgrade` headers.
 *
 * srpc_data_cb does not return data chunks. It will be called arbitrarily
 * many times for each string. E.G. you might get 10 callbacks for "on_url"
 * each providing just a few characters more data.
 */
typedef int (*srpc_data_cb) (srpc_parser*, const char *at, size_t length);
typedef int (*srpc_cb) (srpc_parser*);

enum srpc_parser_type { SRPC_REQUEST, SRPC_RESPONSE };

/* Get an srpc_errno value from an srpc_parser */
#define SRPC_PARSER_ERRNO(p)            ((enum srpc_errno) (p)->srpc_errno)

/* Map for errno-related constants
 *
 * The provided argument should be a macro that takes 2 arguments.
 */
#define SRPC_ERRNO_MAP(XX)                                           \
  /* No error */                                                     \
  XX(OK, "success")                                                  \
                                                                     \
  /* Callback-related errors */                                      \
  XX(CB_message_begin, "the on_message_begin callback failed")       \
  XX(CB_url, "the on_url callback failed")                           \
  XX(CB_header_field, "the on_header_field callback failed")         \
  XX(CB_header_value, "the on_header_value callback failed")         \
  XX(CB_headers_complete, "the on_headers_complete callback failed") \
  XX(CB_body, "the on_body callback failed")                         \
  XX(CB_message_complete, "the on_message_complete callback failed") \
  XX(CB_status, "the on_status callback failed")                     \
  XX(CB_chunk_header, "the on_chunk_header callback failed")         \
  XX(CB_chunk_complete, "the on_chunk_complete callback failed")     \
                                                                     \
  /* Parsing-related errors */                                       \
  XX(INVALID_EOF_STATE, "stream ended at an unexpected time")        \
  XX(HEADER_OVERFLOW,                                                \
     "too many header bytes seen; overflow detected")                \
  XX(CLOSED_CONNECTION,                                              \
     "data received after completed connection: close message")      \
  XX(INVALID_VERSION, "invalid HTTP version")                        \
  XX(INVALID_STATUS, "invalid HTTP status code")                     \
  XX(INVALID_METHOD, "invalid HTTP method")                          \
  XX(INVALID_URL, "invalid URL")                                     \
  XX(INVALID_HOST, "invalid host")                                   \
  XX(INVALID_PORT, "invalid port")                                   \
  XX(INVALID_PATH, "invalid path")                                   \
  XX(INVALID_QUERY_STRING, "invalid query string")                   \
  XX(INVALID_FRAGMENT, "invalid fragment")                           \
  XX(LF_EXPECTED, "LF character expected")                           \
  XX(INVALID_HEADER_TOKEN, "invalid character in header")            \
  XX(INVALID_CONTENT_LENGTH,                                         \
     "invalid character in content-length header")                   \
  XX(UNEXPECTED_CONTENT_LENGTH,                                      \
     "unexpected content-length header")                             \
  XX(INVALID_CHUNK_SIZE,                                             \
     "invalid character in chunk size header")                       \
  XX(INVALID_CONSTANT, "invalid constant string")                    \
  XX(INVALID_INTERNAL_STATE, "encountered unexpected internal state")\
  XX(STRICT, "strict mode assertion failed")                         \
  XX(PAUSED, "parser is paused")                                     \
  XX(UNKNOWN, "an unknown error occurred")


/* Define HPE_* values for each errno value above */
#define SRPC_ERRNO_GEN(n, s) SPE_##n,
enum srpc_errno {
  SRPC_ERRNO_MAP(SRPC_ERRNO_GEN)
};
#undef SRPC_ERRNO_GEN

/* Map errno values to strings for human-readable output */
#define SRPC_STRERROR_GEN(n, s) { "SPE_" #n, s },
static struct {
  const char *name;
  const char *description;
} srpc_strerror_tab[] = {
  SRPC_ERRNO_MAP(SRPC_STRERROR_GEN)
};
#undef SRPC_STRERROR_GEN

struct srpc_parser_settings {
  srpc_cb      on_message_begin;
  srpc_data_cb on_url;
  srpc_data_cb on_status;
  srpc_data_cb on_header_field;
  srpc_data_cb on_header_value;
  srpc_cb      on_headers_complete;
  srpc_data_cb on_body;
  srpc_cb      on_message_complete;
  /* When on_chunk_header is called, the current chunk length is stored
   * in parser->content_length.
   */
  srpc_cb      on_chunk_header;
  srpc_cb      on_chunk_complete;
};

struct srpc_parser {
  /** PRIVATE **/
  unsigned int type : 2;         /* enum srpc_parser_type */
  unsigned int flags : 8;        /* F_* values from 'flags' enum; semi-public */
  unsigned int state : 7;        /* enum state from http_parser.c */
  unsigned int header_state : 7; /* enum header_state from http_parser.c */
  unsigned int index : 7;        /* index into current matcher */
  unsigned int lenient_http_headers : 1;

  uint32_t nread;          /* # bytes read in various scenarios */
  uint64_t content_length; /* # bytes in body (0 if no Content-Length header) */
//   uint16_t len;     /*total length*/
//   uint16_t hlen;

  /** READ-ONLY **/
  unsigned short http_major;
  unsigned short http_minor;
  unsigned int status_code : 16; /* responses only */
  unsigned int method : 8;       /* requests only */
  unsigned int srpc_errno : 7;

  /* 1 = Upgrade header was present and the parser has exited because of that.
   * 0 = No upgrade header present.
   * Should be checked when http_parser_execute() returns in addition to
   * error checking.
   */
  unsigned int upgrade : 1;

  /** PUBLIC **/
  void *data; /* A pointer to get hook to the "connection" or "socket" object */
};

void srpc_parser_init(srpc_parser *parser, enum srpc_parser_type type);

/* Executes the parser. Returns number of parsed bytes. Sets
 * `parser->srpc_errno` on error. */
size_t srpc_parser_execute(srpc_parser *parser,
                           const srpc_parser_settings *settings,
                           const uint8_t *data,
                           size_t len);

/* Checks if this is the final chunk of the body. */
int srpc_body_is_final(const srpc_parser *parser);

/* Return a string description of the given error */
const char *srpc_errno_description(enum srpc_errno err);

#ifdef __cplusplus
}
#endif
#endif
