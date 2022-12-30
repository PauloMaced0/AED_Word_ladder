#
# Tomás Oliveira e Silva, AED, November 2022
#
# makefile to compile the A.02 assignment (word ladder)
#

clean:
	rm -rf a.out word_ladder 


word_ladder:		word_ladder.c
	cc -Wall -Wextra -O2 word_ladder.c -o word_ladder -lm

