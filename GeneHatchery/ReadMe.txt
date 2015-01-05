This is a program using gene algorithm to improve our AgentGo. G_T_P supported
Run "GeneHatchery" with no parameters to show help

Network Mode Limits: 50 genes with 10 DNA positions
Gene positions should be even!
MUTATION_RATE should be int,MUTATION_PERCENTAGE be double!!

Change validate_gene() to make genes valid!!!!! (Don't forget to re-compile the program!!)

Examples :
GeneHatchery
GeneHatchery master "AgentGo_AI.exe" "C:\Users\Menooker\Desktop\Go\gnugo-3.8\gnugo.exe --mode gtp --level 1"  -d3 1000 1 1 -s 2 -c 6 -r 10 -f progress.ghp
GeneHatchery slave "AgentGo_AI.exe" "C:\Users\Menooker\Desktop\Go\gnugo-3.8\gnugo.exe --mode gtp --level 1" 12.34.5.6 3000
