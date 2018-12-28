//
//  client.c
//  File Client
//
//  Created by Alessio Giordano on 26/12/18.
//

// O46001858 - 26/12/18

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

/*
 Generale
 */

typedef char token[50];

char homepath[4097] = "";

int LastIndexOf(char c, char *str) {
    int index = -1; int max = strlen(str);
    for(int i = 0; i < max; i++) {
        if(c == str[i]) index = i;
    };
    return index;
}

void SendCommand(int server, char *cmd, token tok, char *arg1, char *arg2) {
    char buffer[4118] = "";
    sprintf(buffer, "%s %s %s %s", cmd, tok, arg1, arg2);
    send(server, (void*)buffer, sizeof(buffer), 0);
}

/* ***************************************** */

int Logout(int server) {
    char comando[] = "ESCI";
    send(server, (void*)comando, sizeof(comando), 0);
    close(server);
    printf("Grazie per aver usato File Server\n");
    exit(0);
}

void Listing(int server, token tok) {
    printf("************ FILE DISPONIBILI AL DOWNLOAD ************\n\n");
    SendCommand(server, "LIST", tok, "NULL", "NULL");
    char buffer[4097] = "";
    while(strcmp(buffer, "END") != 0) {
        strncpy(buffer, "", sizeof(buffer));
        recv(server, (void*)buffer, sizeof(buffer), 0);
        if(strcmp(buffer, "END") != 0) printf("%s\n", buffer);
    };
    printf("\n******************************************************\n\n");
}

int Download(int server, token tok) {
    printf("****************** SCARICA UN FILE ******************\n\n");
    printf("Inserisci il pathname assoluto del file da scaricare: ");
    char path[4097] = ""; scanf("%s", path); printf("\n");
    printf("Scaricamento in corso... ");
    
    char recvdir[4097];
    strcpy(recvdir, homepath); strcat(recvdir, "recvdir/");
    mkdir(recvdir, S_IRUSR | S_IWUSR | S_IXUSR);
    strcat(recvdir, path+LastIndexOf('/', path)+1);
    int downloaded_file = open(recvdir, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IXUSR);
    if (downloaded_file != -1) {
        SendCommand(server, "DOWNLOAD", tok, path, "NULL");
        char buffer[200] = ""; char EOFcond[200]; sprintf(EOFcond, "%d", EOF); int first = 1;
        while( strcmp(buffer, EOFcond) != 0 ) {
            strncpy(buffer, "", sizeof(buffer));
            recv(server, (void*)buffer, sizeof(buffer), 0);
            if((strcmp(buffer, "ERR") == 0) && first == 1) {
                printf("Errore, controlla il pathname e riprova\n");
                printf("\n*****************************************************\n\n");
                return 0;
            };
            first = 0;
            if(strcmp(buffer, EOFcond) != 0) write(downloaded_file, buffer, sizeof(buffer));
        };
        close(downloaded_file);
        printf("Fatto\n");
        printf("\n*****************************************************\n\n");
        return 1;
    } else {
        printf("Errore, riprova\n");
        printf("\n*****************************************************\n\n");
        return 0;
    };
}

int Upload(int server, token tok) {
    printf("******************* CARICA UN FILE *******************\n\n");
    printf("Inserisci il pathname assoluto del file da caricare: ");
    char path[4097] = ""; scanf("%s", path); printf("\n");
    printf("Caricamento in corso... ");
    
    int uploaded_file = open(path, O_RDONLY, NULL);
    if (uploaded_file != -1) {
        SendCommand(server, "UPLOAD", tok, path+LastIndexOf('/', path)+1, "NULL");
        
        char buffer[200] = "";
        while( read(uploaded_file, (void*)buffer, sizeof(buffer)) != 0) {
            send(server, (void*)buffer, sizeof(buffer), 0);
            strncpy(buffer, "", sizeof(buffer));
        };
        close(uploaded_file);
        sprintf(buffer, "%d", EOF);
        send(server, (void*)buffer, sizeof(buffer), 0);
        printf("Fatto\n");
        printf("\n******************************************************\n\n");
        return 1;
    } else {
        printf("Errore, controlla il pathname e riprova\n");
        printf("\n******************************************************\n\n");
        return 0;
    };
}


int main(void) {
    token tok;
    strcpy(homepath, getenv("HOME")); strcat(homepath, "/");
    
    printf("File Client by Alessio Giordano © 2018\n");
    printf("Inserisci indirizzo IPv4 del Server: ");
    char adr[30]; scanf("%s", adr);
    if (strcmp(adr, "local") == 0) {
        strcpy(adr, "127.0.0.1");
        printf("Cerco il Server su questo Computer... ");
        
    } else {
        printf("Connessione con il server %s ... ", adr);
    };
    int socket_id = socket(AF_INET, SOCK_STREAM, 0); // Creiamo un socket Internet
    if(socket_id < 0) { printf("Errore\n"); exit(1); };
    
    struct sockaddr_in connect_socket_id;
    connect_socket_id.sin_family = AF_INET; // Siamo nel dominio di Internet
    connect_socket_id.sin_addr.s_addr = inet_addr(adr); // All'indirizzo del nostro server
    connect_socket_id.sin_port = htons(8960); // Porta 8960 senza problema di Big/Little End.
    int connect_result = connect(socket_id, (struct sockaddr*)&connect_socket_id, sizeof(connect_socket_id)); // Infine facciamo la Bind
    if(connect_result < 0) { printf("Errore\n"); exit(1); };
    
    printf("Fatto\n\n");
    
    while(1) {
        char nome[50]; char password[50];
        printf("Inserisci il tuo nome utente: ");
        scanf("%s", nome);
        printf("Inserisci la tua password: ");
        scanf("%s", password); printf("\nAccedo...");
        
        SendCommand(socket_id, "LOGIN", "NULL", nome, password);
        
        char login_buffer[50] = "";
        recv(socket_id, (void*)login_buffer, sizeof(login_buffer), 0);
        
        if (strcmp(login_buffer, "ERR") != 0) {
            strcpy(tok, login_buffer);
            printf(" Bentornato/a, il tuo token è %s\n\n", tok);
            break;
        } else {
            printf("Errore, riprova\n\n");
        };
    }
    
    while(1) {
        printf("****************** MENÙ PRINCIPALE ******************\n\n");
        printf("1. Lista i file disponibili al download\n");
        printf("2. Scarica un file\n");
        printf("3. Carica un file\n");
        printf("4. Esci\n\n");
        int result = 0;
        while(result != 1 && result != 2 && result != 3 && result != 4) {
            printf("Digita il numero corrispondente alla funzione da utilizzare, quindi premere invio: ");
            scanf("%d", &result);
            printf("\n");
        }
        printf("*****************************************************\n\n");
        switch(result) {
            case 1: Listing(socket_id, tok); break;
            case 2: Download(socket_id, tok); break;
            case 3: Upload(socket_id, tok); break;
            case 4: Logout(socket_id); break;
        }
    }
}
