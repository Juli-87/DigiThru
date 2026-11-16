/* SPDX-License-Identifier: GPL-2.0-or-later */
/* host test: stubs the firmware, runs DigiThru's handlers */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
static int act=9, autoch=13, rxn=1, chans[8]={0,1,2,3,4,5,6,7};
static unsigned char out[64][3]; static int outlen[64], nout;
static void seq_send(int len,const unsigned char*b){memcpy(out[nout],b,len);outlen[nout++]=len;}
static int mchan(int mt){return chans[mt];}
#define SEQ_SEND_AT seq_send
#define MTRK_CHAN_AT mchan
#define ACTIVE_TRACK_AT &act
#define AUTO_CHAN_AT &autoch
#define RX_NOTES_AT &rxn
#define int32_t int
#define uint32_t unsigned
#define uint8_t unsigned char
#define typedef_skip
#undef int32_t
#undef uint32_t
#undef uint8_t
#include "dt_host.c"
static int fails;
#define CHECK(c,m) do{ if(!(c)){printf("FAIL %s\n",m);fails++;} else printf("ok   %s\n",m);}while(0)
int main(void){
  unsigned char pb[3]={0xE0|13,0x10,0x50}, cp[2]={0xD0|13,99}, pa[3]={0xA0|13,60,77};
  dt_pitchbend(pb,3,16); CHECK(nout==1&&out[0][0]==0xE1&&out[0][1]==0x10&&out[0][2]==0x50,"PB to track 10 (ch 2)");
  dt_chanpress(cp,2,16); CHECK(nout==2&&outlen[1]==2&&out[1][0]==0xD1&&out[1][1]==99,"AT to ch 2");
  dt_polypress(pa,3,16); CHECK(nout==3&&out[2][0]==0xA1&&out[2][1]==60,"poly AT");
  CHECK(dt_cc(9,1,42,1)==1&&nout==4&&out[3][0]==0xB1&&out[3][1]==1&&out[3][2]==42,"MW");
  CHECK(dt_cc(9,74,42,1)==0&&nout==4,"other CC untouched");
  CHECK(dt_cc(9,1,42,0)==0,"MW on track channel untouched");
  CHECK(dt_cc(3,1,42,1)==0,"MW audio track untouched");
  dt_tick(); CHECK(nout==4,"no reset while same track");
  act=11; dt_tick(); CHECK(nout==6&&out[4][0]==0xE1&&out[4][1]==0&&out[4][2]==64&&out[5][0]==0xD1&&out[5][1]==0,"reset on track change");
  dt_tick(); CHECK(nout==6,"reset once");
  dt_pitchbend(pb,3,16); CHECK(out[6][0]==0xE3,"follows new track");
  unsigned char c0[3]={0xE0|13,0,64}; dt_pitchbend(c0,3,16); act=12; dt_tick(); CHECK(nout==8,"no reset when centred");
  act=4; int n=nout; dt_pitchbend(pb,3,16); CHECK(nout==n,"audio track: nothing");
  act=9; chans[1]=-1; dt_pitchbend(pb,3,16); CHECK(nout==n,"CHAN off: nothing"); chans[1]=1;
  unsigned char o[3]={0xE0|5,1,2}; dt_pitchbend(o,3,8); CHECK(nout==n,"other channel: nothing");
  rxn=0; dt_pitchbend(pb,3,16); CHECK(nout==n,"receive notes off: nothing"); rxn=1;
  unsigned char hi[3]={0xED,0xff,0xff}; dt_pitchbend(hi,3,16); CHECK(out[n][1]==127&&out[n][2]==127,"data bytes masked");
  printf(fails?"%d FAILED\n":"all passed\n",fails); return fails;
}
