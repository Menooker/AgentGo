This is a program using gene algorithm to improve our AgentGo. G_T_P supported
Run "GeneHatchery" with no parameters to show help

Network Mode Limits: 50 genes with 10 DNA positions
Gene positions should be even!
MUTATION_RATE should be int,MUTATION_PERCENTAGE be double!!

--------------------
ghp2txt
--------------------
	covert a ghp file to readable txt file.


--------------------
txt2ghp
--------------------
	covert a txt file to ghp file. Then you can use "-f XXX.ghp" to run the genes you have just inputted!


--------------------
structure of the txt file
--------------------

[mutation rate],[mutation percentage],[slaves],[DNA positions],[competitors]
XXX,XXX,XXX,XXX........,XXX
XXX,XXX,XXX,XXX........,XXX
XXX,XXX,XXX,XXX........,XXX
XXX,XXX,XXX,XXX........,XXX


Change validate_gene() in the source code to make genes valid!!!!! (Don't forget to re-compile the program!!)


usage for master mode:
	GeneHatchery master {debugee} {reference} [-mp {mutation percentage(double)}] [-mr {mutation rates(int)}] [-f {path to saved records}] [-s {slaves}] [-r {generations}] [-c {competitors}] [-d {DNAs dna0 dna1 dna2...}]
usage for slave mode:
	GeneHatchery slave {debugee} {reference} {master ip} {port}
usage for ghp2txt or txt2ghp
	GeneHatchery ghp2txt {src ghp file} {dest txt file}
	GeneHatchery txt2ghp {src txt file} {dest ghp file}

Examples :
GeneHatchery -f progress.ghp
GeneHatchery master "AgentGo_AI.exe" "C:\Users\Menooker\Desktop\Go\gnugo-3.8\gnugo.exe --mode gtp --level 1"  -d3 1000 1 1 -s 2 -c 6 -r 10 
GeneHatchery slave "AgentGo_AI.exe" "C:\Users\Menooker\Desktop\Go\gnugo-3.8\gnugo.exe --mode gtp --level 1" 12.34.5.6 3000
GeneHatchery ghp2txt c:\2.ghp out.txt
GeneHatchery txt2ghp hehe.txt out.ghp