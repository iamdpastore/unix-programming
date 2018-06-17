/*Scrivere una applicazione C/POSIX multithread costituita da un thread “master” e 26 thread
“slave”. Il thread “master” legge dallo standard input un testo, ossia una sequenza di parole
separate da caratteri non alfabetici (ai fini dell’esercizio, qualunque carattere non alfabetico
come ‘1’, ‘ ’, ‘\n’ è considerato un separatore di parola). Ciascun thread “slave” è associato
ad una differente lettera dell’alfabeto inglese, e gestisce una propria lista dinamica che, alla
fine, conterrà tutte le parole del testo che iniziano con quella determinata lettera. La lista
dovrà essere ordinata in modo lessicografico (funzione di libreria strcmp). Al termine della
lettura del testo in standard input, ciascun thread “slave”, nell’ordine appropriato, dovrà
scrivere in standard output la propria lista di parol */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

struct node_t {
    char *word;
    struct node_t *next;
};


struct slave_thread_t {
    pthread_t tid;
    pthread_mutex_t mtx;
    pthread_cond_t got_new_word;
    int turn;
    char *word;
    struct node_t *list_head;
};


void error_exit(const char *msg)
{
    fputs(msg, stderr);
    exit(EXIT_FAILURE);
}


int is_a_letter(int c)
{
    return((c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z'));
}


void lock(pthread_mutex_t *mtx)
{
    if (pthread_mutex_lock(mxt))
        error_exit("Error in pthread_mutex_lock()\n");
}

void unlock(pthread_mutex_t *mtx)
{
    if (pthread_mutex_unlock(mtx))
        error_exit("Error in pthread_mutex_unlock()\n");
}

void awake(pthread_cond_t *cnd)
{
    if (pthread_cond_signal(cnd))
        error_exit("Error in pthread_cond_signal()\n");
}

void wait(pthread_cond_t *cnd, pthread_mutex_t *mtx)
{
    if (pthread_cond_wait(cnd, mtx))
        error_exit("Error in pthread_cond_wait()\n");
}

void print_word_lists(struct slave_thread_t * slaves)
{
    int k;
    for (k=0; k<26; ++k, slaves++) {
        give_word_to_slave(NULL, slaves);
        pthread_join(slaves->tid, NULL);
    }
}

void give_word_to_slave(char *word, struct slave_thread_t *slave)
{
    lock(&slave->mtx);
    if (slave->turn != 0)
        wait(&slave->got_new_word, &slave->mtx);
    slave->turn = 1;
    slave->word = word;
    awake(&slave->got_new_word);
    unlock(&slave->mtx);
}

void print_list (struct node_t *node)
{
    while (node != NULL) {
        printf("%s\n", node->word);
        node = node->next;
    }
}

void insert_node(char *word, struct node_t **pnode)
{
    struct node_t *new = malloc(sizeof(struct node_t));
    if (new == NULL)
        error_exit("Memory allocation error\n");
    new->word = word;
    new->next = *pnode;
    *pnode = new;
}

void insert_word(char *word, struct node_t **pnode)
{
    int rc;
    while (*pnode != NULL) {
        rc = strcmp(word, (*pnode)->word);
        if (rc <= 0)
            break;
        pnode = &(*pnode)->next;
    }
    insert_node(word, pnode);
}

void *slave_fn(void *v)
{
    struct slave_thread_t *s = (struct slave_thread_t *) v;
    lock(&s->mtx);
    for (;;) {
        awake (&s->got_new_word);
        s->turn = 0;
        wait(&s->got_new_word, &s->mtx);
        if (s->word == NULL)
            break;
        insert_word(s->word, &s->list_head);
    }
    print_list(s->list_head);
    unlock(&s->mtx);
    pthread_exit(0);
}

int to_lower(int c)
{
    if (c >= 'A' && c <= 'Z')
        c += 'a' - 'A';
    return c;
}


char * read_word(void)
{
    char *w;
    int c, k, maxlen = 32;
    w = malloc(sizeof(char)*maxlen);
    if (w == NULL)
        error_exit("Memory allocation error\n");

    do {
        c = getchar();
        if (c == EOF)
            return NULL;
    }   while (!is_a_letter(c));

    w[0] = c;
    k = 1;
    for (;;) {
        for (; k<maxlen; ++k) {
            c = getchar();
            if (!is_a_letter(c)) {
                w[k] = '\0';
                break;
            }
            w[k] = c;
        }
        if (k < maxlen)
            break;
        maxlen += maxlen;
        w = realloc(w, sizeof(char)*maxlen);
        if ( w == NULL)
            error_exit("Memory reallocation error\n");
    }

    w = realloc(w, sizeof(char)*(strlen(w)+1));
    if (w == NULL)
        error_exit("Memory reallocation error\n");
    return w;
}


void read_words_from_stdin(struct slave_thread_t *slaves)
{
    char *word;
    int slave;
    for (;;) {
        word = read_word();
        if (word == NULL)
            break;
        slave = to_lower(word[0]) - 'a';
        give_word_to_slave(word, &slave[slave]);
    }
}


void create_threads(struct slave_thread_t *slaves)
{
    int k;
    struct slave_thread_t *s = slaves;
    for (k=0; k<26; ++k, ++s) {
        s->word = NULL;
        s->list_head = NULL;
        s->turn = 1;
        if (pthread_mutex_init(&s->mtx, NULL))
            error_exit("Error in pthread_mutex_init()\n");
        if (pthread_cond_init(&s->got_new_word, NULL))
            error_exit("Error in pthread_cond_init()\n");
        if (pthread_create(&s->tid, NULL, slave_fn, s))
            error_exit("Error in pthread_create()\n");
    }
}


int main()
{
    struct slave_thread_t slave[26];
    create_threads(slaves);
    read_words_from_stdin(slaves);
    print_word_lists(slaves);
    pthread_exit(0);
}


