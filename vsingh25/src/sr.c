#include "../include/simulator.h"
#include <stdlib.h>
#include <stdio.h>
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

/* called from layer 5, passed the data to be sent to other side */

struct packetsW
{
  float startTime;
  float endTime;
  char mssg[21];
  int sequenceN;
  int ack;
};
struct packetsW pw[1501];
//struct packetsW *pw;
int N;
int currTimerSeq;

int count;
// float absTimeout = 18.0;
float absTimeout = 25.0;


char buffer[10000][21];
int endA, firstA, eseqA,baseA;
int msgCount=0;


int expected_ackB,window;
struct bufferB
{
  char mssg[21];
  int ack;
  int sequenceN;
};
struct bufferB  Bbuffer[1001];    // increase if it doesnt work :P


int calculateChecksum(char message[20], int acknum, int seq)
{
    int sum =0;
    for(int i=0;i<20;i++)
      sum += message[i];
    
    sum+=acknum;
    sum+=seq;
    return sum;
}

void pushBuffer(char msg[20])
{
  strcpy(buffer[endA++],msg);
  buffer[endA][20]='\0';
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
  /*if(count < N)
  {
    char msg[21];
    strcpy(msg,buffer[firstA++]);
    strcpy(pw[count].mssg,message.data);
    pw[count].startTime = get_sim_time();
    pw[count].endTime = get_sim_time() + absTimeout;
    if(count == 0)
      starttimer(0,absTimeout);
    count++;
    struct pkt packet = make_pkt(0, message.data, 0, 0, 0);
    tolayer3(0,packet);
    //sortList();
  }*/
  if(eseqA < baseA+N && eseqA < endA)
  {
    char msg[21];
   // strcpy(msg,buffer[eseqA]);
    strcpy(msg,pw[eseqA].mssg);
    struct pkt packt;
    int csum = calculateChecksum(msg, 0, eseqA);
    packt = make_pkt(0,msg,eseqA,0,csum); // ack and checksum
    printf("A sent [msg:%s] [seq:%d] [checksum: %d ] [ack:0]\n",msg,eseqA,csum);
    //strcpy(pw[eseqA].mssg,msg);
    pw[eseqA].startTime = get_sim_time();
    pw[eseqA].endTime = get_sim_time() + absTimeout;
    pw[eseqA].sequenceN = eseqA;
    pw[eseqA].ack = 0;
    if(baseA== eseqA)  // 1 check this
    {
   //   printf("Windowed moved\n");
      starttimer(0,absTimeout);   //check this
      currTimerSeq = eseqA;
    }
    /*
    if(count == 0)  
    {
      starttimer(0,absTimeout);
      currTimerSeq = eseqA;
    }*/
    eseqA++;
    tolayer3(0,packt);
  }
}


void A_output(message)
  struct msg message;
{ 
  strcpy(pw[endA].mssg,message.data);
  pw[endA].mssg[20] ='\0';
  endA++;
  printf("A got message from app [msg:%s]\n", message.data);
  //pushBuffer(message.data);
  canSend();
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
  float t = get_sim_time();
  int csum = calculateChecksum("a", packet.acknum, packet.seqnum);
  int ack = packet.acknum;
  bool changeTimer = false;

  if(csum != packet.checksum)
  {
    printf("###WRONG [Rchecksum:%d] [Pchecksum:%d]",csum,packet.checksum);
  }
  else if(ack >=baseA && ack <baseA+N && pw[ack].ack !=1)
  {
    if(ack == baseA)
    {
      changeTimer =true;
     printf("RECEIVED ACK for %d base packet\n",baseA);
      pw[ack].ack = 1;
      for(int j = ack; j< baseA+N && pw[j].ack ==1;j++)
        baseA++;
    }
    else if(ack > baseA)
    {
      changeTimer = true;
      printf("received ack for non base packet [seq:%d] \n",ack);
      pw[ack].ack = 1;
    }
    if( ack == currTimerSeq)
    {
      stoptimer(0);
      int min = 99999999;
      bool isFound = false;
      int index;
      /*for(int i = baseA;i< baseA+N;i++) */  // for 500 values would this be effective?
      for(int i = baseA;i< baseA+N;i++) 
      {                                   // Also check if there are no values present? then what?
        if(pw[i].ack == 0)
        if(pw[i].endTime < min && pw[i].ack == 0)
        {
          min = pw[i].endTime;
          isFound = true;
          index = i;
        }
      }  
      if(isFound)
      {
          currTimerSeq = index;
          starttimer(0,pw[currTimerSeq].endTime - t);
      }
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  int min = 99999999;
  bool isFound = false;
  float t = get_sim_time();

  for(int i = baseA;i< baseA+N;i++)   // for 500 values would this be effective?
  {                                   // Also check if there are no values present? then what?
      if(pw[i].endTime < min && pw[i].ack == 0)
      {
        min = pw[i].endTime;
        currTimerSeq = i;
        isFound = true;
      }
  }  
  if(isFound)
  {   
      float diff = pw[currTimerSeq].endTime - t;
     /* if(diff < 0)
      {
        printf("assigning new timer valuefor [seq:%d] : %f",currTimerSeq, get_sim_time()-get_sim_time());
      printf("previous [endTime:%f] [now:%f]",pw[currTimerSeq].endTime,get_sim_time());
      starttimer(0, 2);
      }  
      else*/
      {
        printf("assigning new timer valuefor [seq:%d] : %f",currTimerSeq,pw[currTimerSeq].endTime - t);
        printf("previous [endTime:%f] [now:%f]",pw[currTimerSeq].endTime,t);
        starttimer(0,pw[currTimerSeq].endTime - t);
      }
      
  }


  //adjusting the timer settings for the next try 
  printf("ack didn't arrive in time. Sending packet again\n");
  char msg[21];
  strcpy(msg,pw[currTimerSeq].mssg);
  int csum = calculateChecksum(msg, 0, currTimerSeq);
  struct pkt packt = make_pkt(0,msg,currTimerSeq,0,csum); // ack and checksum
  pw[currTimerSeq].startTime = t;
  pw[currTimerSeq].endTime = t + absTimeout;  
  printf("Sent [msg:%s] [seq:%d] [checksum: %d ] [ack:0]\n",msg,currTimerSeq,csum);
  printf("New TIMES [startTime:%f] [endTime:%f]\n",pw[currTimerSeq].startTime,pw[currTimerSeq].endTime);
  tolayer3(0,packt);

  // starting the relative timer for the next packet close to rtt
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    N = getwinsize();
    //pw = (struct packetsW*) malloc(sizeof(*pw) * N);
    count = 0;
    endA = 0;
    firstA = 0;
    eseqA = 0;
    baseA = 0;
    for(int i=0;i<1501;i++)
    { 
      pw[i].ack =-999;
      pw[i].sequenceN = i;
    }
    int currTimerSeq = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
  //printf("B recieved packet containg msg %s \n",packet.payload); 
  int seq = packet.seqnum;
  packet.payload[20] ='\0';
  printf("B received from network layer [msg:%s][seq:%d][ack:%d][checksum:%d]\n",packet.payload,packet.seqnum,packet.acknum,packet.checksum);
  int csum = calculateChecksum(packet.payload,packet.acknum,seq);
  if(csum != packet.checksum)
  {
    int acum = calculateChecksum("a",expected_ackB-1,0);
    struct pkt sendpkt = make_pkt(1,'\0',0,expected_ackB-1,acum);
    printf("B received [msg:%s] [seq:%d] [checksum:%d] [calculateChecksum:%d]\n",packet.payload,seq,packet.checksum,csum);
    printf("PAcket was corrupted\n.. sending last success [ack,%d]\n",expected_ackB-1);
    tolayer3(1,sendpkt);
  }
  else if(expected_ackB == seq  )  /// ***check for higher seq which have already been updated and send them to app + inccrese expected sequence number
  {
    char temp[21];
    strcpy(temp,packet.payload);
    temp[20] = '\0';
    // sending packet to app, increasing expected base
    printf("B received [msg:%s] [seq:%d] [checksum:%d] [packetCsum:%d] [ack:%d]\n",temp,seq,csum,packet.checksum,packet.acknum);
    tolayer5(1,temp);
    int acum = calculateChecksum("a",expected_ackB,0);
    struct pkt sendpkt = make_pkt(1,'\0',0,expected_ackB,acum);
    printf("B sending [ack:%d] \n",expected_ackB);
    expected_ackB++;
    // if there are "next" packets in the buffer who have been ack then sending them to app and increasing expected base 
    for(int i =seq+1; i < 1000; i++)
    {
      if(Bbuffer[i].ack == 1)
      {
        printf("packet was already ack, send to app, remove from buffer [msg:%s][seq:%d]\n", Bbuffer[i].mssg,i);
        tolayer5(1,Bbuffer[i].mssg); 
        expected_ackB++;   
      }
      else
        break;
    }
    // sending ack for current packet
    tolayer3(1,sendpkt); 
  }
  else if(seq < expected_ackB)
  {
    int acum = calculateChecksum("a",seq,0);
    struct pkt sendpkt = make_pkt(1,'\0',0,seq,acum);
    printf("B received duplicates.. sending [ack:%d] \n",seq);
    tolayer3(1,sendpkt);
  }
  else if(seq > expected_ackB)
  {
    printf("B received a higher packet then the base packet\n");
    Bbuffer[seq].ack = 1;
    strcpy(Bbuffer[seq].mssg,packet.payload);
    int acum = calculateChecksum("a",seq,0);
    struct pkt sendpkt = make_pkt(1,"a",0,seq,acum);
    printf("B sending [ack:%d] \n",seq);
    tolayer3(1,sendpkt); 

  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  expected_ackB = 0;
  window =  getwinsize();
  for(int i = 0;i<1000;i++)
  {
    Bbuffer[i].sequenceN = i;   // no point of the field sequence number
    Bbuffer[i].ack = -999;
    //strcpy(Bbuffer.mssg);
  }
}
