
 **MC723 Projeto 2: Desempenho do Processador**  
=====

###  **Professor**: *Lucas Wanner* 
###  **Alunos**:
### 	*Renato Toshiaki Shibata* *082674* 
### *Rafael Faria Castelão* *118439*
### *Renan Gomes Pereira* *103927*
### *Rodrigo de Sousa Serafim da Silva* *118607*


#**Relatório**

Neste experimento, avaliamos o desempenho de um processador, de acordo com várias características arquiteturais. São elas:

- Tamanho do pipeline: 5 e 7 estágios
- Hazard de dados e controle
- Branch predictor 
- Cache



### **Benchmarks**

Os benchmarks utilizados foram:

- patricia (small)
- dijkstra (large)
- qsort (large)

### **Critérios avaliados**

*	Número de hazards
*	Número de ciclos
*   CPI médio
*	Tempo de execução
*	Miss rate L1 e L2
*	Branch prediction rate


### **Pipeline**

Todos os nossos experimentos e critérios avaliados serão feitos para pipelines de dois tamanhos diferentes:

* 5 estágios
* 7 estágios

Modificamos o código do simulador para avaliar os efeitos da pipeline para cada estágio escolhido.

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

Pipeline registers:guarda informação produzida no ciclo anterior
A) IF1/IF2
B) IF2/ID
C) ID/EX
D) EX/MEM1
E) MEM1/MEM2
F) MEM2/WB


#### **Branch predictor**

Utilizamos as seguintes estratégias:

- Branch estático: escolhemos sempre não tomar o branch.
- Branch dinâmico: 2-bit predictor. Cria uma máquina de estados e apenas muda a predição em duas predições errados consecutivas.

## **Cache**
Utilizamos seis configurações diferentes de cache L1 e L2, são elas:


| # | l1-usize | l1-ubsize | l1-uassoc | l2-usize | l2-ubsize | l2-uassoc | Replacement |
|---|----------|-----------|-----------|----------|-----------|-----------|-------------|
| 1 | 32KB     | 64B       | 2         | 256KB    | 64B       | 2         | LRU         |
| 2 | 64KB     | 128B      | 2         | 256KB    | 64B       | 2         | LRU         |
| 3 | 32KB     | 64B       | 2         | 512KB    | 128B      | 4         | LRU         |
| 4 | 64KB     | 128B      | 2         | 512KB    | 128B      | 4         | LRU         |
| 5 | 64KB     | 128B      | 4         | 512KB    | 128B      | 4         | LRU         |
| 6 | 64KB     | 128B      | 8         | 512KB    | 128B      | 8         | LRU         |


Utilizamos o software dineroIV para medir os *misses* das caches L1 e L2. Modificamos o simulador para gerar arquivos de saída no formato Dinero File Format.

O número de ciclos adicionados considerando a cache está na tabela abaixo*:

| Evento            | Penalidade |
|-------------------|------------|
| L1 HIT            | 1          |
| L2 HIT            | 12         |
| L2 MISS (RAM HIT) | 71**       |


*valores encontrados em: (http://www.7-cpu.com/cpu/Skylake.html) e (http://www.7-cpu.com/cpu/Mips74K.html)

** considerando o processador MIPS 74K de 560MHz e RAM Latency de 42 + 51ns (segundo o link Skylake).

  O número de bolhas depende dos cache misses.
  

### **Processador**   
Vamos comparar dois tipos de processadores:

* Escalar : processadores com apenas 1 pipeline
* Superescalar: processadores que tem mais de 1 pipeline, no nosso caso 2 pipelines(2-way superscalar)

A comparação será com base no número de ciclos que cada tipo de processador leva para executar os benchmarks.
Sabemos que os processadores super escalar amplificam a penalidade dos stalls.
Porém, para instruções que são indenpendentes entre si, a execução em paralela leva a uma economia de ciclos, pois instruções são executadas em paralelo no mesmo ciclo.

### **Hazards**

#### Conceitos (para pipeline de 5 estágios)

 *	Dados: uma instrução precisa de um resultado que ainda não foi completamente calculado. 
  	Exemplo: registrador *t1* é usado na segunda instrução, mas resultado da soma da primeira instrução ainda não foi armazenado em t1.

>	add $t1, $t2, $t3
  	add $t5, $t1, $t4 	
  
  *	Estrutural: há um conflito no uso de um recurso 
  	Exemplo: instrução *ld* se encontra no estágio MEM do pipeline e tenta ler da memória um dado mas a instrução *mul* também tenta ser lido da memória para o estágio IF.

>	ld $t1,20($s1)
	add $t2,$t3,$t4
	sub $t2,$t3,$t4
	mul $t2,$t3,$t4

  c) Controle: um branch é executado e todo o progresso do pipeline deve ser descartado.
	Exemplo: se o resultado da comparação do *beq* no estágio EXE for igual, então o processador terá que descartar as instruções *add* e *sub* que já estavam nos estágios ID e IF, respectivamente, do pipeline para dar o fetch da instrução da linha 50.
	
>	beq $t1,$t2,50
	add $t2,$t3,$t4
	sub $t2,$t3,$t4
  
  
--------------------------------------------------------------------------------------- 

Dentre esses 3, vamos analizar apenas 2 tipos de hazards, o de dados e o de controle.

### **Hazard de dados**
Esse tipo de hazard ocorre quando temos instruções que dependem de dados de uma instrução anterior da pipeline. 

Podemos resolver esse tipo de hazard com *forwarding*, contudo, existem alguns tipos de hazard em que *stalls* ocorrem mesmo com o uso de *forwarding*. Trataremos o caso dos ***Load-Use Data Hazard***[1] e dos ***Data hazards for Branches***.

Estes tipos de Hazard são os que envolvem (Load/Store) L-type instruction. Há atraso de 1 ciclo por meio de 1 *stall*. Ocorre quando L-type instruction aparece seguida de uma instrução que usa o registrador de destino da linha anterior.		

Desse modo, não há atrasos para o caso de Hazard de dados com instruções do tipo R.
Para instruções do tipo Acesso à memória, há 1 ciclo de atraso.

Os Hazards de controle para branches ocorrem quando uma instrução branch utiliza um registrador que é alterado:

- na ALU de uma instrução (após o estágio EX/MEM) precedente (insere 1 bolha)
- por uma instrução load (após o estágio MEM/WB) precedente (insere 2 bolhas)
- por uma instrução load (após o estágio MEM/WB) que precede a instrução de branch por 2 posições (insere 1 bolha)
  	
Trataremos o caso dos ***Load-Use Data Hazard*** e dos hazards de controle para branches[1] 

#### **Pipelines 7 estágios** 

O que vai mudar é o número de bolhas inseridas caso ocorra um hazard de dados.
Considere a tabela abaixo para as análises:

| 1       | 2       | 3      | 4       | 5         | 6         | 7       |
|---------|---------|--------|---------|-----------|-----------|---------|
| IF1/IF2 | IF2/ID  | ID/EX  | EX/MEM1 | MEM1/MEM2 | MEM2/WB   |         |
|         | IF1/IF2 | IF2/ID | ID/EX   | EX/MEM1   | MEM1/MEM2 | MEM2/WB |

**Para o caso de load-use:**
Resultado do load ocorre no final do estágio MEM2/WB e precisamos para o estágio EX (entre ID/EX e EX/MEM1), logo **2 bolhas** são necessárias.

**Para o caso dos hazards de dados para banches:**
No caso da instrução anterior ser uma instrução aritmética, o número de bolhas **continuará sendo 1** (já que precisamos da entrada de EX/MEM1 para a entrada de ID/EX)

No caso da instrução anterior ser uma instrução de load, **o número de bolhas será 3**. Final de MEM2/WB para inicio de ID/EX.

No caso da instrução anterior da anterior ser uma instrução de load, **o número de bolhas será 2**. Final de MEM2/WB para inicio de ID/EX.

#### **2-way superpipeline 5 estágios** 
O que vai mudar não é só o número de bolhas inseridas como também os casos, quando ocorrer um hazard de dados.
Considere a tabela abaixo para as análises:

| 1       | 2       | 3      | 4       | 5         |
|---------|---------|--------|---------|-----------|
| IF/ID   | ID/EX   | EX/MEM | MEM/WB  |	   |  A
| IF/ID   | ID/EX   | EX/MEM | MEM/WB  |           |  B
|         | IF/ID   | ID/EX  | EX/MEM  | MEM/WB    |  A
|         | IF/ID   | ID/EX  | EX/MEM  | MEM/WB    |  B

**Para o caso de load-use:**
De modo similar ao *Pipeline de 5 estágios*, esse tipo de hazard é o que envolve (Load/Store) L-type instruction. 
Isso ocorre quando L-type instruction aparece seguida de uma instrução que usa o registrador de destino da linha anterior. Porém, como há 2 pipelines, o L-type instrucion pode aparecer tanto no estágio EX/MEM do pipeline A quanto no pipeline B1 Desse modo, deve-se verificar se tanto a instrução no estágio ID/EX do pipeline A quanto do pipeline B usa um dos registradores destino da L-type Instruction. Nesse caso, insere-se 1 bolha a mais.

**Para o caso de instruções em gerais em pipelines em paralelo**
Pode ocorrer de que quaisquer instruções, seja do tipo R ou do tipo L , possam causar um Hazard de dados. Isto é, como temos 2 pipelines em paralelo, em um mesmo estágio pode ocorre esse Hazard quando a instrução do pipeline A dependa do registrador destino da instrução do pipeline B, ou vice-versa. 

Para 2 instruções do tipo R, isso ocorre no estágio ID/EX, pois ainda não foi armazenado no registrador o valor da operação na ALU para a instrução em paralelo poder usar. Insere-se 1 bolha a mais.

Para 2 instruções do tipo L, isso ocorre no estágio EX/MEM, pois ainda não foi escrito no registrador o valor acessado da memória que vai ser utilizado pela instrução em paralelo como endereço base para também acessar da memória. Insere-se 1 bolha a mais.

Para 1 instrução do tipo L e outra do tipo R, insere-se 2 bolhas a mais. Para esse caso, como ambas as instruções estão no mesmo estágio, então para que a instrução tipo R consiga usar o registrador destino da outra instrução, este terá que ficar estacionado no estágio ID/EX enquanto a instrução L deve avançar até MEM/WB, e isso são 2 ciclos a mais.

De forma geral, temos 3 tipos de dependências:

*	True Data dependecies: read after write
Um conflito de recursos,

*	Output dependecies: write after write

*	Anti-dependencies: write after read

### **Hazard de controle**

São os hazards que ocorrem devido aos *branches*. Para estudar esse tipo de hazard, usaremos as diferentes estratégias de branch prediction descritas anteriormente. Quando ocorre um erro na predição do branch, as instruções que estavam na pipeline são perdidas, gerando bolhas na pipeline.

O número de bolhas depende do estágio em que o branch é tomado.

- Prediction correct --> 0 bolhas
- Prediction incorrect --> 2 e 3 bolhas para pipelines de 5 e 7 estágios respectivamente.

**Static branch prediction:**

Escolhemos sempre não tomar o branch

**Dynamic Branch prediction:**

Implementamos o 2-bit predictor.

Abaixo temos as seguintes combinações de bits:

-	00 = fortemente não-tomada = processador assume que branch não é tomado
-	01 = fracamente não-tomada = processador assume que branch não é tomado
-	10 = fracamente tomado = processador assume que branch é tomado
-	11 = fortemente tomado = processador assume que branch é tomado
	
	
	
Desse modo, após o resultado de cada comparação de uma instrução branch, é atualizada a BHT, alterando-se em 1 bit. Isso significa que será necessário 2 branchs não-tomados (ou tomados) consecutivos para que o processador passe a prever diferente se o preditor está em um estado forte, e 1 miss prediction se o preditor está no estado fraco.

Assim, mesmo que uma branch seja incorretamente prevista, como no exemplo anterior dos 2 laços *for* aninhados, durante a saída do loop mais interno, na próxima branch provavelmente não será incorreta. Afinal, na BHT estará marcada *10*, que ainda assume que o branch será tomado.

Inicialmente antes da execução da primeira branch, iremos assumir que o branch é fortemente tomado, ou seja, será setado em *00*.
Segue abaixo uma imagem da máquina de estados descrita acima:

![FSM](https://github.com/renanmach/mc723_proj2/blob/master/img/dynamicBP.png)

#### Jumps

Foi adicionada uma bolha para todas as intruções de jump para os dois tipos de pipeline.
	
## **Resultados**
	
### **Cache**

Com os resultados paras a métrica Data do simulador dineroIV montamos as tabelas abaixo:

#### **dijkstra(large)**:

L1 Demand Fetches =  63292732 (para todos os casos)

| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 307666           | 0.0049       | 338177            | 3153             | 0.0093       |
| 2     | 45438            | 0.0007       | 105362            | 5084             | 0.0483       |
| 3     | 307666           | 0.0049       | 338177            | 2004             | 0.0059       |
| 4     | 45438            | 0.0007       | 52681             | 2004             | 0.0382       |
| 5     | 26375            | 0.0004       | 29581             | 2011             | 0.0762       |
| 6     | 22540            | 0.0004       | 26964             | 1843             | 0.0818       |

#### **patricia(small)**:
	
L1 Demand Fetches = 60890897

| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 506890           | 0.0083       | 698600            | 55633            | 0.0796       |
| 2     | 216166           | 0.0036       | 512914            | 100442           | 0.1958       |
| 3     | 506890           | 0.0083       | 698600            | 25038            | 0.0358       |
| 4     | 216166           | 0.0036       | 256457            | 25219            | 0.0983       |
| 5     | 110338           | 0.0018       | 124394            | 24964            | 0.2007       |
| 6     | 54869            | 0.0009       | 62609             | 20601            | 0.3290       |




#### **qsort(large)**:
	
L1 Demand Fetches = 58954557


| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 48913            | 0.0008       | 70632             | 15743            | 0.2229       |
| 2     | 21540            | 0.0004       | 62298             | 27106            | 0.4351       |
| 3     | 48913            | 0.0008       | 70632             | 12489            | 0.1768       |
| 4     | 21540            | 0.0008       | 31149             | 12475            | 0.4005       |
| 5     | 19933            | 0.0003       | 28691             | 12484            | 0.4351       |
| 6     | 19817            | 0.0003       | 28498             | 12521            | 0.4394       |



#### Análise

Considerando apenas a cache L1, notamos uma diferença expressiva ao dobrar o tamanho da cache unificada e do tamanho do bloco unificado. Por exemplo, para o benchmark dijkstra, para as configurações 1 e 3 o L1 miss rate foi de 0.0049, enquanto que para as configuracões 2 e 4 (o dobro do tamanho da cache L1), o miss rate foi de apenas 0.0007, ou seja, o dobro da cache diminuiu o número de misses em 7 vezes.

As configurações 4, 5 e 6 apresentam os mesmos tamanhos de cache e blocos de cache tanto para L1 quanto para L2, mudando a associatividade das caches. Resolvemos escolher a **configuração 6** da cache para continuar a análise dos resultados devido a um menor número de misses na cache L2 comparado as outras configurações (com exceção do benchmark qsort). Como os misses na cache L2 implicam que o processador deve buscar o dado na memória RAM e a memória RAM apresenta uma penalidade bem maior do que as caches, é interessante minimizar o número de acessos a memória.

### **Resultados processador escalar**

Como dito na seção anterior, a cache escolhida foi a número 6:


| # | l1-usize | l1-ubsize | l1-uassoc | l2-usize | l2-ubsize | l2-uassoc | Replacement |
|---|----------|-----------|-----------|----------|-----------|-----------|-------------|
| 6 | 64KB     | 128B      | 8         | 512KB    | 128B      | 8         | LRU         |

Com o número de misses e hits para essa configuração montamos a tabela a seguir:

Legenda:
P1: Dijkstra(large)
P2: Patricia(small)
P3: Qsort(large)
BP: Branch Predictor
CH5: Control Hazards pipeline 5 stages
DH5: Data Hazards pipeline 5 stages
CH7: Control Hazards pipeline 7 stages
DH7: Data Hazards pipeline 7 stages


| Medida                  | BP Estático | BP Estático | BP Estático | BP Dinâmico | BP Dinâmico | BP Dinâmico |
|-------------------------|-------------|-------------|-------------|-------------|-------------|-------------|
|                         | P1          | P2          | P3          | P1          | P2          | P3          |
| Instruções              | 223690619   | 225953174   | 217701256   | 223690619   | 225953174   | 217701256   |
| Instruções Jump         | 8039342     | 2801415     | 2754523     | 8039342     | 2801415     | 2754523     |
| Instruções Branch       | 41909437    | 24202475    | 23022724    | 41909437    | 24202475    | 23022724    |
| BP Hits                 | 23953352    | 12632075    | 10987129    | 32906998    | 15672745    | 14153446    |
| BP Misses               | 17956085    | 11570400    | 12035595    | 9002439     | 8529730     | 8869278     |
| BP Misses/Instr Branch  | 0.42845     | 0.478067    | 0.52277     | 0.214807    | 0.352432    | 0.38524     |
| CH5 (bolhas)            | 43951512    | 25942215    | 26825713    | 26044220    | 19860875    | 20493079    |
| DH5 (bolhas)            | 34935445    | 17053222    | 17607900    | 34935445    | 17053222    | 17607900    |
| CH7 (bolhas)            | 61907597    | 37512615    | 20493079    | 35046659    | 28390605    | 29362357    |
| DH7 (bolhas)            | 61267042    | 23389183    | 23848560    | 61267042    | 23389183    | 23848560    |
| L1 Hit Penalty (bolhas) | 63270192    | 60836028    | 58934740    | 63270192    | 60836028    | 58934740    |
| L2 Hit Penalty          | 301452      | 504096      | 191724      | 301452      | 504096      | 191724      |
| RAM Hit Penalty         | 130853      | 1462671     | 888991      | 130853      | 1462671     | 888991      |
| Cache Penalty           | 63702497    | 62802795    | 60015455    | 63702497    | 62802795    | 60015455    |
| Ciclos 5 Estágios       | 366280073   | 331751406   | 322150324   | 324307213   | 325670066   | 315817690   |
| CPI 5 Estágios          | 1.6374      | 1.4682      | 1.4798      | 1.4498      | 1.4413      | 1.4507      |
| Tempo 5 Estágios (s)    | 0.1308      | 0.1185      | 0.1151      | 0.1158      | 0.1163      | 0.1128      |
| Ciclos 7 Estágios       | 410567755   | 349657767   | 322058350   | 357486434   | 385069670   | 330927628   |
| CPI 7 Estágios          | 1.8354      | 1.5475      | 1.4794      | 1.5981      | 1.7042      | 1.5201      |
| Tempo 7 Estágios (s)    | 0.1047      | 0.0892      | 0.0821      | 0.0912      | 0.0982      | 0.0844      |


**O número de ciclos foi calculado da forma:**

Ciclos = data hazards(bolhas) + control hazards(bolhas) + numero instruções + cache penalty (bolhas)

**A cache penalty foi calculada da forma:**

Cache penalty = (L1 fetches- L1 misses) x 1 + (L2 fetches - L2 misses)x12 + (L2 misses)x71

**O tempo:**
Supondo que o processador tenha frequência de 560 MHz, que cada estágio da pipeline leva o mesmo tempo e que cada instrução leva 1 ciclo, na pipeline de tamanho 5 teremos 5 instruções por ciclo enquanto que na pipeline de tamanho 7 teremos 7 instruções por ciclo. Logo o tempo foi calculado da forma:

Tempo = (Instruções x CPI)/(número de estágios da pipeline x Clock rate)


### **Referências**
[1] [Slides professor Sandro. Capítulo 4](http://www.ic.unicamp.br/~sandro/cursos/mc722/2s2014/slides/Chapter_04.pdf)
[2] [Implementation and Verification of a 7-Stage Pipeline Processor](http://publications.lib.chalmers.se/records/fulltext/215194/215194.pdf)
