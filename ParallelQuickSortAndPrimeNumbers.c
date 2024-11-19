#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define MAX_NUMBERS 100000
#define MAX_N 1000000  // Límite superior para la criba de Eratóstenes

// Función para intercambiar dos elementos en el arreglo
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Función de partición utilizando un pivote con doble índice
int partition(int array[], int low, int high) {
    int pivot = array[high];
    int leftIndex = low - 1;
    int rightIndex = high;

    while (1) {
        do { leftIndex++; } while (array[leftIndex] < pivot);
        do { rightIndex--; } while (rightIndex >= low && array[rightIndex] > pivot);

        if (leftIndex >= rightIndex) {
            swap(&array[leftIndex], &array[high]);
            return leftIndex;
        }
        swap(&array[leftIndex], &array[rightIndex]);
    }
}

// Función recursiva de Quick Sort
void quickSort(int array[], int low, int high) {
    if (low < high) {
        int pivotIndex = partition(array, low, high);
        quickSort(array, low, pivotIndex - 1);
        quickSort(array, pivotIndex + 1, high);
    }
}

// Función que usa la Criba de Eratóstenes para marcar números primos
void cribaEratostenes(int *esPrimo, int n) {
    int i, j;
    for (i = 2; i <= n; i++) {
        esPrimo[i] = 1;  // Inicializar como primo (1)
    }
    esPrimo[0] = esPrimo[1] = 0;  // 0 y 1 no son primos
    for (i = 2; i <= sqrt(n); i++) {
        if (esPrimo[i]) {
            for (j = i * i; j <= n; j += i) {
                esPrimo[j] = 0;  // No primo (0)
            }
        }
    }
}

// Función para contar los primos en el arreglo
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

// Función para combinar fragmentos ordenados en un arreglo final
void mergeSortedFragments(int *array, int totalSize, int *fragmentSizes, int worldSize) {
    int *tempArray = (int *)malloc(totalSize * sizeof(int));
    int currentIndices[worldSize];
    int fragmentStartIndices[worldSize];
    int i, j;

    printf("Proceso 0: Iniciando fusión de fragmentos\n");

    // Inicialización de los índices de fragmento
    fragmentStartIndices[0] = 0;
    for (i = 0; i < worldSize; i++) {
        currentIndices[i] = fragmentStartIndices[i];
        if (i < worldSize - 1) {
            fragmentStartIndices[i + 1] = fragmentStartIndices[i] + fragmentSizes[i];
        }
    }

    // Fusiona fragmentos en tempArray
    for (i = 0; i < totalSize; i++) {
        int minValue = __INT_MAX__;
        int minIndex = -1;

        // Encuentra el elemento mínimo entre los fragmentos actuales
        for (j = 0; j < worldSize; j++) {
            if (currentIndices[j] < fragmentStartIndices[j] + fragmentSizes[j] && array[currentIndices[j]] < minValue) {
                minValue = array[currentIndices[j]];
                minIndex = j;
            }
        }
        tempArray[i] = minValue;
        currentIndices[minIndex]++;
    }

    // Copia el arreglo temporal de vuelta al arreglo original
    for (i = 0; i < totalSize; i++) {
        array[i] = tempArray[i];
    }
    free(tempArray);
    printf("Proceso 0: Fusión de fragmentos completada\n");
}

int main(int argc, char** argv) {
    int processRank, numProcesses;
    double totalTimeStart, totalTimeEnd;          // Tiempo total
    double sieveTimeStart, sieveTimeEnd;          // Tiempo para la criba
    double primeCountTimeStart, primeCountTimeEnd;// Tiempo para contar primos en fragmentos
    double sortTimeStart, sortTimeEnd;            // Tiempo para ordenar fragmentos
    double mergeTimeStart, mergeTimeEnd;          // Tiempo para la fusión en el proceso raíz
    double fragmentDivisionTimeStart, fragmentDivisionTimeEnd;  // Tiempo para la división de fragmentos
    double scatterTimeStart, scatterTimeEnd;      // Tiempo para el Scatter de fragmentos

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &processRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);

    int numElements = 0;
    int *fullArray = NULL;
    int *localFragment = NULL;
    int *fragmentSizes = NULL;
    int *displacements = NULL;
    int i;
    int *esPrimo = NULL;

    // Generar los números aleatorios fuera del tiempo total
    if (processRank == 0) {
        fullArray = (int*)malloc(MAX_NUMBERS * sizeof(int));
        printf("Proceso 0: Generando datos aleatorios\n");
        for (numElements = 0; numElements < MAX_NUMBERS; numElements++) {
            fullArray[numElements] = rand() % 1000000;
        }
        printf("Proceso 0: Generación de datos completada, total de elementos: %d\n", numElements);
    }

    // Iniciar el tiempo total después de generar los datos
    if (processRank == 0) {
        totalTimeStart = MPI_Wtime();
    }

    // Solo el proceso raíz calcula la criba
    if (processRank == 0) {
        sieveTimeStart = MPI_Wtime();
        esPrimo = (int *)malloc((MAX_N + 1) * sizeof(int));
        cribaEratostenes(esPrimo, MAX_N);
        sieveTimeEnd = MPI_Wtime();
    }

    // Distribuir la criba de Eratóstenes a todos los procesos
    if (processRank != 0) {
        esPrimo = (int *)malloc((MAX_N + 1) * sizeof(int));
    }
    MPI_Bcast(esPrimo, MAX_N + 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Solo el proceso raíz prepara fragmentSizes y displacements
    if (processRank == 0) {
        fragmentDivisionTimeStart = MPI_Wtime();

        fragmentSizes = (int*)malloc(numProcesses * sizeof(int));
        displacements = (int*)malloc(numProcesses * sizeof(int));

        int baseFragmentSize = numElements / numProcesses;
        int remainingElements = numElements % numProcesses;

        for (i = 0; i < numProcesses; i++) {
            fragmentSizes[i] = baseFragmentSize + (i < remainingElements ? 1 : 0);
            displacements[i] = (i == 0) ? 0 : displacements[i - 1] + fragmentSizes[i - 1];
        }

        fragmentDivisionTimeEnd = MPI_Wtime();
    }

    // Solo el proceso raíz mide el tiempo del Scatter
    if (processRank == 0) {
        scatterTimeStart = MPI_Wtime();
    }

    // Distribuir fragmentos a cada proceso
    int localFragmentSize;
    MPI_Scatter(fragmentSizes, 1, MPI_INT, &localFragmentSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    localFragment = (int*)malloc(localFragmentSize * sizeof(int));
    MPI_Scatterv(fullArray, fragmentSizes, displacements, MPI_INT, localFragment, localFragmentSize, MPI_INT, 0, MPI_COMM_WORLD);

    if (processRank == 0) {
        scatterTimeEnd = MPI_Wtime();
    }

    // Contar primos en el fragmento local
    primeCountTimeStart = MPI_Wtime();
    int localPrimeCount = contarPrimos(localFragment, localFragmentSize, esPrimo);
    primeCountTimeEnd = MPI_Wtime();

    // Sumar todos los contadores locales en el proceso raíz
    int totalPrimeCount;
    MPI_Reduce(&localPrimeCount, &totalPrimeCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Ordenar fragmento local
    sortTimeStart = MPI_Wtime();
    quickSort(localFragment, 0, localFragmentSize - 1);
    sortTimeEnd = MPI_Wtime();

    // Recoger fragmentos ordenados en el proceso raíz
    MPI_Gatherv(localFragment, localFragmentSize, MPI_INT, fullArray, fragmentSizes, displacements, MPI_INT, 0, MPI_COMM_WORLD);

    // Fusionar fragmentos ordenados en el proceso raíz y medir el tiempo de fusión
    if (processRank == 0) {
        mergeTimeStart = MPI_Wtime();
        mergeSortedFragments(fullArray, numElements, fragmentSizes, numProcesses);
        mergeTimeEnd = MPI_Wtime();

        totalTimeEnd = MPI_Wtime();

        printf("Cantidad total de números primos en el arreglo: %d\n", totalPrimeCount);
        printf("Tiempo total de ejecución (desde criba hasta fusión): %.9f segundos\n", totalTimeEnd - totalTimeStart);
        printf("Tiempo para calcular la criba de Eratóstenes: %.9f segundos\n", sieveTimeEnd - sieveTimeStart);
        printf("Tiempo para dividir fragmentos en el proceso raíz: %.9f segundos\n", fragmentDivisionTimeEnd - fragmentDivisionTimeStart);
        printf("Tiempo para el Scatter de fragmentos: %.9f segundos\n", scatterTimeEnd - scatterTimeStart);
        printf("Tiempo para contar primos en fragmentos: %.9f segundos\n", primeCountTimeEnd - primeCountTimeStart);
        printf("Tiempo para ordenar fragmentos: %.9f segundos\n", sortTimeEnd - sortTimeStart);
        printf("Tiempo para fusionar fragmentos en el proceso raíz: %.9f segundos\n", mergeTimeEnd - mergeTimeStart);

        // Liberar memoria en el proceso raíz
        free(fullArray);
        free(fragmentSizes);
        free(displacements);
        free(esPrimo);
    }

    // Liberar memoria local y finalizar
    free(localFragment);
    MPI_Finalize();
    return 0;
}
