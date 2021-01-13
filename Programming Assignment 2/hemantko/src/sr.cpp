#include "../include/simulator.h"
#include "../include/utils.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

struct buffer
{
  struct msg *m;
  float time;
  bool acked;
};

struct buffer *buffer_A[1000];
struct msg *msg_buff[1000];

// Sequence numbers
int sequence_number_A = 0, sequence_number_B = 0;

// Acknowledgement numbers
int acknowledgment_number_A = 0, acknowledgment_number_B = 0;

int nextseqnum = 0, send_base_A = 0, send_base_B = 0, window_size;

// Buffer messages
queue<msg> msg_queue;

struct pkt *make_packet(int sequence_number, int acknowledgement_number, char *data)
{
  struct pkt *packet = new pkt();

  packet->acknum = acknowledgement_number;
  packet->seqnum = sequence_number;
  strncpy(packet->payload, data, PAYLOAD_SIZE);

  packet->checksum = checksum(packet);

  return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  cout << "SR A_output()" << endl;

  msg_queue.push(message);

  struct msg m;
  if (!msg_queue.empty())
  {
    m = msg_queue.front();
    cout << "Message to be processed: " << m.data << endl;
    msg_queue.pop();
  }   
  else
  {
    cerr << "Empty buffer queue. No messages to process" << endl;
    return;
  }

  struct buffer *a_type = new buffer();
  a_type->m = new msg();
  strncpy(a_type->m->data, m.data, PAYLOAD_SIZE);
  a_type->time = get_sim_time();
  a_type->acked = false;
  buffer_A[nextseqnum] = a_type;

  if (nextseqnum < send_base_A + window_size)
  {
    struct pkt *packet = make_packet(nextseqnum, acknowledgment_number_A, buffer_A[nextseqnum]->m->data);
    tolayer3(A, *packet);
  }

  nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  cout << "SR A_input()" << endl;

  if (packet.checksum != checksum(&packet))
  {
    cerr << "Checksum mismatch" << endl;
    return;
  }

  if (packet.acknum < send_base_A || packet.acknum > min(send_base_A + window_size, nextseqnum))
  {
    cerr << "Packets arrived out of order" << endl;
    return;
  }

  buffer_A[packet.acknum]->acked = true;
  send_base_A = (packet.acknum == send_base_A) ? packet.acknum + 1 : send_base_A;
  acknowledgment_number_A = packet.seqnum;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  cout << "SR A_timerinterrupt()" << endl;

  int current_time = get_sim_time();

  for (int i = send_base_A; i < min(send_base_A + window_size, nextseqnum); i++)
  {
    if (!buffer_A[i]->acked && TIMEOUT < current_time - (int)buffer_A[i]->time)
    {
      struct pkt *packet = make_packet(i, acknowledgment_number_A, buffer_A[i]->m->data);
      tolayer3(A, *packet);
      buffer_A[i]->time = current_time;
    }
  }

  starttimer(A, TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  cout << "SR A_init()" << endl;
  window_size = getwinsize();
  starttimer(A, TIMEOUT);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  cout << "SR B_input()" << endl;

  if (packet.checksum != checksum(&packet))
  {
    cerr << "Checksum mismatch" << endl;
    return;
  }

  if (packet.seqnum < send_base_B - window_size || packet.seqnum > send_base_B + window_size)
  {
    cerr << "Packets arrived out of order" << endl;
    return;
  }

  msg_buff[packet.seqnum] = new msg();
  strncpy(msg_buff[packet.seqnum]->data, packet.payload, PAYLOAD_SIZE);

  if (send_base_B == packet.seqnum)
  {
    while (msg_buff[send_base_B] != NULL)
      tolayer5(B, msg_buff[send_base_B++]->data);
  }

  struct pkt *ack_packet = make_packet(packet.acknum + 1, packet.seqnum, packet.payload);
  tolayer3(B, *ack_packet);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  cout << "SR B_init()" << endl;
}
