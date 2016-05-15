# Edite o arquivo boot.sh, mudando a linha glibtoolize para:
# glibtoolize --version 2> /dev/null

./boot.sh
./configure --prefix=--prefix=/home/renan/semestre_13/mc723/mc723_proj2/archc/build/ --with-systemc=--prefix=/home/renan/semestre_13/mc723/mc723_proj2/systemc/build/ --with-tlm=--prefix=/home/renan/semestre_13/mc723/mc723_proj2/systemc/include/
# COLOCAR O ARQUIVO archc_lex.c na pasta src/acpp
make
make install
