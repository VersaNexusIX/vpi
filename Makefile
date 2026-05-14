CC     = gcc
CFLAGS = -std=c11 -Wno-unused-parameter -O2
LFLAGS = -lcurl -lz -lm
TARGET = vpi
SRCDIR = src
SRCS   = $(SRCDIR)/main.c $(SRCDIR)/vpi.c $(SRCDIR)/utils.c \
         $(SRCDIR)/http.c $(SRCDIR)/zip.c $(SRCDIR)/registry.c \
         $(SRCDIR)/commands.c $(SRCDIR)/cmd_init.c $(SRCDIR)/miniz.c
OBJS   = $(SRCS:.c=.o)
UNAME := $(shell uname -s 2>/dev/null || echo Unknown)
ifeq ($(UNAME),Darwin)
  LFLAGS += -framework CoreFoundation
endif
ifeq ($(UNAME),Windows)
  TARGET  = vpi.exe
  LFLAGS += -lshell32 -lole32
  CC      = x86_64-w64-mingw32-gcc
endif
DESTDIR ?= $(HOME)
all: $(TARGET)
$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS) && echo "[OK] Build sukses -> $(TARGET)"
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -I$(SRCDIR) -c -o $@ $< && echo "  CC $<"
install: all
	@cp $(TARGET) $(DESTDIR)/$(TARGET)
	@chmod 755 $(DESTDIR)/$(TARGET)
	@echo "[OK] Installed -> $(DESTDIR)/$(TARGET)"
uninstall:
	@rm -f $(DESTDIR)/$(TARGET) && echo "[OK] Removed $(DESTDIR)/$(TARGET)"
clean:
	@rm -f $(OBJS) $(TARGET) && echo "[OK] Cleaned"
.PHONY: all install uninstall clean
