/*
 *  © Copyright 1976, 2005 Hewlett-Packard Development Company, L.P.
 *
 * Confidential computer software.  Valid license from HP and/or
 * its subsidiaries required for possession, use, or copying.
 *
 * Consistent with FAR 12.211 and 12.212, Commercial Computer Software,
 * Computer Software Documentation, and Technical Data for Commercial
 * Items are licensed to the U.S. Government under vendor's standard
 * commercial license.
 *
 * Neither HP nor any of its subsidiaries shall be liable for technical
 * or editorial errors or omissions contained herein.  The information
 * in this document is provided "as is" without warranty of any kind and
 * is subject to change without notice.  The warranties for HP products
 * are set forth in the express limited warranty statements accompanying
 * such products.  Nothing herein should be construed as constituting an
 * additional warranty.
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <errno.h>
#include  <string.h>
#include  <sys/types.h>
#include  <sys/time.h>
#include  <sys/socket.h>
#include  <netinet/in.h>
#include  <netdb.h>             /* change hostent to comply with BSD 4.3 */
#include  <arpa/inet.h>
#include  <pthread.h>
#include  <signal.h>
#include  <openssl/ssl.h>
#include  <openssl/err.h>
#ifdef __VMS
#include <unixio.h>
#else
#include <unistd.h>
#endif
#if defined (_XOPEN_SOURCE_EXTENDED) && defined (__hpux)
#include <sys/byteorder.h>
#endif

#ifndef PTHREAD_STACK_MIN
/*
 * PTHREAD_STACK_MIN is undefined on my SunOS V5.8
 * likely due to out dated patches.
 */
#define PTHREAD_STACK_MIN 65535
#endif

#if !defined (USE_PTHREAD_KILL) && defined (BAD_PRACTICE)
/*
 * BAD_PRACTICE is only valid with USE_PTHREAD_KILL
 */
#undef BAD_PRACTICE
#endif

typedef struct {
   int verbose_mode;
   int verify_depth;
   int always_continue;
} mydata_t;

static int mydata_index;
const int DELAY=1;
const int AUXSERVER_PORT=60000;
const int MAX_MESSAGE_SIZE=1000;
const char *KEYFILE="server.pem";
//const char *CA_LIST="server.crt";
const char *HTML_PREFIX="HTTP/1.1 200 OK\n\
Content-Type:text/html\n\n\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n\
<HTML>\n\
<HEAD>\n\
<TITLE>sslserver echo</TITLE>\n\
<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
</HEAD>\n\
<BODY>\n\
<PRE>\n";
const char *HTML_SUFFIX="</PRE>\n\
</BODY>\n\
</HTML>\n";

#if defined (__VMS)
#define socklen_t unsigned int
#endif
/*
 * A printf is not guaranteed to be atomic on all systems.
 * Hence the ScreenMutex
 */
pthread_mutex_t ScreenMutex;
pthread_mutex_t WatchdogMutex;
const char *pass="password";

#if defined (BAD_PRACTICE)
/*
 * The #warning preprocessor directive is Linux specific
 */
void warning(void){
printf ("******** WARNING **********\n");
#if defined (__osf__)
printf ("pthread_sigmask has an undefined behavior in \
signal-handling functions on Tru64\n");
#endif
printf("pthread_exit has an undefined behavior in signal-handling functions.\n\
It is better advised to not -DBAD_PRACTICE.\n\
This call in the context of this program appears to behave correctly on \n\
Linux FC5, HP-UX 11iV1, HP Tru64 V5.1B PK4\n");
printf ("\nRead comments in SIGUSR1_handler routine\n\n");
printf ("***************************\n");
}
#endif

static int password_cb(char *buf,int num,int rwflag, void *userdata) {
   if (num<strlen(pass)+1) return 0;
   strcpy(buf,pass);
   return(strlen(pass));
}
typedef struct {
   int s;
   int http_request;
   SSL *ssl;
   BIO *sbio;
   char *message;
   pthread_t connection_thread;
   pthread_t watchdog_thread;
#if !defined (USE_PTHREAD_KILL) || defined (BAD_PRACTICE)
   int signalled; /* to handle possible spurious wakeups */
   pthread_mutex_t mutex;
   pthread_cond_t cond;
#endif
} arg_t;

/*
 * Small error handling routine, that prints out the error, plus OpenSSL
 * relevant information upon why the call failed.
 */
void berr_exit(char *str){
   fprintf(stderr,"%s\n",str);
   ERR_print_errors_fp(stderr);
   exit(EXIT_FAILURE);

}

void finish(void *ptr){
  arg_t *arg =(arg_t *)ptr;
  int retval;

/*
 * Shutdown SSL connection and close socket.
 */
  if (arg->http_request)
      SSL_write(arg->ssl,HTML_SUFFIX,strlen(HTML_SUFFIX));
  SSL_free(arg->ssl);
  if (arg->message)
      free(arg->message);
//  BIO_free_all(arg->sbio);
  /*
   * wait until watchdog thread has termined.
   * If the SSL_accept returns in error (because
   * one specify http instead of https), it calls
   * pthread_exit which will invoke this
   * finish routine. As the *arg is allocated in main
   * with a calloc, arg->watchdog_thread will be NULL.
   * Free used resources.
   */
  if (arg->watchdog_thread){
      /*
       * Unlocking a mutex which is unlocked can yield to
       * unpredictable results. The mutex is locked again
       * after the pthread_cond_timedwait and is
       * unlocked in case of timeout.
       */
#if defined (BAD_PRACTICE)
      /*
       * a pthread_kill on a pthread_cond_wait has an 
       * undefined behavior regarding the mutex lock.
       */
      pthread_mutex_trylock(&arg->mutex);
#endif
#if !defined (USE_PTHREAD_KILL) || defined (BAD_PRACTICE)
      pthread_mutex_unlock(&arg->mutex);
#endif
      pthread_join(arg->watchdog_thread,NULL);
  }
#if !defined (USE_PTHREAD_KILL) || defined (BAD_PRACTICE)
  pthread_mutex_destroy(&arg->mutex);
  pthread_cond_destroy(&arg->cond);
#endif
  free(arg);
}
#if defined (BAD_PRACTICE)
void SIGUSR1_handler(int sig){
  /*
   * Here is a good description in the book THREADTIME
   * (see the REFERENCE section)
   * From CHAPTER 7
   * Threads and Signals
   *    Async-Signal Safe Thread Functions
   * 2nd paragraph.

   * ===================
   * 
   * pp. 181:
   *
   * What is an async-signal safe function? It is a function that may be
   * invoked from a signal handler. No function should be considered
   * async-signal safe unless it is explicitly stated to be safe.
   *
   * None of the pthread_*() functions are async-signal safe. Do not call any
   * of these functions from within a signal handler. Initially, this may not
   * seem like a severe restriction. However, a thread may want to terminate
   * upon receipt of a specific signal. A thread cannot safely call
   * pthread_exit() from a signal handler.
   * ====================
   *
   * See also:
   *
   * http://mailgate.supereva.com/comp/comp.programming.threads/msg12068.html
   *
   * From the following URL:
   * http://www.calpoly.edu/cgi-bin/man-cgi?sigaction+2
   * 
   *         The following table defines a set of functions that are either
   *         reentrant or not interruptible by signals.  Therefore
   *         applications may invoke them, without restriction, from
   *         signal-catching functions:
   *
   * So what about adding the following small code before pthread_exit to prevent it
   * from being interruptible by signals ?
  sigset_t set;

  sigfillset(&set);
  sigprocmask(SIG_BLOCK,&set,NULL);
    *
    * On Linux FC5, the following is specified:
    * The use of sigprocmask() is unspecified in a multithreaded process;
    * see pthread_sigmask(3).
    * The (pthread_sigmask) function shall be equivalent to sigprocmask(),
    * without the restriction the call be made in a single-threaded process.
    *
    * On HP-UX, man sigprocmask specifies the following:
    * APPLICATION USAGE
    *    Threads Considerations
    *      Since each thread maintains its own blocked signal mask, sigprocmask()
    *      modifies only the calling thread's blocked signal mask.
    *
    *      For more information regarding signals and threads, refer to
    *      signal(5).
    *
    *    LWP (Lightweight Processes) Considerations
    *      sigprocmask() modifies only the calling LWP's blocked signal mask.
    *
    * On Tru64 V5.1B, man 4 signal indicates:
    *
    *   When signal-catching functions are invoked asynchronously with process exe-
    *  cution, the behavior of some of the functions defined by this standard is
    *  unspecified if they are called from a signal-catching function.  The fol-
    *  lowing set of functions are reentrant with respect to signals (that is,
    *  applications can invoke them, without restriction, from signal-catching
    *  functions):
    *
    *  _exit()        access()        alarm()         chdir()
    *  chmod()        chown()         close()         creat()
    *  dup2()         dup()           exec()          fcntl()
    *  fork()         fstat()         getegid()       geteuid()
    *  getgid()       getgroups()     getpgrp()       getpid()
    *  getppid()      getuid()        kill()          link()
    *  lseek()        mkdir()         mkfifo()        open()
    *  pause()        pipe()          read()          rename()
    *  rmdir()        sem_post()      setgid()        setpgrp()
    *  setuid()       sigaction()     sigaddset()     sigdelset()
    *  sigfillset()   siginitset()    sigismember()   signal()
    *  sigpending()   sigprocmask()   sigsuspend()    sleep()
    *  statx()        tcdrain()       tcflow()        tcflush()
    *  tcgetattr()    tcgetprgp()     tcsendbreak()   tcsetattr()
    *  tcsetpgrp()    time()          times()         umask()
    *  uname()        unlink()        ustat()         utime()
    *  wait2()        wait()          write()
    *
    *  All other system calls should not be called from signal-catching functions
    *  since their behavior is undefined.
    *
    * Still on Tru64, man sigprocmask shows it acts on process whereas pthread_sigmask
    * acts on a thread.
    * How useful a pthread_kill can be on Tru64 unless it is used with a sigwait ???
    */
  sigset_t set;

  sigfillset(&set);
#if defined (__hpux)
  sigprocmask(SIG_BLOCK,&set,NULL);
#elif defined (__linux__)
  pthread_sigmask(SIG_BLOCK,&set,NULL);
#elif defined (__osf__)
  pthread_sigmask(SIGBLOCK,&set,NULL);
#endif
  pthread_exit(NULL);
}
/*
 * The pthread_cond_timedwait can be interrupted by a SIGUSR1 signal
 * from the the serve_connection thread.
 * This call is NOT async-signal safe. The pthread_exit inside the
 * SIGUSR1_handler also not being async-signal safe, this leads to an
 * undefined behavior on all Unixes including Linux.
 */
void *connection_watchdog(void *ptr){
  arg_t *arg =(arg_t *)ptr;
  struct timespec abstime;
  int ret;
  struct timeval tm;
  /*
   * On Unix systems the signal mask is inherited from its creator.
   * As we pthread_kill(this_thread,SIGUSR1), unblock this signal.
   */
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set,SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK,&set,NULL);
  for (;;){
       gettimeofday(&tm,NULL);
       abstime.tv_sec = DELAY+tm.tv_sec; 
       abstime.tv_nsec = tm.tv_usec*1000;
       if (abstime.tv_nsec >= 1000000000){
	  ++abstime.tv_sec;
	  abstime.tv_nsec -= 1000000000;
        }
       pthread_mutex_lock(&arg->mutex);
       /*
	* From HP-UX pthread_cond_timedwait man
	* Spurious wakeups may occur when waiting on
	* a condition variable.  A spurious wakeup occurs when a thread returns
	* from a condition wait when it should really continue waiting. A normal
	* signal being delivered to a thread may cause a spurious wakeup during
	* a condition wait.  Since the return values from pthread_cond_wait()
	* and pthread_cond_timedwait() do not imply anything about the value of
	* the predicate, the predicate should be re-evaluated.
	*
        */
       ret=0;
       while ((!arg->signalled) && (ret==0))
               ret=pthread_cond_timedwait(&arg->cond,&arg->mutex,&abstime);
       arg->signalled=0;
       /*
        * If timeout elapsed, just cancel the serve_connection thread
        * It shall do socket cleanups
        */
        if (ret==ETIMEDOUT){
            pthread_kill(arg->connection_thread,SIGUSR1);
	    return(NULL);
	}
	pthread_mutex_unlock(&arg->mutex);
   }
}
#else /* Good pratice */

#if !defined (USE_PTHREAD_KILL)
void *connection_watchdog(void *ptr){
  arg_t *arg =(arg_t *)ptr;
  struct timespec abstime;
  int ret;
  struct timeval tm;

  for (;;){
       gettimeofday(&tm,NULL);
       abstime.tv_sec = DELAY+tm.tv_sec; 
       abstime.tv_nsec = tm.tv_usec*1000;
       if (abstime.tv_nsec >= 1000000000){
	  ++abstime.tv_sec;
	  abstime.tv_nsec -= 1000000000;
        }
       pthread_mutex_lock(&arg->mutex);
       /*
	* From HP-UX pthread_cond_timedwait man
	* Spurious wakeups may occur when waiting on
	* a condition variable.  A spurious wakeup occurs when a thread returns
	* from a condition wait when it should really continue waiting. A normal
	* signal being delivered to a thread may cause a spurious wakeup during
	* a condition wait.  Since the return values from pthread_cond_wait()
	* and pthread_cond_timedwait() do not imply anything about the value of
	* the predicate, the predicate should be re-evaluated.
        */
       ret=0;
       while ((!arg->signalled) && (ret==0))
               ret=pthread_cond_timedwait(&arg->cond,&arg->mutex,&abstime);
       arg->signalled=0;
       /*
        * If timeout elapsed, just cancel the serve_connection thread
        * It shall do socket cleanups
        */
        if (ret==ETIMEDOUT){
            pthread_cancel(arg->connection_thread);
	    return(NULL);
        }
	pthread_mutex_unlock(&arg->mutex);
    }/*end for */
}
#else /* good practice and use pthread_kill */
void *connection_watchdog(void *ptr){
  arg_t *arg =(arg_t *)ptr;
  struct itimerval abstime;
  sigset_t set;
  int sig;
  static struct timeval tm;
  struct timeval tm1;
  static int NumberOfThreads;

  /*
   * A good practice when using signals in multi-threaded application
   * is to use the pthread_kill/pthread_sigmmask/sigwait. No other calls
   * should be considered.
   */
  sigemptyset(&set);
  sigaddset(&set,SIGUSR1);
  sigaddset(&set,SIGALRM);
  sigaddset(&set,SIGUSR2);
  pthread_mutex_lock(&WatchdogMutex);
  if (NumberOfThreads == 0)
      gettimeofday(&tm,NULL);
      abstime.it_interval.tv_sec = DELAY;
      abstime.it_interval.tv_usec = abstime.it_value.tv_usec = 0;
      abstime.it_value.tv_sec = abstime.it_interval.tv_sec;
      if (setitimer(ITIMER_REAL,&abstime,NULL)){
          perror("setitimer");
          exit(EXIT_FAILURE);
      }
  NumberOfThreads++;
  pthread_mutex_unlock(&WatchdogMutex);
  for(;;){
      sigwait(&set,&sig);
      switch (sig){
      case SIGUSR2:
	  continue;
      case SIGALRM:
          pthread_cancel(arg->connection_thread);
          gettimeofday(&tm1,NULL);
      case SIGUSR1:
          pthread_mutex_lock(&WatchdogMutex);
          NumberOfThreads--;
          if (!NumberOfThreads){
              memset(&abstime,0,sizeof(struct itimerval));
              if (setitimer(ITIMER_REAL,&abstime,NULL)){
                  perror("setitimer");
                  exit(EXIT_FAILURE);
              }
          }
          pthread_mutex_unlock(&WatchdogMutex);
          printf("tm1 -tm = { %d, %d }\n",
                  tm1.tv_sec - tm.tv_sec , tm1.tv_usec - tm.tv_usec);
          return(NULL);
      }
   }
}
#endif /* #else !defined (USE_PTHREAD_KILL) */
#endif /* else defined (BAD_PRACTICE) */


static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
  char    buf[256];
  X509   *err_cert;
  int     err, depth;
  SSL    *ssl;
  mydata_t *mydata;

  err_cert = X509_STORE_CTX_get_current_cert(ctx);
  err = X509_STORE_CTX_get_error(ctx);
  depth = X509_STORE_CTX_get_error_depth(ctx);
  /*
   * Retrieve the pointer to the SSL of the connection currently treated
   * and the application specific data stored into the SSL object.
   */
  ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
  mydata = SSL_get_ex_data(ssl, mydata_index);

  X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);
  /*
   * Catch a too long certificate chain. The depth limit set using
   * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
   * that whenever the "depth>verify_depth" condition is met, we
   * have violated the limit and want to log this error condition.
   * We must do it here, because the CHAIN_TOO_LONG error would not
   * be found explicitly; only errors introduced by cutting off the
   * additional certificates would be logged.
   */
  if (depth > mydata->verify_depth) {
      preverify_ok = 0;
      err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
      X509_STORE_CTX_set_error(ctx, err);
  }
  if (!preverify_ok) {
      printf("verify error:num=%d:%s:depth=%d:%s\n", err,
      X509_verify_cert_error_string(err), depth, buf);
  }
  else if (mydata->verbose_mode) {
      printf("depth=%d:%s\n", depth, buf);
  }

  /*
   * At this point, err contains the last verification error. We can
   * use it for something special
   */
  if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT))
  {
      X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
      printf("issuer= %s\n", buf);
  }

  if (mydata->always_continue)
      return 1;
  else
      return preverify_ok;
}

/*
 * This is our per socket thread.
 */
void *serve_connection (void *ptr){
  arg_t *arg =(arg_t *)ptr;
  pthread_attr_t attr;
  int len=0,verify_depth=1;
  char *current_cipher_name;
  X509 *peer;
  mydata_t mydata; 
#if defined (__VMS)
  struct timeval timeout={0,10000};
  fd_set rmask;
#endif
#if defined (BAD_PRACTICE)
  sigset_t set;

  /*
   * On Unix systems the signal mask is inherited from its creator.
   */
  sigemptyset(&set);
  sigaddset(&set,SIGUSR1);
#endif
  pthread_cleanup_push(finish,ptr);
  /*
   * initialize the synchronization pthread mutex
   * and condition.
   */
#if !defined (USE_PTHREAD_KILL) || defined (BAD_PRACTICE)
  /*
   * Synchronization needed for the pthread_cond_timedwait
   * These two calls must occur once the finish cleanup handler
   * has been established and before any pthread_exit/pthread_testcancel
   * or the creation of the watchdog timer which may pthread_kill this thread.
   * Doing this initialization here is safe.
   */
  pthread_mutex_init(&arg->mutex,NULL);
  pthread_cond_init(&arg->cond,NULL);
#endif
  /*
   * Complete the SSL connection from the remote peer.
   */
  arg->sbio=BIO_new_socket(arg->s,BIO_CLOSE);
  SSL_set_bio(arg->ssl,arg->sbio,arg->sbio);
  /*
   * Choose the encryption algorythm by selecting a cipher.
   * As bio_html is a SSL V2 client, one must choose a sslv2 cipher.
   * To get available ciphers to choose for this application, enter the
   * command:
   * openssl ciphers -v -ssl2
   * The DES-CBC3-MD5 cipher is a sslv2 cipher and can only be used
   * with a sslv2 client.
   * Here we let the client chose its cipher or have OpenSSL chose one
   * during the SSL_accept/SSL_connect negociation.
   */
   if (!SSL_set_cipher_list(arg->ssl,"ALL"))
      berr_exit("cipher could not be set");
  /*
   * This server  will send its X509 certificate to the client when the
   * connection is accepted, but using SSL_VERIFY_NONE, this program does
   * not require a certificate from the remote SSL peer.
   */
  mydata_index = SSL_get_ex_new_index(0, "mydata index", NULL, NULL, NULL);
  SSL_set_verify(arg->ssl,SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,
                 verify_callback);
  /*
   * Let the verify_callback catch the verify_depth error so that we get
   * an appropriate error in the logfile.
   */
  SSL_set_verify_depth(arg->ssl,verify_depth+1);

  /*
   * Set up the SSL specific data into "mydata" and store it into the SSL
   * structure.
   */
  mydata.verify_depth = verify_depth; 
  mydata.verbose_mode=1;
  mydata.always_continue=0;
  SSL_set_ex_data(arg->ssl, mydata_index, &mydata);

  /*
   * Do accept the SSL connection.
   */
  if ((len=SSL_accept(arg->ssl))<=0){
       fprintf(stderr,"SSL accept error\n");
       ERR_print_errors_fp(stderr);
       pthread_exit(NULL);
  }
  if (peer = SSL_get_peer_certificate(arg->ssl))
  {
      if (SSL_get_verify_result(arg->ssl) != X509_V_OK)
      {
          /* The client did not send a certificate which verified OK */
          fprintf(stderr,"SSL_get_verify_result error\n");
          ERR_print_errors_fp(stderr);
          pthread_exit(NULL);
      }
  }

  current_cipher_name=(char *)SSL_get_cipher_name(arg->ssl);
  pthread_mutex_lock(&ScreenMutex);
  printf("Using cipher %s\n",current_cipher_name);
  pthread_mutex_unlock(&ScreenMutex);
  /*
   * Start the connection activity watchdog.
   */
  pthread_attr_init(&attr);
  /*
   * This call is not necessary as it is a default. However it is
   * there to highlight the fact we pthread_join the watchdog thread
   * in the finish cleanup handler.
   */
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
  pthread_create(&arg->watchdog_thread,&attr,connection_watchdog,arg);
  /*
   * endless loop waiting for activity on the socket.
   */
  for (;;){
#if defined (__VMS)
       FD_ZERO(&rmask);
       FD_SET(arg->s,&rmask);
       /*
        * A select is usually cancelable on most Unix by a pthread_cancel.
        * Look at pthread man to ensure it is actually cancelable.
        * On Unixes, the code can be much simplified, withdrawing the need for
        * the select, as behind the SSL_read ought to be either a recv or read
        * eventually with a select which are all cancelable points on most
	* Unixes.
        *
        * On VMS, select/read/recv are not currently cancelable points. Hence,
        * for OpenVMS, the use of a select's timeout in order to execute the
        * portable pthread_testcancel() which  will test a pending
        * pthread_cancel and will, via the  pthread_cleanup_push/pop execute 
	* the finish routine which closes the SSL link.
	*
        * The bio_html Web browser emulator or an actual Web browser such as
	* Firefox or Internet Explorer partner program blocking on a SSL_read
        * will therefore be unblocked, the SSL_read returning zero and 
	* gracefully end.
        */
       switch (select(arg->s+1,&rmask,NULL,NULL,&timeout)){
           case 0:  pthread_testcancel();
                    continue;
           case -1: perror("select");
                    pthread_exit(NULL);
                    break;
           default:
#elif !defined (__unix__) && !defined (__unix)
#error "Unsupported operating system"
#endif
#if defined (USE_PTHREAD_KILL) && !defined (BAD_PRACTICE)
		    /*
		     * Unblock the sigwait with the SIGUSR2 signal to tell
		     * we have activity.
		     */
		    pthread_kill(arg->watchdog_thread,SIGUSR2);
#else
                    /*
                     * Signal the watchdog thread we have socket activity
                     */
                    pthread_mutex_lock(&arg->mutex);
		    arg->signalled=1;
                    pthread_cond_signal(&arg->cond);
                    pthread_mutex_unlock(&arg->mutex);
#endif
		    /*
                     * Read the available data on the SSL link.
		     * Allocate the buffer here with a calloc, just like
		     * in any real SSL application. We will receive len
		     * character and we later strcat(msg,arg->message).
		     * the received data is not necessarily zero terminated.
		     * This message buffer is freed just before the for loop
		     * end.
                     */
                    arg->message=(char *)calloc(MAX_MESSAGE_SIZE+2,sizeof(char));
#if defined (BAD_PRACTICE)
		    /*
		     * Change this thread signal mask so we can be killed
		     * with the SIGUSR1 signal if the SSL_read is hanging.
		     */
                    pthread_sigmask(SIG_UNBLOCK,&set,NULL);
#endif
		    /*
		     * On Linux, the SSL_read man says:
		     * The underlying BIO is blocking, SSL_read will return,
		     * once the read operation has been finished or an error
		     * occured, except when a renegociation take place, in
		     * which case a SSL_ERROR_WANT_READ may occur.
		     */
                    len=SSL_read(arg->ssl,arg->message,MAX_MESSAGE_SIZE);
#if defined (BAD_PRATICE)
		    /*
                     * Block the SIGUSR1 signal again.
		     */
                    pthread_sigmask(SIG_BLOCK,&set,NULL);
#endif
#if !defined (BAD_PRACTICE)
		    /*
		     * We come here either because the SSL_read has been
		     * canceled, either after a completed alarm(2) or
		     * after a cond_timedwait returning a timeout.
		     *
		     * The testcancel is compulsary on HP-UX and Tru64, but 
		     * not on Linux FC5. The fprintf (write) is a cancelation
                     * point. On HP-UX and Tru64 this leaves the ScreenMutex
                     * locked.
		     */
#if !defined (__linux__) && !defined (HPUX_READ_EINTR_PATCH)
		    pthread_testcancel();
#endif
#endif
                    if (len <=0){
                        pthread_mutex_lock(&ScreenMutex);
                        fprintf(stdout,"Client disconnected\n");
                        pthread_mutex_unlock(&ScreenMutex);
			/*
			 * Abort the pthread_cond_timedwait or the sigwait
			 * waiting watchdog thread.
			 */
#if defined (USE_PTHREAD_KILL)
			pthread_kill(arg->watchdog_thread,SIGUSR1);
#else
			pthread_cancel(arg->watchdog_thread);
#endif
                        pthread_exit(NULL);
                    }
                    pthread_mutex_lock(&ScreenMutex);
                    fprintf(stdout,"Received %.*s\n",len,arg->message);
                    pthread_mutex_unlock(&ScreenMutex);
                    /*
                     * Echo via the SSL link the data just received.
                     */
                    {
                        int i;
                        char *msg;
			int sent_html=0;
                        int size=0;
                        if (strstr(arg->message,"GET / HTTP"))
                            arg->http_request=1;
                        for (i=0;i<2;i++){
                             switch(i){
                             case 0: if ((arg->http_request) && !(sent_html)){
                                         size=strlen(HTML_PREFIX);
                                     }
                                     break;
                             case 1: size+=len;
                                     break;
                             }
                        }/* end for */
                        msg=(char *)calloc(size+1,sizeof(char));
                        if (!sent_html){
                            strcpy(msg,HTML_PREFIX);
                            sent_html++;
                        }
                        strcat(msg,arg->message);
                        size=SSL_write(arg->ssl,msg,strlen(msg));
                        free(msg);
                        if (size<=0){
                            /*
                             * The remote partner exited.
                             * Exit the thread meanwhile invoking the
                             * finish routine.
                             */
                            pthread_mutex_lock(&ScreenMutex);
                            fprintf(stdout,"Client disconnected\n");
                            pthread_mutex_unlock(&ScreenMutex);
			    /*
			     * Abort the pthread_cond_timedwait or the sigwait
			     * waiting watchdog thread.
			     */
#if defined (USE_PTHREAD_KILL)
			    pthread_kill(arg->watchdog_thread,SIGUSR1);
#else
			    pthread_cancel(arg->watchdog_thread);
#endif
                            pthread_exit(NULL);
			}
                  }
		  free(arg->message);
		  arg->message=NULL;
#if defined (__VMS)
         }/* end switch */
#endif
  }/* end for */
   pthread_cleanup_pop(1);
   return(NULL);
}
long get_ia(char *str)
{
        unsigned long ia;
        unsigned long temp[4];
        char ch;

        for(;;) {
            if (sscanf (str, "%d.%d.%d.%d%c",
                &temp[0],&temp[1],&temp[2],
                &temp[3],&ch) != 4) return (-1);

            if ((temp[0] > 255) ||
                (temp[1] > 255) ||
                (temp[2] > 255) ||
                (temp[3] > 255)) return(-1);

            break;
        }
        ia = temp[0] + (temp[1] << 8) + (temp[2] << 16) + (temp[3] << 24);
        return (ia);
}
/*
 * The routine setup_sockets enables to create sockets, bind and listen on them 
 * on a variable number of computer's network interfaces, specified on the
 * command line . The last parameter in the command line is an optional port.
 * If no argument is specified then it creates one socket bound to
 * 0.0.0.0:default_port. For a program, it enables specifying the following
 * syntax : $ ./<prog> [IP of Interface1 .. [IP of InterfaceN]] [port]
 * All arguments between square brackets are optional.
 */
int setup_sockets(int argc, char **argv, int *s, unsigned short default_port)
{
  struct  sockaddr_in s_name;    /* Address struct for socket2.*/
  int     on = 1,i;

  argc--;
  argv++;
  if (argc == 0){
      i=argc;
      memset(&s_name,0,sizeof(s_name));
      s_name.sin_port = htons(default_port) ;
      s_name.sin_addr.s_addr = htonl(INADDR_ANY);
      s_name.sin_family = AF_INET ;
      if ((s[i] = socket (AF_INET, SOCK_STREAM, 0)) == -1){
          perror("socket error") ;
          exit(EXIT_FAILURE);
      }
      if (setsockopt(s[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0){
          perror("setsockopt error") ;
          exit(EXIT_FAILURE);
      }
      if (bind (s[i],(struct sockaddr *)&s_name, sizeof (s_name))<0){
          perror("bind error") ;
          exit(EXIT_FAILURE);
      }
      if (listen (s[i], 5) < 0){
          perror("listen error") ;
          exit(EXIT_FAILURE);
      }
  }
  else{
      for (i=0;i<argc;i++){
           memset(&s_name,0,sizeof(s_name));
           s_name.sin_family = AF_INET ;
           if ((argc >= 1) && ((get_ia(argv[argc-1]) == -1)))
               s_name.sin_port = htons(atoi(argv[argc-1])) ;
           else
               s_name.sin_port = htons(default_port) ;
           if ((argc == 1) && ((get_ia(argv[argc-1]) == -1)))
               s_name.sin_addr.s_addr = htonl(INADDR_ANY);
           else    
               if ((s_name.sin_addr.s_addr =get_ia(argv[i])) == -1)
                   break;
           if ((s[i] = socket (AF_INET, SOCK_STREAM, 0)) == -1){
               perror("socket error") ;
               exit(EXIT_FAILURE);
           }
           if (setsockopt(s[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0){
               perror("setsockopt error") ;
               exit(EXIT_FAILURE);
           }
           if (bind (s[i],(struct sockaddr *)&s_name, sizeof (s_name))<0){
               perror("bind error") ;
               exit(EXIT_FAILURE);
           }
           if (listen (s[i], 5) < 0){
               perror("listen error") ;
               exit(EXIT_FAILURE);
           }
      }/* end for */
  }/* end else */
  argc=i;
  if (argc == 0) argc++;
  return argc;
}
int main(int argc,char **argv)
{
  int     s[10];                    /* sockets */
  int     i;
  struct  sockaddr_in s_name;    /* Address struct for socket2.*/
  socklen_t namelength;
  fd_set rd_msk;
  arg_t *arg;
  pthread_t dynthread;
  SSL_CTX *ctx;
  pthread_attr_t attr;
#if defined (__unix__) || defined (__unix)
  sigset_t set;
#endif
  /*
   * For Unix systems we use a pthread_kill instead of a pthread_cancel
   * So set signal mask and use pthread_sigmask to block or unblock
   * SIGUSR1 signal. If good practice, then block SIGALRM as well.
   */
#if defined (BAD_PRACTICE)
  struct sigaction action;

  warning();
#endif
  fprintf(stderr," beginning of the program\n ");
  argc=setup_sockets(argc,argv,s,AUXSERVER_PORT);
  pthread_mutex_init(&ScreenMutex,NULL);
  pthread_mutex_init(&WatchdogMutex,NULL);
  SSL_load_error_strings();
  // Initialize the SSL library.
  SSL_library_init();
  OpenSSL_add_all_ciphers();
  /*
   * Create an SSL context that will be used to communicate over SSLv2, SSLv3,
   * or TLSv1 protocol, depending on the choice made by the remote partner
   * program.
   */
  if ((ctx=SSL_CTX_new(SSLv23_server_method()))==NULL)
      berr_exit("Can't initialize SSLv23 context");
  if (!SSL_CTX_use_certificate_file(ctx,KEYFILE, SSL_FILETYPE_PEM))
      berr_exit("Can't read certificate");
  SSL_CTX_set_default_passwd_cb(ctx,password_cb);
  if (!SSL_CTX_use_PrivateKey_file(ctx, KEYFILE, SSL_FILETYPE_PEM))
      berr_exit("Can't read key file");
  if (!SSL_CTX_load_verify_locations(ctx,NULL,"./"))
      berr_exit("Can't load verify locations");
//  SSL_CTX_set_client_CA_list(ctx,SSL_load_client_CA_file(KEYFILE));
#if defined (__unix__) || defined (__unix)
  sigemptyset(&set);
  sigaddset(&set,SIGPIPE);
#if defined (USE_PTHREAD_KILL)
  sigaddset(&set,SIGUSR1);
#if !defined (BAD_PRATICE)
  sigaddset(&set,SIGUSR2);
  sigaddset(&set,SIGALRM);
#endif
#endif
  pthread_sigmask(SIG_SETMASK,&set,NULL);
  pthread_sigmask(SIG_BLOCK,&set,NULL);
#endif
#if defined (BAD_PRACTICE)
  /*
   * On Unix, a signal handler is shared by all threads.
   * This handler is established for both the serve_connection
   * thread and the watchdog thread. We do not use the signal
   * statement as on Linux it has an undefined behavior in 
   * multi-threaded applications.
   */
  action.sa_handler=SIGUSR1_handler;
  action.sa_flags=SA_RESTART;
  sigemptyset(&action.sa_mask);
  sigaction(SIGUSR1,&action,NULL);
#endif
  pthread_attr_init(&attr);
#if 0
  /* the call below is unnecessary in this case as the default stack size
   * seems to be sufficient on all tested operating systems. It is there
   * to highlight how one can set a thread stacksize before it gets created.
   * The stacksize parameter for the CreateThread on Windows must be
   * specified.
   */
  pthread_attr_setstacksize(&attr,10*PTHREAD_STACK_MIN);
#endif
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  for (;;){
      namelength = sizeof (s_name);
      arg = (arg_t *)malloc(sizeof (arg_t));
      memset(arg,0,sizeof(arg_t));
      FD_ZERO(&rd_msk);
      for (i=0;i<argc;i++)
          FD_SET(s[i],&rd_msk);
      select(getdtablesize(),&rd_msk,NULL,NULL,NULL);
      for (i=0;i<argc;i++){
           if (FD_ISSET(s[i],&rd_msk))
               arg->s = accept (s[i],(struct sockaddr *)&s_name,&namelength) ;
      } 
      if (arg->s == -1){
          pthread_mutex_lock(&ScreenMutex);
          perror ("accept error") ;
          pthread_mutex_unlock(&ScreenMutex);
          free(arg);
          exit(EXIT_FAILURE);
      }
      pthread_mutex_lock(&ScreenMutex);
      fprintf (stderr,"Client address : %s\n",inet_ntoa(s_name.sin_addr));
      fprintf (stderr,"Client port : %d\n",ntohs(s_name.sin_port) ) ;
      pthread_mutex_unlock(&ScreenMutex);
      /*
       * Create a new SSL context based upon what we chose on the CTX context
       * which will handle the SSL link with the remote peer.
       */
      arg->ssl=SSL_new(ctx);
      /*
       * Start the thread that will serve the connection
       */
      pthread_create(&arg->connection_thread,
                     &attr,
                     serve_connection,
                     arg);
   }
   return 1;
} /* end main */
