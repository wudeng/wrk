CFLAGS  += -g -std=c99 -Wall -pthread
LIBS    := -lm -lssl -lcrypto -pthread

TARGET  := $(shell uname -s | tr '[A-Z]' '[a-z]' 2>/dev/null || echo unknown)

ifeq ($(TARGET), sunos)
	CFLAGS += -D_PTHREADS -D_POSIX_C_SOURCE=200112L
	LIBS   += -lsocket
else ifeq ($(TARGET), darwin)
	LDFLAGS += -pagezero_size 10000 -image_base 100000000
else ifeq ($(TARGET), linux)
	CFLAGS  += -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -D_DEFAULT_SOURCE
	LIBS    += -ldl
	LDFLAGS += -Wl,-E
else ifeq ($(TARGET), freebsd)
	CFLAGS  += -D_DECLARE_C99_LDBL_MATH
	LDFLAGS += -Wl,-E
endif

SRC  := wrk.c net.c ssl.c aprintf.c stats.c script.c units.c \
		ae.c zmalloc.c http_parser.c srpc_parser.c lua_cjson.c strbuf.c fpconv.c
BIN  := wrk
VER  ?= $(shell git describe --tags --always --dirty)

ODIR := obj
OBJ  := $(patsubst %.c,$(ODIR)/%.o,$(SRC)) $(ODIR)/version.o #$(ODIR)/bytecode.o
LIBS := -llua $(LIBS)

DEPS    :=
CFLAGS  += -I$(ODIR)/include
LDFLAGS += -L$(ODIR)/lib

ifneq ($(WITH_OPENSSL),)
	CFLAGS  += -I$(WITH_OPENSSL)/include
	LDFLAGS += -L$(WITH_OPENSSL)/lib
else
	DEPS += $(ODIR)/lib/libssl.a
endif

ifneq ($(WITH_LUA),)
	CFLAGS  += -I$(WITH_LUA)/include
	LDFLAGS += -L$(WITH_LUA)/lib
else
	DEPS += $(ODIR)/lib/liblua.a
endif

all: $(BIN)

clean:
	$(RM) -rf $(BIN) obj/*

$(BIN): $(OBJ)
	@echo LINK $(BIN)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ): config.h Makefile $(DEPS) | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(ODIR)/version.o:
	@echo 'const char *VERSION="$(VER)";' | $(CC) -xc -c -o $@ -

$(ODIR)/%.o : %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies

LUA  := $(notdir $(patsubst %.tar.gz,%,$(wildcard deps/lua-5.3*.tar.gz)))
OPENSSL := $(notdir $(patsubst %.tar.gz,%,$(wildcard deps/openssl*.tar.gz)))

OPENSSL_OPTS = no-shared no-psk no-srp no-dtls no-idea --prefix=$(abspath $(ODIR))

$(ODIR)/$(LUA):  deps/$(LUA).tar.gz  | $(ODIR)
	@tar -C $(ODIR) -xf $<

$(ODIR)/$(OPENSSL): deps/$(OPENSSL).tar.gz | $(ODIR)
	@tar -C $(ODIR) -xf $<

$(ODIR)/lib/liblua.a: $(ODIR)/$(LUA)
	@echo Building Lua...
	@$(MAKE) -C $< INSTALL_TOP=$(abspath $(ODIR)) linux install

$(ODIR)/lib/libssl.a: $(ODIR)/$(OPENSSL)
	@echo Building OpenSSL...
ifeq ($(TARGET), darwin)
	@$(SHELL) -c "cd $< && ./Configure $(OPENSSL_OPTS) darwin64-x86_64-cc"
else
	@$(SHELL) -c "cd $< && ./config $(OPENSSL_OPTS)"
endif
	@$(MAKE) -C $< depend
	@$(MAKE) -C $<
	@$(MAKE) -C $< install_sw
	@touch $@

# ------------

.PHONY: all clean
.PHONY: $(ODIR)/version.o

.SUFFIXES:
.SUFFIXES: .c .o .lua

vpath %.c   src 3rd/lua-cjson
vpath %.h   src
vpath %.lua scripts
