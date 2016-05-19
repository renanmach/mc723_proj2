
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
| L1 HIT            | 4          |
| L2 HIT            | 12         |
| L2 MISS (RAM HIT) | 71**       |


*valores encontrados em: (http://www.7-cpu.com/cpu/Skylake.html) e (http://www.7-cpu.com/cpu/Mips74K.html)

** considerando o processador MIPS 74K de 560MHz e RAM Latency de 42 + 51ns (segundo o link Skylake).

  O número de bolhas depende dos cache misses.
  

### **Processador**    **<<<<<<<<<< ALGUEM VAI FAZER ISSO? APAGAR SE NAO FIZEREM**
Vamos comparar dois tipos de processadores:

* Escalar : processadores com apenas 1 pipeline
* Superescalar: processadores que tem mais de 1 pipeline

A comparação será com base no número de ciclos que cada tipo de processador leva para executar os benchmarks.


### **Hazards**

#### Conceitos (para pipeline de 5 estágios)

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
>	00 = fortemente não-tomada = processador assume que branch não é tomado
	01 = fracamente não-tomada = processador assume que branch não é tomado
	10 = fracamente tomado = processador assume que branch é tomado
	11 = fortemente tomado = processador assume que branch é tomado
	
Desse modo, após o resultado de cada comparação de uma instrução branch, é atualizada a BHT, alterando-se em 1 bit. Isso significa que será necessário 2 branchs não-tomados (ou tomados) consecutivos para que o processador passe a prever diferente se o preditor está em um estado forte, e 1 missprediction se o preditor está no estado fraco.

Assim, mesmo que uma branch seja incorretamente prevista, como no exemplo anterior dos 2 laços *for* aninhados, durante a saída do loop mais interno, na próxima branch provavelmente não será incorreta. Afinal, na BHT estará marcada *10*, que ainda assume que o branch será tomado.

Inicialmente antes da execução da primeira branch, iremos assumir que o branch é fortemente tomado, ou seja, será setado em *00*.
Segue abaixo uma imagem da máquina de estados descrita acima:

![FSM](https://github.com/renanmach/mc723_proj2/blob/master/img/dynamicBP.png)

#### Jumps

Foi adicionada uma bolha para todas as intruções de jump para os dois tipos de pipeline.
	
## **Resultados**
	
### **Cache**

#### **dijkstra(large)**:

L1 Demand Fetches = 286983351 (para todos os casos)

| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 365560           | 0.0013       | 396071            | 3763             | 0.0095       |
| 2     | 56382            | 0.0002       | 127250            | 5736             | 0.0451       |
| 3     | 365560           | 0.0013       | 396071            | 2244             | 0.0057       |
| 4     | 56382            | 0.0002       | 63625             | 2250             | 0.0354       |
| 5     | 36108            | 0.0001       | 39314             | 2251             | 0.0573       |
| 6     | 30146            | 0.0001       | 34570             | 2141             | 0.0619       |


#### **patricia(small)**:
	
L1 Demand Fetches = 286844071


| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 3487811          | 0.0122       | 3679521           | 67213            | 0.0183       |
| 2     | 802317           | 0.0028       | 1685216           | 124516           | 0.0739       |
| 3     | 3487811          | 0.0122       | 3679521           | 25559            | 0.0069       |
| 4     | 802317           | 0.0028       | 842608            | 25825            | 0.0306       |
| 5     | 346215           | 0.0012       | 360271            | 25722            | 0.0714       |
| 6     | 153961           | 0.0005       | 161701            | 21213            | 0.1312       |


#### **qsort(large)**:
	
L1 Demand Fetches = 276655813

| Cache | L1 Demand Misses | L1 Miss rate | L2 Demand Fetches | L2 Demand Misses | L2 Miss rate |
|-------|------------------|--------------|-------------------|------------------|--------------|
| 1     | 157063           | 0.0006       | 178782            | 16541            | 0.0925       |
| 2     | 24003            | 0.0001       | 67224             | 28128            | 0.4184       |
| 3     | 157063           | 0.0006       | 178782            | 12812            | 0.0717       |
| 4     | 24003            | 0.0001       | 33612             | 12807            | 0.3810       |
| 5     | 20415            | 0.0001       | 29173             | 12805            | 0.4389       |
| 6     | 20264            | 0.0001       | 28945             | 12841            | 0.4436       |

#### Análise

Considerando apenas a cache L1, notamos uma diferença expressiva ao dobrar o tamanho da cache unificada e do tamanho do bloco unificado. Por exemplo, para o benchmark dijkstra, para as configurações 1 e 3 o L1 miss rate foi de 0.0013, enquanto que para as configuracões 2 e 4 (o dobro do tamanho da cache L1), o miss rate foi de apenas 0.0002.

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
| L1 Hit Penalty (bolhas) |             |             |             |             |             |             |
| L2 Hit Penalty          |             |             |             |             |             |             |
| RAM Hit Penalty         |             |             |             |             |             |             |
| Ciclos 5 Estágios       |             |             |             |             |             |             |
| CPI 5 Estágios          |             |             |             |             |             |             |
| Tempo 5 Estágios        |             |             |             |             |             |             |
| Ciclos 7 Estágios       |             |             |             |             |             |             |
| CPI 7 Estágios          |             |             |             |             |             |             |
| Tempo 7 Estágios        |             |             |             |             |             |             |




### **Referências**
[1] [Slides professor Sandro. Capítulo 4](http://www.ic.unicamp.br/~sandro/cursos/mc722/2s2014/slides/Chapter_04.pdf)
[2] [Implementation and Verification of a 7-Stage Pipeline Processor](http://publications.lib.chalmers.se/records/fulltext/215194/215194.pdf)
