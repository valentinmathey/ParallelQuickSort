# 🚀 Algoritmo QuickSort Paralelo en C con MPI

## 📚 Proyecto de Computación Paralela: QuickSort y Criba de Eratóstenes

Este proyecto implementa una versión paralela del algoritmo **QuickSort** en C, utilizando **MPI** para distribuir el trabajo entre múltiples procesos. Además, incluye una funcionalidad para identificar números primos mediante la **Criba de Eratóstenes**, optimizando el análisis y el ordenamiento de grandes volúmenes de datos.

## 🧩 Algoritmo QuickSort Paralelo

### Estrategia y Distribución de Trabajo
1. **División**: El conjunto de datos se divide en fragmentos, distribuidos entre los procesos disponibles.
2. **Ordenamiento Local**: Cada proceso aplica el algoritmo QuickSort secuencial a su fragmento.
3. **Fusión**: El proceso raíz fusiona los fragmentos ordenados para obtener un conjunto de datos completo en orden ascendente.

## 🔍 Características Principales

1. **Paralelización con MPI**:
   - Uso de `MPI_Scatterv` para distribuir fragmentos de datos.
   - Uso de `MPI_Gatherv` para recolectar fragmentos ordenados.
2. **Criba de Eratóstenes**:
   - Detecta números primos hasta un límite definido (`MAX_N = 1,000,000`).
   - Mejora el análisis numérico en arreglos grandes.
3. **Medición de Tiempo Precisa**:
   - Usa `MPI_Wtime()` para mediciones de rendimiento en nanosegundos.
4. **Escalabilidad**:
   - Manejo de arreglos de hasta **100 millones** de elementos.
   - Balanceo dinámico de carga para fragmentos no uniformes.
5. **Eficiencia**:
   - Mejoras significativas en tiempo de ejecución usando hasta **10 nodos**.

## 💻 Distribución y Recolección de Datos con MPI
#### Distribuir Tamaños de Fragmento: `MPI_Scatter`
```c
int localFragmentSize;
MPI_Scatter(fragmentSizes, 1, MPI_INT, &localFragmentSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
```
**Propósito**: Distribuir el tamaño de cada fragmento de datos a los procesos.
**Funcionamiento**: Envía a cada proceso el tamaño del fragmento que procesará, para reservar la memoria necesaria en cada proceso.

#### Enviar Fragmentos de Datos: `MPI_Scatterv`
```c
localFragment = (int *)malloc(localFragmentSize * sizeof(int));
MPI_Scatterv(fullArray, fragmentSizes, displacements, MPI_INT, localFragment, localFragmentSize, MPI_INT, 0, MPI_COMM_WORLD);
```
**Propósito**: Enviar el fragmento de datos correspondiente a cada proceso.
**Funcionamiento**: Distribuye fragmentos específicos de `fullArray` a cada proceso basado en `fragmentSizes` y `displacements`.

#### Recolectar Fragmentos Ordenados: `MPI_Gatherv`
```c
MPI_Gatherv(localFragment, localFragmentSize, MPI_INT, fullArray, fragmentSizes, displacements, MPI_INT, 0, MPI_COMM_WORLD);
```
**Propósito**: Recoger los fragmentos ordenados de cada proceso en el proceso raíz.
**Funcionamiento**: Usa `fragmentSizes` y `displacements` para colocar los fragmentos en sus posiciones correctas en `fullArray`.

## 🛠️ Compilación y Ejecución Paralela
### Compilación
El programa utiliza la biblioteca matemática (`-lm`) y la biblioteca de tiempo real (`-lrt`). Para compilar:

Compila el programa simple con el compilador MPI (`mpicc`):
```bash
mpicc -o quickParallel1 ParallelQuickSort.c
```
Compila el programa con contador de primos con el compilador MPI (`mpicc`):
```bash
mpicc -o quickParallel2 ParallelQuickSortAndPrimeNumbers.c -lm -lrt
```

### Ejecución
Ejecuta el programa en modo paralelo especificando la cantidad de procesos con `mpiexec`:
```bash
mpiexec -n (cantidad procesos) ./quickParallel
```
Por ejemplo, para ejecutar el programa con 10 procesos:
```bash
mpiexec -n 10 ./quickParallel1
```
```bash
mpiexec -n 10 ./quickParallel2
```

## 📊 Rendimiento y Mediciones en Clúster
Se realizaron pruebas en un clúster con 10 nodos para evaluar el rendimiento de la versión paralela en comparación con la versión secuencial.

| Cantidad de Datos | Tipo | Nodos | Tiempo de Ejecución (s) |
| ----------------- | ---- | ------ | ----------------------- |
| 100 millones de números | Paralelo | 10 | 68.239076138 |
| 100 millones de números | Secuencial (sin MPI) | 1 | 253.353401354 |

**Conclusiones**:
- La versión paralela en clúster reduce significativamente el tiempo de ejecución en comparación con la versión secuencial, logrando una mejora de casi 4 veces en este caso.
- El uso de 10 nodos permite balancear la carga de trabajo y aprovechar la capacidad de procesamiento distribuido.

## 💻 Funciones para Generación, Distribución y Fusión del Arreglo

### Generación del Arreglo Principal
#### `fullArray = (int *)malloc(MAX_NUMBERS * sizeof(int))`

#### Propósito
> Crear un arreglo grande que contendrá todos los elementos a ordenar.

#### Funcionamiento
* El proceso raíz genera un conjunto de datos aleatorios en este arreglo para simular una gran cantidad de datos (hasta `MAX_NUMBERS`)
* Los valores generados están en el rango de 0 a 999999

### Cálculo de Fragmentos y Desplazamientos
#### `fragmentSizes` y `displacements`

```c
fragmentSizes = (int *)malloc(numProcesses * sizeof(int));
displacements = (int *)malloc(numProcesses * sizeof(int));
int baseFragmentSize = numElements / numProcesses;
int remainingElements = numElements % numProcesses;

for (int i = 0; i < numProcesses; i++) {
    fragmentSizes[i] = baseFragmentSize + (i < remainingElements ? 1 : 0);
    displacements[i] = (i == 0) ? 0 : displacements[i - 1] + fragmentSizes[i - 1];
}
```

#### Propósito
> Determinar los tamaños de fragmento y las posiciones de inicio para cada proceso.

#### Funcionamiento
* Calcula el tamaño base de fragmento (`baseFragmentSize`) dividiendo el número de elementos por la cantidad de procesos
* Balanceo de carga: Los elementos restantes se asignan a los primeros procesos para garantizar un reparto equitativo
* `fragmentSizes` almacena el tamaño de fragmento para cada proceso
* `displacements` guarda el índice de inicio de cada fragmento en `fullArray`

### Fusión de Fragmentos
#### `mergeSortedFragments(int *array, int totalSize, int *fragmentSizes, int worldSize)`

```c
int *tempArray = (int *)malloc(totalSize * sizeof(int));
int currentIndices[worldSize];
int fragmentStartIndices[worldSize];
fragmentStartIndices[0] = 0;

for (int i = 0; i < worldSize; i++) {
    currentIndices[i] = fragmentStartIndices[i];
    if (i < worldSize - 1) {
        fragmentStartIndices[i + 1] = fragmentStartIndices[i] + fragmentSizes[i];
    }
}

for (int i = 0; i < totalSize; i++) {
    int minValue = __INT_MAX__;
    int minIndex = -1;
    for (int j = 0; j < worldSize; j++) {
        if (currentIndices[j] < fragmentStartIndices[j] + fragmentSizes[j] && 
            array[currentIndices[j]] < minValue) {
            minValue = array[currentIndices[j]];
            minIndex = j;
        }
    }
    tempArray[i] = minValue;
    currentIndices[minIndex]++;
}

for (int i = 0; i < totalSize; i++) {
    array[i] = tempArray[i];
}
free(tempArray);
```

#### Propósito
> Fusionar los fragmentos ordenados en un solo arreglo en el proceso raíz.

#### Funcionamiento
* Usa un arreglo temporal `tempArray` para mantener el resultado de la fusión
* Mantiene índices en `currentIndices` que avanzan por cada fragmento, seleccionando el valor mínimo entre los fragmentos actuales y colocándolo en `tempArray`
* Finalmente, `tempArray` se copia a `array`, que contiene los datos fusionados y ordenados

## 🧮 Funciones para la Criba de Eratóstenes y Conteo de Primos

### Construcción de la Criba
#### `cribaEratostenes(int *esPrimo, int n)`

```c
void cribaEratostenes(int *esPrimo, int n) {
    int i, j;
    for (i = 2; i <= n; i++) {
        esPrimo[i] = 1;  // Inicializar como primo (1)
    }
    
    esPrimo[0] = esPrimo[1] = 0;  // 0 y 1 no son primos
    
    for (i = 2; i <= sqrt(n); i++) {
        if (esPrimo[i]) {
            for (j = i * i; j <= n; j += i) {
                esPrimo[j] = 0;  // Marcar como no primo (0)
            }
        }
    }
}
```

#### Propósito
> Crear una criba para identificar números primos hasta un límite definido (`n`).

#### Funcionamiento
* Inicializa un arreglo `esPrimo` donde cada índice representa si un número es primo (1) o no (0)
* Marca 0 y 1 como no primos
* Itera desde 2 hasta la raíz cuadrada de `n`, marcando múltiplos como no primos
* Optimiza el uso de memoria al evitar marcar números previamente descartados

### Conteo de Primos
#### `contarPrimos(int array[], int size, int *esPrimo)`

```c
int contarPrimos(int array[], int size, int *esPrimo) {
    int contadorPrimos = 0;
    int i;
    
    for (i = 0; i < size; i++) {
        if (array[i] <= MAX_N && esPrimo[array[i]]) {
            contadorPrimos++;
        }
    }
    
    return contadorPrimos;
}
```

#### Propósito
> Contar cuántos números en un arreglo son primos utilizando la criba generada previamente.

#### Funcionamiento
* Itera a través del arreglo de entrada `array`
* Comprueba si cada número es primo consultando el arreglo `esPrimo`
* Incrementa un contador cada vez que encuentra un número primo
* Devuelve el total de números primos encontrados en el arreglo

## 📊 Medición de Rendimiento en Tiempo Real
El programa mide el tiempo de ejecución total desde la distribución hasta la fusión, 

- Segundos
- Milisegundos
- Nanosegundos
  
utilizando `MPI_Wtime()` para obtener una medición precisa del tiempo de ejecución.

## 🔬 Análisis de Complejidad
- **Mejor Caso**: O(n log n) - Particiones equilibradas.
- **Peor Caso**: O(n²) - Arreglo ya ordenado o inversamente ordenado.
- **Caso Promedio**: O(n log n) - Con datos aleatorios.

## 💻 Tecnologías

<div align="center">
  <img src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/c/c-original.svg" height="40" alt="c logo" />
  <img src="https://www.open-mpi.org/images/open-mpi-logo.png" height="40" alt="Open MPI logo" />
</div>

## 🧑🏻‍💻 Autor:

Valentin Mathey | <a href="https://github.com/valentinmathey">@valentinmathey</a>

[![Discord](https://img.shields.io/badge/Discord-%237289DA.svg?logo=discord&logoColor=white)](https://discord.gg/valentinmathey) [![Facebook](https://img.shields.io/badge/Facebook-%231877F2.svg?logo=Facebook&logoColor=white)](https://facebook.com/ValentinEzequielMathey) [![Instagram](https://img.shields.io/badge/Instagram-%23E4405F.svg?logo=Instagram&logoColor=white)](https://instagram.com/valen.mathey/) [![LinkedIn](https://img.shields.io/badge/LinkedIn-%230077B5.svg?logo=linkedin&logoColor=white)](https://linkedin.com/in/valentin-mathey) [![X](https://img.shields.io/badge/X-%231DA1F2.svg?logo=X&logoColor=white)](https://twitter.com/valen_mathey)
