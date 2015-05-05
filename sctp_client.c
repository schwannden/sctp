#include <libsnp/np_header.h>
#include <libsnp/np_lib.h>

#ifndef SERV_MAX_SCTP_STRM
#define SERV_MAX_SCTP_STRM 10
#endif

// simple echo reply
void sctpstr_cli(int in_fd, int sock_fd, struct sockaddr *to, socklen_t tolen);
// echo reply to all streams
void sctpstr_cli_echoall(int in_fd, int sock_fd, struct sockaddr *to, socklen_t tolen);
void sctpSubscribeEvent (struct sctp_event_subscribe * events);

int
main(int argc, char **argv)
{
  int                         sock_fd;
  struct sockaddr_in          servaddr;
  struct sctp_event_subscribe events;
  int                         echo_to_all=0;

  if (argc < 2)
    err_quit ("Missing host argument - use '%s host [echo]'\n", argv[0]);
  if (argc > 2) {
    printf ("Echoing messages to all streams\n");
    echo_to_all = 1;
  }
  // AF_INET + SOCK_SEQPACKET defines SCTP protocol in POSIX
  sock_fd = Socket (AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port        = htons (SERV_PORT);
  inet_pton (AF_INET, argv[1], &servaddr.sin_addr);

  bzero (&events, sizeof (events));
  events.sctp_data_io_event = 1;
  Setsockopt (sock_fd, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof (events));
  sctpSubscribeEvent (&events);
  Setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
       &events, sizeof(events));
  if(echo_to_all == 0)
    sctpstr_cli (STDIN_FILENO, sock_fd, (SA*) &servaddr, sizeof (servaddr));
  else
    sctpstr_cli_echoall (STDIN_FILENO, sock_fd, (SA*) &servaddr, sizeof (servaddr));
  Close (sock_fd);
  return (0);
}

void
sctpstr_cli (int in_fd, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
  struct sockaddr_in     peeraddr;
  struct sctp_sndrcvinfo sri;
  char                   sendline[SCTP_MAXBLOCK], recvline[SCTP_MAXBLOCK];
  socklen_t              len;
  int                    out_sz, rd_sz, msg_flags, max_fd, eof = 0;
  fd_set                 rset;

  FD_ZERO (&rset);
  bzero (&sri, sizeof (sri));
  max_fd = max (in_fd, sock_fd) + 1;
  while (1)
    {
      if (eof == 0)
        FD_SET( in_fd, &rset );
      FD_SET( sock_fd, &rset );
      //blocking for either user or socket inputs
      Select( max_fd, &rset, NULL, NULL, NULL);

      if( FD_ISSET(in_fd, &rset) )
        {
          if ( Readline (in_fd, sendline, SCTP_MAXBLOCK) == 0 )
            {
              eof = 1;
              FD_CLR (in_fd, &rset);
            }
          if (sendline[0] != '[')
            {
              printf ("Error, line must be of the form '[streamnum]text'\n");
              continue;
            }
          sri.sinfo_stream = strtol (&sendline[1], NULL, 0);
          out_sz = strlen (sendline);
          Sctp_sendmsg (sock_fd, sendline, out_sz, to, tolen, 
                        0, 0, sri.sinfo_stream, 0, 0);
        }
      if( FD_ISSET(sock_fd, &rset) )
        {
          len = sizeof (peeraddr);
          rd_sz = Sctp_recvmsg (sock_fd, recvline, sizeof(recvline),
                               (SA*) &peeraddr, &len,
                               &sri,&msg_flags);
          if (msg_flags & MSG_NOTIFICATION)
            {
              print_notification (sock_fd, recvline);
              if (((union sctp_notification*) recvline)->sn_header.sn_type == SCTP_SHUTDOWN_EVENT)
                return;
            }
          else
            {
              printf ("From str:%d seq:%d (assoc:0x%x):",
                      sri.sinfo_stream,sri.sinfo_ssn,
                      (u_int)sri.sinfo_assoc_id);
              printf ("%.*s",rd_sz,recvline);
            }
        }
    }
}

void
sctpstr_cli_echoall (int in_fd, int sock_fd, struct sockaddr *to, socklen_t tolen)
{
  struct sockaddr_in     peeraddr;
  struct sctp_sndrcvinfo sri;
  char                   sendline[SCTP_MAXBLOCK], recvline[SCTP_MAXBLOCK];
  socklen_t              len;
  int                    out_sz, rd_sz, i, strsz, msg_flags, max_fd, eof = 0;
  fd_set                 rset;

  FD_ZERO (&rset);
  bzero (sendline, sizeof (sendline));
  bzero (&sri, sizeof (sri));
  max_fd = max (in_fd, sock_fd) + 1;
  while (1)
    {
      if (eof == 0)
        FD_SET( in_fd, &rset );
      FD_SET( sock_fd, &rset );
      //blocking for either user or socket inputs
      Select( max_fd, &rset, NULL, NULL, NULL);

      if( FD_ISSET(in_fd, &rset) )
        {
          if ( Readline (in_fd, sendline, SCTP_MAXLINE - 9) == 0 )
            {
              eof = 1;
              FD_CLR (in_fd, &rset);
            }
          else
            {
              strsz = strlen (sendline);
              if (sendline[strsz-1] == '\n')
                {
                  sendline[strsz-1] = '\0';
                  strsz--;
                }
              for (i=0 ; i<SERV_MAX_SCTP_STRM ; i++)
                {
                  snprintf (sendline + strsz, sizeof (sendline) - strsz, ".msg.%d 1", i);
                  Sctp_sendmsg (sock_fd, sendline, strsz, 
                                to, tolen, 0, 0, i, 0, 0);
                  snprintf (sendline + strsz, sizeof (sendline) - strsz, ".msg.%d 2", i);
                  Sctp_sendmsg (sock_fd, sendline, strsz, 
                                to, tolen, 0, 0, i, 0, 0);
                }
            }
        }
      if( FD_ISSET(sock_fd, &rset) )
        {
          len = sizeof(peeraddr);
          rd_sz = Sctp_recvmsg (sock_fd, recvline, sizeof(recvline),
                                (SA*) &peeraddr, &len, &sri,&msg_flags);
          if (msg_flags & MSG_NOTIFICATION)
            {
              print_notification (sock_fd, recvline);
              if (((union sctp_notification*) recvline)->sn_header.sn_type == SCTP_SHUTDOWN_EVENT)
                return;
            }
          else
            {
              printf ("From str:%d seq:%d (assoc:0x%x):", sri.sinfo_stream,
                       sri.sinfo_ssn, (u_int) sri.sinfo_assoc_id);
              printf ("%.*s\n",rd_sz,recvline);
            }
        }
    }
}

void
sctpSubscribeEvent (struct sctp_event_subscribe * events)
{
  events->sctp_data_io_event = 1;
  events->sctp_association_event = 1;
  events->sctp_address_event = 1;
  events->sctp_send_failure_event = 1;
  events->sctp_peer_error_event = 1;
  events->sctp_shutdown_event = 1;
  events->sctp_partial_delivery_event = 1;
  events->sctp_adaptation_layer_event = 1;
}
