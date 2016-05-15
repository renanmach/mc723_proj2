#!/bin/bash

# Executa o software dinero para os traces dos branchmarks

export curr_dir=$(pwd);

export DINERO=/home/renan/semestre_13/mc723/mc723_proj2/dinero4sbc/;

# configuracoes da cache
export L1_USIZE=64K
export L1_UBSIZE=128
export L1_UASSOC=2
export L2_USIZE=256K
export L2_UBSIZE=64
export L2_UASSOC=2

cd $DINERO;

CACHE[0]="-l1-usize 32K -l1-ubsize 64 -l1-uassoc 2 -l2-usize 256K -l2-ubsize 64 -l2-uassoc 2";
CACHE[1]="-l1-usize 64K -l1-ubsize 128 -l1-uassoc 2 -l2-usize 256K -l2-ubsize 64 -l2-uassoc 2";
CACHE[2]="-l1-usize 32K -l1-ubsize 64 -l1-uassoc 2 -l2-usize 512K -l2-ubsize 128 -l2-uassoc 4";
CACHE[3]="-l1-usize 64K -l1-ubsize 128 -l1-uassoc 2 -l2-usize 512K -l2-ubsize 128 -l2-uassoc 4";
CACHE[4]="-l1-usize 64K -l1-ubsize 128 -l1-uassoc 4 -l2-usize 512K -l2-ubsize 128 -l2-uassoc 4";
CACHE[5]="-l1-usize 64K -l1-ubsize 128 -l1-uassoc 8 -l2-usize 512K -l2-ubsize 128 -l2-uassoc 8";

i=1;

for conf in "${CACHE[@]}"
do
    echo "cache: ";
    echo $conf;
    
    echo "";
    
    echo "Executando dinero para patricia";
    ./dineroIV $conf -informat d < trace_patricia.txt >> results/result_patricia_dinero_$i.txt;
    if [ $? -eq 0 ]; then
        echo "    Simulacao para patricia executada com sucesso!";
    else
        echo "    Erro ao executar o dinero para patricia!";
    fi

    echo "";
    
    echo "Executando dinero para qsort";
    ./dineroIV $conf -informat d < trace_qsort.txt >> results/result_qsort_dinero_$i.txt;
    if [ $? -eq 0 ]; then
        echo "    Simulacao para qsort executada com sucesso!";
    else
        echo "    Erro ao executar o dinero para qsort!";
    fi

    echo "";

    echo "Executando dinero para dijkstra";
    ./dineroIV $conf -informat d < trace_dijkstra.txt >> results/result_dikjstra_dinero_$i.txt;
    if [ $? -eq 0 ]; then
        echo "    Simulacao para dijkstra executada com sucesso!";
    else
        echo "    Erro ao executar o dinero para dijkstra!";
    fi
    
    echo "";
    
    i=$((i+1));
done;

# Descomentar para deletar os traces
rm -f trace_patricia.txt;
rm -f trace_qsort.txt;
rm -f trace_dijkstra.txt;

# volta para o diretorio de scripts
cd $curr_dir;
