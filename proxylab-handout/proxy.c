#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16
#define MAX_CACHE 10

struct Uri {
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
};

typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

typedef struct {
    char obj[MAX_OBJECT_SIZE];
    char uri[MAXLINE];
    int time_stamp;
    int empty;

    int readcnt;
    sem_t w;
    sem_t mutex;
} Block;

typedef struct {
    Block blocks[MAX_CACHE];
    int n;
} Cache;

sbuf_t sbuf;
Cache cache;

void doit(int fd);
void parse_uri(char *uri, struct Uri *uri_data);
void build_header(char *http_header, struct Uri *uri_data, rio_t *client_rio);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void *thread(void *vargp);
void init_cache();
int get_cache(char *uri);
int get_index();
void update_LRU(int index);
void read_cache(int i, int fd);
void write_cache(char *buf, char *uri);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s)\n", hostname, port);
    }
    return 0;
}

void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char server[MAXLINE];
    rio_t rio, server_rio;

    char cache_uri[MAXLINE];
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(cache_uri, uri);

    if (strcasecmp(method, "GET")) {
        printf("Proxy does not implement the method");
        return;
    }

    if (get_cache(cache_uri) != -1) {
        read_cache(get_cache(cache_uri), fd);
        return;
    }

    struct Uri *uri_data = (struct Uri *) malloc(sizeof(struct Uri));
    parse_uri(uri, uri_data);
    build_header(server, uri_data, &rio);

    int serverfd = Open_clientfd(uri_data->host, uri_data->port);
    if (serverfd < 0) {
        printf("Connection failed");
        return;
    }

    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, server, strlen(server));


    char cache_buf[MAX_OBJECT_SIZE];
    int buf_size = 0;
    size_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE))) {
        buf_size += n;
        if (buf_size < MAX_OBJECT_SIZE) {
            strcat(cache_buf, buf);
        }
        printf("Proxy received %d bytes,then send", (int) n);
        Rio_writen(fd, buf, n);
    }
    Close(serverfd);

    if (buf_size < MAX_OBJECT_SIZE) {
        write_cache(cache_buf, cache_uri);
    }
}

void parse_uri(char *uri, struct Uri *uri_data) {
    char *host = strstr(uri, "//");
    if (host == NULL) {
        char *path = strstr(uri, "/");
        if (path != NULL) {
            strcpy(uri_data->path, path);
        }
        strcpy(uri_data->port, "80");
        return;
    } else {
        char *port = strstr(host + 2, ":");
        if (port != NULL) {
            int tmp;
            sscanf(port + 1, "%d%s", &tmp, uri_data->path);
            sprintf(uri_data->port, "%d", tmp);
            *port = '\0';
        } else {
            char *path = strstr(host + 2, "/");
            if (path != NULL) {
                strcpy(uri_data->path, path);
                strcpy(uri_data->port, "80");
                *path = '\0';
            }
        }
        strcpy(uri_data->host, host + 2);
    }

    return;
}

void build_header(char *http_header, struct Uri *uri_data, rio_t *client_rio) {
    char *User_Agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
    char *conn_hdr = "Connection: close\r\n";
    char *prox_hdr = "Proxy-Connection: close\r\n";
    char *host_hdr_format = "Host: %s\r\n";
    char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
    char *endof_hdr = "\r\n";

    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    sprintf(request_hdr, requestlint_hdr_format, uri_data->path);
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0) {
        if (strcmp(buf, endof_hdr) == 0)
            break; /*EOF*/

        if (!strncasecmp(buf, "Host", strlen("Host"))) {            /*Host:*/ 
            strcpy(host_hdr, buf);
            continue;
        }

        if (!strncasecmp(buf, "Connection", strlen("Connection")) &&
            !strncasecmp(buf, "Proxy-Connection", strlen("Proxy-Connection")) && !strncasecmp(buf, "User-Agent", strlen("User-Agent"))) {
            strcat(other_hdr, buf);
        }
    }

    if (strlen(host_hdr) == 0) {
        sprintf(host_hdr, host_hdr_format, uri_data->host);
    }

    sprintf(http_header, "%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            User_Agent,
            other_hdr,
            endof_hdr);

    return;
}

void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                      /* Buffer holds max of n items */
    sp->front = sp->rear = 0;       /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);     /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);     /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);     /* Initially, buf has 0 data items */
}

void sbuf_deinit(sbuf_t *sp) {
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear) % (sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front) % (sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

void init_cache() {
    cache.n = 0;
    for (int i = 0; i < MAX_CACHE; i++) {
        cache.blocks[i].time_stamp = 0;
        cache.blocks[i].empty = 1;
        cache.blocks[i].readcnt = 0;
        Sem_init(&cache.blocks[i].w, 0, 1);
        Sem_init(&cache.blocks[i].mutex, 0, 1);
    }
}

int get_cache(char *uri) {
    for (int i = 0; i < MAX_CACHE; i++) {
        P(&cache.blocks[i].mutex);
        cache.blocks[i].readcnt++;
        if (cache.blocks[i].readcnt == 1) {
            P(&cache.blocks[i].w);
        }
        V(&cache.blocks[i].mutex);

        if (!cache.blocks[i].empty && !strcmp(uri, cache.blocks[i].uri)) {
            //P(&cache.blocks[i].mutex);
            //cache.blocks[i].readcnt--;
            //if (cache.blocks[i].readcnt == 0) {
                //V(&cache.blocks[i].w);
            //}
            //V(&cache.blocks[i].mutex);
            return i;
        }

        P(&cache.blocks[i].mutex);
        cache.blocks[i].readcnt--;
        if (cache.blocks[i].readcnt == 0) {
            V(&cache.blocks[i].w);
        }
        V(&cache.blocks[i].mutex);
    }

    return -1;
}

int get_index() {
    int max = 0;
    int max_index = 0;
    for (int i = 0; i < MAX_CACHE; i++) {
        P(&cache.blocks[i].mutex);
        cache.blocks[i].readcnt++;
        if (cache.blocks[i].readcnt == 1) {
            P(&cache.blocks[i].w);
        }
        V(&cache.blocks[i].mutex);

        if (cache.blocks[i].empty) {
            P(&cache.blocks[i].mutex);
            cache.blocks[i].readcnt--;
            if (cache.blocks[i].readcnt == 0) {
                V(&cache.blocks[i].w);
            }
            V(&cache.blocks[i].mutex);

            return i;
        }

        if (cache.blocks[i].time_stamp > max) {
            max_index = i;
            P(&cache.blocks[i].mutex);
            cache.blocks[i].readcnt--;
            if (cache.blocks[i].readcnt == 0) {
                V(&cache.blocks[i].w);
            }
            V(&cache.blocks[i].mutex);

            continue;
        }

        P(&cache.blocks[i].mutex);
        cache.blocks[i].readcnt--;
        if (cache.blocks[i].readcnt == 0) {
            V(&cache.blocks[i].w);
        }
        V(&cache.blocks[i].mutex);
    }

    return max_index;
}

void update_LRU(int index) {
    for (int i = 0; i < MAX_CACHE; i++) {
        if (!cache.blocks[i].empty && i != index) {
            P(&cache.blocks[i].w);
            cache.blocks[i].time_stamp++;
            V(&cache.blocks[i].w);
        }
    }
}

void read_cache(int i, int fd) {
    P(&cache.blocks[i].mutex);
    cache.blocks[i].readcnt++;
    if (cache.blocks[i].readcnt == 1) {
        P(&cache.blocks[i].w);
    }
    V(&cache.blocks[i].mutex);

    Rio_writen(fd, cache.blocks[i].obj, strlen(cache.blocks[i].obj));

    P(&cache.blocks[i].mutex);
    cache.blocks[i].readcnt--;
    if (cache.blocks[i].readcnt == 0) {
        V(&cache.blocks[i].w);
    }
    V(&cache.blocks[i].mutex);

    //update_LRU(i);
}

void write_cache(char *buf, char *uri) {
    int i = get_index();
    P(&cache.blocks[i].w);
    strcpy(cache.blocks[i].obj, buf);
    strcpy(cache.blocks[i].uri, uri);
    cache.blocks[i].empty = 0;
    cache.blocks[i].time_stamp = 0;
    update_LRU(i);
    V(&cache.blocks[i].w);
}