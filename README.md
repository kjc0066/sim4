# sim4
A simple simulator for cloud-morphing with 4 nodes

## Usage

To compile:

	make

Then, make a plot using gnuplot:

	make plot

To make a plot for a proportional cost scenario:

	make plot2

## Settings

Simulation step: 0.01 sec

Loadaverage: EWMA weight=0.01, loadavg(i) = load(i) * w + loadavg(i-1) * (1 - w)

Affinity by gain: try to avoid switching the node when the cost
difference is small.
If the gain is less than 20%, linearly increase the node-switch
probability.

Affinity by elapsed time: try to avoid switching the node when the
elapsed time since the last node-switch is small.
If the elapsed time is less than 5 sec, linearly increase the node
switch probability.

