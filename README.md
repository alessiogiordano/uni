# Università
Alessio Giordano - O46001858 - Sistemi Operativi - A.A. 2018-19

# Consegna di Sistemi Operativi

##CONSEGNA 1
La prima consegna indicizza l'intero file system a partire dalla radice e produce una lista di Pathname Assoluti; dopo che il main thread ha indicizzato la radice, vengono lanciati N thread quante sono gli N processori disponibili, che scansionano la lista creata in cerca di directory da indicizzare; quando la lista non conterrà più directory, gli N thread verranno terminati; nel frattempo, il main thread scansiona la lista in cerca di pathname assoluti associati a file, che vengono stampati a video.

##CONSEGNA 2
La seconda consegna si compone di due software, un client ed un server, che comunicano tramite socket TCP su porta "8960" e consentono:
- il download di tutti i file indicizzati, a partire dalla home directory del server, sulla cartella "$HOMEDIR/recvdir" nel client (se non esiste viene creata).
- l'upload di un file del client che viene salvato sulla directory "$HOMEDIR/recvdir" nel server (se non esiste viene creata).

Queste operazioni richiedono la previa autenticazione del client, tramite nome e password, che restituisce un token da allegare alle chiamate successive; in assenza di un token valido, la richiesta sarà rifiutata.

Le procedure di download e upload falliranno se il file eseiste già nella "recvdir" poiché viene specificata la flag "O_CREAT | O_EXCL" - Se si preferisce evitare questo e sovrascrivere il file esistente ogni volta basterà rimuovere l'OR con la O_EXCL nelle funzioni "Upload()" del sorgente "server.c" e "Download()" del sorgente "client.c" nella riga in cui è eseguita la system call open() - Riga 270 nel server - Riga 80 nel client.

Poichè non è stato implementato un sistema all'interno del server tramite il quale gestire gli utenti abilitati all'autenticazione, ciò avviene nel seguente modo:
- Il numero massimo di utenti - ovvero il numero di elementi dell'array di struct utenti "CLIENTI[N]", dove la funzione "Login()" del server andrà a verificare se la combinazione di nome e password fornita dal client sia accetabile, e il numero massimo di connessioni che la listen() accetterà - è definito come costante nella riga 26 del sorgente "server.c" con l'espressione "#define UTENTI 5" - è sufficiente modificare quel numero per ottenere il numero massimo di utenti desiderato (almeno 1)
- L'utente che ospita il server può, anche dopo la compilazione del sorgente, specificare le combinazioni di nome e password accettabili che popoleranno l'array di struct utenti "CLIENTI[N]" inserendole in file di testo dal nome "clients.txt" da posizionare nella stessa directory in cui è posizionato l'eseguibile - questo file all'esecuzione del programma sarà aperto e il suo contenuto copiato nell'array - al più saranno copiate N combinazioni, dove N è il numero massimo di utenti specificato nel sorgente, almeno finché non si arriva alla fine del file
- Il nome utente deve essere almeno di 5 caratteri a causa del metodo di generazione del token
- Se l'apertura del file fallisce, o perché non esiste o per altri impedimenti, il programma popolerà il primo elemento dell'array con le credenziali nome="default" e password="default"
- Esempio di struttura di un file "clients.txt", dove ogni riga ha la sintassi ("%s %s", NOME, PASSWORD):
	alessio twinings
	aurelia pompadour
	greta kimbo
	vittorio lipton
	giorgia nescafe

##ESEMPIO DI ESECUZIONE
// Work in progress...

##SVILUPPO
La prima criticità da affrontare nello sviluppo è stata la scelta del sistema di login. L'uso di una comunicazione in chiaro mi ha fatto preferire un sistema di autenticazione basato su token, in modo tale che nome e password vengano scambiati una volta sola, quindi anche se la comunicazione fra client e server dovesse essere intercettata dopo il login iniziale, le credenziali scelte non sono compromesse e basta un riavvio del server per eliminare il vecchio token dal sistema; tuttavia la scelta del metodo di generazione del token ha richiesto della riflessione: deve essere univoco, anche se due utenti lo richiedono contemporaneamente; ho deciso, dunque, di generare una stringa contenente tre caratteri presi dal nome utente, uno all'inizio e gli altri due alla fine, quindi l'UNIX timestamp in mezzo. Il token così generato viene inviato al client, che lo userà per le chiamate successive, e salvato nel campo token della struct utente nell'array dei clienti (sostituendo un eventuale token precedentemente creato).

La seconda è stata il posizionamento dei file scaricati dal client/caricati nel server. Se inizalmente pensavo di replicare la struttura del percorso del mittente nel destinatario, ciò avrebbe complicato il programma nonché la facilità per il desitinatario di trovare i file ricevuti dall'esterno: creare una sola cartella dei file ricevuti "recvdir" nel primo livello della home directory (che ho indicato in precedenza come "$HOMEDIR/" - es. "/home/alessiogiordano/") semplifica tutto ciò. Tuttavia, questo richiede l'estrazione del nome del file a partire dal percorso assoluto fornito, al cui scopo ho implementato una funzione "LastIndexOf()" che utilizzo per trovare l'ultima occorrenza del carattere "/" nel pathname assoluto, quindi sommo questo indice al puntatore della stringa quando faccio la strcpy() per ottenere il nome del file

Infine, l'ultima scelta progettutale è stata rappresentata dalla sintassi della comunicazione fra client e server, che avviene per mezzo della seguente stringa: ("%s %s %s %s", COMANDO, TOKEN, ARGOMENTO1, ARGOMENTO2); nel dettaglio:
- Per effettuare il login: "LOGIN NULL nome password"
- Per richiedere la lista dei file disponibili: "LIST token NULL NULL"
- Per scaricare un file: "DOWNLOAD token pathnameassoluto NULL"
- Per caricare un file: "UPLOAD token nomedelfile NULL"
- Per terminare la sessione: "ESCI NULL NULL NULL"
È utile conoscerli qualora si desideri interagire con il server direttamente tramite TELNET.
Dopo aver usato questi comandi, la comunicazione continua con, a seconda dei casi, o una risposta secca (es. "ERR" oppure "a1545959683ss" - il token") oppure con uno stream di messaggi, contenenti i pathname assoluti dei file disponibili al download, oppure il contenuto del file del quale si procede a fare il download/upload.

Inoltre, data la complessità di codice superiore al tipico "compitino" svolto in precedenza, sicuramente effettuare il debugging di sviste come: 

- l'assenza del flag "O_WRONLY" all'atto di chiamare la system call "open()" nella funzione "Upload()" - server - e "Download()" - client - che permetteva comunque la creazione del file, ma non di riempirlo con il contenuto spedito dal mittente...
- ...oppure l'assenza della stringa nella quale effettuare la sscanf() impiegata nel "void *ClientHandler(void *args)" per processare i comandi inviati dal client - in pratica chiamare la sscanf() come se fosse una scanf() normale - causando che tutti i comandi spediti dal client fossero ignorati dal server - dal momento che la stringa in cui la ricerca della sscanf avveniva diventava la stringa di formato "%s %s %s %s" che non rappresenta un comando riconosciuto...
- ...o ancora che strcpy(stringa, "") non svuota affatto la stringa - cosa che ho risolto con strncpy(stringa, "", sizeof(stringa))

non è affatto semplice, dal momento che nessuna di esse rappresenta un errore sintattico riconosciuto in fase di compilazione, ma si tratta di errori logici che compromettono l'esecuzione del programma.

##SVILUPPI FUTURI
Dal punto di vista della sicurezza, passare da una comunicazione in chiaro ad una crittografata è sicuramente un obiettivo di sviluppo futuro di primaria importanza.

Dal punto di vista delle prestazioni, eseguire l'indicizzazione della home directory all'avvio può rappresentare un forte problema nel caso di un numero elevato di file e cartelle, sia per i tempi sia per il grande consumo di memoria ram - è quindi preferibili in versioni future, eseguire questa operazione periodicamente e salvarne il risultato su un file di testo, da richiamare all'occorrenza quando il client richiede un LIST.

Dal punto di vista delle funzionalità, implementare dei parametri da specificare all'avvio, come "./server 7669" per impostare l'ascolto sulla porta "7669" invece che su quella di default per questo programma "8960", oppure "./client nome password COMANDO ARGOMENTO", per eseguire direttamente COMANDO senza passare per il menù principale del client renderebbe significativamente più utile e versatile il programma, oltre ovviamente alle funzionalità opzionali specificate nel testo della Consegna 2.
