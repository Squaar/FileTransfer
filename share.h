#ifndef SHARE_H
#define SHARE_H

	void networkizeHeader(CS450Header *header){
        header->version = htonl(6);
        header->UIN = htonl(675005893);
        header->transactionNumber = htonl(header->transactionNumber);
        header->sequenceNumber = htonl(header->sequenceNumber);
        header->ackNumber = htonl(header->ackNumber);

        header->from_IP = htonl(header->from_IP);
        header->to_IP = htonl(header->to_IP);
        header->trueFromIP = htonl(header->trueFromIP);
        header->trueToIP = htonl(header->trueToIP);
        header->nbytes = htonl(header->nbytes);
        header->nTotalBytes = htonl(header->nTotalBytes);

        header->from_Port = htons(header->from_Port);
        header->to_Port = htons(header->to_Port);
        header->checksum = htons(header->checksum);

        const char *ACCC = "mdumfo2";
        memcpy(header->ACCC, ACCC, strlen(ACCC));
    }

    void deNetworkizeHeader(CS450Header *header){
        header->version = ntohl(6);
        header->UIN = ntohl(675005893);
        header->transactionNumber = ntohl(header->transactionNumber);
        header->sequenceNumber = ntohl(header->sequenceNumber);
        header->ackNumber = ntohl(header->ackNumber);

        header->from_IP = ntohl(header->from_IP);
        header->to_IP = ntohl(header->to_IP);
        header->trueFromIP = ntohl(header->trueFromIP);
        header->trueToIP = ntohl(header->trueToIP);
        header->nbytes = ntohl(header->nbytes);
        header->nTotalBytes = ntohl(header->nTotalBytes);

        header->from_Port = ntohs(header->from_Port);
        header->to_Port = ntohs(header->to_Port);
        header->checksum = ntohs(header->checksum);
    }

    //checksum entire packet with 0 for checksum, then put checksum in.
    //after putting the checksum in, calulating again should give 0xFFFF
    uint16_t calcChecksum(void *data, size_t len){
    	uint16_t *ints = (uint16_t *) data;
    	uint32_t sum = 0;

    	for(uint i=0; i<len/sizeof(uint16_t); i++){
    		sum += ints[i];
    	}

        if(sum > 0xFFFF){
            sum += sum >> 16;
        }

    	return ~((uint16_t) sum & 0x0000FFFF);
    }

#endif