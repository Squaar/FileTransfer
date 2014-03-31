#ifndef SHARE_H
    #define SHARE_H

	void networkizeHeader(CS450Header *header){
        header->version = htonl(header->version);
        header->UIN = htonl(header->UIN);
        header->transactionNumber = htonl(header->transactionNumber);
        header->sequenceNumber = htonl(header->sequenceNumber);
        header->ackNumber = htonl(header->ackNumber);

        // header->from_IP = htonl(header->from_IP);
        // header->to_IP = htonl(header->to_IP);
        // header->trueFromIP = htonl(header->trueFromIP);
        // header->trueToIP = htonl(header->trueToIP);

        header->nbytes = htonl(header->nbytes);
        header->nTotalBytes = htonl(header->nTotalBytes);

        header->from_Port = htons(header->from_Port);
        header->to_Port = htons(header->to_Port);
        header->checksum = htons(header->checksum);
    }

    void deNetworkizeHeader(CS450Header *header){
        header->version = ntohl(header->version);
        header->UIN = ntohl(header->UIN);
        header->transactionNumber = ntohl(header->transactionNumber);
        header->sequenceNumber = ntohl(header->sequenceNumber);
        header->ackNumber = ntohl(header->ackNumber);

        // header->from_IP = ntohl(header->from_IP);
        // header->to_IP = ntohl(header->to_IP);
        // header->trueFromIP = ntohl(header->trueFromIP);
        // header->trueToIP = ntohl(header->trueToIP);
        
        header->nbytes = ntohl(header->nbytes);
        header->nTotalBytes = ntohl(header->nTotalBytes);

        header->from_Port = ntohs(header->from_Port);
        header->to_Port = ntohs(header->to_Port);
        header->checksum = ntohs(header->checksum);
    }

    //checksum entire packet with 0 for checksum, then put checksum in.
    //after putting the checksum in, calulating again should give 0xFFFF
    uint16_t calcChecksum(void *data, size_t len){
        uint16_t *data16 = (uint16_t *) data;
        uint8_t *data8 = (uint8_t *) data;
        uint32_t sum = 0;

        for(uint i=0; i<len/2; i++){
            sum += data16[i];
            if(sum > 0xFFFF)
                sum -= 0xFFFF;
        }
        if(len % 2 == 1){
            sum += data8[len-1];
            if(sum > 0xFFFF)
                sum -= 0xFFFF;
        }

        return ~sum;
    }

    void print16(uint16_t in){
        for (int i = 0; i < 16; i++) {
            printf("%d", (in & 0x8000) >> 15);
            in <<= 1;
        }
        printf("\n");
    }
    
#endif