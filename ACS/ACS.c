#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct customer{	//customer node
	int cid;				//customer id
	int qid;				//queue id (0 = economy class, 1 = business class)
	int atime;				//arrival time				
	int stime;				//service time
	int index;				//where customer is stored in customers
	int clerk;				//clerk who serves customer
	float wait_start;		//start time of service
	float wait_end;			//end time of service
}customer;

typedef struct clerk{		//clerk node
	int id;					//clerk id
	int busy;				//availability of clerk (0 = free, 1 = busy)
}clerk;

customer* customers;		//list of all customers
customer* queue[2];			//queues (0 = economy class, 1 = business class)
clerk clerks[4];			//list of all clerks
int qlength[2] = {0,0};		//queue lengths (0 = economy class, 1 = business class)
float waitTime[2] = {0,0};	//wait times (0 = economy class, 1 = business class)
int lineLength[3] = {0,0,0};//total line lengths (0 = economy class, 1 = business class, 2 = total customers)
struct timeval program_start;//start time of program
/*
mutex:
	0 = queue
	1 = clerk 1
	2 = clerk 2
	3 = clerk 3
	4 = clerk 4
*/
pthread_mutex_t mutex[5];
/*
convar:
	0 = economy class
	1 = business class
	2 = clerk 1
	3 = clerk 2
	4 = clerk 3
	5 = clerk 4
*/
pthread_cond_t convar[6];

//insert customer p into queue k
void insertQueue(customer* p, int k){
	queue[k][qlength[k]] = *p;
	qlength[k]++;
}

//pop customer from queue k
int popQueue(int k){
	int cindex = queue[k][0].index;
	int a;
	for(a = 0; a < qlength[k]-1; a++){
		queue[k][a] = queue[k][a+1];
	}
	qlength[k]--;
	return cindex;
}

//get current machine time
float getTime(){
	struct timeval program_now;
	gettimeofday(&program_now, NULL);
	return (program_now.tv_sec - program_start.tv_sec) + (program_now.tv_usec - program_start.tv_usec)/1000000.0f;
}

//thread function for customers
void* customer_runner(void* info){
	customer* p = (customer*) info;
	usleep(p->atime * 100000);
	printf("A customer arrives: customer ID %2d.\n", p->cid);
	if(pthread_mutex_lock(&mutex[0]) != 0){
		printf("Error: failed to lock mutex.\n");
		exit(1);
	}
	insertQueue(p, p->qid);
	printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d.\n", p->qid, qlength[p->qid]);
	p->wait_start = getTime();
	while(p->clerk == -1){
		if(pthread_cond_wait(&convar[p->qid], &mutex[0]) != 0){
			printf("Error: failed to wait.\n");
			exit(1);
		}
	}
	p->wait_end = getTime();
	waitTime[p->qid] += p->wait_end - p->wait_start;
	printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d.\n", p->wait_end, p->cid, p->clerk);
	if(pthread_mutex_unlock(&mutex[0]) != 0){
		printf("Error: failed to unlock mutex.\n");
		exit(1);
	}
	usleep(p->stime * 100000);
	printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n", getTime(), p->cid, p->clerk);
	int clerk = p->clerk;
	if(pthread_mutex_lock(&mutex[clerk]) != 0){
		printf("Error: failed to lock mutex.\n");
		exit(1);
	}
	clerks[clerk-1].busy = 0;
	if(pthread_cond_signal(&convar[clerk+1]) != 0){
		printf("Error: failed to signal convar.\n");
		exit(1);
	}
	if(pthread_mutex_unlock(&mutex[clerk]) != 0){
		printf("Error: failed to unlock mutex.\n");
		exit(1);
	}
	return NULL;
}

//thread function for clerks
void* clerk_runner(void* info){
	clerk* p = (clerk*) info;
	while(1){
		if(pthread_mutex_lock(&mutex[0]) != 0){
			printf("Error: failed to lock mutex.\n");
			exit(1);
		}
		int qindex = 1;
		if(qlength[qindex] <= 0){
			qindex = 0;
		}
		if(qlength[qindex] > 0){
			int cindex = popQueue(qindex);
			customers[cindex].clerk = p->id;
			clerks[p->id-1].busy = 1;
			if(pthread_cond_broadcast(&convar[qindex]) != 0){
				printf("Error: failed to broadcast convar.\n");
				exit(1);
			}
			if(pthread_mutex_unlock(&mutex[0]) != 0){
				printf("Error: failed to unlock mutex.\n");
				exit(1);
			}
		}
		else{
			if(pthread_mutex_unlock(&mutex[0]) != 0){
				printf("Error: failed to unlock mutex.\n");
				exit(1);
			}
			usleep(250);
		}
		if(pthread_mutex_lock(&mutex[p->id]) != 0){
			printf("Error: failed to lock mutex.\n");
			exit(1);
		}
		if(clerks[p->id-1].busy){
			if(pthread_cond_wait(&convar[p->id+1], &mutex[p->id]) != 0){
				printf("Error: failed to wait.\n");
				exit(1);
			}
		}
		if(pthread_mutex_unlock(&mutex[p->id]) != 0){
			printf("Error: failed to unlock mutex.\n");
			exit(1);
		}
	}
	return NULL;
}

//get customers from input file
void getCustomers(char* file){
	FILE* fp = fopen(file, "r");
	if(fp == NULL || fscanf(fp, "%d", &lineLength[2]) < 1){
		printf("Error: failed to read file\n");
		exit(1);
	}
	if(lineLength[2] < 1){
		printf("Error: invalid number of customers.\n");
		exit(1);
	}
	queue[0] = (customer*) malloc(lineLength[2] * sizeof(customer));
	queue[1] = (customer*) malloc(lineLength[2] * sizeof(customer));
	customers = (customer*) malloc(lineLength[2] * sizeof(customer));
	int a;
	int n = 0;
	customer p;
	for(a = 0; a < lineLength[2]; a++){
		if(fscanf(fp, "%d:%d,%d,%d", &p.cid, &p.qid, &p.atime, &p.stime) != 4){
			printf("Error: invalid customer attribute. (skipping cusotmer)\n");
			continue;
		}
		if(p.cid < 0 || p.qid < 0 || p.qid > 1 || p.atime < 0 || p.stime < 0){
			printf("Error: invalid customer attribute. (skipping customer)\n");
			continue;
		}
		p.index = n;
		p.clerk = -1;
		customers[n] = p;
		n++;
		lineLength[p.qid]++;
	}
	lineLength[2] = n;
	fclose(fp);
}

//main function to control program
int main(int argc, char* argv[]){
	if(argc != 2){
		printf("To use: ./ACS <file>.txt\n");
		exit(1);
	}
	getCustomers(argv[1]);
	gettimeofday(&program_start, NULL);
	int a;
	for(a = 0; a < 6; a++){//initialize mutex and convar
		if(a < 4){//set clerk id and availability
			clerks[a].id = a+1;
			clerks[a].busy = 0;
		}
		if(a < 5 && pthread_mutex_init(&mutex[a], NULL) != 0){
			printf("Error: failed to initialize mutex.\n");
			exit(1);
		}
		if(pthread_cond_init(&convar[a], NULL) != 0){
			printf("Error: failed to initialize convar.\n");
			exit(1);
		}
	}
	for(a = 0; a < 4; a++){//create clerk threads
		pthread_t clerkThread;
		if(pthread_create(&clerkThread, NULL, clerk_runner, (void*)&clerks[a]) != 0){
			printf("Error: failed to create thread.\n");
			exit(1);
		}
	}
	pthread_t customerThread[lineLength[2]];
	for(a = 0; a < lineLength[2]; a++){//create customer threads
		if(pthread_create(&customerThread[a], NULL, customer_runner, (void*)&customers[a]) != 0){
			printf("Error: failed to create thread.\n");
			exit(1);
		}
	}
	for(a = 0; a < lineLength[2]; a++){//join customer threads
		if(pthread_join(customerThread[a], NULL) != 0){
			printf("Error: failed to join threads.\n");
			exit(1);
		}
	}
	for(a = 0; a < 6; a++){//destory mutexs and convars
		if(a < 5 && pthread_mutex_destroy(&mutex[a]) != 0){
			printf("Error: failed to destroy mutex.\n");
			exit(1);
		}
		if(pthread_cond_destroy(&convar[a]) != 0){
			printf("Error: failed to destroy convar.\n");
			exit(1);
		}
	}
	printf("The average waiting time for all customers in the system is: %.2f seconds.\n", (waitTime[0]+waitTime[1])/lineLength[2]);
	printf("The average waiting time for all business-class customers is: %.2f seconds.\n", waitTime[1]/lineLength[1]);
	printf("The average waiting time for all economy-class customers is: %.2f seocnds.\n", waitTime[0]/lineLength[0]);
	free(customers);
	free(queue[0]);
	free(queue[1]);
	return 0;
}