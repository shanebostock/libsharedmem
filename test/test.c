#include "../src/shmemory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE 3

typedef struct packet {
	uint8_t done;
	uint8_t seq;
	char msgbuf[64];
} packet_s;



size_t get_size(){
    size_t mem_size = sizeof(packet_s) * BUFFERSIZE;
    return mem_size;
}

void generate_packet(packet_s *ptr, uint8_t seq){
    ptr->done = 0;
    ptr->seq = seq;
    sprintf(ptr->msgbuf,"Message %d.\n",seq);
}

uint8_t has_been_read(packet_s *ptr){

    packet_s msg;
    packet_s *m_ptr = &msg;
    memcpy(m_ptr,ptr,sizeof(packet_s));

    return msg.done;
    
}

int write_packets(){
    packet_s *pkt = get_shared_memory(get_size());
    packet_s *writer = pkt;
    uint8_t write_count = 0;  
    uint8_t buf_pos = 0;
    packet_s msg;
    packet_s *m_ptr = &msg;

    while(write_count < 3){
        generate_packet(m_ptr,write_count);    

        if (buf_pos >= BUFFERSIZE){
            buf_pos = 0;
            writer = pkt;
        }
        if(write_count < 3 || writer->done == 1){
            memcpy(writer,m_ptr,sizeof(packet_s));
            buf_pos++;
            writer++;
        }
        write_count++;
    }

    return write_count;
    
}

int read_packets(){
    packet_s *pkt = get_shared_memory(get_size());
    packet_s *reader = pkt;
    packet_s msg;
    packet_s *m_ptr = &msg;
    uint8_t read_count = 0;  
    uint8_t buf_pos = 0;
    

    while (read_count < 3){
        if (buf_pos >= BUFFERSIZE) {
            buf_pos = 0;
            reader = pkt;
        }
        if(reader->done == 0){
            memcpy(m_ptr,reader,sizeof(packet_s));
            reader->done = 1;
            reader++;
            buf_pos++;
        }

        if (reader->done == 0){

        }
        read_count++;
        /* Do something with the data */
        printf("done: %d, seq: %d, msg: %s",msg.done, msg.seq,msg.msgbuf); 
          
    }

    return read_count;
}

int main(int argc, char* argv){
    
    packet_s *pkt_write, *pkt_read;
    packet_s *pkt = create_shared_memory(get_size());
    pkt_write = pkt;

    int write_count = write_packets();
    printf("Write count: %d\n",write_count);
    int read_count = read_packets();
    printf("Read count: %d\n",read_count);
    
    return 0;
}