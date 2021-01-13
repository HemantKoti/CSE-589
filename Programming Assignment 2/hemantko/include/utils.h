#ifndef UTILS_H_
#define UTILS_H_

// Include references
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <cstring>
#include <algorithm> 

// Preprocessor constants
#define PAYLOAD_SIZE 20
#define TIMEOUT 20.0
#define DEFAULT_ACK_NUM 999
#define A 0
#define B 1

using namespace std;

// Functions
int checksum(struct pkt *packet);
int checksum(struct pkt *packet)
{
  int _checksum = 0;

  if (packet != NULL)
  {
    _checksum += packet->acknum + packet->seqnum;
    for (int i = 0; i < 20; i++)
      _checksum += packet->payload[i];
  }

  cout << "checksum(): " << checksum << endl;
  return _checksum;
}

#endif
