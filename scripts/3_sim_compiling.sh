# Auxilia na compilacao do simulador
# Nota que devemos: 
# adicionar -B/usr/lib/gold-ld/ 
# na variavel libs em Makefile.archc do simulador manualmente

export curr_dir=$(pwd);

export PATH=$PATH:/home/renan/semestre_13/mc723/mc723_proj2/archc/build/bin/:/home/renan/semestre_13/mc723/mc723_proj2/mips_compiler/bin/;
export LD_LIBRARY_PATH=/home/renan/semestre_13/mc723/mc723_proj2/systemc/build/lib-linux64:$LD_LIBRARY_PATH;
export MIPS_SIM=/home/renan/semestre_13/mc723/mc723_proj2/mips-1.0.2/;

cd $MIPS_SIM;

#acsim mips.ac -abi;
make -f Makefile.archc clean;
make -f Makefile.archc;

cd $curr_dir;
