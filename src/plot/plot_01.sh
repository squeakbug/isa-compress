#!/usr/bin/gnuplot -persist

set terminal png size 800,600
set output 'plot_01.png'

set xlabel font "Helvetica,18"
set ylabel font "Helvetica,18"
set title  font "Helvetica,17"
set xtics ("Словарь" 1, "М32" 2, "М16/16" 3, \
	 "М16/8/8" 4, "М8/8/8/8" 5, "М24/8" 6)

# Устанавливаем заголовок графика и подписи осей
set title "Эксперимент №1"
set xlabel "Формат сжатой инструкции"
set ylabel "Коэффициент сжатия, у.е."

# Считываем данные из файла и строим гистограмму
plot "distribution_01.txt" using 1:2 \
s f w boxes fs notitle fs transparent solid .15 #rgb "#77000000"

set arrow from graph 0,first 1.22 to graph 1,first 1.22 nohead 
