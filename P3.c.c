#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define NUM_QUEUE 10
#define ROW_SIZE 10
#define DEBUG 1

int h_cust = 0, m_cust = 0, l_cust = 0;
int h1_cust_counter=0,m1_cust_counter = 0,m2_cust_counter = 0,m3_cust_counter = 0, l1_cust_counter = 0,l2_cust_counter = 0,l3_cust_counter = 0,l4_cust_counter = 0,l5_cust_counter = 0,l6_cust_counter = 0;
typedef struct customer {
	int arrival_time;
	int custId;
} customer;


typedef struct customerQ {
	customer *cust;
	int front, rear;
} customerQ;


typedef struct row {
	int next_avail;
	pthread_mutex_t mutex;
} row;

typedef struct pthread_args{
	char *seller_type;
	int row_id;
	int tid;
	customerQ* cq;
} pthread_args;

customerQ *custQ;
row tickets[10];

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int serve_t = 0;
int pt =0;

// Number of customer to service
int N;

enum seat_state{
    AVAILABLE   = 0,
    SOLD        = 1,
    PROCESSING  = 2,
};

typedef struct seat {
    enum seat_state state;
    int id;
    customer* cust;
    pthread_args* p_args;
    int counter;
} seat;
seat theater[100];

struct seat_manager {
    seat* h_seat;
    seat* m_seat;
    seat* l_seat;
    int free_seats;
};

void print_theater(){
    int i, j, idx;
    for (i=0; i<10; i++){
        printf("ROW %02d |", i);
        for (j=0; j<10; j++){
            idx = i*10 + j;
            if (theater[idx].state == AVAILABLE){
                printf(" S%02d: ------ |", theater[idx].id);
            }else{
                if(theater[idx].cust == NULL){
                    printf("error found NULL\n");
                }else{
                printf(" S%02d: %s%d=%02d=C%02d |", theater[idx].id, theater[idx].p_args->seller_type,theater[idx].p_args->tid,theater[idx].counter, theater[idx].cust->custId);
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}

struct seat_manager the_seat_manager;


// Some init routines for the seat manager and the theater array:
void seat_manager_init(){
    the_seat_manager.h_seat = &theater[0];
    the_seat_manager.m_seat = &theater[40];
    the_seat_manager.l_seat = &theater[90];
    the_seat_manager.free_seats = 100;
}
void theater_init(){
    int i;
    for (i=0; i<100; i++){
        theater[i].state = AVAILABLE;
        theater[i].id = i;
        theater[i].cust = NULL;
//
        theater[i].p_args = NULL;
    }
}

pthread_mutex_t seat_lock = PTHREAD_MUTEX_INITIALIZER;  // The lock for the seat manager struct

void increment_h_seat(){
    seat* tmp_seat = the_seat_manager.h_seat;
    while (tmp_seat->state != AVAILABLE && the_seat_manager.free_seats > 0){
            tmp_seat++;
    }
    the_seat_manager.h_seat = tmp_seat;
}
seat* get_H_seat_to_sell(){
    seat* allocated_seat_to_sell;
    pthread_mutex_lock(&seat_lock);
    if (the_seat_manager.free_seats > 0){
        // If the seat pointed to by the seat_manager is not free, get a free seat.
        if(the_seat_manager.h_seat->state != AVAILABLE) increment_h_seat();
        allocated_seat_to_sell = the_seat_manager.h_seat;
        allocated_seat_to_sell->state = PROCESSING;
        the_seat_manager.free_seats--;
        h_cust++;
    }else{
        allocated_seat_to_sell = NULL;
    }
    pthread_mutex_unlock(&seat_lock);
    return allocated_seat_to_sell;
}

void increment_m_seat(){
    seat* tmp_seat = the_seat_manager.m_seat;
    while (tmp_seat->state != AVAILABLE && the_seat_manager.free_seats > 0){
        // if seat id is 99, 89, 79, etc subtract 19 to move to previous row

        switch (tmp_seat->id){

            case 59 :
                tmp_seat = &theater[30];
                continue;

            case 39:
                tmp_seat = &theater[60];
                continue;

            case 69:
                tmp_seat = &theater[20];
                continue;

            case 29:
                tmp_seat = &theater[70];
                continue;

            case 79:
                tmp_seat = &theater[10];
                continue;

            case 19:
                tmp_seat = &theater[80];
                continue;

            case 89:
                tmp_seat = &theater[0];
                continue;

            case 9:
                tmp_seat = &theater[90];
                continue;

            default :
                tmp_seat++;
        }
    }
    the_seat_manager.m_seat = tmp_seat;
}

seat* get_M_seat_to_sell(){
    seat* allocated_seat_to_sell;
    pthread_mutex_lock(&seat_lock);
    if (the_seat_manager.free_seats > 0){
        // If the seat pointed to by the seat_manager is not free, get a free seat.
        if(the_seat_manager.m_seat->state != AVAILABLE) increment_m_seat();
        allocated_seat_to_sell = the_seat_manager.m_seat;
        allocated_seat_to_sell->state = PROCESSING;
        the_seat_manager.free_seats--;
        m_cust++;
    }else{
        allocated_seat_to_sell = NULL;
    }
    pthread_mutex_unlock(&seat_lock);
    return allocated_seat_to_sell;
}

void increment_l_seat(){
    seat* tmp_seat = the_seat_manager.l_seat;

    while (tmp_seat->state != AVAILABLE && the_seat_manager.free_seats > 0){

        // if seat id is 99, 89, 79, etc subtract 19 to move to previous row
        if (tmp_seat->id % 10 == 9 && tmp_seat->id > 9){
            tmp_seat = &theater[tmp_seat->id - 19];
            continue;
        }

        // try the next seat
        tmp_seat++;

    }
    the_seat_manager.l_seat = tmp_seat;
}
seat* get_L_seat_to_sell(){
    seat* allocated_seat_to_sell;
    pthread_mutex_lock(&seat_lock);
    if (the_seat_manager.free_seats > 0){
        // If the seat pointed to by the seat_manager is not free, get a free seat.
        if(the_seat_manager.l_seat->state != AVAILABLE) increment_l_seat();
        allocated_seat_to_sell = the_seat_manager.l_seat;
        allocated_seat_to_sell->state = PROCESSING;
        the_seat_manager.free_seats--;
        l_cust++;
    }else{
        allocated_seat_to_sell = NULL;
    }
    pthread_mutex_unlock(&seat_lock);
    return allocated_seat_to_sell;
}



// seller thread to serve one time slice
void * sell(pthread_args *pargs)
{
    int customer_idx = 0;
    int all_done = 0;
    int process_time = 0;
    int done_time = 0;
    seat * sts;
    customer* nc;


	while (1)
	{
	    // clock barrier
	    pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);

	    // Check to see if we are in the middle of a sale and continue
	    if (serve_t < done_time){
            if (DEBUG == 1) printf("%s%02d(P,T%1d)        |", pargs->seller_type, pargs->tid, done_time - serve_t);
            continue;
        }

        // Skip this turn if next customer has not arrived yet.
        if (pargs->cq->cust[customer_idx].arrival_time > serve_t){
            continue;
        }

        // get the customer if we are not done yet
        if (customer_idx < N){
            nc = &(pargs->cq->cust[customer_idx++]);
        }else{
            if (all_done==0){
                if (DEBUG ==1 ) printf("%s%02d(ALL_DONE)    |", pargs->seller_type, pargs->tid);
                all_done =1;
            }
            continue;
        }

        // We have a customer to process now, get seat and random wait time depending on the seller type
	if(strcmp(pargs->seller_type, "H") == 0){
            sts = get_H_seat_to_sell();
            process_time = 1 + rand() % 2;
        } else if(strcmp(pargs->seller_type, "M") == 0) {
            sts = get_M_seat_to_sell();
            process_time = 2 + rand() % 3;
        } else {
            sts = get_L_seat_to_sell();
            process_time = 4 + rand() % 4;
        };

        if (sts == NULL){
            printf(" THEATER SOLD OUT!!!\n");
            break;
        }
	
	

        // Finish processing the customer
        if (DEBUG == 1) printf("%s%02d(S%02d,C%03d,T%1d) |", pargs->seller_type, pargs->tid, sts->id, nc->custId, process_time);
        done_time = serve_t + process_time;
        sts->cust = nc;
	sts->p_args = pargs;
        sts->state = SOLD;
	if(strcmp(pargs->seller_type, "H") == 0){
	    h1_cust_counter++;
	    sts->counter = h1_cust_counter;
        } else if(strcmp(pargs->seller_type, "M") == 0) {
	    if(pargs->tid==1){m1_cust_counter++;sts->counter = m1_cust_counter;}
	    if(pargs->tid==2){m2_cust_counter++;sts->counter = m2_cust_counter;}
	    if(pargs->tid==3){m3_cust_counter++;sts->counter = m3_cust_counter;}
        } else {	
	    if(pargs->tid==1){l1_cust_counter++;sts->counter = l1_cust_counter;}
	    if(pargs->tid==2){l2_cust_counter++;sts->counter = l2_cust_counter;}
	    if(pargs->tid==3){l3_cust_counter++;sts->counter = l3_cust_counter;}
	    if(pargs->tid==4){l4_cust_counter++;sts->counter = l4_cust_counter;}
	    if(pargs->tid==5){l5_cust_counter++;sts->counter = l5_cust_counter;}
	    if(pargs->tid==6){l6_cust_counter++;sts->counter = l6_cust_counter;}
        };
        pt++;
	}
	// thread exits
	return NULL;
}

// Wake up everyone waiting on &cond.
void wakeup_all_seller_threads()
{
	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}

// For sorting:
int compare_arrival_times(const void * a, const void * b)
{
    customer *c1 = (customer *)a;
    customer *c2 = (customer *)b;
    int r = c1->arrival_time - c2->arrival_time;
    if (r>0) return 1;
    if (r<0) return -1;
    return r;
}

// Generate the customer queues
void setupQueue(int N)
{
	int i, j, arrival_time;
	custQ = (customerQ *) malloc(sizeof (customerQ) * NUM_QUEUE);

    // for reproducibility
    srand(1);
	for(i = 0; i < NUM_QUEUE; ++i)
    {
    	custQ[i].cust = (customer *) malloc(sizeof(customer) * N);
    	for (j = 0; j < N; ++j)
    	{
    	/* code for debugging, biasing H, M or L
//    	    if (i == 0){
//    	    if (i > 0 && i <4){
//          if (i > 3){
    	        arrival_time = rand() % 10;
    	    }else{
    		    arrival_time = 600 + rand() % 60;
    		}
    		*/ // END debug code

            arrival_time = rand() % 60;
    		custQ[i].cust[j].custId = i * N + j; // i*N+j to get a global customer ID
    		custQ[i].cust[j].arrival_time = arrival_time;
    	}
    }

    // sort customer based on arrival times
    for(i = 0; i < NUM_QUEUE; ++i){
    	qsort((void *)custQ[i].cust, N, sizeof(customer), compare_arrival_times);
    }

    // initialize all mutex value
    for(i = 0; i < ROW_SIZE; ++i){
		pthread_mutex_init(&tickets[i].mutex, NULL);
    }

}

void printCustomerQ(int N)
{
	int i, j;

	for(i = 0; i < NUM_QUEUE; ++i)
    {

    	for (j = 0; j < N; ++j)
    	{

    		printf(" %d:%d\t|", custQ[i].cust[j].custId, custQ[i].cust[j].arrival_time);
    	}
    	printf("\n----------------------------------------------");
    	printf("\n");
    }
}

// Creates all threads and then starts the simulation loop
int main(int argc, char *argv[])
{

	pthread_args *pargs;

	theater_init();
	seat_manager_init();

        N = 5;
        if(argc != 2) printf("Accepting the default value of customers (%d)\n",N);
        else N = atoi(argv[1]);
        printf("The number of customers entered is %d\n",N);
	setupQueue(N);

	printCustomerQ(N);

	int i;
	pthread_t tids[10];
	// Create necessary data structures for the simulator.
	// Create buyers list for each seller ticket queue based on the
	// N value within an hour and have them in the seller queue.
	// Create 10 threads representing the 10 sellers.

	pargs = (pthread_args *) malloc(sizeof(pthread_args));

	pargs->seller_type = (char *) malloc(strlen("H") + 1);
	memcpy(pargs->seller_type, "H", strlen("H"));
	pargs->row_id = 0;
	pargs->tid = 0;
	pargs->cq = &custQ[0];
	pthread_create(&tids[0], NULL, (void *)sell, (void *)pargs);

	for (i = 1; i < 4; i++) {
		pargs = (pthread_args *) malloc(sizeof(pthread_args));
		pargs->seller_type = (char *) malloc(strlen("M") + 1);
		memcpy(pargs->seller_type, "M", strlen("M"));
		pargs->row_id = i;
		pargs->tid = i;
		pargs->cq = &custQ[i];
		pthread_create(&tids[i], NULL, (void *)sell, (void *)pargs);
	}


	for (i = 4; i < 10; i++) {
		pargs = (pthread_args *) malloc(sizeof(pthread_args));
		pargs->seller_type = (char *) malloc(strlen("L") + 1);
		memcpy(pargs->seller_type, "L", strlen("L"));
		pargs->row_id = i;
		pargs->tid = i;
		pargs->cq = &custQ[i];
		pthread_create(&tids[i], NULL, (void *)sell, (void *)pargs);
	}


	// start simulation clock
	for (i=0; i<60; i++){
	    if (DEBUG == 1) printf("\nclock tick %02d:  ", serve_t);
	    wakeup_all_seller_threads();
	    usleep(7);
	    if (pt && DEBUG == 2){
//	        printf("\n New Seat Sold!\n");
	        print_theater();
	        pt =0;
	    }
	    serve_t++;

	}

	printf("\n\n Theater after sale period:\n");
	print_theater();
        
        printf("\n\n Customers allocated seats: (H) %d  (M) %d  (L) %d [Total] %d [Turned away] %d\n",h_cust,m_cust,l_cust,(h_cust+m_cust+l_cust),N*10-(h_cust+m_cust+l_cust));

}
