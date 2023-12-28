
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define RESOLUTION	100	// simulation step: 1/100 second
#define JOB_MAX		100	// max jobs / node
#define JOB_DURATION	(2.0 * RESOLUTION)  // 2 seconds
#define NUM_NODES	4	// number of nodes
#define NUM_USERS	350	// number of users
#define COST_THRESH	100	// max cost
#define SIM_DURATION	(800 * RESOLUTION)	// 800 sec

struct node {
	int n_active;	// number of active jobs
	double load;	// instantaneous load
	double loadavg;	// load average over last 1 sec
};

struct node nodes[NUM_NODES];  // server nodes

struct user {
	struct node *np;	// currently assigned node
	struct node *last_np;	// previously assigned node
	double job_end;		// end time for the current job
	double start_time;	// start time for the user
	double end_time;	// end time for the user
	double migration_time;	// last migration time
};

struct user users[NUM_USERS];	// users

#define COSTMODE_CONVEX		0	// strandard convex
#define COSTMODE_CONVEXPROP	1	// proportional

int cost_mode = COSTMODE_CONVEX;	// for dofferent cost models

int cur_time = 0;	// current time, 00.1 sec resolution

// Uniform() returns uniform random number in (0.0, 1.0)
double Uniform( void ){
	return ((double)random()+1.0)/((double)RAND_MAX+2.0);
}

double rand_normal( double mu, double sigma ){
	double z=sqrt( -2.0*log(Uniform()) ) * sin( 2.0*M_PI*Uniform() );
	return mu + sigma*z;
}

double rand_exp( double lambda ){
	return -log(Uniform())/lambda;
}

// convex cost function
double
load2cost(double load, int node_id)
{
	double cost;

	if (load < 0.0)
		load = 0.0;
	else if (load > 1.0)
		load = 1.0;
	cost = pow((2.0 * load - 1.0), (double)2) / (1.0 - load) + 1;

	if (cost_mode == COSTMODE_CONVEXPROP)
		// proportional cost
		cost *= pow(2, node_id);
	return (cost);
}

// select the node with min cost
struct node *
select_node(struct node *last_np, double elapsed_time)
{
	struct node *np, *best_np = NULL;
	double load, cost, cost_prev, min_cost;
	int n;

	cost_prev =  min_cost = COST_THRESH * 2;
	for (n = 0; n < NUM_NODES; n++) {
		np = &nodes[n];
		if (np->n_active >= JOB_MAX)
			continue;  // no room for the job
		load = np->loadavg;
		cost = load2cost(load, n);
		if (cost < min_cost) {
			min_cost = cost;
			best_np = np;
		}
		if (np == last_np)
			cost_prev = cost;  // cost for prev node
	}
	if (min_cost > COST_THRESH)
		return (NULL);
#if 1  // affinity
	// node affinity: try to use the previously assigned node
	// if the difference between min_cost and cost_prev is less than 20%,
	// and if the elapsed time since the last migration is less than 5 sec
	// stick to the prev node with linear probability
	if (best_np != last_np && cost_prev != 0.0 
			&& last_np != NULL && last_np->n_active < JOB_MAX) {
		double prob_gain, prob_time; // migration probabilities
		double gain = 1.0 - min_cost / cost_prev;
		prob_gain = 1.0;
#if 1  // use gain
#define	GAIN_THRESH	0.2
		if (gain < GAIN_THRESH)
			prob_gain = gain / GAIN_THRESH;
#endif
		assert(prob_gain >= 0.0 && prob_gain <= 1.0);
		prob_time = 1.0;
#if 1  // use elapsed time
#define ELAPSED_THRESH	(5.0 * RESOLUTION)	// 5 sec
		if (elapsed_time < ELAPSED_THRESH)
			prob_time = elapsed_time / ELAPSED_THRESH;
#endif
		assert(prob_time >= 0.0 && prob_time <= 1.0);
		if (Uniform() > prob_gain * prob_time) {
			best_np = last_np; // stay at the prev node
			//fprintf(stderr, "affinity: gain=%.3f elapsed=%.1f prob: %.3f %.3f\n", gain, elapsed_time,prob_gain, prob_time);
		}
	}
#endif		
	return (best_np);
}

void init_users(void)
{
	int i;

	// set up start_time and end_time for each user
	//  user[i] starts at time (i*100) and ends at (8000 - i*100)
	//  randomize the last 2 digits to avoid synchronization
	for (i = 0; i < NUM_USERS; i++) {
		struct user *up = &users[i];
		up->np = NULL;
		up->start_time = (double)(i * RESOLUTION + random() % RESOLUTION);
		up->end_time = (double)(SIM_DURATION - i * RESOLUTION + random() % RESOLUTION);
	}
}

#define assign_job(np)	((np)->n_active++)
#define release_job(np)	((np)->n_active--)

void user_action(struct user *up, double t)
{
	if (up->np == NULL && (t < up->start_time || t >= up->end_time))
		return;	// not running

	if (up->np != NULL && t >= up->job_end) {
		// stop the current job
		release_job(up->np);
		up->last_np = up->np;  // remember the current node
		up->np = NULL;
		// up->job_end = 0.0; // not needed
	}
	if (up->np == NULL && t < up->end_time) {
		// start a new job
		up->np = select_node(up->last_np, t - up->migration_time);
		if (up->np != NULL) {
			assign_job(up->np);
			assert(up->np->n_active <= JOB_MAX);
			up->job_end = t + rand_normal(JOB_DURATION, JOB_DURATION/8.0);
			if (up->last_np != up->np)
				up->migration_time = t;
		}
	}
}

void do_actions(double t)
{
	int i;

	for (i = 0; i < NUM_USERS; i++)
		user_action(&users[i], t);
}

void update_loads(void)
{
	int n;

	for (n = 0; n < NUM_NODES; n++) {
		struct node *np = &nodes[n];
		double w; // weight for ewma

		// instantaneous load in the previous step
		np->load = (double)np->n_active / JOB_MAX;
		// load average: ewma over last 1 sec
		// np->loadavg = np->load * 0.01 + np->loadavg * 0.99;
#if 0
		w = 1.0;  // use instantaneous load
#else		
		w = 1.0 / RESOLUTION;
#endif
		np->loadavg = np->load * w + np->loadavg * (1.0 - w);
	}
}

void print_nodes(void)
{
	int i, jobs = 0;

	printf("%-4d", (int)cur_time/RESOLUTION);
	for (i = 0; i < NUM_NODES; i++) {
		struct node *np = &nodes[i];
		printf(" %.3f ", np->loadavg);
		jobs += np->n_active;
	}
	printf(" [%d jobs, %.3f load]\n", jobs, (double)jobs/JOB_MAX);
}

int
main(int argc, char **argv)
{
	int ch;
	int end_time = SIM_DURATION * 1025 / 1000;	// 820 sec

	while ((ch = getopt(argc, argv, "ph")) != -1) {
		switch (ch) {
		case 'h':
			fprintf(stderr, "usage: sim4 [-hp]\n");
			exit(0);
		case 'p':
			cost_mode = COSTMODE_CONVEXPROP; // proportional cost
			break;
		}
	}
#if 1
	srandom(time(NULL));
#else	
	srandom(66);
#endif	
	init_users();

	for (cur_time = 0; cur_time < end_time; cur_time++) {

		do_actions((double)cur_time);

		update_loads();	// update the current load and load averages

		// print every 1 sec
		if (cur_time % RESOLUTION == 0)
			print_nodes();
	}
}
