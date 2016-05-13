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


echo "Executando dinero para patricia\n";
./dineroIV -l1-usize $L1_USIZE -l1-ubsize $L1_UBSIZE -l2-usize $L2_USIZE -l2-ubsize $L2_UBSIZE -l2-uassoc $L2_UASSOC -informat d < trace_patricia.txt >> results/result_patricia.txt;
if [ $? -eq 0 ]; then
    echo "Simulacao para patricia executada com sucesso!\n";
    rm -f trace_patricia.txt;
else
    echo "Erro ao executar o dinero para patricia!\n";
fi

echo "Executando dinero para qsort\n";
./dineroIV -l1-usize $L1_USIZE -l1-ubsize $L1_UBSIZE -l2-usize $L2_USIZE -l2-ubsize $L2_UBSIZE -l2-uassoc $L2_UASSOC -informat d < trace_qsort.txt >> results/result_qsort.txt;
if [ $? -eq 0 ]; then
    echo "Simulacao para qsort executada com sucesso!\n";
    rm -f trace_qsort.txt;
else
    echo "Erro ao executar o dinero para qsort!\n";
fi

echo "Executando dinero para dijkstra\n";
./dineroIV -l1-usize $L1_USIZE -l1-ubsize $L1_UBSIZE -l2-usize $L2_USIZE -l2-ubsize $L2_UBSIZE -l2-uassoc $L2_UASSOC -informat d < trace_dijkstra.txt >> results/result_dikjstra.txt;
if [ $? -eq 0 ]; then
    echo "Simulacao para dijkstra executada com sucesso!\n";
    rm -f trace_dijkstra.txt;
else
    echo "Erro ao executar o dinero para dijkstra!\n";
fi

# volta para o diretorio de scripts
cd $curr_dir;
