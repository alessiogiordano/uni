// O46001858 - 04/12/18

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>

int cpu_num = 0;

pthread_mutex_t locker;

typedef struct pathnode {
    char path[4097];
    struct pathnode *next;
} pathnode_t;

typedef pathnode_t* pathlist_t;

pathlist_t ind = NULL;
pathlist_t cur = NULL;
pathlist_t mai = NULL;

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
    if (mai == NULL) mai = temp_list;
}

void ScanDir(char *pat) {
    //printf("%s\n", pat);
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

void *handler(void *args) {
    while(1) {
        //printf("Sono %d\nIndirizzo INDEX: %p\nIndirizzo CURRENT: %p\nIndirizzo MAIN: %p\nStringa CURRENT: %s\n\n",
        //        (int)pthread_self(), ind, cur, mai, cur->path);
        char pat[4097];
        pthread_mutex_lock(&lista);
        if ( ScanList(pat) ) {
            ScanDir(pat);
        } else {
            cpu_num = 0;
            int *retval = 0;
            pthread_mutex_unlock(&lista);
            pthread_exit( (void*)retval );
        };
        pthread_mutex_unlock(&lista);
    }
}

int main(int argc, const char *argv[]) {
    // Apertura directory iniziale e verifica della sua correttezza
    if(argc == 1) { printf("Nessun Argomento"); exit(1); };
    DIR *d; char argomento[4097]; strcpy(argomento, argv[1]);
    if ( (d = opendir(argomento)) == NULL ) {
        printf("Argomento non valido\nFornire un path assoluto a una directory esistente\n");
        exit(1);
    };
    closedir(d);
    // Inizializzazione del Mutex e verifica dell'esito
    if (pthread_mutex_init(&lista, NULL) != 0) exit(1);
    // Verifica del numero di processori
    FILE *f;
    if ( (f = fopen("/proc/cpuinfo", "r")) == NULL ) exit(1);
    while (!feof(f)) {
        char num[10];
        fscanf(f, "%s", num);
        if (strcmp(num, "processor") == 0) cpu_num++;
    };
     fclose(f);
    printf("Numero di Processori: %d\n", cpu_num);
    ScanDir(argomento);
    for (int i = 0; i < cpu_num; i++) {
        char messaggio[] = ""; pthread_t tid;
        pthread_create(&tid, NULL, handler, (void*)messaggio);
    };
    printf("/********* File a partire da %s *********/\n", argomento);
    while (1) {
        if ( (mai == NULL) && (cpu_num == 0) ) exit(0);
        pthread_mutex_lock(&lista);
        while (mai != NULL) {
            if (mai->path[0] != 'd') printf("%s\n", mai->path);
            mai = mai->next;
        };
        pthread_mutex_unlock(&lista);
    };
}
