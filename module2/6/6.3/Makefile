CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDFLAGS = -ldl
TARGET = calculator

PLUGIN_DIR = plugins
LIB_DIR = libs
PLUGIN_SRCS = $(wildcard $(PLUGIN_DIR)/*.c)
PLUGIN_SOS = $(patsubst $(PLUGIN_DIR)/%.c,$(LIB_DIR)/%.so,$(PLUGIN_SRCS))

all: $(TARGET) $(PLUGIN_SOS)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(LIB_DIR)/%.so: $(PLUGIN_DIR)/%.c
	@mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) -shared -fPIC -o $@ $<

clean:
	rm -f $(TARGET)
	rm -f $(PLUGIN_SOS)

.PHONY: all clean
