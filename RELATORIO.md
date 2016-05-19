
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

-Hazards:conceitos (para pipeline de 5 estágios)

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
 
Vamos analizar dois tipos de hazards:

**Hazard de dados**
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

Para o caso das **pipelines de tamanho 7,** o que vai mudar é o número de bolhas inseridas caso ocorra um hazard de dados.
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

**Hazard de controle**

São os hazards que ocorrem devido aos *branches*. Para estudar esse tipo de hazard, usaremos as diferentes estratégias de branch prediction descritas anteriormente. Quando ocorre um erro na predição do branch, as instruções que estavam na pipeline são perdidas, gerando bolhas na pipeline.

O número de bolhas depende do estágio em que o branch é tomado.

- Prediction correct --> 0 bolhas
- Prediction incorrect --> 2 e 3 bolhas para pipelines de 5 e 7 estágios respectivamente.

Static branch prediction:

Escolhemos sempre não tomar o branch

Dynamic Branch prediction:

- Máquina de estados:
incluir aki link para a figura da maquina de estados
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
