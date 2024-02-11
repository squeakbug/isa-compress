#!/usr/bin/gnuplot -persist
set terminal png size 800,600
set output 'plot_02.png'

set xlabel font "Helvetica,18"
set ylabel font "Helvetica,18"
set title  font "Helvetica,18"
set xtics ("М8/56" 1, "М16/48" 2, \
	 "М32/32" 3, "М48/16" 4, "М56/8" 5)

# Устанавливаем заголовок графика и подписи осей
set title "Эксперимент №2"
set xlabel "Формат сжатой инструкции"
set ylabel "Коэффициент сжатия, у.е."

# Считываем данные из файла и строим гистограмму
plot "distribution_02.txt" using 1:2 \
s f w boxes fs notitle fs transparent solid .15 #rgb "#77000000"

#set arrow from graph 0,first 1.22 to graph 1,first 1.22 nohead 
