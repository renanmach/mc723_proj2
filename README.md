 Projeto 2: MC723  
=====
##  Professor: *Lucas Wanner* 
##  Alunos:
## 	*Renato Toshiaki Shibata* *082674* 
##   	*Rafael Faria Castelão* *  118439 *
##   	*Renan Gomes Pereira* *  103927 *
##   	*	Rodrigo de Sousa Serafim da Silva* *  	118607 *

--> Coisas que precisamos aprender:
	- configurar ArchC para detectar Hazards
	- como usar Pipelines? Com 5, 7 e 13 estagios?
	- como calcular brach correct/incorrect prediction?

--> Coisas que já sabemos:
	- configurar cache e ver cache miss/hit rate ->DineroIV
	- numero de ciclos totais e CPI médio -> simulador MIPS
	- tempo gasto para rodar um programa -> primeiro exercicio

Roteiro
----------

## Benchmarks
*	jpeg coder
*	algum outro
*	algum outro

## Critérios avaliados
*	número de hazards
*	número de ciclos e CPI médio
*	tempo de execução
*	miss rate L1 e L2
*	branch prediction rate

## Configuração de branch
*	Branch Estático: Supor que todos os branchs ocorrem, e calcular penalidade de ciclos, caso não tenha ocorrido o branch.
*	Branch Dinâmico: Utilizar uma tabela de 2 bits que contabiliza se houve ou não recentemente um branch, e armazenar essas informações para criar uma máquina de estados para gerar a previsão de branch ou não.

## Roteiro dos experimentos:
