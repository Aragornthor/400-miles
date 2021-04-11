CC=gcc
SRC=./src
DEPENDENCY := -I. -I./dependencies
BUILD=./build

all : serveur client

serveur : $(SRC)/serveur.c
	$(CC) $(SRC)/serveur.c -lpthread -o $(BUILD)/serveur

client : $(SRC)/client.c
	$(CC) $(SRC)/client.c -lpthread -o $(BUILD)/client

test : $(SRC)/HelloWorld.c
	$(CC) $(SRC)/HelloWorld.c -o $(BUILD)/HelloWorld

clean :
	rm $(BUILD)/*

install : 
	echo "Aucuns fichiers à télécharger"
