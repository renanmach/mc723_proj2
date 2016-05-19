
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

**Pipeline de 5 estágios no MIPS** 

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

**Pipeline de 7 estágios no MIPS** [2]

Todos os estágios da pipeline de 5 estágios estão presentes na pipeline de 7 estágios, porém os estágios IF e MEM são subdivididos em 2 estágios cada. IF1, IF2 e MEM1, MEM2.

	IF1-(A)-IF2-(B)-ID-(C)-EX-(D)-MEM1-(E)-MEM2-(F)-WB

Pipeline registers:guarda informação produzida no ciclo anterior
A) IF1/IF2
B) IF2/ID
C) ID/EX
D) EX/MEM1
E) MEM1/MEM2
F) MEM2/WB


## **Branch predictor**

Utilizaremos as seguintes estratégias:

* Branch estático: escolhemos sempre NÃO tomar o branch.  ####Acho melhor sempre tomar o branch,tenho referencias para explicar porque escolhemos isso depois:https://www.cs.umd.edu/class/fall2013/cmsc411-0201/lectures/lec09.pdf####
* Branch dinâmico: 2-bit predictor. Cria uma máquina de estados e apenas muda a predição em duas predições errados consecutivas.

## **Cache**
Utilizaremos seis configurações diferentes de cache L1 e L2 conforme tamanhos de caches utilizadas em processadores reais, são elas:


| # | l1-usize | l1-ubsize | l1-uassoc | l2-usize | l2-ubsize | l2-uassoc | Replacement |
|---|----------|-----------|-----------|----------|-----------|-----------|-------------|
| 1 | 32KB     | 64B       | 2         | 256KB    | 64B       | 2         | LRU         |
| 2 | 64KB     | 128B      | 2         | 256KB    | 64B       | 2         | LRU         |
| 3 | 32KB     | 64B       | 2         | 512KB    | 128B      | 4         | LRU         |
| 4 | 64KB     | 128B      | 2         | 512KB    | 128B      | 4         | LRU         |
| 5 | 64KB     | 128B      | 4         | 512KB    | 128B      | 4         | LRU         |
| 6 | 64KB     | 128B      | 8         | 512KB    | 128B      | 8         | LRU         |


Utilizaremos o software dineroIV para medir os *misses* das caches L1 e L2. Modificaremos o simulador para gerar arquivos de saída no formato Dinero File Format.

## **Processador**
Vamos comparar dois tipos de processadores:

* Escalar : processadores com apenas 1 pipeline
* Superescalar: processadores que tem mais de 1 pipeline

A comparação será com base no número de ciclos que cada tipo de processador leva para executar os benchmarks.


## **Hazards**

-Hazards:conceitos (para pipeline de 5 estágios)

 *	Dados: uma instrução precisa de um resultado que ainda não foi completamente calculado. 
  	Exemplo: registrador *t1* é usado na segunda instrução, mas resultado da soma da primeira instrução ainda não foi armazenado em t1.

>	add $t1, $t2, $t3
  	add $t5, $t1, $t4 	
  
  *	Estrutural: há um conflito no uso de um recurso 
  	Exemplo: instrução *ld* se encontra no estágio MEM do pipeline e tenta ler da memória um dado mas a instrução *mul* também tenta ser lido da memória para o estágio IF. ** NAO SEI SE O Q EU DISSE ESTA CERTO!!!!!!**

>	ld $t1,20($s1)
	add $t2,$t3,$t4
	sub $t2,$t3,$t4
	mul $t2,$t3,$t4

  c) Controle: um branch é executado e todo o progresso do pipeline deve ser descartado.
	Exemplo: se o resultado da comparação do *beq* no estágio EXE for igual, então o processador terá que descartar as instruções *add* e *sub* que já estavam nos estágios ID e IF, respectivamente, do pipeline para dar o fetch da instrução da linha 50.**NAO SEI SE O Q EU DISSE ESTA CERTO!!!!!!**
	
>	beq $t1,$t2,50
	add $t2,$t3,$t4
	sub $t2,$t3,$t4
  
  
--------------------------------------------------------------------------------------- 
Dentre esses 3, vamos analizar apenas 2 tipos de hazards, o de dados e o de controle. Não iremos estudar o hazard estrutural, pois no simulador dineroIV já é possível adotar um mecanismo para contornar esse problema. Isto é, nós especificamos que as caches de dados e de instruções devem ser separados, e desse modo não haverá conflito no acesso à memória entre um dado utilizado por uma instrução do tipo L(load/store) no estágio MEM e o fetch de uma instrução no estágio IF. **A única possibilidade de ainda ocorrer um hazard estrutural nesse caso, seria se ocorresse *cache miss* em L1 e L2 na cache de dados e de instrução ao mesmo tempo, e então ambas as intruções tentariam acessar a memória ao mesmo tempo, mas a probabilidade de ocorrer isso é muito baixa. NAO SEI SE O Q EU DISSE ESTA CERTO!!!!!!**



**Hazard de dados**
Esse tipo de hazard ocorre quando temos instruções que dependem de dados de uma instrução anterior da pipeline. 

Podemos resolver esse tipo de hazard com *forwarding*, contudo, existem alguns tipos de hazard em que *stalls* ocorrem mesmo com o uso de *forwarding*. Trataremos o caso dos ***Load-Use Data Hazard***[1] e dos ***Data hazards for Branches***.

Estes tipos de Hazard são os que envolvem (Load/Store) L-type instruction. Há atraso de 1 ciclo por meio de 1 *stall*. Ocorre quando L-type instruction aparece seguida de uma instrução que usa o registrador de destino da linha anterior.
Desse modo, não há atrasos para o caso de Hazard de dados com instruções do tipo R.

Os Hazards de controle para branches ocorrem quando uma instrução branch utiliza um registrador que é alterado:

- na ALU de uma instrução (após o estágio EX/MEM) precedente (insere 1 bolha)
- por uma instrução load (após o estágio MEM_WB) precedente (insere 2 bolhas)
- por uma instrução load (após o estágio MEM_WB) que precede a instrução de branch por 2 posições (insere 1 bolha)**ESSE HAZARD NÃO É CONTORNADO SEPARANDO CACHE DE DADOS E DE INSTRUÇÃO???**
  	
Trataremos o caso dos ***Load-Use Data Hazard*** e dos hazards de controle para branches[1] 

Para o caso das **pipelines de tamanho 7,** o que vai mudar é o número de bolhas inseridas caso ocorra um hazard de dados.
Considere a tabela abaixo para as análises:

| 1       | 2       | 3      | 4       | 5         | 6         | 7       |
|---------|---------|--------|---------|-----------|-----------|---------|
| IF1/IF2 | IF2/ID  | ID/EX  | EX/MEM1 | MEM1/MEM2 | MEM2/WB   |         |
|         | IF1/IF2 | IF2/ID | ID/EX   | EX/MEM1   | MEM1/MEM2 | MEM2/WB |

**Para o caso de load-use:**
Resultado do load ocorre no final do estágio MEM2/WB e precisamos para o estágio EX (entre ID/EX e EX/MEM1), logo **2 bolhas** são necessárias.

**Para o caso dos hazards de dados para branches:**
No caso da instrução anterior ser uma instrução aritmética, o número de bolhas **continuará sendo 1** (já que precisamos da entrada de EX/MEM1 para a entrada de ID/EX)

No caso da instrução anterior ser uma instrução de load, o número de bolhas será **3**. Final de MEM2/WB para inicio de ID/EX.

No caso da instrução anterior da anterior ser uma instrução de load, o número de bolhas será **2**. Final de MEM2/WB para inicio de ID/EX.

**Hazard de controle**

São os hazards que ocorrem devido aos *branches*. Para estudar esse tipo de hazard, usaremos as diferentes estratégias de branch prediction descritas anteriormente. Quando ocorre um erro na predição do branch, as instruções que estavam na pipeline são perdidas, gerando bolhas na pipeline.

 O número de bolhas depende dos cache misses.
  
  *	Se a instrução já estiver na Cache L1, não há penalidade
  *	Se a instrução não estiver na Cache L1, há penalidade de X1 ciclos para ir até a Cache L2.
  *	Se a instrução não estiver na Cache L2, há penalidade de X2 ciclos para ir buscar até a memória.
  Supondo que há apenas uma memória que guardam tanto dados e instruções, quando há um acesso à memória por uma instrução do tipo L no quarto estágio do Pipeline (MEM), isso pode causar mais uma bolha para o fetch da instrução no primeiro estágio do Pipeline (IF).Ou seja, deve-se esperar que o acesso a memória de outra instrução seja finalizado para poder buscar a instrução atual. Porém no nosso caso, iremos separar uma cache somente para instruções e outra somente para dados.

	--------------------------------------

####**Static branch prediction**

Escolhemos sempre NÃO tomar o branch **EU TENHO UMA EXPLICAÇÃO PARA OUTRO BRANCH E ESTA AKI EMBAIXO**

Escolhemos deixar por padrão sempre tomar o branch. A justificativa para adotarmos essa política foi porque devido a estudos do benchmark SPEC92 testado em diversos tipos de compiladores como gcc, compress, li, espresso, notou-se que na média a frequencia com que um branch não é tomada é de apenas 34%. [3]

#### **Dynamic Branch prediction:**

Como no static prediction todas as decisões são feitas em tempo de compilação, isso não permite que o esquema de previsão se adapte ao comportamento do programa que muda no decorrer do tempo.

Há uma Branch History Table(BHT) onde se armazena se comportamento das branch de um programa em tempo de execução. 
Por exemplo, se o bit é 0 então processador assume que o branch não é tomado e põe no IF da pipeline a instrução seguinte; se o bit é 1, então assume que o branch é tomado e pôe no IF da pipeline a instrução do alvo da branch.
O valor desse bit é atualizado após obter cada resultado da comparação da instrução de branch.

Porém há um problema com o Dynamic Branch Prediction de 1-bit. Em casos onde um branch quase sempre é tomado, como num laço duplo de *for* por exemplo, este esquema irá provavelmente prever incorretamente 2 vezes. Supondo 2 laços *for* aninhado, durante a inicial execução deles não há problema. Porém, no final do laço mais interno de *for*, haverá uma previsão incorreta. O bit está setado em 1, assumindo que se tomará um branch como das últimas vez, mas este sairá do loop interno, pois a condição do loop já não é mais satisfeita. Desse modo o bit seta-se para 0. Então chegando à última linha de execução do *for* mais externo, ocorrerá mais um erro de previsão, pois o branch será tomado para reiniciar esse loop mas o bit de previsão nos diz o contrário.

Desse modo, para evitar esses atrasos adicionais devido a previsões incorretas, que influenciam principalmente para p pipelines mais longas, como a de 7 estágios, resolvemos optar em usar o Branch predictorde 2-bits que contorna esse problema. Abaixo temos as seguintes combinações de bits:
>	00 = fortemente não-tomada = processador assume que branch não é tomado
	01 = fracamente não-tomada = processador assume que branch não é tomado
	10 = fracamente tomado = processador assume que branch é tomado
	11 = fortemente tomado = processador assume que branch é tomado
	
Desse modo, após o resultado de cada comparação de uma instrução branch, é atualizada a BHT, alterando-se em 1 bit. Isso significa que será necessário 2 branchs não-tomados(ou tomados) consecutivos para que o processador passe a prever diferente.

Assim, mesmo que uma branch seja incorretamente prevista, como no exemplo anterior dos 2 laços *for* aninhados, durante a saída do loop mais interno, na próxima branch provavelmente não será incorreta. Afinal, na BHT estará marcada *10*, que ainda assume que o branch será tomado.

Inicialmente antes da execução da primeira branch, iremos assumir que o branch é fracamente tomado, ou seja, será setado em *10*. **RENAN VC PODE ME CONFIRMAR ISSO NA SUA IMPLEMENTACAO???**

O esquema da máquina de estados de preditor de 2-bits se encontra na imagem abaixo.
**INCLUIR LINK AKI DA IMAGEMMMMMMMMMMMMMMMM**
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

	
	

	




## **Referências**
[1] [Slides professor Sandro. Capítulo 4](http://www.ic.unicamp.br/~sandro/cursos/mc722/2s2014/slides/Chapter_04.pdf)
[2] [Implementation and Verification of a 7-Stage Pipeline Processor](http://publications.lib.chalmers.se/records/fulltext/215194/215194.pdf)
[3] [Computer Systems Architecture. Lecture 9](https://www.cs.umd.edu/class/fall2013/cmsc411-0201/lectures/lec09.pdf)
