#set terminal png
set terminal pdf enhanced color solid
set multiplot layout 2,1
set grid linewidth 2
set xtics 100 font "Helvetica, 18"
set ytics 0.25 font "Helvetica, 18"
set xlabel font "Helvetica, 20"
set ylabel font "Helvetica, 20"
set key font "Helvetica, 18"
set format x ""
set ylabel "load"
set xrange [0:820]
set yrange [0:1.0]
plot "plot.txt" using 1:2  title "r0" with lines lw 2, \
     "plot.txt" using 1:3  title "r1" with lines lw 2, \
     "plot.txt" using 1:4  title "r2" with lines lw 2, \
     "plot.txt" using 1:5  title "r3" with lines lw 2
set xtics 100 font "Helvetica, 18"
set ytics 1.0 font "Helvetica, 18"
set xlabel font "Helvetica, 20"
set ylabel font "Helvetica, 20"
set key font "Helvetica, 18"
set xlabel "time"
set format x "%g"
set format y "%5.0f"
set ylabel "total load"
set xrange [0:820]
set yrange [0:4.0]
plot "plot.txt" using 1:2 with filledc above y=0 title "r0", \
     "plot.txt" using 1:2:($2+$3) with filledc title "r1", \
     "plot.txt" using 1:($2+$3):($2+$3+$4) with filledc title "r2", \
     "plot.txt" using 1:($2+$3+$4):($2+$3+$4+$5) with filledc title "r3"
