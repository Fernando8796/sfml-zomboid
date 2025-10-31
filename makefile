# Nome do arquivo-fonte e do executável
SRC = main.cpp
OUT = jogo

.DEFAULT_GOAL := all

# Compilar o programa
build:
	g++ $(SRC) -o $(OUT) -std=c++17 -lsfml-graphics -lsfml-window -lsfml-system

# Executar o programa
run:
	./$(OUT)

# Compilar e rodar de uma vez
all: build run

# Limpar
clean:
	rm -f jogo
