#include "../include/simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

char buffer[10000][21];
int endA, firstA, eseqA, baseA, N;
int TIME_OUT =40;
/*bool sendFive;
*/static int Asuccess;
int expected_ackB;
bool firstTime = true;

int calculateChecksum(char message[20], int acknum, int seq)
{
    int sum =0;
    for(int i=0;i<20;i++)
      sum += message[i];
    sum+=acknum;
    sum+=seq;
    return sum;
}

struct pkt make_pkt(int AorB, char messg[20], int seqnum, int acknum, int checksum)
{
    char AB;
    if(AorB ==1)
      AB = 'B';
    else
      {
        AB = 'A';
      }
    char mssg[20];
    if(messg != NULL)
    {
      strcpy(mssg,messg);
      mssg[20] = '\0';
    }
    else
      mssg[0] = '\0';
    struct pkt send_pkt;
    send_pkt.seqnum = seqnum;
    send_pkt.acknum = acknum;
    send_pkt.checksum = checksum;
    strcpy(send_pkt.payload,mssg);
    return send_pkt;
}

void canSend()
{
  while(eseqA < baseA+N && firstA< endA)
  {
    printf("Sending packet with seq %d\n",eseqA);
    if(firstTime)
    {
      printf("TIMER STARTED [baseA:%d]\n",baseA);
        starttimer(0,TIME_OUT);
        firstTime = false;
    }
    char msg[21];
    strcpy(msg,buffer[firstA]);
    struct pkt packt;
    int csum = calculateChecksum(msg, 0, eseqA);
    packt = make_pkt(0,msg,eseqA,0,csum); // ack and checksum
    printf("A sent [msg:%s] [seq:%d] [checksum:%d] [ack:0]\n",msg,eseqA,csum);
    firstA++;
    eseqA++;
    tolayer3(0,packt);
  }
}


void A_output(message)
  struct msg message;
{ 
  strcpy(buffer[endA],message.data);
  buffer[endA][20]='\0';
  endA++;
  // if(!sendFive)
     canSend();
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  int ack = packet.acknum;
  if(ack >= baseA && ack < baseA+N)
  {
    printf("received ACK for all packets till %d\n",ack);
    printf("Stopping timer for [seq:%d]\n",baseA);
    stoptimer(0);
    baseA = ack+1;
    printf("Starting timer for [seq:%d]\n",baseA);
    starttimer(0,TIME_OUT);
    canSend();
  }
  else
  {
    printf("Duplicate or corrupted ack [ack:%d]: Ignore\n",ack);
    /*if(ack > 0 && ack<baseA)
      A_timerinterrupt();*/

   // eseqA = baseA;
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("timed out sending packets again from [seq:%d]\n",baseA);
  starttimer(0,TIME_OUT);
  firstA = baseA;
  eseqA = baseA;
  
  /*sendFive = true;
  for(eseqA = baseA ; eseqA < baseA+N && eseqA <endA;eseqA++)
   {
    char msg[21];
    strcpy(msg,buffer[firstA]);
    struct pkt packt;
    int csum = calculateChecksum(msg, 0, eseqA);
    packt = make_pkt(0,msg,eseqA,0,csum); // ack and checksum
    // printf("A sending in window after timeout [msg:%s] [seq:%d] [checksum:%d] [ack:0]\n",msg,eseqA,csum);
    tolayer3(0,packt);
    firstA++;
   }
   sendFive = false;*/
   canSend();
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  Asuccess =0;
  endA = 0;
  firstA = 0;
  eseqA = 0;
  baseA = 0;
  N = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  int seq = packet.seqnum;
  packet.payload[20] ='\0';
  char temp[21];
  strcpy(temp,packet.payload);
  temp[20] = '\0';
  int csum = calculateChecksum(packet.payload,packet.acknum,seq);
  if(csum != packet.checksum)
  {
    struct pkt sendpkt = make_pkt(1,'\0',0,expected_ackB-1,0);
    printf("B received [msg:%s] [seq:%d] [checksum:%d] [calculateChecksum:%d]\n",packet.payload,seq,packet.checksum,csum);
    printf("PAcket was corrupted\n.. sending last success [ack,%d]\n",expected_ackB-1);
    tolayer3(1,sendpkt);
  }
  else if(expected_ackB == seq)
  {
    printf("B received [msg:%s] [seq:%d] [checksum:%d] [packetCsum:%d] [ack:%d]\n sending to app\n",temp,seq,csum,packet.checksum,packet.acknum);
    tolayer5(1,temp);
    struct pkt sendpkt = make_pkt(1,'\0',0,expected_ackB,0);
    tolayer3(1,sendpkt);
    expected_ackB++;
  } 
  else
  {
    // if checksum is required
    struct pkt sendpkt = make_pkt(1,'\0',0,expected_ackB-1,0);
    printf("B received [seq:%d] didn't receive the expected packet\n.. sending last success [ack,%d]\n",seq,expected_ackB-1);
    tolayer3(1,sendpkt);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  expected_ackB = 0;
}
