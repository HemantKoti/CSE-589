#include "../include/simulator.h"
#include "../include/utils.h"

#define TIMEOUT 25.0

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

struct msg *msg_buff[1000];

// Sequence numbers
int sequence_number_A = 0, sequence_number_B = 0;

// Acknowledgement numbers
int acknowledgment_number_A = 0, acknowledgment_number_B = 0;

int nextseqnum = 0, send_base_A = 0, send_base_B = 0, window_size;

// Buffer messages
queue<msg> buffer;

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
    cout << "GBN A_output()" << endl;

    buffer.push(message);

    struct msg m;
    if (!buffer.empty())
    {
        m = buffer.front();
        cout << "Message to be processed: " << m.data << endl;
        buffer.pop();
    }
    else
    {
        cerr << "Empty buffer queue. No messages to process" << endl;
        return;
    }

    msg_buff[nextseqnum] = new msg();
    strncpy(msg_buff[nextseqnum]->data, m.data, PAYLOAD_SIZE);
    if (nextseqnum < send_base_A + window_size)
    {
        cout << "Before sending to layer 3" << endl;
        struct pkt *packet = make_packet(nextseqnum, acknowledgment_number_A, msg_buff[nextseqnum]->data);
        tolayer3(A, *packet);

        if (send_base_A == nextseqnum)
            starttimer(A, TIMEOUT);
    }

    nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    cout << "GBN A_input()" << endl;

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

    acknowledgment_number_A = packet.seqnum;
    send_base_A = packet.acknum + 1;

    stoptimer(A);
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    cout << "GBN A_timerinterrupt()" << endl;

    for (int i = send_base_A; i < min(send_base_A + window_size, nextseqnum); i++)
    {
        struct pkt *packet = make_packet(i, acknowledgment_number_A, msg_buff[i]->data);
        tolayer3(A, *packet);
    }

    starttimer(A, TIMEOUT);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    cout << "GBN A_init()" << endl;
    window_size = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    cout << "GBN B_input()" << endl;

    if (packet.checksum != checksum(&packet))
    {
        cerr << "Checksum mismatch" << endl;
        return;
    }

    if (packet.seqnum != send_base_B)
    {
        cerr << "Packets arrived out of order" << endl;
        return;
    }

    acknowledgment_number_B = packet.seqnum;
    sequence_number_B = packet.acknum + 1;
    struct pkt *ack_packet = make_packet(sequence_number_B, acknowledgment_number_B, packet.payload);
    tolayer3(B, *ack_packet);

    tolayer5(B, packet.payload);
    send_base_B++;
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    cout << "GBN B_init()" << endl;
}