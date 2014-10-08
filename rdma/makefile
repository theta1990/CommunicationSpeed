ib_test: ib_test.c
	cc -Wall -g -o	ib_test ib_test.c -I/usr/include -L/usr/lib64 -libverbs
rdma: rc_example.c
	cc -c -Wall -g rc_example.c -I /usr/include
	cc -g -o rc_example rc_example.o -L /usr/lib64 -libverbs
hello: hello.c
	cc -c -Wall -g hello.c
	cc -g -o hello hello.o
