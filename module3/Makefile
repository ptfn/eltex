CC = gcc
CFLAGS = -Wall
LDFLAGS = -lrt -lpthread

.PHONY: all clean

all:
	@for f in $$(find . -name '*.c'); do \
		bin=$${f%.c}; \
		$(CC) $(CFLAGS) -o "$$bin" "$$f" $(LDFLAGS); \
	done

clean:
	@for f in $$(find . -name '*.c'); do \
		bin=$${f%.c}; \
		$(RM) -f "$$bin"; \
	done
