#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <endian.h>
#include <byteswap.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <assert.h>

#define MAX_POLL_CQ_TIMEOUT 2000
#define MSG	"SEND operation	"
#define RDMAMSGR	"RDMA read operation"
#define RDMAMSGW	"RDMA write opertion"
#define MSG_SIZE	64


#if _BYTE_ORDER == __LITLE_ENDIAN
static inline uint64_t htonll(uint64_t x) {return bswap_64(x);}
static inline uint64_t ntohll(uint64_t x) {return bswap_64(x);}
#elif _BYTE_ORDER == __BIG_ENDIAN
static inline uint64_t htonll(uint64_t x) {return x;}
static inline uint64_t ntohll(uint64_t x) {return x;}
#else
#error __BYTE_ORDER is neither __LITTLE_ENDIAN nor __BIG_ENDIAN
#endif


struct config_t{

	const char	*dev_name;
	char		*server_name;
	u_int32_t	tcp_port;
	int		ib_port;
	int		gid_idx;
};

struct cm_con_data_t{

	uint64_t	addr;
	uint32_t	rkey;	/*Remote key*/
	uint32_t	qp_num;
	uint16_t	lid;
	uint8_t		gid[16];
};


struct resources{

	struct ibv_device_attr	device_attr;
	struct ibv_port_attr	port_attr;
	struct cm_con_data_t	remote_props;
	struct ibv_context	*ib_ctx;
	struct ibv_pd		*pd;
	struct ibv_cq		*cq;
	struct ibv_qp		*qp;
	struct ibv_mr		*mr;
	char			*buf;

	int 			sock;
};


struct config_t config = {
	NULL,
	NULL,
	19875,
	1,
	-1
};


/************************************************************
 * Socket operations
 **********************************************************/
static int sock_connect(const char *servername, int port){

	struct sockaddr_in 	addr;
	int			sockfd=-1;
	int			listenfd = 0;
	int			tmp ;

	if( servername ){

		sockfd = socket(PF_INET, SOCK_STREAM, 0);

		memset(&addr, 0 ,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(servername);

		if( (tmp = connect(sockfd,(struct sockaddr *)&addr, sizeof(addr))) < 0  ){
			perror("connect failed");
			close(sockfd);
			sockfd = -1;
		}
	}
	else{
		/*Server mode*/
		listenfd = socket(PF_INET, SOCK_STREAM, 0);
		assert(listenfd > 0);
		sockfd = -1;
	
		memset(&addr, 0 ,sizeof(addr));	
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY;
		
		if( bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) ) {
			perror("bind");
		}
		if( listen(listenfd, 1) ) {
			perror("listen");
		}
		sockfd = accept(listenfd, NULL, 0);
		if( sockfd > 0 ){
			printf("accept connection with fd: %d\n", sockfd);	
		}
	}

	close(listenfd);

	return sockfd;
}

int sock_sync_data(int sock , int xfer_size, char *local_data, char *remote_data){

	int 	rc;
	int 	read_bytes=0;
	int 	total_read_bytes = 0;

	rc = write(sock, local_data, xfer_size);
	assert(rc == xfer_size);
	if( rc < xfer_size)
		fprintf(stderr, "Failed writing data buring sock_sync_data\n");
	else 
		rc = 0;

	while(!rc && total_read_bytes < xfer_size){
		read_bytes = read(sock, remote_data, xfer_size);
		if( read_bytes > 0 )
			total_read_bytes += read_bytes;
		else 
			rc = read_bytes;	
	}

	return rc;
}

static int poll_completion(struct resources *res){

	struct ibv_wc 	wc;
	unsigned long 	start_time_msec;
	unsigned long	cur_time_msec;
	struct timeval 	cur_time;
	int		poll_result;
	int		rc = 0;


	gettimeofday(&cur_time, NULL);

	start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec/1000);

	do{
	
		poll_result = ibv_poll_cq(res->cq, 1, &wc);
		gettimeofday(&cur_time, NULL);
		cur_time_msec = (cur_time.tv_sec * 1000 ) + (cur_time.tv_usec/1000);
	}while((poll_result == 0) && ((cur_time_msec - start_time_msec)< MAX_POLL_CQ_TIMEOUT));
	if( poll_result < 0){

		fprintf(stderr, "poll CQ failed\n");
		rc = 1;
	}else if(poll_result == 0 ){
		fprintf(stderr, "Completion wasn't found in the CQ after timeout\n");
		rc = 1;
	}else {

		printf("completion was found in CQ with status 0x%x\n", wc.status);

		if( wc.status != IBV_WC_SUCCESS ) {
			fprintf(stderr, "got bad completion with status: 0x%x, vendor syndrome: 0x%x\n", wc.status, wc.vendor_err);
			rc = 1;
		}

	}
	return rc;
}

static int post_send(struct resources *res, int opcode){

	struct ibv_send_wr	sr;
	struct ibv_sge		sge;
	struct ibv_send_wr	*bad_wr = NULL;
	int 			rc;


	memset(&sge, 0, sizeof(sge));

	sge.addr = (uintptr_t) res->buf;
	sge.length = MSG_SIZE;
	sge.lkey = res->mr->lkey;

	memset(&sr, 0 ,sizeof(sr));

	sr.next = NULL;
	sr.wr_id = 0;
	sr.sg_list = &sge;
	sr.num_sge = 1;
	sr.opcode = opcode;
	sr.send_flags = IBV_SEND_SIGNALED;

	/*
	 * send request need local address and remote addr
	 * for 'send mode', we do not need remote addr
	 *
	 * modes: send, rdma_read, rdma_write, rdma_atomic_write
	 *
	 * for rdma read, how to specify the length of string
	 */
	if( opcode != IBV_WR_SEND ){
		sr.wr.rdma.remote_addr = res->remote_props.addr;
		sr.wr.rdma.rkey = res->remote_props.rkey;
	}
	
	rc = ibv_post_send(res->qp, &sr, &bad_wr);

	if( rc != 0 ) fprintf(stderr, "failed to post SR\n");
	else printf("send request posted opcode %d\n", opcode);
	return rc;
}

/****************************
 *question:
 * what is the behavior of post receive?
 *
 */
static int post_recv(struct resources *res){

	struct ibv_recv_wr 	rr;
	struct ibv_sge 		sge;
	struct ibv_recv_wr	*bad_wr;
	int 			rc;

	memset(&sge, 0 ,sizeof(sge));
	sge.addr = (uintptr_t) res->buf;
	sge.length = MSG_SIZE;
	sge.lkey = res->mr->lkey;

	memset(&rr, 0 , sizeof(rr));

	rr.next = NULL;
	rr.wr_id = 0;
	rr.sg_list = &sge;
	rr.num_sge = 1;

	rc = ibv_post_recv(res->qp, &rr, &bad_wr);

	if( rc != 0 ) fprintf(stderr, "failed to post RR\n");
	else printf("receive request posted\n");
	return rc;
}

static void resources_init(struct resources *res){
	memset(res, 0 ,sizeof(*res));
	res->sock = -1;
}

static int resources_create(struct resources *res){

	struct ibv_device **dev_list = NULL;
	struct ibv_qp_init_attr qp_init_attr;
	struct ibv_device *ib_dev = NULL;
	int rc = 0;
	int num_devices;
	int i;
	int cq_size = 0;
	int size;
	int mr_flags = 0;
	if( config.server_name != NULL){
	//client mode	
		res->sock = sock_connect(config.server_name, config.tcp_port);
		if( res->sock < 0 ){
		       	fprintf(stderr, "failed to establised TCP connection to server\n");
			rc = -1;
			goto resources_create_exit;
		}
	}else {
	//server mode
		printf("waiting on port %d for tcp connection \n", config.tcp_port);
		
		res->sock = sock_connect(NULL, config.tcp_port);
		if( res->sock< 0 ){
			fprintf(stderr, "failed to establise tcp connection with client\n");
			rc = -1;
			goto resources_create_exit;
		}
	}

	printf("TCP connection was established\n");

	printf("Searching for IB devices in host\n");

	dev_list = ibv_get_device_list(&num_devices);
	if( dev_list == NULL || num_devices == 0 ){
		fprintf(stderr, "failed to get IB devices list\n");
		rc = 1;
		goto resources_create_exit;
	}

	fprintf(stdout, "found %d devices\n", num_devices);

	for(i=0;i<num_devices;++i){
		
		if(!config.dev_name){
			config.dev_name = strdup(ibv_get_device_name(dev_list[i]));
			printf("device not specified, using first one found: %s\n", config.dev_name);
		}
		if( 0 == strcmp(ibv_get_device_name(dev_list[i]), config.dev_name)) {
			ib_dev = dev_list[i];
			break;
		}
	}

	if( 0 == ib_dev ){
		fprintf(stderr, "IB device %s wasn't found\n", config.dev_name);
		rc = 1;
		goto resources_create_exit;
	}

	res->ib_ctx = ibv_open_device(ib_dev);
	if( res->ib_ctx == NULL ){

		fprintf(stderr, "failed to open device %s\n", config.dev_name);
		rc = 1;
		goto resources_create_exit;
	}

	ibv_free_device_list(dev_list);
	dev_list = NULL;
	ib_dev = NULL;

	if( ibv_query_port(res->ib_ctx, config.ib_port, &res->port_attr)!= 0 ){
		
		fprintf(stderr, "ibv_query_port on port %u failed\n", config.ib_port);
		rc = 1;
		goto resources_create_exit;
	}

	res->pd = ibv_alloc_pd(res->ib_ctx);
	if( NULL == res->pd ){

		fprintf(stderr, "ibv_alloc_pd failed\n");
		rc = 1;
		goto resources_create_exit;
	}

	cq_size = 1;
	res->cq = ibv_create_cq(res->ib_ctx, cq_size, NULL, NULL, 0);
	if( !res->cq ){
		fprintf(stderr, "failed to create CQ with %u entries\n", cq_size);
		rc = 1;
		goto resources_create_exit;
	}

	size = MSG_SIZE;
	res->buf = (char *)malloc(size);

	if( res->buf == NULL ){

		fprintf(stderr, "failed to malloc %d bytes to memory buffer\n",size);
		rc = 1;
		goto resources_create_exit;
	}

	memset(res->buf, 0 ,size);

	if( !config.server_name ){
		strcpy(res->buf, MSG);
		strcpy( res->buf, MSG);
		printf("Server: going to send the message: %s\n", res->buf);
	}else 
		memset(res->buf, 0 ,size);


	/*register the memory buffer*/

	mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

	res->mr = ibv_reg_mr(res->pd, res->buf, size, mr_flags);
	if( !res->mr ){

		fprintf(stderr, "ibv_reg_mr failed with mr_flags = 0x%x\n", mr_flags);
		rc = 1;
		goto resources_create_exit;
	}

	fprintf(stdout, "MR was registerd with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n", res->buf, res->mr->lkey, res->mr->rkey, mr_flags);


	memset(&qp_init_attr, 0, sizeof( qp_init_attr ));


	qp_init_attr.qp_type	 = 	IBV_QPT_RC;	// reliable connection
	qp_init_attr.sq_sig_all	 =	1;
	qp_init_attr.send_cq	 = 	res->cq;	// send completion queue
	qp_init_attr.recv_cq	 =	res->cq;	// recv completion queue
	qp_init_attr.cap.max_send_wr	=	1;
	qp_init_attr.cap.max_recv_wr	=	1;
	qp_init_attr.cap.max_send_sge	=	1;
	qp_init_attr.cap.max_recv_sge	= 	1;

	res->qp	=	ibv_create_qp(res->pd, &qp_init_attr);
	if( !res->qp ){
		fprintf(stderr, "failed to create QP\n");
		rc = 1;
		goto resources_create_exit;
	}

	fprintf(stdout, "QP was created, QP number = 0x%x\n", res->qp->qp_num);

resources_create_exit:

	if( rc ){
		
		if( res->qp ) {
			ibv_destroy_qp(res->qp);
			res->qp = NULL;
		}

		if( res->mr ){
			ibv_dereg_mr(res->mr);
			res->mr = NULL;
		}

		if( res->buf ){
			free(res->buf);
			res->buf = NULL;
		}

		if( res->cq ){
			ibv_destroy_cq(res->cq);
			res->cq = NULL;
		}

		if( res->pd ){
			ibv_dealloc_pd(res->pd);
			res->pd = NULL;
		}
		
		if( dev_list ){
			ibv_free_device_list(dev_list);
			dev_list = NULL;
		}

		if( res->sock >= 0 ){

			if(close(res->sock)) fprintf(stderr, "failed to close socket\n");
			res->sock = -1;
		}

	}
	
	return rc;
}

static int modify_qp_to_init(struct ibv_qp *qp){
	
	struct 	ibv_qp_attr attr;
	int 	flags;
	int 	rc;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_INIT;
	attr.port_num = config.ib_port;
	attr.pkey_index = 0;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

	rc = ibv_modify_qp(qp, &attr, flags);

	if( rc ) fprintf(stderr, "failed to modify QP state to INIT\n");

	return rc;
}


static int modify_qp_rtr(struct ibv_qp *qp, uint32_t remote_qpn, uint16_t dlid, uint8_t *dgid){

	struct ibv_qp_attr	attr;
	int			flags;
	int 			rc;

	memset(&attr, 0 ,sizeof(attr));

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = remote_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 1;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dlid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = config.ib_port;

	if( config.gid_idx >= 0 ){
		
		attr.ah_attr.is_global = 1;
		attr.ah_attr.port_num = 1;
		memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
		attr.ah_attr.grh.flow_label = 0;
		attr.ah_attr.grh.hop_limit = 1;
		attr.ah_attr.grh.sgid_index = config.gid_idx;
		attr.ah_attr.grh.traffic_class = 0;
	}

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	rc = ibv_modify_qp(qp, &attr, flags);

	if( rc ) fprintf(stderr, "failed to modify QP state to RTR\n");

	return rc;
}

static int modify_qp_rts(struct ibv_qp *qp){
	struct ibv_qp_attr attr;
	int 		flags;
	int		rc;

	memset(&attr, 0 ,sizeof(attr));

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0 ;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 1;

	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
	rc = ibv_modify_qp(qp, &attr, flags);
	if( rc ) fprintf(stderr, "failed to modify QP state to RTS\n");

	return rc;
}

static int connect_qp(struct resources *res){
	struct cm_con_data_t 	local_con_data;
	struct cm_con_data_t	remote_con_data;
	struct cm_con_data_t	tmp_con_data;

	int	rc = 0;
	char 	temp_char;
	union	ibv_gid	my_gid;
	int i;

	if(config.gid_idx >= 0){
		rc = ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid);
		if(rc) {
			fprintf(stderr, "cound not get gid for port %d, index %d\n",config.ib_port, config.gid_idx);
			return rc;
		}

	}else memset(&my_gid, 0 ,sizeof(my_gid));

	local_con_data.addr = htonll((uintptr_t) res->buf);
	local_con_data.rkey = htonl(res->mr->rkey);
	local_con_data.qp_num = htonl(res->qp->qp_num);
	local_con_data.lid = htons(res->port_attr.lid);

	memcpy(local_con_data.gid, &my_gid, 16);

	fprintf(stdout, "\nLocal LID	=0x%x\n", res->port_attr.lid);

	if( sock_sync_data(res->sock, sizeof(struct cm_con_data_t), (char *) &local_con_data, (char *)&tmp_con_data) < 0 ) {
		
		fprintf(stderr, "failed to exchange connection data between sides\n");
		rc = 1;
		goto connect_qp_exit;
	}

	remote_con_data.addr = ntohll(tmp_con_data.addr);
	remote_con_data.rkey = ntohl(tmp_con_data.rkey);
	remote_con_data.qp_num = ntohl(tmp_con_data.qp_num);
	remote_con_data.lid = ntohs(tmp_con_data.lid);
	memcpy(remote_con_data.gid, tmp_con_data.gid ,16);

	res->remote_props = remote_con_data;
	fprintf(stdout, "Remote address = 0x%"PRIx64"\n", remote_con_data.addr);
	fprintf(stdout, "Remote rkey = 0x%x\n", remote_con_data.rkey);

	fprintf(stdout, "Remote QP number = 0x%x\n", remote_con_data.qp_num);
	fprintf(stdout, "Remote LID = 0x%x\n", remote_con_data.lid);

	if(config.gid_idx >= 0 ){
		uint8_t *p = remote_con_data.gid;
		fprintf(stdout, "Remote GID = ");
		for(i = 0;i<16;++i) {
		
			fprintf(stdout, "%02x:", p[i]);
		}
		fprintf(stdout, "\n");
	}

	/*modify the QP to init*/
	rc = modify_qp_to_init(res->qp);
	if( rc ) {
		fprintf(stderr, "change QP state to INIT failed\n");
		goto connect_qp_exit;
	}

	/*let the client post RR to be prepared for incoming messages*/
	if( config.server_name){
		rc = post_recv(res);
		if( rc ){
			fprintf(stderr, "failed to post RR\n");
			goto connect_qp_exit;
		}
	}

	/*modify the QP to RTR*/
	rc = modify_qp_rtr(res->qp, remote_con_data.qp_num, remote_con_data.lid, remote_con_data.gid);
	if( rc ){
		fprintf(stderr, "failed to modify QP state to RTR\n");
		goto connect_qp_exit;
	}
	
	rc = modify_qp_rts(res->qp);
	if( rc ){
		fprintf(stderr, "failed to modify QP state to RTS\n");
		goto connect_qp_exit;
	}
	
	fprintf(stdout, "QP state was change to RTS\n");
	
	if( sock_sync_data(res->sock, 1, "Q", &temp_char)) {
		fprintf(stderr, "sync error after QPs are were moved to RTS\n");
		rc = 1;
	}

connect_qp_exit:

	return rc;
}


static int resources_destroy(struct resources *res){

	int rc = 0;
	
	if( res->qp ){
		if( ibv_destroy_qp(res->qp) ) {
			fprintf(stderr, "failed to destroy QP\n");
			rc = 1;
		}
	}
	if( res->mr ){
		if( ibv_dereg_mr(res->mr)) {
			fprintf(stderr, "failed to deregister MR\n");
			rc = 1;
		}
	}
	if( res->buf )
		free(res->buf);

	if( res->cq ){
		if(ibv_destroy_cq(res->cq)) {
			fprintf(stderr, "failed to destroy CQ\n");
			rc = 1;
		}
	}

	if( res->pd ) {
		if( ibv_dealloc_pd(res->pd)){
			fprintf(stderr, "failed to deallocate PD\n");
			rc = 1;
		}
	}

	if( res->ib_ctx ){
		if( ibv_close_device(res->ib_ctx)){
			fprintf(stderr, "failed to close device context\n");
			rc = 1;
		}
	}

	if( res->sock >=0 ){
		if( close(res->sock) ){
			fprintf(stderr, "failed to close socket\n");
			rc = 1;
		}
	}

	return rc;
}

static void print_config(){
	fprintf(stdout, "------------------------------\n");
	fprintf(stdout, "Device name 	:\"%s\"\n", config.dev_name);
	fprintf(stdout, "IB port	:%u\n", config.ib_port);
	if( config.server_name ) fprintf(stdout, "IP	:%s\n", config.server_name);
	fprintf(stdout,	"TCP port	:%u\n", config.tcp_port);
	if( config.gid_idx >= 0 ) fprintf(stdout, "GID index	:%u\n", config.gid_idx);
	fprintf(stdout, "------------------------------\n\n");
}

int main(int argc, char *argv[]){

	struct resources	res;
	int			rc = 1;
	char			temp_char;

	while(1){

		int c;

		static struct option long_options[] = {
			{.name = "port", .has_arg = 1, .val='p'},
			{.name = "ib_dev", .has_arg = 1, .val = 'd'},
			{.name = "ib_port", .has_arg = 1, .val = 'i'},
			{.name = "gid_idx", .has_arg = 1, .val = 'g'},
			{.name = NULL, .has_arg = 0, .val = '\0'}
		};

		c = getopt_long(argc, argv, "p:d:i:g:", long_options, NULL);

		if( c == -1 ) break;

		switch(c) {
			case 'p':
				config.tcp_port = strtoul(optarg, NULL, 0);
				break;
			case 'd':
				config.dev_name = strdup(optarg);
				break;
			case 'i':
				config.ib_port = strtoul(optarg, NULL, 0);
				if( config.ib_port < 0 ) {
					fprintf(stderr,"error ib port < 0 \n");
					return 1;
				}
				break;
			case 'g':
				config.gid_idx = strtoul(optarg, NULL, 0);
				if( config.gid_idx < 0 ){
					fprintf(stderr,"error gid idx  < 0 \n");
					return 1;
				}
				break;

			default:
				return 1;

		}
	}
	if( optind == argc - 1 ) config.server_name = argv[optind];
	else if(optind < argc) {
		fprintf(stderr, "unknown options" );
		return 1;
	}

	print_config();

	resources_init(&res);

	if( resources_create(&res) ){
		fprintf(stderr, "failed to create resources\n" );
		goto main_exit;
	}

	if( connect_qp(&res) ){
		fprintf(stderr, "failed to connect QPs\n" );
		goto main_exit;
	}

	if( !config.server_name ) //server using 'send mode' to send message to client
		if(post_send(&res, IBV_WR_SEND)){
			fprintf(stderr, "failed to post sr\n");
			goto main_exit;
		}

	if( poll_completion(&res) ){
		fprintf(stderr, "poll completion failed\n");
		goto main_exit;
	}

	if( config.server_name )
		fprintf(stdout, "Message is: %s\n", res.buf);
	else {
		strcpy(res.buf, RDMAMSGR);
	}

	if( sock_sync_data(res.sock, 1, "R", &temp_char) ){
		fprintf(stderr, "sync error before RDMA ops\n");
		rc = 1;
		goto main_exit;
	}

	if( config.server_name ){
		
		if( post_send(&res, IBV_WR_RDMA_READ) ){
			
			fprintf(stderr, "failed to post SR 2\n");
			rc = 1;
			goto main_exit;
		}


		if( poll_completion(&res) ){
			fprintf(stderr, "poll completion failed 2\n");
			rc = 1;
			goto main_exit;
		}
		
		fprintf(stdout, "Contents of server's buffer: '%s'\n", res.buf);
		
		strcpy(res.buf, RDMAMSGW);

		fprintf(stdout, "Replace it with: '%s'\n", res.buf);

		if(post_send(&res, IBV_WR_RDMA_WRITE)){
			fprintf(stderr, "failed to post SR 3\n");
			rc = 1;
			goto main_exit;
		}

		if( poll_completion(&res) ){
			fprintf(stderr, "poll completion failed 3\n");
			rc = 1;
			goto main_exit;
		}
	}

	if( sock_sync_data(res.sock, 1, "W", &temp_char)){
		fprintf(stderr, "sync error after RDMA write operation\n");
		rc = 1;
		goto main_exit;
	}

	if( !config.server_name ) fprintf(stdout, "Contents of server buffer: '%s'\n", res.buf);

	rc = 0;
	

	main_exit:
	if( resources_destroy(&res) ){
		fprintf(stderr, "failed to destroy resources\n");
		rc = 1;
	}

	if( config.dev_name )
		free((char *) config.dev_name);

	fprintf(stdout, "\ntest result is %d\n", rc);

	return rc;
}



/*
int main(int argc, char **argv){

	int port = 19999;
	char *serverName = NULL;
	char *ld;
	char *rd;
	int sockfd;
	ld = malloc(sizeof(char) * strlen("hello world") + 1);
	rd = malloc(sizeof(char) * strlen("hello world") + 1);

	strcpy(ld, "hello world");
	strcpy(rd, "hello world");

	while(1){
		int c;
		static struct option long_options[] = {
			{.name="server", .has_arg=1, .val='s'},
			{.name="port", 	 .has_arg=1, .val='p'}
		};
		c = getopt_long(argc, argv, "p:s:", long_options, NULL);

		if( c < 0 ) break;

		switch (c){
			case 'p':
				port = strtoul(optarg, NULL, 0);
				break;
				case 's':
				serverName = strdup(optarg);
				break;
			default:
				printf("%c %s\n", c, optarg);
		}
	}

	printf("Server name: %s\n", serverName);
	printf("Port number: %d\n", port);

	sockfd = sock_connect(serverName, port);

	if( sockfd > 0 ) printf("Connect successful\n");
	else printf("Connect failed\n");

	if( sock_sync_data(sockfd, strlen(ld), ld, rd) == 0 ) printf("sync successful\n");
	else printf("sync error\n");

	


	free(ld);
	free(rd);

	return 0;
}*/
