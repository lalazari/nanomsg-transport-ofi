/**
 * NanoMsg libFabric Transport - Shared Functions
 * Copyright (c) 2015 Ioannis Charalampidis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define OFI_DEBUG_LOG
#include <hlapi.h>

//#define snd_data_size 1073741824
//#define snd_data_size 8000


//////////////////////////////////////////////////////////////////////////////////////////
// Entry Point
//////////////////////////////////////////////////////////////////////////////////////////

struct timespec t0, t1;
int iterations = 10000;

void help()
{
	printf("Usage:   fab server <node> [<service>]\n");
	printf("         fab client <node> [<service>]\n");
	printf("\n");
}

int64_t get_elapsed(const struct timespec *b, const struct timespec *a)
{
	int64_t elapsed;

	elapsed = (a->tv_sec - b->tv_sec) * 1000 * 1000 * 1000;
	elapsed += a->tv_nsec - b->tv_nsec;
	return elapsed / 1000;  // microseconds
}

int main(int argc, char ** argv)
{
	int ret;
	//int i;

	//printf("1.%s 2.%s 3.%s 4.%s", argv[1], argv[2], argv[3], argv[4] );
	long snd_data_size = strtol(argv[3], NULL, 0);
	long double_snd_data_size = strtol(argv[4], NULL, 0);
	printf("I/O Buffer size = %ld\n", snd_data_size);

	/* Validate arguments */
	if (argc < 3) {
		printf("ERROR: Too few arguments!\n");
		help();
		return 1;
	}

	/* Create and initialize OFI structure */
	struct ofi_resources ofi = {0};
	ofi_alloc( &ofi, FI_EP_RDM );

	/* Active endoint */
	struct ofi_active_endpoint ep = {0};

	/* Get port if specified */
	const char * node = argv[2]; 
	const char * service = "5125";
	//if (argc >= 4) service = argv[3];

	/* Allocate a data pointer */
	size_t msg_len;
	char * data = malloc( double_snd_data_size );
	if (data == NULL) {
		printf("ERROR: Unable to allocate memory!\n");
		return 255;
	}

	/* Setup OFI resources */
	if (!strcmp(argv[1], "server")) {

		printf("OFI: Listening on %s:%s\n", node, service );

		/* Initialize an RDM endpoint (bind there) */
		ret = ofi_init_connectionless( &ofi, &ep, FI_SOURCE, FI_SOCKADDR, node, service );
		if (ret)
			return ret;

		/* Manage this memory region */
		struct ofi_mr *mr;
		ofi_mr_alloc( &ep, &mr ); 
		ofi_mr_manage( &ep, mr, data, double_snd_data_size, 1, MR_SEND | MR_RECV );

		/* Receive data */
		printf("Receiving data...");

		clock_gettime(CLOCK_MONOTONIC, &t0);
		int64_t temp_time=0;
		uint32_t bRecv = 0;
		float sum_bandwith=0;
		float av_bandwith=0;

		//for (int i=0; i<10000; i++) {
		for (;;) {

			ret = ofi_rx_data( &ep, data, double_snd_data_size, fi_mr_desc( mr->mr ), &msg_len, -1 );
			
			if (ret) {
				printf("Error sending message!\n");
				return 1;
			}

			clock_gettime(CLOCK_MONOTONIC, &t1);

			bRecv += msg_len;
			
			temp_time = get_elapsed(&t0, &t1);
			if(temp_time >=1000000){
				printf("Bandwith %zu Mbps \n", bRecv/temp_time);
				clock_gettime(CLOCK_MONOTONIC, &t0);
				bRecv = 0;			
			}
			sum_bandwith = av_bandwith + bRecv/temp_time;
			//printf("'%s'\n", data );

		}	
		clock_gettime(CLOCK_MONOTONIC, &t1);
	    printf("time per message: %8.2f us\n", get_elapsed(&t0, &t1));

	    av_bandwith = sum_bandwith / 10000;
		printf("Average Bandwith = %.3f \n", av_bandwith);

	} else if (!strcmp(argv[1], "client")) {

		printf("OFI: Connecting to %s:%s\n", node, service );

		/* Initialize an RDM endpoint (connect there) */
		ret = ofi_init_connectionless( &ofi, &ep, 0, FI_SOCKADDR, node, service );
		if (ret)
			return ret;

		/* Add a remote address to the AV */
		ret = ofi_add_remote( &ofi, &ep, node, service );
		if (ret)
			return ret;

		/* Manage this memory region */
		struct ofi_mr *mr;
		ofi_mr_alloc( &ep, &mr ); 
		ofi_mr_manage( &ep, mr, data, snd_data_size, 1, MR_SEND | MR_RECV );

		/* Send data */
		printf("Sending data...");
		sprintf( data, "Hello World" );

		clock_gettime(CLOCK_MONOTONIC, &t0);

		//for (int i=0; i<10000; i++) {
		for (;;) {
			ret = ofi_tx_data( &ep, data, snd_data_size, fi_mr_desc( mr->mr ), 1 );
			if (ret) {
				printf("Error sending message!\n");
				printf("ret = %d \n", ret);
				return 1;
			}
		}	
		printf("OK\n");
		clock_gettime(CLOCK_MONOTONIC, &t1);
		//printf("'%s'i =  %d\n",data, i );
		//printf("time per message: %8.2f us\n", get_elapsed(&t0, &t1)/i/2.0);

	} else {
		printf("ERROR: Unknown action! Second argument must be 'server' or 'client'\n");
		help();
		return 1;

	}


	return 0;
}
