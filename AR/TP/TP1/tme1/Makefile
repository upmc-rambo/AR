


all: mon_programme test test_send mpi_send test_hello



mpi_send: mpi_send.c
	mpicc -o $@ $^


hello: hello.c
	mpicc -o $@ $^


test: mon_programme
	mpirun -np 10 --hostfile my_hostfile $^

test_send: mpi_send
	mpirun -np 10 --hostfile my_hostfile $^

test_hello: hello
	mpirun -np 10 --hostfile my_hostfile $^

