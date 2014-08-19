/****************************************************************************
*
*  File: tl1client.c
*  Author: Paul Greenberg (http://www.greenberg.pro)
*  Created: 2007
*  Purpose: Transaction Language 1 (TL1) Client
*  Version: 1.0
*  Copyright: (c) 2014 Paul Greenberg <paul@greenberg.pro>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
* 
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
* 
*  You should have received a copy of the GNU General Public License
*  along with this program. If not, see <http://www.gnu.org/licenses/>.
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/****************************************************************************
* DEFINITIONS                                                               *
****************************************************************************/

#define APP_NAME        "tl1client"
#define APP_VERSION     "0.1"
#define APP_DESC        "Transaction Language 1 (TL1) Client"
#define APP_COPYRIGHT   "Copyright (c) 2014 Paul Greenberg <paul@greenberg.pro>"
#define APP_DISCLAIMER  "THERE IS ABSOLUTELY NO WARRANTY FOR THIS PROGRAM."
#define BUFSIZE 4096

void throwError(char *, char *, char *);
void throwAppBanner(void);
void throwAppUsage(void);
void writeLog(char *);
void sigProc(int);
char *chomp(char *);
int tl1Socket(const char *, const char *);

bool verbose_flag, port_flag, output_flag;

char * func =     "main";
char * wCmdCode = "0001";
char * wFmt =     "txt";
char * wHost =    "localhost";
char * wLog =     "tl1client.log";
char * wPort =    "2025";
int    wPortNum = 0;
char * wSec =     "password";
char * wUser =    "root";

/****************************************************************************
* FUNCTIONS                                                                 *
****************************************************************************/

int tl1Socket(const char *host, const char *service) {

 char * iFunc =     "tl1Socket";
 // Tell the system what kind(s) of address info we want
 struct addrinfo addrCriteria;                   // Criteria for address match
 memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
 addrCriteria.ai_family = AF_UNSPEC;             // v4 or v6 is OK
 addrCriteria.ai_socktype = SOCK_STREAM;         // Only streaming sockets
 addrCriteria.ai_protocol = IPPROTO_TCP;         // Only TCP protocol

 // Get address(es)
 struct addrinfo *servAddr; // Holder for returned list of server addrs
 int rtnVal = getaddrinfo(host, service, &addrCriteria, &servAddr);
 if (rtnVal != 0)
  printf("%s ::: %s => %s\n", iFunc, "getaddrinfo() failed", gai_strerror(rtnVal));

 int sock = -1;
 for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) {
  // Create a reliable, stream socket using TCP
  sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (sock < 0) {
   printf("%s ::: %s => %s\n", iFunc, "socket()", "socket creation failed; try next address");
   continue;  // Socket creation failed; try next address
  }

  // Establish the connection to the echo server
  if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
   printf("%s ::: %s => %s\n", iFunc, "connect()", "socket connection succeeded");
   break;     // Socket connection succeeded; break and return socket
  }

  close(sock); // Socket connection failed; try next address
  sock = -1;
 }
 freeaddrinfo(servAddr); // Free addrinfo allocated in getaddrinfo()
 return sock;
}

char *chomp(char *str) {
 /* removes the newline character from a string */
 int i;
 for (i = 0; i < (int)strlen(str); i++) {
  if ( str[i] == '\n' || str[i] == '\r' ) {
   str[i] = '\0';
  }
 }
 return str;
}

void throwError(char *err_func, char *err_cat, char *err_msg ) {
 printf ("%s ::: %s => %s\n", err_func, err_cat, err_msg);
 exit(1);
}

void throwAppBanner(void) {
 printf("\n");
 printf("%s - %s\n", APP_NAME, APP_DESC);
 printf("%s\n", APP_COPYRIGHT);
 printf("%s\n", APP_DISCLAIMER);
 printf("\n");
 return;
}

void throwAppUsage(void) {
 printf("Usage:\n");
 printf("   %s --host HOSTNAME --port PORT --user USER --pass PASS --cmdcode 0001 --verbose\n", APP_NAME);
 printf("   %s --log ts1client.log --format raw\n", APP_NAME);
 printf("\n");
 return;
}

void writeLog(char * rMsg) {
 /* http://www.acm.uiuc.edu/webmonkeys/book/c_guide/2.15.html */
 time_t timer;
 timer=time(NULL);
 //printf("The current time is %s.\n",chomp(asctime(localtime(&timer))));
 FILE *fp;
 fp=fopen( wLog, "a");
 if(fp == NULL) {
  fprintf(stdout, "Error: unable to write to '%s'\n", wLog);
  exit(8);
 } else {
  fprintf(fp, "%s => %s", chomp(asctime(localtime(&timer))), rMsg);
 }
 fclose(fp);
 return;
}

void sigProc(int sigMsg) {
 /*
  catching SIGINT SIGHUP SIGTERM
  SIGHUP(1)    kill -HUP <pid>
  SIGINT(2)    Ctrl + C
  SIGTERM(15)  kill or killall
  On many Unix systems during shutdown, init issues SIGTERM to all processes
  that are not essential to powering off, waits a few seconds, and then
  issues SIGKILL to forcibly terminate any such processes that remain.
 */
 char sigCode[16][80] = {"SIG0","SIGHUP","SIGINT","SIG3",
 "SIG4","SIG5","SIG6","SIG7","SIG8","SIG9","SIG10","SIG11",
 "SIG12","SIG13","SIG14","SIGTERM"};
 time_t timer;
 timer=time(NULL);
 FILE *fp;
 fp=fopen( wLog, "a");
 if(fp == NULL) {
  fprintf(stdout, "Error: unable to write to '%s'\n", wLog);
  exit(8);
 } else {
  fprintf(fp, "%s => signal %s #%d. %s.%s service stopped...\n", chomp(asctime(localtime(&timer))), sigCode[sigMsg], sigMsg, APP_NAME, APP_VERSION);
 }
 fclose(fp);
 exit(0);
}


/****************************************************************************
* MAIN ROUTINE                                                              *
****************************************************************************/

int main(int argc, char **argv) {

 verbose_flag = false;
 port_flag = false;
 output_flag = false;

 /* http://www.kernel.org/doc/man-pages/online/pages/man3/getopt.3.html */

 int c;
 while (1) {
  static struct option long_options[] = {
   {"cmdcode", required_argument, 0, 'c'},
   {"format",  required_argument, 0, 'f'},
   {"host",    required_argument, 0, 'h'},
   {"log",     required_argument, 0, 'l'},
   {"port",    required_argument, 0, 'p'},
   {"secret",  required_argument, 0, 's'},
   {"verbose", no_argument,       0, 'v'},
   {"user",    required_argument, 0, 'u'},
   {0, 0, 0, 0}
  };

  int option_index = 0;
  c = getopt_long (argc, argv, "c:f:h:l:p:s:v:u:v", long_options, &option_index);

  /* Detect the end of the options. */
  if (c == -1)
   break;

  switch (c) {
   case 0:
    /* If this option set a flag, do nothing else now. */
    if (long_options[option_index].flag != 0)
     break;
    printf ("%s ::: stage 1 => option %s", func, long_options[option_index].name);
    if (optarg)
     printf (" with arg %s", optarg);
     printf ("\n");
     break;

   case 'c':
    /*printf ("%s ::: => option -c|--cmdcode with value '%s' is set\n", func, optarg);*/
    if((wCmdCode=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --cmdcode\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --cmdcode\n", func);
     strncpy(wCmdCode,optarg,strlen(optarg));
    }
    break;

   case 'f':
    /*printf ("%s ::: => option -f|--format with value '%s' is set\n", func, optarg);*/
    if((wFmt=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --format\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --format\n", func);
     strncpy(wFmt,optarg,strlen(optarg));
    }
    break;

   case 'h':
    /*printf ("%s ::: => option -h|--host with value '%s' is set\n", func, optarg);*/
    if((wHost=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --host\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --host\n", func);
     strncpy(wHost,optarg,strlen(optarg));
    }
    break;

   case 'l':
    //printf ("%s ::: => option -l|--log with value '%s' (size: %u) is set\n", func, optarg, strlen(optarg));
    if((wLog=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --log\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --log\n", func);
     strncpy(wLog,optarg,strlen(optarg));
     //memcpy(wLog,optarg,strlen(optarg));
     //strcpy(wLog,optarg);
    }
    output_flag = true;
    break;

   case 'p':
    if((wPort=realloc(NULL, (strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --port\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --port\n", func);
     strncpy(wPort,optarg,strlen(optarg));
    }
    /* check whether wPort is numeric in range from 32768-65535 */
    sscanf(wPort,"%d",&wPortNum);
    //printf("%s -> %d\n", wPort, wPortNum);
    if (wPortNum < 23 || wPortNum > 65535 ) {
     throwError( func, "getopt_long()", "--port MUST be between 23 and 65535" );
    }
    port_flag = true;
    break;

   case 's':
    /*printf ("%s ::: => option -s|--secret with value '%s' is set\n", func, optarg);*/
    if((wSec=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --secret\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --secret\n", func);
     strncpy(wSec,optarg,strlen(optarg));
    }
    break;

   case 'v':
    /* printf ("%s ::: => option -v|--verbose is set\n", func, optarg); */
    verbose_flag = true;
    break;

   case 'u':
    /*printf ("%s ::: => option -u|--user with value '%s' is set\n", func, optarg);*/
    if((wUser=realloc(NULL, ( strlen(optarg) * 4 + 1) ))==NULL) {
     printf("%s ::: => memory reallocation failed for --user\n", func);
     exit(1);
    } else {
     //printf("%s ::: => memory reallocation succeeded for --user\n", func);
     strncpy(wUser,optarg,strlen(optarg));
    }
    break;

   default:
    throwError( func, "getopt_long()", "unrecognized option" );
  }
 }

 if (optind < argc) {
  throwError( func, "getopt_long()", "unexpected option argument" );
 }

 if (port_flag == false || output_flag == false) {
  throwError( func, "getopt_long()", "both --port and --log options MUST be specified" );
 }

 if (verbose_flag) {
  throwAppBanner();
  printf ("%s ::: => DEBUG is ON\n", func);
  printf ("%s ::: getopt_long() => option -c|--cmdcode with value '%s' is set\n", func, wCmdCode);
  printf ("%s ::: getopt_long() => option -f|--format with value '%s' is set\n", func, wFmt);
  printf ("%s ::: getopt_long() => option -h|--host with value '%s' is set\n", func, wHost);
  printf ("%s ::: getopt_long() => option -l|--log with value '%s' is set\n", func, wLog);
  printf ("%s ::: getopt_long() => option -p|--port with value '%d' is set\n", func, wPortNum);
  printf ("%s ::: getopt_long() => option -s|--secret with value '%s' is set\n", func, wSec);
  printf ("%s ::: getopt_long() => option -u|--user with value '%s' is set\n", func, wUser);
 }
 //else {
  //throwAppBanner();
  //printf ("%s ::: => DEBUG is OFF\n", func);
 //}

 writeLog("ts1client.0.1 service started...\n");

 if (signal(SIGINT, sigProc) == SIG_ERR) {
  printf("%s ::: => cannot handle SIGINT!\n", func);
 }
 if (signal(SIGHUP, sigProc) == SIG_ERR) {
  printf("%s ::: => cannot handle SIGHUP!\n", func);
 }
 if (signal(SIGTERM, sigProc) == SIG_ERR) {
  printf("%s ::: => cannot handle SIGTERM!\n", func);
 }

 int sock = tl1Socket(wHost, wPort);
 if (sock < 0) {
  throwError( func, "tl1Socket()", "socket() failed" );
 } else {
  printf("%s ::: tl1Socket() => %s\n", func, "socket() succeeded");
 }

 char * tlMsg = "ACT-USER:DEVICE:USER:100::PASS;";
 size_t tlMsgLen = strlen(tlMsg);
 ssize_t numBytes = send(sock, tlMsg, tlMsgLen, 0);
 if (numBytes < 0) {
  throwError( func, "tl1Socket()", "send() failed" );
 } else {
  printf("%s ::: %s => %s\n", func, "tl1Socket()", "send() succeeded");
 }

 unsigned int totalBytesRcvd = 0; // Count of total bytes received
 fputs("Received: ", stdout);     // Setup to print the echoed string
 while (totalBytesRcvd < tlMsgLen) {
  char buffer[BUFSIZE];
  numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
  if (numBytes < 0)
   throwError( func, "tl1Socket()", "recv() failed" );
  else if (numBytes == 0)
   throwError( func, "tl1Socket()", "recv() connection closed prematurely" );
   totalBytesRcvd += numBytes; // Keep tally of total bytes
   buffer[numBytes] = '\0';    // Terminate the string!
   fputs(buffer, stdout);      // Print the buffer
 }
 fputc('\n', stdout); // Print a final linefeed
 close(sock);
 printf("%s ::: %s => %s\n", func, "tl1Socket()", "close() succeeded");

 //for(;;) {
 //
 //}



exit(0);
}
//end of main() function

