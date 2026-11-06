# Настройки компилятора
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = foxc

# Поиск всех файлов .c в папке src
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

# Главная цель сборки
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Правило для компиляции каждого файла .c в объектный .o
# Теперь файлы также зависят от всех заголовков .h в папке src
src/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Очистка проекта от временных файлов
clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean
