# Charte des messages entre le client et le serveur

## Client > Serveur

- `client_id;LOGON` -> Connexion d'un client vers le serveur, spécifiant son identifiant
- `client_id;USERNAME=username` -> Définition du nom d'utilisateur
- `client_id;LOGOUT` -> Déconnexion de l'utilisateur (le rend disponible pour le prochain s'y connectant)
- `client_id;PLAY=ID_CARTE [client_id]` -> Joue une carte, spécifie sur quel joueur appliquer une carte (si carte négative)

## Serveur > Client

- `REMAIN=x` -> Spécifie le nombre de joueurs restant à connecter
- `START` -> Spécifie le début de la partie
- `PIOCHE=x,y,z` -> Spécifie la pioche du joueur
- `ENDGAME=pseudo` -> Défini le gagnant de la partie
- `TRAVELED=x` -> Renvoie le nombre de miles parcouru
- `EFFECT=x` -> Réception d'un effet d'un autre joueur 