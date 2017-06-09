struct stat;
struct rtcdate;
struct mutex_t;
struct cond_t;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int halt(void) __attribute__((noreturn));

// #5. thread support
int thread_create(void *(*)(void *), int, void *, void *);
void thread_exit(void *) __attribute__((noreturn));
int thread_join(int, void **);
int gettid(void);
int mutex_init(struct mutex_t *);
int mutex_lock(struct mutex_t *);
int mutex_unlock(struct mutex_t *);
int cond_init(struct cond_t *);
int cond_wait(struct cond_t *, struct mutex_t *);
int cond_signal(struct cond_t *);

// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
