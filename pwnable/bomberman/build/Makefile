LIBS=-lncurses

all: game ../files/game

game: game.cpp
	g++ $< -o $@ $(LIBS)

../files/game: game.cpp
	g++ dist-$< -o $@ $(LIBS)
	cp dist-$< ../files/game.cpp
