#!/bin/bash

echo "Début de l'installation"

# On vérifie si le répertoire build existe
count='ls | grep "build" | wc -l'
if [ $count -ne 0 ] 
then
    echo "Création du répertoire build"
    mkdir build
fi

# On vérifie si il y a déjà des executables
count='ls build | wc -l'
if [ $count -ne 0 ] 
then
    echo "Suppresion des anciens exécutables"
    make -s clean
fi

# On recupere les fichiers necessaires
echo "Téléchargement des fichiers nécessaires"
make -s install

# On compile
make serveur