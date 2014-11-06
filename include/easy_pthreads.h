struct pthread_job {
    pthread_t tid;
    int       done;
    void    **args;
    void     *ret;
    void     (*func)(struct pthread_job *);
};

typedef struct pthread_job pj_t;

void *pj_exec(void *pthread_attrs);
pj_t *pj_create(void **args, void *ret, void (*func)(pj_t *));
void pj_join(pj_t *job);
