CC=gcc
SRC=./src
DEPENDENCY := -I. -I./dependencies
BUILD=./build

all : serveur

serveur : $(SRC)/serveur.c
	$(CC) $(SRC)/serveur.c -o $(BUILD)/serveur

test : $(SRC)/HelloWorld.c
	$(CC) $(SRC)/HelloWorld.c -o $(BUILD)/HelloWorld

clean :
	rm $(BUILD)/*

install : 
	echo "Aucuns fichiers à télécharger"