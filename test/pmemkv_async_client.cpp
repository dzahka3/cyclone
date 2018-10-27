
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <rte_launch.h>

#include "../core/logging.hpp"
#include "../core/clock.hpp"
#include "../core/libcyclone.hpp"
#include "pmemkv.hpp"


unsigned long tx_block_cnt;
unsigned long total_latency;
unsigned long tx_begin_time;


int driver(void *arg);


/* callback context */
typedef struct cb_st
{
	unsigned long request_id;
} cb_t;


void async_callback(void *args, int code, unsigned long msg_latency)
{
	if(code == REP_SUCCESS){
		tx_block_cnt++;
	}
	total_latency += msg_latency;	
	if (tx_block_cnt > 5000)
	{
		unsigned long total_elapsed_time = (rtc_clock::current_time() - tx_begin_time);
		BOOST_LOG_TRIVIAL(info) << "LOAD = "
		                        << ((double)1000000 * tx_block_cnt) / total_elapsed_time
		                        << " tx/sec "
		                        << "LATENCY = "
		                        << ((double)total_latency) / tx_block_cnt
		                        << " us ";
		tx_block_cnt   = 0;
		total_latency  = 0;
		tx_begin_time = rtc_clock::current_time();
	}
}

typedef struct driver_args_st
{
	int leader;
	int me;
	int mc;
	int replicas;
	int clients;
	int partitions;
	void **handles;
	void operator() ()
	{
		(void)driver((void *)this);
	}
} driver_args_t;

int driver(void *arg)
{
	driver_args_t *dargs = (driver_args_t *)arg;
	int me = dargs->me;
	int mc = dargs->mc;
	int replicas = dargs->replicas;
	int clients = dargs->clients;
	int partitions = dargs->partitions;
	void **handles = dargs->handles;
	char *buffer = new char[DISP_MAX_MSGSIZE];
	//struct proposal *prop = (struct proposal *)buffer;
	srand(time(NULL));
	int ret;
	int rpc_flags;
	int my_core;

	pmemkv_t *kv = (pmemkv_t *)buffer;

	double frac_read = 0.5;
	const char *frac_read_env = getenv("KV_FRAC_READ");
	if (frac_read_env != NULL)
	{
		frac_read = atof(frac_read_env);
	}
	BOOST_LOG_TRIVIAL(info) << "FRAC_READ = " << frac_read;


	unsigned long keys = pmemkv_keys;
	const char *keys_env = getenv("KV_KEYS");
	if (keys_env != NULL)
	{
		keys = atol(keys_env);
	}
	BOOST_LOG_TRIVIAL(info) << "KEYS = " << keys;

	srand(rtc_clock::current_time());
	for( ; ; ){
		double coin = ((double)rand()) / RAND_MAX;

		if (coin > frac_read)
		{
			rpc_flags = 0;
			kv->op    = OP_PUT;
		}
		else
		{
			rpc_flags = RPC_FLAG_RO;
			kv->op    = OP_GET;
		}

		kv->key   = rand() % keys;
		my_core = kv->key % executor_threads;
		do{
			ret = make_rpc_async(handles[0],
		              buffer,
		              sizeof(pmemkv_t),
									async_callback,
		              (void *)NULL,
		              1UL << my_core,
		              rpc_flags);
			if(ret == EMAX_INFLIGHT){
				continue;
			}
		}while(!ret);
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	if (argc != 10)
	{
		printf("Usage: %s client_id_start client_id_stop mc replicas clients partitions cluster_config quorum_config_prefix server_ports inflight_cap\n", argv[0]);
		exit(-1);
	}

	int client_id_start = atoi(argv[1]);
	int client_id_stop  = atoi(argv[2]);
	driver_args_t *dargs;
	void **prev_handles;
	cyclone_network_init(argv[7], 1, atoi(argv[3]), 2 + client_id_stop - client_id_start);
	driver_args_t ** dargs_array =
	    (driver_args_t **)malloc((client_id_stop - client_id_start) * sizeof(driver_args_t *));
	for (int me = client_id_start; me < client_id_stop; me++)
	{
		dargs = (driver_args_t *) malloc(sizeof(driver_args_t));
		dargs_array[me - client_id_start] = dargs;
		if (me == client_id_start)
		{
			dargs->leader = 1;
		}
		else
		{
			dargs->leader = 0;
		}
		dargs->me = me;
		dargs->mc = atoi(argv[3]);
		dargs->replicas = atoi(argv[4]);
		dargs->clients  = atoi(argv[5]);
		dargs->partitions = atoi(argv[6]);
		dargs->handles = new void *[dargs->partitions];
		char fname_server[50];
		char fname_client[50];
		for (int i = 0; i < dargs->partitions; i++){
			sprintf(fname_server, "%s", argv[7]);
			sprintf(fname_client, "%s%d.ini", argv[8], i);
			dargs->handles[i] = cyclone_client_init(dargs->me,
			                                        dargs->mc,
			                                        1 + me - client_id_start,
			                                        fname_server,
			                                        atoi(argv[9]),
			                                        fname_client,
																							CLIENT_ASYNC,
																							atoi(argv[10]));
		}
	}
	for (int me = client_id_start; me < client_id_stop; me++){
		 cyclone_launch_clients(dargs_array[me-client_id_start]->handles[0],driver, dargs_array[me-client_id_start], 1+  me-client_id_start);	
	}
	rte_eal_mp_wait_lcore();
}
