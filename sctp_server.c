#include <libsnp/np_header.h>
#include <libsnp/np_lib.h>
#include <libsnp/log.h>
#include "common.h"

/* this function disables ctrl+C, so that process can only be killed by ctrl+\ */
void disableSIGINT (void);
void SIGINT_handler (int signal);
/* this function enables SIGCHID signal, so that we can handle dead child */
void enableSIGCHILD (void);
void SIGCHILD_handler (int signal);
// this function controls how many events to subscribe
void sctpSubscribeEvent (struct sctp_event_subscribe * events);

int
main(int argc, char **argv)
{
  int                         sock_fd, msg_flags;
  char*                       readbuf;
  struct sockaddr_in          servaddr, cliaddr;
  struct sctp_sndrcvinfo      sri;
  struct sctp_event_subscribe events;
  int                         stream_increment=1;
  socklen_t                   len;
  int                         rd_sz;
  int                         close_time = 120;
  union sctp_notification*    sn;
  struct sctp_paddr_change*   spc;
  // Set heartbeat interval in ms
  int                         interval = 10;
  // set interval to SCTP_NO_HB (0) to disable heartbeat
  // set interval to SCTP_ISSUE_HB (0xffffffff) to request immediate heartbeat

  //Program initialization, bind, and listen
  // AF_INET + SOCK_SEQPACKET defines SCTP protocol in POSIX
  sock_fd = Socket (AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  if (argc < 2)
    {
      bzero (&servaddr, sizeof (servaddr));
      servaddr.sin_family = AF_INET;
      // in unspecified by argv[1], server side sctp  address is bind 
      // to INADDR_ANY, which is usually two addresses
      // 1. the primary interface's IP (if it exists and is up)
      // 2. 127.0.0.1 
      // initialize serv struct sockaddr_in servaddr
      servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
      servaddr.sin_port = htons (SERV_PORT);
      Bind (sock_fd, (SA*) &servaddr, sizeof (servaddr));
    }
  else
    {
      //bind address set
      if (sctp_bind_arg_list (sock_fd, argv + 1, argc - 1))
        {
          log_error ("can't bind address set");
          exit (-1);
        }
    }

  //Terminate an association after 2 minutee
  Setsockopt (sock_fd, IPPROTO_SCTP, SCTP_AUTOCLOSE, &close_time, sizeof (close_time));
  
  // Subscribe to all events before alling listen
  bzero (&events, sizeof (events));
  sctpSubscribeEvent (&events);
  Setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS,
       &events, sizeof(events));
  // All setsockopt operation should be finished by now.
  Listen(sock_fd, 10);

  disableSIGINT ();
  enableSIGCHILD ();

  // main program
  while (true)
    {
      //len is initialized everytime, because sctp_recvmsg takes it as in-out parameter
      len = sizeof(struct sockaddr_in);
      log_debug ("blocking to receive message");
      // rd_sz = Sctp_recvmsg (sock_fe, readbuf, SCTP_PDAPI_INCR_SIZE,
      //                       (SA*) &cliaddr, &len, &sri, &msg_flags);
      // When receiving message, filling in sri (struct sctp_sndrcvinfo)
      // structure. This structure records all sender, receiver related
      // informations. Including association id, stream number, 
      // sequence number. Or in case this is an event, it contain event
      // related information.
      readbuf = pdapi_recvmsg (sock_fd, &rd_sz, (SA*) &cliaddr, 
                               &len, &sri, &msg_flags);
      // whenever receiving from socket, check to if it is
      // 1. local event
      // 2. peer data message (peer does not generate event)
      if (msg_flags & MSG_NOTIFICATION)
        {
          print_notification (sock_fd, readbuf);
          if (((union sctp_notification*) readbuf)->sn_header.sn_type == SCTP_PEER_ADDR_CHANGE)
            {
              spc = &((union sctp_notification*) readbuf)->sn_paddr_change;
              if (spc->spc_state == SCTP_ADDR_CONFIRMED )
                {
                  heartbeat_action (sock_fd, (SA*)&spc->spc_aaddr, sizeof(spc->spc_aaddr), interval);
                  log_info ("heart beat is set for %d", interval);
                }
            }
        }
      else
        {
          log_debug ("%d bytes of message received", rd_sz);
          if (rd_sz > 0)
            {
              if (stream_increment)
                {
                  // increment the stream number to reply to a different stream.
                  // This has the benefit or avoiding head of line blocking
                  sri.sinfo_stream++;
                  int retsz;
                  // getting the maximum available stream
                  struct sctp_status status;
                  bzero (&status, sizeof(status));
                  retsz = sizeof (status);
                  int assoc_id = sri.sinfo_assoc_id;
                  // Get status of a sctp socket
                  Sctp_opt_info (sock_fd, assoc_id, SCTP_STATUS, &status, &retsz);
                  // if the incremented stream exceeds system limit, sending message back on
                  // stream 0
                  if (sri.sinfo_stream >= status.sstat_outstrms)
                    sri.sinfo_stream = 0;
                }
              // sending the message back
              Sctp_sendmsg (sock_fd, readbuf, rd_sz, (SA*) &cliaddr, len,
                            sri.sinfo_ppid, sri.sinfo_flags,
                            sri.sinfo_stream, 0, 0);
            }
        }
    }

  free (readbuf);
  return 0;
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
      printf ("\n");
      log_info ("Terminating sctp server.");
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
    log_debug ("child %4d terminated\n", pid);

  return;
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
