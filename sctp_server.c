#include "nplib/np_header.h"
#include "nplib/np_lib.h"
#include <netinet/sctp.h>

int
main(int argc, char **argv)
{
  int                         sock_fd, msg_flags;
  char                        readbuf[BUFF_SIZE];
  struct sockaddr_in          servaddr, cliaddr;
  struct sctp_sndrcvinfo      sri;
  struct sctp_event_subscribe evnts;
  int                         stream_increment=1;
  socklen_t                   len;
  size_t                      rd_sz;

  //Program initialization, bind, and listen
  if (argc == 2)
    stream_increment = atoi (argv[1]);
  sock_fd = Socket (AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (SERV_PORT);

  Bind (sock_fd, (SA*) &servaddr, sizeof (servaddr));
  
  bzero (&evnts, sizeof (evnts));
  evnts.sctp_data_io_event = 1;
  Setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
       &evnts, sizeof(evnts));
  Listen(sock_fd, LISTEN_Q);

  // main program
  while (true) {
    //len is initialized everytime, because sctp_recvmsg takes it as in-out parameter
    len = sizeof(struct sockaddr_in);
    rd_sz = sctp_recvmsg (sock_fd, readbuf, sizeof (readbuf),
                          (SA*) &cliaddr, &len, &sri, &msg_flags);
    if (stream_increment) {
      sri.sinfo_stream++;
      int retsz;
      struct sctp_status status;
      bzero (&status, sizeof(status));
      if (sri.sinfo_stream >= 10) 
        sri.sinfo_stream = 0;
    }
    sctp_sendmsg (sock_fd, readbuf, rd_sz, (SA*) &cliaddr, len,
                  sri.sinfo_ppid, sri.sinfo_flags,
                  sri.sinfo_stream, 0, 0);
  }
}
