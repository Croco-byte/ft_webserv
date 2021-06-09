		>> Explications de la boucle principale

1. Gestion des connexions et implémentation de select

Sujet :
"trying to read/recv or write/send in any FD without going through a select will give you a mark equal to 0 and the end of the evaluation."
"[The server] must be non blocking and use only 1 select for all the IO between the client andthe server (listens includes)."

La relecture du sujet a bien confirmé qu'on devait implémenter un mécanisme de select() pour la gestion de tous nos envois et toutes nos réceptions de données
sur nos différents serveurs. Après avoir un peu cherché, je me suis rendu compte qu'il allait être impossible d'adapter la gestion actuelle des connexions pour
implémenter select(). Du coup, j'ai pas mal modifié le système de gestion des connexions, et je l'ai rattaché au reste du projet :

		-	Une nouvelle classe "ConnexionManager" centralise toutes les opérations de gestion de connexion. Elle setup d'abord tous les serveurs un par un, qui, suite à
			leur setup, disposent tous d'un socket en mode listen(), représenté par l'attribut privé de l'objet Server _fd.
			La classe ConnexionManager dispose des attributs suivants :
				> _vecServers	: un vector rassemblant tous les serveurs (était auparavant dans la classe Webserv).
				> _fd_set		: l'ensemble des fd de toutes les socket de lecture (en mode listen, ou non).
				> _listen_fds	: une map rassemblant toutes les sockets en mode listen() des serveurs (en clé), et un pointeur vers le serveur associé à la socket (en valeur).
				> _read_fds		: une map rassemblant toutes les sockets (autres que les listen()) prêtes en lecture (en clé), et un pointeur vers le serveur associé à la socket (en valeur).
				> _write_fds	: une map rassemblant toutes les sockets prêtes en écriture (en clé), et un pointeur vers le serveur associé à la socket (en valeur).

		-	Une fois le setup réalisé, la fonction run() est appelée. Il s'agit de la boucle principale du programme. L'idée de cette fonction est la suivante :
				>	La fonction select() surveille des ensembles de fd : un ensemble de fd utilisés pour lire des données, et un ensemble de fd utilisés pour écrire des données.
					Dès qu'un ou plusieurs fd d'un des ensembles est prêt (en lecture / en écriture), il renvoie un nombre positif, correspondant au nombre de fd qui sont prêts ;
					Il modifie également les reading_set et writing_set, qui n'incluent désormais plus que les fd qui sont prêts en lecture / écriture.
				
				>	La première boucle (while ret = 0) copie l'intégralité des sockets de lecture (_fd_set) dans l'ensemble reading_set, puis copie l'intégralité des sockets d'écriture (_writing_fds)
					dans l'ensemble writing_set. Tant que select() ne renvoie pas un résultat positif, on boucle : aucun des fd n'est prêt, ni en lecture, ni en écriture.

				>	Dès que select() renvoie un nombre positif (et donc qu'un fd est prêt), on passe à la suite de la fonction. 3 cas de figures sont à envisager :
						-	Première boucle for()	: On parcourt nos fd en écriture (_writing_fds). Si l'un d'entre eux est dans writing_set, c'est que le serveur
							doit répondre à une requête sur ce fd. On fait en sorte que le serveur associé au fd appelle sa fonction send(), puis on retire le fd
							du writing_set : la réponse a été envoyée, plus besoin d'écrire.
						
						-	Deuxième boucle for()	: On parcourt nos fd en lecture autres que listen() (_listen_fds). Si l'un d'entre eux est dans reading_set, c'est que
							le serveur a reçu une requête sur ce fd. On fait en sorte que le serveur associé au fd appelle sa fonction recv(). Puis on place ce fd dans
							_write_fds : en effet, puisqu'on a reçu une requête sur ce fd, il faut maintenant y répondre : au prochain tour de boucle, c'est le premier "for"
							décrit ci-dessus qui s'en occupera.
						
						-	Troisième boucle for()	: On parcourt nos fd en mode listen(). Si l'un d'entre eux est dans reading_set, c'est que le serveur a reçu une nouvelle
							demande de connexion sur son fd de listen(). On fait en sorte que le serveur associé au fd appelle sa fonction accept(). Puis on place le fd généré
							par le call à accept() dans nos fd de lecture autres que listen (_read_fds) : en effet, cette connexion sera utilisée par le client pour envoyer des
							requêtes, au prochain tour de boucle on sera maintenant prêts à les réceptionner (par le second "for" décrit ci-dessus).
		
		En bref, cette nouvelle classe fait la circulation : elle patiente jusqu'à recevoir une nouvelle connexion ; lorsqu'elle en reçoit, elle place le fd généré dans un ensemble
		de fd de lecture attendant de recevoir des requêtes ; lorsqu'une requête est reçue sur ce fd, elle place ce même fd dans un ensemble de fd d'écriture attendant d'envoyer la réponse
		à ce fd.


		-	La classe "Server" a donc été légèrement modifiée pour lui permettre de fonctionner avec le ConnexionManager : les fonctions relatives à l'ancien système (waitRequest, loop...)
			ont été supprimées. Les nouvelles fonctions requises par le ConnexionManager (send, recv, accept...) ont été implémentées, en utilisant les fonctions de parsing et de génération
			de la réponse que tu avais déjà codé.
		
		-	Le setup des "Server" a été également un peu revu : l'IP est passée en SO_REUSEADDR (pas très important, mais toujours bien), les sockets de listen ont désormais une
			capacité de 1000 requêtes (au lieu de 5 auparavant).
		
		-	La classe "Client" a été supprimée, car elle n'est plus nécessaire dans le nouveau système de gestion des connexions.


2. Autres modifications

J'ai également un peu modifié l'affichage des messages de debug à réception / envois de requêtes.
Je code sur la VM, j'ai donc dû rajouter 2-3 includes pour compiler le projet (limits.h par exemple).



		>> Remarques / FIXME

[X]		Quand j'ai téléchargé le projet vendredi, j'ai eu du mal à le faire fonctionner sur la VM : sur tous les navigateurs autres que Chrome la requête bloquait avec un temps de
		chargement infini. Finalement, il s'agissait d'un problème de header : un espace avant les ":" du header Content-Length faisait que la plupart des navigateurs ignoraient
		ce header, et donc n'avaient pas d'indications sur la taille de la réponse, et tournaient à l'infini sauf si on fermait le socket. Bien noter du coup que pour les headers,
		le formattage HTTP ne permet pas d'espace avant les deux points.

[DONE]	Dans le setup des serveurs, on écoute pour le moment sur "n'importe quelle interface" (INADDR_ANY) : il faut remplacer ça par le véritable hôte du fichier de configuration.

[DONE]	Lorsque je requiers un fichier en .php, le programme crash sur la VM (message d'erreur : "AddressSanitizer failed to allocate 0x7f154aabf321 bytes"). C'est sûrement un problème spécifique à Linux puisque de ton côté ça fonctionnait, mais il faudra voir
		ce qui bloque sur Linux, vu qu'on devrait être corrigé sur la VM.

[DONE]	On a des problèmes de parsing du fichier de configuration lorsque plusieurs serveurs sont présents dans ce fichier. Par exemple, la première ligne "server {" du second serveur est traitée
		comme une instruction pour ce second serveur.

[DONE]	Bind call échoue lorsque plusieurs serveurs sont présents dans le fichier de configuration, sur des ports pourtant différents (et non utilisés en local).
		EDIT : c'est simplement lié au problème du parsing du fichier de configuration relevé ci-dessus. S'occuper du parsing réglera ce problème.

[DONE]	Dans la fonction loadConfiguration (Webserv.cpp), le parsing des blocks "server" peut être revue : ligne 45, on s'arrête à la première '}' qui clôt un bloc de serveur,
		puis on envoie l'intégralité des lignes du fichier de configuration à la fonction createServer. La fonction createServer parcourt ensuite le fichier en entier, et s'occuppe
		de chaque bloc de serveur.
		Il serait peut-être plus logique que la fonction loadConfiguration soit celle qui s'occupe de repérer les blocs de serveurs, puis de passer chaque bloc de serveur à la fonction
		createServer, qui gérerait seulement les blocs de serveur individuels et leurs routes (au lieu de parcourir tout le fichier). Ce qui permettrait également de régler les
		problèmes de parsing liés à la présence de plusieurs blocs "server".

[DONE]	Attention, la fonction to_string (utilisée pour l'affichage de Console notamment) est du C++11, de même que d'autres fonctions de manipulation des strings dans la classe Console. Si on
		conserve l'affichage des messages de logs (sur stdout, ou dans un fichier de log), il faudra remplacer ces fonctions.
		
[DONE]	De même pour memcpy (utilisé dans ConnexionManager), qui ne fait pas partie des fonctions autorisées, remplacer par un ft_memcpy.

[DONE]	La fonction remove_char (par exemple utilisée dans Webserv, fonction createServer) pourrait être optimisée (et raccourcie). En général, lorsqu'on veut retirer un élément
		particulier d'un container (ou d'un string), on utilise le "erase-remove idiom", voir : https://en.wikipedia.org/wiki/Erase%E2%80%93remove_idiom
		On peut donc remplacer l'appel à "remove_char" (qui utilise elle-même split, et est au final assez complexe, avec un vector temporaire) par la ligne :
			*start.erase(std::remove(*start.begin(), *start.end(), ';'), *start.end());

[DONE]	Le parsing de la page d'erreur ne fonctionne pas encore. Dans la fonction addErrorPageLocation, "code" est 0 si l'on en précise pas, et location est systématiquement une chaîne vide.
		Sur ce dernier point, c'est plutôt logique car on envoie à la fonction le string vecParam[1], qui n'existe pas car il n'y a pas de virgule sur la ligne.
		Toujours au niveau des errorPageLocations, on définit par défaut des pages d'erreur pour les codes 2xx et 3xx, ce qui n'est pas nécessaire, ces pages ne seront jamais des erreurs.

[DONE]	Récupérer le contenu du fichier dans la réponse du serveur et l'afficher
		Gestion des relative paths (fichiers css par exemple)
		Gestion de l'autoindex

[DONE]	Finaliser le CGI (petits leaks qui restent)

[DONE]	Gestion des pages et des codes d'erreur.
		Gestion des pages d'erreur par défaut hardcodées.

[DONE]	Gestion des server_names (virtualHosts) et serveur par défaut
		Gérer le cas particulier du port 80 pour les virtualHosts.

[TODO]	Valider le fichier de configuration

[DONE]	Gerer les autres methodes (PUT, DELETE)

[DONE]	CGI : prefixe HTTP_ pour les headers
		CGI : Passer les data POST au CGI sur l entree standard

[DONE]	Ajouter un path par défaut pour la configuration.

[DONE]	Gérer les headers. Liste des headers :
			> Accept-Charset					OK
			> Accept-Language					OK
			> Allow								OK
			> Authorization						OK
			> Content-Language					OK
			> Content-Length					OK
			> Content-Location					OK
			> Content-Type						OK
			> Date								OK
			> Host								OK
			> Last-Modified						OK
			> Location							OK
			> Referrer							OK
			> Retry-After						OK
			> Server							OK
			> Transfer-Encoding					OK
			> User-Agent						OK
			> WWW-Authenticate					OK


> TESTER :
	- Renvoyer une erreur 500 quand on arrive pas à ouvrir le binary du CGI, pour l'instant ça ne renvoie rien du tout, et ça produit des leaks.
	- S'occuper de la destination des fichiers uploadés. Pour le moment, quand on PUT /put_test, ça upload dans put_test, or cela devrait upload dans un dossier du choix de l'utilisateur.
	- Le test avec /directory/youpla.bla ne fonctionne que si le fichier youpla.bla existe, or d'autres webserv permettent de passer *.bla au CGI avec des wildcards.
	- Parfois, la requête GET http://127.0.0.1:8002/directory/oulalala retourne un "200" alors que le fichier n'existe pas.
	- Lors d'une redirection de dossier (par exemple de "/post_body" à "/post_body/"), la méthode originelle de la requête n'est pas préservée : on passe automatiquement à un GET. Après qques recherches, il faudrait utiliser un 307. Sauf que ce n'est vraiment pas ce que NGINX fait. --> Plutôt édicter une règle spéciale quand on a un dossier et une méthode POST autorisée, pas de redirect.
	- Segfault pour un fichier de configuration avec un server_block vide.
