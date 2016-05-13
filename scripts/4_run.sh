## roda os simuladores para o benchmark criado

export curr_dir=$(pwd);

export BENCHMARKS=/home/renan/semestre_13/mc723/mc723_proj2/benchmarks;
export DINERO=/home/renan/semestre_13/mc723/mc723_proj2/dinero4sbc/;
export MIPS_SIM_DIR=/home/renan/semestre_13/mc723/mc723_proj2/mips-1.0.2;
export MIPSX=$MIPS_SIM_DIR/mips.x;

export PATRICIA=$BENCHMARKS/network/patricia/patricia
export PATRICIA_DATA=$BENCHMARKS/network/patricia/small.udp
export QSORT=$BENCHMARKS/automotive/qsort/qsort_large
export QSORT_DATA=$BENCHMARKS/automotive/qsort/input_large.dat
export DIJKSTRA=$BENCHMARKS/network/dijkstra/dijkstra_large
export DIJKSTRA_DATA=$BENCHMARKS/network/dijkstra/input.dat

cd $MIPS_SIM_DIR;

# patricia
$MIPSX --load=$PATRICIA $BENCHMARKS/network/patricia/small.udp;
if [ $? -eq 0 ]; then
    mv -f traces/trace.txt $DINERO/trace_patricia.txt;
else
    echo "Erro ao executar o mips para patricia!\n";
fi

#qsort
$MIPSX --load=$QSORT $QSORT_DATA;
if [ $? -eq 0 ]; then
    mv -f traces/trace.txt $DINERO/trace_qsort.txt;
else
    echo "Erro ao executar o mips para qsort!\n";
fi

#dijkstra
$MIPSX --load=$DIJKSTRA $DIJKSTRA_DATA
if [ $? -eq 0 ]; then
    mv -f traces/trace.txt $DINERO/trace_dijkstra.txt;
else
    echo "Erro ao executar o mips para dijkstra!\n";
fi

cd $curr_dir;
