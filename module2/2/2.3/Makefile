# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = program          # Имя исполняемого файла (можно изменить)

# Находим все .c файлы в текущей папке
SOURCES = $(wildcard *.c)
# Превращаем список .c в список .o
OBJECTS = $(SOURCES:.c=.o)

# Цель по умолчанию
all: $(TARGET)

# Линковка: собираем бинарник из всех объектных файлов
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Компиляция каждого .c в .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(OBJECTS) $(TARGET)

# Объявляем цели, не связанные с файлами
.PHONY: all clean
