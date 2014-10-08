#include <infiniband/verbs.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>







int main(){

	struct ibv_device **dev_list;
	int deviceNum;
	int i;
	struct ibv_context *ctx;
	struct ibv_device_attr dev_attr;
	dev_list = ibv_get_device_list(&deviceNum);
	if(!dev_list)
		exit(1);
	else{
		for(i=0;i<deviceNum;++i){
			
			printf("%s\n", ibv_get_device_name(dev_list[i]));
			
			ctx = ibv_open_device(dev_list[i]);

			if( !ctx ){
				fprintf(stderr, "Error, failed to open the device '%s' \n", ibv_get_device_name(dev_list[i]));
			}else {

				ibv_query_device(ctx, &dev_attr);	
				printf("fw_ver: %s\n", dev_attr.fw_ver);
				ibv_close_device(ctx);
			}
		}
	}
	ibv_free_device_list(dev_list); 
	return 0;
}
