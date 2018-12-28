//
//  server.c
//  File Server
//
//  Created by Alessio Giordano on 23/12/18.
//

// O46001858 - 23/12/18

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#define UTENTI 5

/*
 Generale
 */

char homepath[4097] = "";

int LastIndexOf(char c, char *str) {
    int index = -1; int max = strlen(str);
    for(int i = 0; i < max; i++) {
        if(c == str[i]) index = i;
    };
    return index;
}

/* ***************************************** */

/*
 Gestione della lista dei path
 */

pthread_mutex_t locker;

typedef struct pathnode {
    char path[4097];
    struct pathnode *next;
} pathnode_t;

typedef pathnode_t* pathlist_t;

pathlist_t ind = NULL;
pathlist_t cur = NULL;

pthread_mutex_t lista;

void AddPathList(char *pat) {
    pathlist_t temp_list;
    temp_list = (pathlist_t)malloc(sizeof(pathnode_t));
    strncpy(temp_list->path, pat, 4097);
    temp_list->next = NULL;
    if (cur == NULL) {
        ind = temp_list;
        cur = temp_list;
    } else {
        cur->next = temp_list;
        cur = temp_list;
    };
}

void ScanDir(char *pat) {
    DIR *dir; struct dirent *dir_info;
    if( (dir = opendir(pat)) != NULL) {
        while ( (dir_info = readdir(dir)) != NULL) {
            char abs_path[4097];
            if ( ((dir_info->d_type == DT_DIR) || (dir_info->d_type == DT_REG)) &&
                ((strcmp(dir_info->d_name, ".") != 0) && (strcmp(dir_info->d_name, "..") != 0)) ) {
                strcpy(abs_path, pat);
                strcat(abs_path, dir_info->d_name);
                if (dir_info->d_type == DT_DIR) {
                    abs_path[0] = 'd';
                    strcat(abs_path, "/");
                };
                AddPathList(abs_path);
            };
        };
        closedir(dir);
    };
}

int ScanList(char *dir) {
    pathlist_t tem = ind; pathlist_t pre = NULL;
    while (tem != NULL) {
        if (tem->path[0] == 'd') {
            strcpy(dir, tem->path);
            dir[0] = '/';
            if (tem == ind) {
                if (ind->next == NULL) {
                    ind = NULL;
                    cur = NULL;
                } else {
                    ind = tem->next;
                };
            } else if (tem->next == NULL) {
                pre->next = NULL;
                cur = pre;
            } else {
                pre->next = tem->next;
            };
            free(tem);
            return 1;
        };
        pre = tem;
        tem = tem->next;
    };
    return 0;
}

void *ListHandler(void *args) {
    while(1) {
        char pat[4097];
        pthread_mutex_lock(&lista);
        if ( ScanList(pat) ) {
            ScanDir(pat);
        } else {
            int *retval = 0;
            pthread_mutex_unlock(&lista);
            pthread_exit( (void*)retval );
        };
        pthread_mutex_unlock(&lista);
    }
}

pthread_t Scanner(char *dir) {
    ScanDir(dir);
    char messaggio[] = ""; pthread_t tid;
    pthread_create(&tid, NULL, ListHandler, (void*)messaggio);
    return tid;
}

/* ***************************************** */

/*
 Sistema di Login
 */

typedef struct utente {
    char nome[50];
    char password[50];
    char token[50];
} utente;

typedef char token[50];

utente clienti[UTENTI];

int Login(int login_to, char *nome, char *password, token tok) {
    char login_buffer[50] = "";
    for(int i = 0; i < UTENTI; i++) {
        if( (strcmp(nome, clienti[i].nome) == 0) && (strcmp(password, clienti[i].password) == 0) && (strlen(nome) > 4) ) {
            // Genero il token
            sprintf(clienti[i].token, "%c%d%c%c", clienti[i].nome[0], (int)time(NULL), clienti[i].nome[3], clienti[i].nome[4]);
            strcpy(tok, clienti[i].token);
            strcpy(login_buffer, clienti[i].token);
            send(login_to, (void*)login_buffer, sizeof(login_buffer), 0);
            return 1;
        };
    };
    strcpy(login_buffer, "ERR");
    send(login_to, (void*)login_buffer, sizeof(login_buffer), 0);
    return 0;
}

int IsLogged(token tok) {
    for(int i = 0; i < UTENTI; i++) {
        if( strcmp(tok, clienti[i].token) == 0 ) return 1;
    };
    return 0;
}

int PopulateClients(char *path) {
    FILE *f;
    f = fopen(path, "r");
    if(f != NULL) {
        for(int i = 0; i < UTENTI; i++) {
            char temp_nome[50] = ""; char temp_password[50] = "";
            int result = fscanf(f, "%s %s", temp_nome, temp_password);
            if( (result == 2) && (strlen(temp_nome) > 4) ) {
                strcpy(clienti[i].nome, temp_nome); strcpy(clienti[i].password, temp_password);
            } else if (result == EOF) {
                break;
            } else {
                i--;
            };
        };
        return 1;
    } else {
        return 0;
    };
}

/* ***************************************** */

/*
 Listing dei File disponibili
 */

void Listing(int list_to) {
    pathlist_t position = ind;
    char buffer[4097] = "";
    while (position != NULL) {
        strncpy(buffer, "", sizeof(buffer));
        strcpy(buffer, position->path);
        send(list_to, (void*)buffer, sizeof(buffer), 0);
        position = position->next;
    };
    strncpy(buffer, "", sizeof(buffer));
    strcpy(buffer, "END");
    send(list_to, (void*)buffer, sizeof(buffer), 0);
}

/* ***************************************** */

/*
 Funzionalità di Download
 */

int Download(char *path, int download_to) {
    int target = strlen(homepath);
    if ( strncmp(path, homepath, target) == 0) {
        int requested_file = open(path, O_RDONLY, NULL);
        if (requested_file != -1) {
            char buffer[200] = "";
            while( read(requested_file, (void*)buffer, sizeof(buffer)) != 0) {
                send(download_to, (void*)buffer, sizeof(buffer), 0);
                strncpy(buffer, "", sizeof(buffer));
            };
            close(requested_file);
            sprintf(buffer, "%d", EOF);
            send(download_to, (void*)buffer, sizeof(buffer), 0);
            return 1;
        } else {
            char buffer[200] = "ERR";
            send(download_to, (void*)buffer, sizeof(buffer), 0);
            return 0;
        };
    } else {
        char buffer[200] = "ERR";
        send(download_to, (void*)buffer, sizeof(buffer), 0);
        return 0;
    };
};

/* ***************************************** */

/*
 Funzionalità di Upload
 */

int Upload(char *name, int upload_from) {
    char recvdir[4097];
    strcpy(recvdir, homepath); strcat(recvdir, "recvdir/");
    mkdir(recvdir, S_IRUSR | S_IWUSR | S_IXUSR);
    strcat(recvdir, name);
    int uploaded_file = open(recvdir, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IXUSR);
    if (uploaded_file != -1) {
        char buffer[200] = ""; char EOFcond[200]; sprintf(EOFcond, "%d", EOF);
        while( strcmp(buffer, EOFcond) != 0 ) {
            strncpy(buffer, "", sizeof(buffer));
            recv(upload_from, (void*)buffer, sizeof(buffer), 0);
            if(strcmp(buffer, EOFcond) != 0) write(uploaded_file, buffer, sizeof(buffer));
        };
        close(uploaded_file);
        return 1;
    } else {
        return 0;
    };
};

/* ***************************************** */

/*
 Gestione delle richieste TCP del client
 */

void *ClientHandler(void *args) {
    int socket_client = (*(int *)args);
    while(1) {
        char buffer[4118] = "";
        recv(socket_client, (void*)buffer, sizeof(buffer), 0);
        char comando[20] = ""; token tok; char argomento1[4097]; char argomento2[50];
        sscanf(buffer, "%s %s %s %s", comando, tok, argomento1, argomento2);
        if(strcmp(comando, "LOGIN") == 0) {
            // Verifica che l'utente in questione sia collegato
            if(Login(socket_client, argomento1, argomento2, tok)) {
                printf("Login Effettuato - Token: %s\n", tok);
            } else {
                printf("Login Fallito\n");
            }
        } else if(strcmp(comando, "LIST") == 0) {
            // Invia al client la lista di file disponibili al Download
            if(IsLogged(tok)) {
                Listing(socket_client);
                printf("Listing dei file disponibili a %s\n", tok);
            } else {
                printf("Tentativo di Listing non autorizzato\n");
            };
        } else if(strcmp(comando, "DOWNLOAD") == 0) {
            // Incomincia la procedura di download
            if(IsLogged(tok)) {
                if(Download(argomento1, socket_client)) {
                    printf("%s ha scaricato il file %s\n", tok, argomento1);
                } else {
                    printf("Fallito Download del file %s\n", argomento1);
                };
            } else {
                printf("Tentativo di Download non autorizzato\n");
            };
        } else if(strcmp(comando, "UPLOAD") == 0) {
            // Incomincia la procedura di upload
            if(IsLogged(tok)) {
                if(Upload(argomento1, socket_client)) {
                    printf("%s ha caricato il file %s\n", tok, argomento1);
                } else {
                    printf("Fallito Upload del file %s\n", argomento1);
                };
            } else {
                printf("Tentativo di Upload non autorizzato\n");
            };
        } else if(strcmp(comando, "ESCI") == 0) {
            close(socket_client);
            printf("Disconnesso\n");
            pthread_exit(0);
        };
        strncpy(buffer, "", sizeof(buffer));
    };
}

/* ***************************************** */

int main(int argc, const char *argv[]) {
    
    printf("Attendi mentre indicizzo la cartella Home... ");
    
    // Inizializzazione del Mutex e verifica dell'esito
    if (pthread_mutex_init(&lista, NULL) != 0) exit(1);
    
    // Creazione della lista a partire dalla home
    pthread_t tid;
    // La variabile Path è definita nell'header
    strcpy(homepath, getenv("HOME")); strcat(homepath, "/");
    // strcat(homepath, "Desktop/"); // Usato nella demo mostrata in README.md
    tid = Scanner(homepath);
    pthread_join(tid, NULL);
    printf("Fatto\n");
    
    // Popoliamo il vettore dei Client
    if(!PopulateClients("clients.txt")) {
        strcpy(clienti[0].nome, "default"); strcpy(clienti[0].password, "default");
    };
    
    // Preparazione al Socket TCP
    int socket_id = socket(AF_INET, SOCK_STREAM, 0); // Creiamo un socket Internet
    if(socket_id < 0) exit(1);
    
    struct sockaddr_in bind_socket_id;
    bind_socket_id.sin_family = AF_INET; // Siamo nel dominio di Internet
    bind_socket_id.sin_addr.s_addr = INADDR_ANY; // Accettiamo da qualsiasi interfaccia
    bind_socket_id.sin_port = htons(8960); // Porta 8960 senza problema di Big/Little End.
    int bind_result = bind(socket_id, (struct sockaddr*)&bind_socket_id, sizeof(bind_socket_id)); // Infine facciamo la Bind
    if(bind_result < 0) exit(1);
    
    int listen_result = listen(socket_id, UTENTI); // Il numero massimo di client consentiti è definito nella costante UTENTI a inizio file...
    if(listen_result < 0) exit(1);
    // E 24/12/18 17:51
    
    // TCP stuff
    while (1) {
        printf("Attendo il client... ");
        fflush(stdout);
        pthread_t tid;
        int accept_result = accept(socket_id, NULL, NULL);
        printf("Connesso\n");
        pthread_create(&tid, NULL, ClientHandler, (void *)&accept_result);
    };
}
