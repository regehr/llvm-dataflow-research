set terminal pdf color

set xlabel "time (seconds)"
set ylabel "precision (bits)"

set output "or.pdf"
plot [:10] "or.txt" using 1:2:3 with labels, "or.txt" using 1:2

set output "and.pdf"
plot [:10] "and.txt" using 1:2:3 with labels, "and.txt" using 1:2
