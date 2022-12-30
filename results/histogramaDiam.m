clear;
clc;

X = [0, 1 ,2, 3, 4, 5, 6,7, 8, 9 , 10, 11, 12, 13, 14, 15];
Y = [163, 19, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
hf = figure();
a = bar(X,Y,"histc");
xlim([0 18]);
xticks(0:18);
ylim([0 170]);
yticks(0:10:170);
legend(a,{'Nº de componentes conexos'});
title("Nº de componentes com determinado diâmetro");
xlabel("Diâmetro do componente conexo")
print(hf, "diamComponenteconexo", "-dpdflatexstandalone");
system ("pdflatex diamComponenteconexo");
open diamComponenteconexo.pdf