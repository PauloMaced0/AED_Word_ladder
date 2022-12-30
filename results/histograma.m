clear;
clc;

h = load("histograma.txt");
hf = figure();
entries = h(:,2);
plot(2,1,1);
a = bar(entries,'k');
legend(a,{'Nº de entradas'});
title("Distribuição das entradas da Hash Table");

print(hf, "hash_table_entries", "-dpdflatexstandalone");
system ("pdflatex hash_table_entries");
open hash_table_entries.pdf