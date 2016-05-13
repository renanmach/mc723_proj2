#!/bin/sh
#
#Descobrir o tamanho de cache ideal mantendo resto dos parametros como default
#programa selecionado:164.gzip.f2b

#supondo que me encontro na rede do IC
traces="/home/staff/lucas/mc723/traces/164.gzip.f2b"
p1="164.gzip.f2b"
home_ex2="/home/ec2010/ra082674/MC723-LucasWanner/MC723A-profLucas/exercicio2/output.txt"


cd ${traces}

#parametros a serem avaliados


i_="1024"
d_="1024"
ib_=16 #default
db_=16

for j in $(seq 1 1 10) # 2K,4K,8K,16K,32K, 64K,128K,256K,512K,1M
	do
		i_=$(( i_ * 2 ))
		d_=$(( d_ * 2 )) 		
		out_="${home_ex2/output./${d_}__${p1}.}"
		echo "---------"	
		echo "aqui est $out_"
		echo "data cache tam: $d_ bits"
		echo "inst cache tam: $i_ bits"
		echo "data: $db_ B"
		echo "instrucao: $ib_ B"
		echo "---------"
		#executa e joga output em output.txt na minha pasta no ic
		../../dinero4sbc/dineroIV -informat s -trname gzip_f2b -maxtrace 20 -l1-isize ${i_} -l1-dsize ${d_} -l1-ibsize ${ib_} -l1-dbsize ${db_} > ${out_}
	done
name="gzip"
mv *.*[$name].* ${name}/


