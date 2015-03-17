#include "nplib/np_header.h"
#include "nplib/np_lib.h"
#include <netinet/sctp.h>

void disableSIGINT (void);
void SIGINT_handler (int signal);
void enableSIGCHILD (void);
void SIGCHILD_handler (int signal);

int
main(int argc, char **argv)
{
  int                         sock_fd, msg_flags;
  //char                        readbuf[BUFF_SIZE];
  char*                       readbuf;
  struct sockaddr_in          servaddr, cliaddr;
  struct sctp_sndrcvinfo      sri;
  struct sctp_event_subscribe evnts;
  int                         stream_increment=1;
  socklen_t                   len;
  int                         rd_sz;
  int                         close_time = 120;

  //Program initialization, bind, and listen
  if (argc == 2)
    stream_increment = atoi (argv[1]);
  sock_fd = Socket (AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  //Terminate an association after 2 minutee
  Setsockopt (sock_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &close_time, sizeof close_time);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  if (argc == 2)
    inet_pton (AF_INET, argv[1], &servaddr.sin_addr);
  else
    servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (SERV_PORT);

  Bind (sock_fd, (SA*) &servaddr, sizeof (servaddr));
  
  bzero (&evnts, sizeof (evnts));
  evnts.sctp_data_io_event = 1;
  Setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
       &evnts, sizeof(evnts));
  Listen(sock_fd, 10);

  disableSIGINT ();
  enableSIGCHILD ();

  // main program
  while (true) {
    //len is initialized everytime, because sctp_recvmsg takes it as in-out parameter
    len = sizeof(struct sockaddr_in);
    printf ("blocking to receive message\n");
    //rd_sz = Sctp_recvmsg (sock_fd, readbuf, sizeof (readbuf),
    //                      (SA*) &cliaddr, &len, &sri, &msg_flags);
    readbuf = pdapi_recvmsg (sock_fd, &rd_sz, (SA*) &cliaddr, 
                             &len, &sri, &msg_flags);
    printf ("%d bytes of message received\n", rd_sz);
    if (rd_sz > 0) {
      if (stream_increment) {
        sri.sinfo_stream++;
        int retsz;
        struct sctp_status status;
        bzero (&status, sizeof(status));
        retsz = sizeof (status);
        int assoc_id = sri.sinfo_assoc_id;
        Sctp_opt_info (sock_fd, assoc_id, SCTP_STATUS, &status, &retsz);
        if (sri.sinfo_stream >= status.sstat_outstrms)
          sri.sinfo_stream = 0;
      }
      Sctp_sendmsg (sock_fd, readbuf, rd_sz, (SA*) &cliaddr, len,
                    sri.sinfo_ppid, sri.sinfo_flags,
                    sri.sinfo_stream, 0, 0);
    }
  }
}

void
disableSIGINT (void)
{
  struct sigaction newDisp;
  newDisp.sa_handler = SIGINT_handler;
  newDisp.sa_flags = 0;
  sigemptyset (&newDisp.sa_mask);
  Sigaction (SIGINT, &newDisp, NULL);
  Sigaction (SIGQUIT, &newDisp, NULL);
}

void
SIGINT_handler (int signal)
{
  if (signal == SIGINT)
    {
      printf ("  Ouch! Type Ctrl + \\ to quit.\n");
      return;
    }
  else if (signal == SIGQUIT)
    {
      printf ("  Terminating sctp server.\n");
      exit (EXIT_SUCCESS);
    }
}

void
enableSIGCHILD (void)
{
  struct sigaction newDisp;
  newDisp.sa_handler = SIGCHILD_handler;
  newDisp.sa_flags = 0;
  sigemptyset (&newDisp.sa_mask);
  Sigaction (SIGCHLD, &newDisp, NULL);
}

void
SIGCHILD_handler (int signal)
{
  int old_errno = errno;
  pid_t pid;

  while (pid = waitpid (-1, NULL, WNOHANG))
    printf ("child %4d terminated\n", pid);

  return;
}
