#include "nplib/np_header.h"
#include "nplib/np_lib.h"

static char ip_addr_presentation[INET_ADDRSTRLEN];

void print_notification (char* notify_buf)
{
  union sctp_notification* notification;
  struct sctp_assoc_change*     sac;
  struct sctp_paddr_change*     spc;
  struct sctp_remote_error*     sre;
  struct sctp_send_failed*      ssf;
  struct sctp_shutdown_event*   sse;
  struct sctp_adaptation_event* sae;
  struct sctp_pdapi_event*      pdapi;
  const char* str; 

  notification = (union sctp_notification*) notify_buf;
  switch (notification->sn_header.sn_type)
    {
      case SCTP_ASSOC_CHANGE:
        sac = &notification->sn_assoc_change;
        switch (sac->sac_state)
          {
            case SCTP_COMM_UP:
              str = "COMMUNICATION UP";
              break;
            case SCTP_COMM_LOST:
              str = "COMMUNICATION LOST";
              break;
            case SCTP_RESTART:
              str = "RESTART";
              break;
            case SCTP_SHUTDOWN_COMP:
              str = "SHUTDOWN COMPLETE";
              break;
            case SCTP_CANT_STR_ASSOC:
              str = "CAN'T START ASSOCIATION";
              break;
            default:
              str = "UNKNOWN";
              break;
          }
        printf ("SCTP_ASSOC_CHANGE: %s, assoc=0x%x\n",
                 str, (uint32_t) sac->sac_assoc_id);
        break;
      case SCTP_PEER_ADDR_CHANGE:
        spc = &notification->sn_paddr_change;
        switch (spc->spc_state)
          {
            case SCTP_ADDR_AVAILABLE:
              str = "ADDRESS AVAILABLE";
              break;
            case SCTP_ADDR_UNREACHABLE:
              str = "ADDRESS UNREACHABLE";
              break;
            case SCTP_ADDR_REMOVED:
              str = "ADDRESS REMOVED";
              break;
            case SCTP_ADDR_ADDED:
              str = "ADDRESS ADDED";
              break;
            case SCTP_ADDR_MADE_PRIM:
              str = "ADDRESS MADE PRIMARY";
              break;
            default:
              str = "UNKNOWN";
              break;
          }
        Inet_ntop (AF_INET, (SA*) spc->scp_aaddr, ip_addr_presentation, sizeof (spc_aaddr)); 
        printf ("SCTP_PEER_ADDR_CHANGE: %s, addr=%s, assoc=0x%x\n",
                 str, ip_addr_presentation, (uint32_t) spc->spc_assoc_id);
        break;
      case SCTP_REMOTE_ERROR:
        sre = &notification->sn_remote_error;
        printf ("SCTP_REMOTE_ERROR: asoc=0x%x, error=%d\n",
                 (uint32_t) sre->sre_assoc_id, sre->sre_error);
        break;
      case SCTP_SEND_FAILED:
        ssf = &notification->sn_send_failed;
        printf ("SCTP_SEND_FAILED: asoc=0x%x, error=%d\n",
                 (uint32_t) ssf->ssf_assoc_id, ssf->ssf_error);
        break;
      case SCTP_ADAPTATION_INDICATION:
        sai = &notification->sn_adaptation_event;
        printf ("SCTP_ADAPTATION_INDICATION: 0x%x\n", (u_int) sai->sai_adaptation_ind);
        break;
      case SCTP_PARTIAL_DELIVERY_EVENT:
        pdapi = &notification->sn_pdapi_event;
        if (pdapi->pdapi_indication == SCTP_PARTIAL_DELIVERY_ABORTED)
          printf ("SCTP_PARTIAL_DELIVERY_ABORTED\n");
        else
          printf ("Unknown SCTP_PARTIAL_DELIVERY_EVENT 0x%x\n",
                  pdapi->pdapi_indication);
        break;
      case SCTP_SHUTDOWN_EVENT:
        sse = &notification->sn_shutdown_event;
        printf ("SCTP_SHUTDOWN_EVENT: assoc_id=0x%x\n", (uint32_t)sse->sse_assoc_id);
        break;
      default:
        nonfatal_user_ret ("unknown notification type 0x%x\n", notification->sn_header.sn_type);
    }
}

