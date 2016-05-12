
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

## **Branch predictor**

Utilizaremos as seguintes estratégias:

* Sem branch predictor
* Branch estático: escolhemos sempre tomar o branch.
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

* Escalar
* Superescalar:
	* Todos os nossos experimentos e critérios avaliados serão feitos para pipelines de dois tamanhos diferentes:
		* 5 estágios
		* 7 estágios

Modificaremos o código do simulador para avaliar os efeitos da pipeline para cada estágio escolhido.


## **Hazards**

Vamos analizar dois tipos de hazards:

**Hazard de dados**
Esse tipo de hazard ocorre quando temos instruções que dependem de dados de uma instrução anterior da pipeline. 
Podemos resolver esse tipo de hazard com *forwarding*, contudo, existem alguns tipos de hazard em que *stalls* ocorrem mesmo com o uso de *forwarding*. Trataremos o caso dos ***Load-Use Data Hazard***[1] 

**Hazard de controle**

São os hazards que ocorrem devido aos *branches*. Para estudar esse tipo de hazard, usaremos as diferentes estratégias de branch prediction descritas anteriormente. Quando ocorre um erro na predição do branch, as instruções que estavam na pipeline são perdidas, gerando bolhas na pipeline.

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
