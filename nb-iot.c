#include <stdio.h>
#include <stdlib.h>
#include "pcg_basic.h"

#define simulation_time_ms ( 10 * 60 * 1000 ) //length of simulation time: 10min
#define slot_time_ms 5
#define simulation_time_slot ( simulation_time_ms / slot_time_ms )
long t;

#define N_sc 24 //number of subcarriers
#define M_max 200 //number of maximum user possible in this simulation
int M;//number of user

int P_pkt;//packet uplink probability in percent

//all simulation operations are in slot time unit
#define W_BO 10 //backoff window on first transmission/retransmission
#define D_ACK 2 
#define T_ACK 1 
#define N_PTmax 5 

//desired output: normalized throughput.
int nb_packet_ack[M_max+1];
int nb_packet_gen[M_max+1];

//desired output: average delay, in ms
long packet_delay_avg;
int packet_delay_stats[M_max+1]; //packet delay statistics per user

//desired output:resource efficiency
//no additional variable here

//additional statistics:
//average number of packet created per slot
//average number of packet sent (transmission/retransmission) per slot
//average number of collision per slot
//average empty subcarrier per slot

int user_status[M_max+1];//user status definition
int bo_counter[M_max+1];//backoff counter per user
int retry_counter[M_max+1];//retry counter per user
int ack_timer[M_max+1];//timer for waiting for ACK from subcarrier
int sc_ack_timer[N_sc];//timer for subcarrier to send ACK;
int user_sc[M_max+1];//subcarrier chosen per user
unsigned char user_packet_buf[M_max+1];//packet buffer, used when user need to resend packet
//IDLE: not having packet,  not waiting for backoff, or not waiting for ACK
#define IDLE 0
//BACKLOGGED: otherwise
#define BACKLOGGED 1

//subcarrier array that contains current number of user using them
int sc[N_sc];
unsigned char sc_buf[N_sc];
//packet content code
#define ACK 252
#define EMPTY 255

//global iteration counter index
int user_index;
int sc_index;

//debug flag
//#define DEBUG

//send/receive functions
void tx_packet(int sc_id, unsigned char packet){
	//check existing packet in subcarrier
	if (sc_buf[sc_id] == EMPTY) sc_buf[sc_id] = packet;
	//if subcarrier is not empty, collision occur.
	//destroy packet
	else{
		sc_buf[sc_id] = EMPTY;
		#ifdef DEBUG
		printf ("Collision occur on sc %d\n", sc_id);
		#endif
	}
}
unsigned char rx_packet (int sc_id) {
	unsigned char buf = sc_buf[sc_id];
	sc_buf[sc_id] = EMPTY;
	return buf;
}


int main (void) {
	//seed randomizer
	srand((unsigned) time(NULL));
	pcg32_random_t rng;
	pcg32_srandom_r(&rng,rand(), rand());
	
	printf ("Simulation start for %d timeslot:\n", simulation_time_slot);
	for (P_pkt = 10 ; P_pkt <= 30 ; P_pkt= P_pkt + 10) {
		printf ("P_pkt: %d%%\n", P_pkt);
		//stats header
		printf("Packet ACK, Packet generated,Nb.users,Throughput,avg.delay,efficiency\n");
		for (M=5;M<=M_max;M=M+5) {
			//printf ("M = %d: \n", M);
			nb_packet_ack[M] = 0;
			nb_packet_gen[M] = 0;
			packet_delay_stats[M] = 0;
			packet_delay_avg = -1;
			//initialize all user
			for (user_index=0; user_index<M; user_index++){
				user_status[user_index]=IDLE;
				bo_counter[user_index]=-1;
				retry_counter[user_index] = 0;
				ack_timer[user_index] = -1;
				user_sc[user_index] = -1;
				user_packet_buf[user_index]=EMPTY;
				packet_delay_stats[user_index] = 0;
			}
			#ifdef DEBUG
			printf ("User initialization done\n");
			#endif
			//initialize all subcarrier
			for (sc_index = 0; sc_index < N_sc;sc_index++){
				sc[sc_index]=0;
				sc_ack_timer[sc_index] = -1;
				sc_buf[sc_index] = EMPTY;
			}
			//empty all subcarrier
			for (sc_index=0; sc_index<N_sc; sc_index++) sc[sc_index]=0;
			//start simulation
			for (t=0; t<simulation_time_slot; t++)
			{
				//check random access slot, try to generate packet if idle
				for (user_index=0; user_index<M;user_index++){
					#ifdef DEBUG
					//printf ("User %d:\n", user_index);
					#endif
					if (user_status[user_index] == IDLE){
						if (pcg32_boundedrand_r(&rng,100)<P_pkt){
							bo_counter[user_index] = pcg32_boundedrand_r(&rng,W_BO);
							user_status[user_index] = BACKLOGGED;
							user_sc[user_index] = pcg32_boundedrand_r(&rng,N_sc);
							user_packet_buf[user_index] = pcg32_boundedrand_r(&rng,250);
							//statistics:
							nb_packet_gen[M]++;
							#ifdef DEBUG
							printf ("Packet generated on user %d\n", user_index);
							#endif
						}
					} else {
						//check for backoff counter and not waiting for ACK
						if (bo_counter[user_index] == 0) {
							//mark subcarrier for usage
							sc[user_sc[user_index]]++;
							tx_packet(user_sc[user_index], user_packet_buf[user_index]);
							ack_timer[user_index] = D_ACK;
							//reset backoff counter
							bo_counter[user_index] = -1;
							#ifdef DEBUG
							printf ("%d:Packet #%d sent from %d using SC %d.\n",
								t,
								nb_packet_gen[M],user_index,
								user_sc[user_index]);
								
							#endif
						} else if (bo_counter[user_index] > 0){
							#ifdef DEBUG
							printf ("Send packet from user %d after %d.\n", 
								user_index,
								bo_counter[user_index]);
							#endif
							bo_counter[user_index]--;
							if (ack_timer[user_index]>0) ack_timer[user_index]--;
							packet_delay_stats[user_index]++;
							
						}
					}
				}
				for (sc_index=0;sc_index<N_sc;sc_index++){
					if (sc[sc_index] == 1) {
						//send ACK after T_ACK
						if (sc_ack_timer[sc_index] == -1){
							//initialize sc ack timer
							sc_ack_timer[sc_index] = T_ACK;
							#ifdef DEBUG
							printf ("Received packet on sc %d\n", sc_index);
							printf ("Send ACK from sc %d after %d\n", 
								sc_index, sc_ack_timer[sc_index]);
							#endif
							sc_ack_timer[sc_index]--;
						} else if (sc_ack_timer[sc_index] == 0) {
							#ifdef DEBUG
							printf ("Sent ACK from sc %d\n", sc_index);
							#endif
							sc_buf[sc_index]=EMPTY;
							tx_packet (sc_index, ACK);
							//reset subcarrier ACK timer after sending ACK
							sc_ack_timer[sc_index] = -1;
						} else sc_ack_timer[sc_index]--;
					}
				}
				for (user_index=0; user_index<M;user_index++){
					if (rx_packet(user_sc[user_index]) == ACK) {
						#ifdef DEBUG
						printf ("Packet ACK'd for user %d on sc %d\n", 
							user_index, user_sc[user_index]);
						#endif
						//packet is successfully transmitted, clear buffer
						user_packet_buf[user_index] = EMPTY;
						//release subcarrier
						//since the subcarrier is not used by any other user
						//it is safe to set the subcarrier user to zero
						sc[user_sc[user_index]] = 0;
						//revert user status to idle
						user_status[user_index] = IDLE;
						//reset ACK timer
						ack_timer[user_index] = -1;
						//stats update
						nb_packet_ack[M]++;
						packet_delay_avg = (packet_delay_avg*nb_packet_ack[M]+
							(packet_delay_stats[user_index] + T_ACK) * 
							slot_time_ms)/(nb_packet_ack[M]+1);
						packet_delay_stats[user_index]=0;
					} else if (ack_timer[user_index]==0){
						if (retry_counter[user_index] == N_PTmax) {
							#ifdef DEBUG
							printf ("Max retry reached on user %d\n", user_index);
							#endif
							//drop the packet, clear buffer
							user_packet_buf[user_index]=EMPTY;
							//release the subcarrier
							sc[user_sc[user_index]]--;
							//revert user status to idle
							user_status[user_index] = IDLE;
							//reset user statistics
							packet_delay_stats[user_index] = 0;
						} else {
							
							//wait for randomized backoff counter
							bo_counter[user_index] = pcg32_boundedrand_r(&rng,W_BO);
							//increase user retry counter
							retry_counter[user_index]++;
							//statistics update
							packet_delay_stats[user_index] + D_ACK;
						}
					}
				}
			}//end of simulation
			#ifdef DEBUG
			printf ("End of simulation with %d timeslot\n", t);
			#endif
			//Throughput, avg.delay, efficiency
			
			//#ifdef DEBUG
			printf ("%d,%d, ", nb_packet_ack[M], nb_packet_gen[M]);
			//#endif
			printf ("%d,%f,%d,%f\n", 
				M,
				(float)nb_packet_ack[M]/(float)nb_packet_gen[M],
				packet_delay_avg,
				(float)nb_packet_ack[M]/(float)(N_sc * simulation_time_slot));
			
		}//end of iteration of maximum number of user M
	}//end of iteration of packet generation probability
}
