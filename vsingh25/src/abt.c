#include "../include/simulator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

int endA;
int firstA;
int seqNo;
static int canSend;
char *buffer[10000];
struct pkt currentPacket;
int checksum;


int B_COUNT;
int B_acknum;
int TIMEOUT = 9;

/* called from layer 5, passed the data to be sent to other side */

//implement make_pkt()
//udt_send()
//start_time

void udt_send(int n, struct pkt packet)
{
  tolayer3(n,packet);
}

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
    //    printf("--> For %c Sending packet details : [seqNo:%d] [mssg:%s]\n",AB,seqnum, message);
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
    //printf("--> For %c Sending packet details : [seqNo:%d] [mssg:%s]\n",AB,seqnum, send_pkt.payload );
    return send_pkt;
}

int getNext(int num)
{
  if(num == 0)
    return 1;
  else 
    return 0;
}

void A_output(message)
  struct msg message;
{
    //printf("\nA got message from application layer : %s\n",message.data);
    printf("GATEWAY TO SEND [canSend:%d]\n",canSend);
    if(canSend == 1)
    {
        canSend = 0;
        printf("\n");
        checksum = calculateChecksum(message.data,0,seqNo);
        struct pkt sendpkt = make_pkt(0,message.data, seqNo, 0, checksum);
        currentPacket = sendpkt;
        starttimer(0, TIMEOUT);
        printf("TIMER STARTED & A sent : %s [seqNo:%d] [checksum:%d]\n",message.data,seqNo,checksum);
        udt_send(0,sendpkt);
    }
    else
    {
       // printf("A is %d and message is %s\n",endA,message.data);
        endA++;
        buffer[endA] = malloc(sizeof(char) *21);
        memset(buffer[endA],'\0',21);
        strcpy(buffer[endA],message.data);
        buffer[endA][20]='\0';
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)  // to check if ack seqno mistmatch possible here
  struct pkt packet;
{
  printf("A [seq:%d] received [ack:%d] \n",seqNo,packet.acknum);
  // check if buffer is empty & then go to the bogus queue buffer
  if(seqNo ==  packet.acknum)
  {       
    stoptimer(0);
    printf("Timer Stopped succes [msg:%s]\n",currentPacket.payload);
    seqNo = getNext(seqNo);
    if(endA >= firstA)
    {
      struct msg send;
      strcpy(send.data,buffer[firstA]);
      send.data[20] ='\0';
      printf("Buffered data available- buffer:%s \n",buffer[firstA]);
      canSend =1;
      A_output(send);
      firstA++;
    }
    else
      canSend = 1;
  }
  else
   {
      // do nothing , for now atleast :P
   } 
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  printf("## Timed out & Timer started  ## [msg:%s] [ack:%d]\n",currentPacket.payload,currentPacket.acknum); 
  starttimer(0, TIMEOUT);
  canSend = 0;
  udt_send(0,currentPacket);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  endA = -1;
  firstA =0;
  seqNo = 0;
  canSend = 1;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  //B_COUNT++;
  printf("B received [msg:%s] [seq:%d]\n",packet.payload,packet.seqnum);
  printf("canSend %d\n", canSend);
  if(packet.seqnum == B_acknum)
  {
    char mssg[20];
    packet.payload[20] = '\0';
    strcpy(mssg,packet.payload);
    mssg[20] = '\0';
    int csum = calculateChecksum(mssg,packet.acknum,packet.seqnum);
    printf("$$$ [s checksum:%d]  [r checksum:%d]\n",packet.checksum,csum);

    if(csum == packet.checksum)
    {
      tolayer5(1,mssg);
      printf("B received the packet with [seq:%d] [mssg:%s] and sending [ack:%d] \n",packet.seqnum,mssg,B_acknum);
      struct pkt ack = make_pkt(1,NULL,0,B_acknum,0);
      B_acknum = getNext(B_acknum);
      tolayer3(1,ack);
    }
    else
    {
      printf("Packed was corrupted\n");
      struct pkt ack = make_pkt(1,NULL,0,getNext(packet.seqnum),0);
      tolayer3(1,ack);
    }
  }
  else
  {
    struct pkt ack = make_pkt(1,NULL,0,packet.seqnum,0);
    printf("B seqno and ack did not match, mostly a duplicate\n");
    tolayer3(1,ack);
  }

}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  B_acknum = 0;
}
