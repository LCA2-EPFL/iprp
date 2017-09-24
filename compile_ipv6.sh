#!/bin/bash
rm -rf bin
mkdir bin

gcc -w src/icd/* src/lib/* -o bin/icd -std=c99 -I inc/ -lpthread -lnfnetlink -lnetfilter_queue -lm -Wfatal-errors -D IPRP_IPV6
gcc -w src/isd/* src/lib/* -o bin/isd -std=c99 -I inc/ -lpthread -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w src/ird/* src/lib/* -o bin/ird -std=c99 -I inc/ -lpthread -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w src/imd/* src/lib/* -o bin/imd -std=c99 -I inc/ -lpthread -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6

mkdir bin/tools

gcc -w tools/iprptest.c      src/lib/* -o bin/tools/iprptest      -std=c99 -I inc/ -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w tools/iprptracert.c   src/lib/* -o bin/tools/iprptracert   -std=c99 -I inc/ -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w tools/iprpping.c      src/lib/* -o bin/tools/iprpping      -std=c99 -I inc/ -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w tools/iprprecvstats.c src/lib/* -o bin/tools/iprprecvstats -std=c99 -I inc/ -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6
gcc -w tools/iprpsendstats.c src/lib/* -o bin/tools/iprpsendstats -std=c99 -I inc/ -lnfnetlink -lnetfilter_queue -Wfatal-errors -D IPRP_IPV6