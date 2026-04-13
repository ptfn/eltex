CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -ldl
PLUGIN_DIR = libs
PLUGIN_SRC = $(wildcard plugins/*.c)
PLUGIN_LIBS = $(patsubst plugins/%.c,$(PLUGIN_DIR)/lib%.so,$(PLUGIN_SRC))

.PHONY: all plugins clean

all: plugins calculator

plugins: $(PLUGIN_LIBS)

$(PLUGIN_DIR):
	mkdir -p $(PLUGIN_DIR)

$(PLUGIN_DIR)/lib%.so: plugins/%.c | $(PLUGIN_DIR)
	$(CC) $(CFLAGS) -shared -o $@ $< -lm

calculator: main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PLUGIN_DIR)/*.so calculator
