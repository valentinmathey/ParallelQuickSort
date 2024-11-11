#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_NUMBERS 100000000 

// Intercambia dos elementos en el arreglo
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Partición del arreglo para Quick Sort usando el último elemento como pivote
int partition(int array[], int low, int high) {
    int pivot = array[high];
    int i = low - 1; // Índice del menor elemento

    // Recorre el subarreglo y coloca elementos menores que el pivote a su izquierda
    for (int j = low; j < high; j++) {
        if (array[j] <= pivot) {
            i++;
            swap(&array[i], &array[j]);
        }
    }
    swap(&array[i + 1], &array[high]); // Coloca el pivote en su posición final
    return i + 1; // Retorna el índice del pivote
}

// Quick Sort recursivo
void quickSort(int array[], int low, int high) {
    if (low < high) {
        // Encuentra el índice del pivote y aplica Quick Sort en los subarreglos
        int pivotIndex = partition(array, low, high);
        quickSort(array, low, pivotIndex - 1);
        quickSort(array, pivotIndex + 1, high);
    }
}

// Fusiona fragmentos ordenados en un solo arreglo ordenado final
void mergeSortedFragments(int *array, int totalSize, int *fragmentSizes, int worldSize) {
    int *tempArray = (int *)malloc(totalSize * sizeof(int)); // Arreglo temporal para almacenar el arreglo fusionado
    int currentIndices[worldSize]; // Índices actuales de cada fragmento
    int fragmentStartIndices[worldSize]; // Inicio de cada fragmento en el arreglo original

    printf("Proceso 0: Iniciando fusión de fragmentos\n");

    // Inicializar los índices iniciales de cada fragmento
    fragmentStartIndices[0] = 0;
    for (int i = 0; i < worldSize; i++) {
        currentIndices[i] = fragmentStartIndices[i];
        if (i < worldSize - 1) {
            fragmentStartIndices[i + 1] = fragmentStartIndices[i] + fragmentSizes[i];
        }
    }

    // Fusiona fragmentos ordenados en un solo arreglo en orden ascendente
    for (int i = 0; i < totalSize; i++) {
        int minValue = __INT_MAX__;
        int minIndex = -1;

        // Encuentra el menor elemento entre los fragmentos en sus índices actuales
        for (int j = 0; j < worldSize; j++) {
            if (currentIndices[j] < fragmentStartIndices[j] + fragmentSizes[j] && array[currentIndices[j]] < minValue) {
                minValue = array[currentIndices[j]];
                minIndex = j;
            }
        }
        tempArray[i] = minValue; // Añade el mínimo al arreglo temporal
        currentIndices[minIndex]++; // Avanza el índice del fragmento de donde se tomó el mínimo
    }

    // Copia el contenido del arreglo temporal al arreglo original
    for (int i = 0; i < totalSize; i++) {
        array[i] = tempArray[i];
    }
    free(tempArray); // Libera la memoria temporal
    printf("Proceso 0: Fusión de fragmentos completada\n");
}

int main(int argc, char **argv) {
    int processRank, numProcesses;
    double startTime, endTime;  // Variables para medir el tiempo de ejecución

    // Inicializa MPI y obtiene el rango y tamaño de cada proceso
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &processRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);

    printf("Proceso %d: Iniciando ejecución\n", processRank);

    int numElements = 0; // Número total de elementos en el arreglo
    int *fullArray = NULL; // Arreglo completo en el proceso raíz
    int *localFragment = NULL; // Fragmento local para cada proceso
    int *fragmentSizes = NULL; // Tamaño de fragmento para cada proceso
    int *displacements = NULL; // Desplazamiento de cada fragmento

    // Inicialización y distribución en el proceso raíz
    if (processRank == 0) {
        // Reserva memoria para el arreglo completo y genera datos aleatorios
        fullArray = (int *)malloc(MAX_NUMBERS * sizeof(int));
        for (numElements = 0; numElements < MAX_NUMBERS; numElements++) {
            fullArray[numElements] = rand() % 1000000; // Rango de 0 a 999999
        }

        // Inicia el cronómetro
        startTime = MPI_Wtime();

        // Crea arreglos para el tamaño de fragmento y los desplazamientos de cada proceso
        fragmentSizes = (int *)malloc(numProcesses * sizeof(int));
        displacements = (int *)malloc(numProcesses * sizeof(int));

        int baseFragmentSize = numElements / numProcesses; // Tamaño base de fragmento
        int remainingElements = numElements % numProcesses; // Elementos restantes para balancear

        // Define tamaños de fragmento y desplazamientos
        for (int i = 0; i < numProcesses; i++) {
            fragmentSizes[i] = baseFragmentSize + (i < remainingElements ? 1 : 0);
            displacements[i] = (i == 0) ? 0 : displacements[i - 1] + fragmentSizes[i - 1];
        }
    }

    // Distribuye los tamaños de fragmento a cada proceso
    int localFragmentSize;
    MPI_Scatter(fragmentSizes, 1, MPI_INT, &localFragmentSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Reserva espacio para cada fragmento local en cada proceso
    localFragment = (int *)malloc(localFragmentSize * sizeof(int));

    // Distribuye los datos fragmentados a cada proceso
    MPI_Scatterv(fullArray, fragmentSizes, displacements, MPI_INT, localFragment, localFragmentSize, MPI_INT, 0, MPI_COMM_WORLD);

    // Cada proceso ordena su fragmento localmente usando Quick Sort
    quickSort(localFragment, 0, localFragmentSize - 1);

    // Recoge los fragmentos ordenados de cada proceso en el proceso raíz
    MPI_Gatherv(localFragment, localFragmentSize, MPI_INT, fullArray, fragmentSizes, displacements, MPI_INT, 0, MPI_COMM_WORLD);

    if (processRank == 0) {
        // Fusiona los fragmentos ordenados en el proceso raíz
        mergeSortedFragments(fullArray, numElements, fragmentSizes, numProcesses);
        endTime = MPI_Wtime(); // Detiene el cronómetro

        // Calcula e imprime el tiempo total de ejecución
        printf("Tiempo de ejecución: %.9f segundos\n", endTime - startTime);

        // Libera la memoria utilizada en el proceso raíz
        free(fullArray);
        free(fragmentSizes);
        free(displacements);
    }

    // Libera la memoria de cada proceso y finaliza MPI
    free(localFragment);
    printf("Proceso %d: Finalizando ejecución\n", processRank);
    MPI_Finalize();
    return 0;
}
