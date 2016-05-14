
 **MC723 Projeto 2: Desempenho do Processador**  
=====

###  **Professor**: *Lucas Wanner* 
###  **Alunos**:
### 	*Renato Toshiaki Shibata* *082674* 
### *Rafael Faria Castelão* *118439*
### *Renan Gomes Pereira* *103927*
### *Rodrigo de Sousa Serafim da Silva* *118607*


#**Roteiro**

## **Benchmarks**
*	patricia (small)
* dijkstra (large)
* qsort (large)

## **Critérios avaliados**
*	Número de hazards
*	Número de ciclos
*   CPI médio
*	Tempo de execução
*	Miss rate L1 e L2
*	Branch prediction rate


## **Pipeline**

Todos os nossos experimentos e critérios avaliados serão feitos para pipelines de dois tamanhos diferentes:

* 5 estágios
* 7 estágios

Modificaremos o código do simulador para avaliar os efeitos da pipeline para cada estágio escolhido.


## **Branch predictor**

Utilizaremos as seguintes estratégias:

* Branch estático: escolhemos sempre NÃO tomar o branch.
* Branch dinâmico: 2-bit predictor. Cria uma máquina de estados e apenas muda a predição em duas predições errados consecutivas.

## **Cache**
Utilizaremos quatro configurações diferentes de cache L1 e L2 conforme tamanhos de caches utilizadas em processadores reais, são elas:


| # | l1-usize | l1-ubsize | l1-uassoc | l2-usize | l2-ubsize | l2-uassoc | Replacement |
|---|----------|-----------|-----------|----------|-----------|-----------|-------------|
| 1 | 32KB     | 64B       | 2         | 256KB    | 64B       | 2         | LRU         |
| 2 | 64KB     | 128B      | 2         | 256KB    | 64B       | 2         | LRU         |
| 3 | 32KB     | 64B       | 2         | 512KB    | 128B      | 4         | LRU         |
| 4 | 64KB     | 128B      | 2         | 512KB    | 128B      | 4         | LRU         |


Utilizaremos o software dineroIV para medir os *misses* das caches L1 e L2. Modificaremos o simulador para gerar arquivos de saída no formato Dinero File Format.

## **Processador**
Vamos comparar dois tipos de processadores:

* Escalar : processadores com apenas 1 pipeline
* Superescalar: processadores que tem mais de 1 pipeline

Pipeline de 5 estágios no MIPS: 

IF-(A)-ID-(B)-EX-(C)-MEM-(D)-WB

Legenda:
1)Instruction fetch
2)Instruction decode & register read
3)Execute operation 
4)Access memory operand
5)Write back

Pipeline registers:guarda informação produzida no ciclo anterior
A) IF/ID
B) ID/EX
C) EX/MEM
D) MEM/WB


A comparação será com base no número de ciclos que cada tipo de processador leva para executar os benchmarks.


## **Hazards**

-Hazards:conceitos

  a) Dados: Uma instrução precisa de um resultado que ainda não foi completamente calculado. ex:
  add t1, t2, t3
  add t5, t1, t4
  Mecanismo para contornar isso:forwarding; 
  Mandar o resultado da ALU assim que for computado através de conexões usando os pipeline registers
  
  b) Estrutural: há um conflito no uso de um recurso
  	Exemplo: instruções do tipo L(load/store) exigem acesso a memória e causam conflito com o fetch de uma instrução
  Mecanismo: cache de instruções e dados separados
  
  c) Controle: Um branch é executado e todo o progresso do pipeline deve ser descartado
  Mecanismo: branch prediction
  
  
  O número de bolhas depende dos cache misses.
  
  i)Se a instrução já estiver na Cache L1, não há penalidade
  ii)Se a instrução não estiver na Cache L1, há penalidade de X1 ciclos para ir até a Cache L2.
  iii)Se a instrução não estiver na Cache L2, há penalidade de X2 ciclos para ir buscar até a memória.
  Supondo que há apenas uma memória que guardam tanto dados e instruções, quando há um acesso à memória por uma instrução do tipo L no quarto estágio do Pipeline (MEM), isso pode causar mais uma bolha para o fetch da instrução no primeiro estágio do Pipeline (IF).
  Ou seja, deve-se esperar que o acesso a memória de outra instrução seja finalizado para poder buscar a instrução atual.
  
Vamos analizar dois tipos de hazards:

**Hazard de dados**
Esse tipo de hazard ocorre quando temos instruções que dependem de dados de uma instrução anterior da pipeline. 
Podemos resolver esse tipo de hazard com *forwarding*, contudo, existem alguns tipos de hazard em que *stalls* ocorrem mesmo com o uso de *forwarding*. 

Estes tipos de Hazard são os que envolvem (Load/Store) L-type instruction. Há atraso de 1 ciclo por meio de 1 *stall*. Ocorre quando L-type instruction aparece seguida de uma instrução que usa o registrador de destino da linha anterior.
  		ex.: 	lw $s0, 20($t1)
  			    sub $t2,$s0,$t3
  			     # $s0 usado como registrador destino na primeira instrução e como operando na instrução seguinte 

Desse modo, não há atrasos para o caso de Hazard de dados com instruções do tipo R.
Para instruções do tipo Acesso à memória, há 1 ciclo de atraso.
  	
Trataremos o caso dos ***Load-Use Data Hazard***[1] 

**Hazard de controle**

São os hazards que ocorrem devido aos *branches*. Para estudar esse tipo de hazard, usaremos as diferentes estratégias de branch prediction descritas anteriormente. Quando ocorre um erro na predição do branch, as instruções que estavam na pipeline são perdidas, gerando bolhas na pipeline.

O número de bolhas depende do estágio em que o branch é tomado.
ex.:      add $4, $5 ,$6  
          beq $1,$2,40
          lw $3,300($0)

linha 40: or $7,$8,$9

- Prediction correct --> 0 bolhas
- Prediction incorrect --> N bolhas
Se não usar o branch prediction --> X bolhas 

	--------------------------------------

Static branch prediction:

### Realmente sempre não tomar o branch? ##
	a) se for branch para alguma linha atras 
		=> branch taken
	b) se for branch para alguma linha pra frente
		=> branch not taken

Dynamic Branch prediction:

- Máquina de estados:
- incluir aki link para a figura da maquina de estados
- 
## **Experimentos**

Utilizaremos as seguintes configurações do processador MIPS:

#### **Pipeline 5 e 7 estágios* **

| Configuração | Cache | Tipo         | Branch prediction |
|--------------|-------|--------------|-------------------|
| 1            | 3     | Escalar      | Nenhum            |
| 2            | 3     | Escalar      | Estático          |
| 3            | 3     | Escalar      | Dinâmico          |
| 4            | 1     | Escalar      | Estático          |
| 5            | 2     | Escalar      | Estático          |
| 6            | 4     | Escalar      | Nenhum            |
| 7            | 1     | Escalar      | Dinâmico          |
| 8            | 4     | Escalar      | Dinâmico          |
| 9            | 4     | Superescalar | Dinâmico          |
| 10           | 4     | Escalar      | Nenhum            |
| 11           | 4     | Superescalar | Nenhum            |

* *todas as medidas  serão feitas tanto para pipelines de 5 e 7 estágios.*

	Cáluclo da CPI:
	####o CPI será computado de acordo com issue (escalar vs. superescalar), bolhas introduzidas devido a hazards, atraso no acesso à memória, e
 atraso por branches

	
	

	




## **Referências**
[1] [Slides professor Sandro. Capítulo 4](http://www.ic.unicamp.br/~sandro/cursos/mc722/2s2014/slides/Chapter_04.pdf)
