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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hlapi.h>

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
	int i;
	/* Validate arguments */
	if (argc < 3) {
		printf("ERROR: Too few arguments!\n");
		help();
		return 1;
	}

	size_t msg_len;
	char * data = malloc( MAX_MSG_SIZE );
	if (data == NULL) {
		printf("ERROR: Unable to allocate memory!\n");
		return 255;
	}
	/* Create and initialize OFI structure */
	struct ofi_resources ofi = {0};
	ofi_alloc( &ofi, FI_EP_MSG );

	/* Get port if specified */
	const char * node = argv[2]; 
	const char * service = "5125";
	if (argc >= 4) service = argv[3];

	/* Setup OFI resources */
	if (!strcmp(argv[1], "server")) {

		/* Prepare endpoints */
		struct ofi_passive_endpoint pep = {0};
		struct ofi_active_endpoint ep = {0};

		printf("OFI: Listening on %s:%s\n", node, service );

		/* Create a server */
		ret = ofi_init_server( &ofi, &pep, FI_SOCKADDR, node, service );
		if (ret)
			return ret;

		/* Listen for connections */
		ret = ofi_server_accept( &ofi, &pep, &ep );
		if (ret)
			return ret;

		/* Setup memory region */
		//ret = ofi_active_ep_init_mr( &ofi, &ep, 1024, 1024 );
		if (ret)
			return ret;

		/* Receive data */
		struct ofi_mr *mr;
		ofi_mr_alloc( &ep, &mr ); 
		ofi_mr_manage( &ep, mr, data, MAX_MSG_SIZE, 1, MR_SEND | MR_RECV );
		

		/* Receive data */
		printf("Receiving data...");
		clock_gettime(CLOCK_MONOTONIC, &t0);
		int64_t temp_time=0;

		for (i=0; i<iterations; i++) {

			ret = ofi_rx_data( &ep, data, MAX_MSG_SIZE, fi_mr_desc( mr->mr ), &msg_len, -1 );
			if (ret) {
				printf("Error sending message!\n");
				return 1;
			}
 
			clock_gettime(CLOCK_MONOTONIC, &t1);

			temp_time = get_elapsed(&t0, &t1)/i/2.0;
			if(temp_time >=1000000){
				printf("Bandwith %f Mbps \n", (i*MAX_MSG_SIZE)/temp_time;


			}

			
			// start time measurement on first receive
			
		}	


		clock_gettime(CLOCK_MONOTONIC, &t1);
        printf("time per message: %8.2f us\n", get_elapsed(&t0, &t1)/i/2.0);

	} else if (!strcmp(argv[1], "client")) {

		/* Prepare endpoints */
		struct ofi_active_endpoint ep = {0};

		printf("OFI: Connecting to %s:%s\n", node, service );

		/* Create a client & connect */
		ret = ofi_init_client( &ofi, &ep, FI_SOCKADDR, node, service );
		if (ret)
			return ret;

		/* Setup memory region */

		//ret = ofi_active_ep_init_mr( &ofi, &ep, 1024, 1024 );
		//if (ret)
		//	return ret;
		
		struct ofi_mr *mr;
		ofi_mr_alloc( &ep, &mr ); 
		ofi_mr_manage( &ep, mr, data, MAX_MSG_SIZE, 1, MR_SEND | MR_RECV );


		/* Send data */

		printf("Sending data...");
		sprintf( data, "Hello World" );
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (i=0; i<iterations; i++) {
			ret = ofi_tx_data( &ep, data, 12, fi_mr_desc( mr->mr ), 1 );
			if (ret) {
				printf("Error sending message!\n");
				return 1;
			}
		}	
		printf("OK\n");

		clock_gettime(CLOCK_MONOTONIC, &t1);
		printf("'%s' %d\n",data, i );
		printf("time per message: %8.2f us\n", get_elapsed(&t0, &t1)/i/2.0);

		/* Receive data */
		/*printf("Receiving data...");
		ret = ofi_rx_data( &ep, data, MAX_MSG_SIZE, fi_mr_desc( mr->mr ), &msg_len, -1 );
		if (ret) {
			printf("Error sending message!\n");
			return 1;
		}
		printf("'%s'\n",data ); */

	} else {
		printf("ERROR: Unknown action! Second argument must be 'server' or 'client'\n");
		help();
		return 1;

	}

	return 0;
}
