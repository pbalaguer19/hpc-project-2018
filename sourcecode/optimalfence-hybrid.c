#include <mpi.h>
#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ConvexHull.h"

#define DMaxArboles 	32
#define DMaximoCoste 999999
#define S 10000
#define DDebug 0
#define MASTER 0

  //////////////////////////
 // Estructuras de datos //
//////////////////////////

// Tree structure definition.
struct  Arbol{
	int IdArbol;
	Point Coord;			// Posicin �bol
	int Valor;				// Valor / Coste �bol.
	int Longitud;			// Cantidad madera �bol
};
typedef struct Arbol TArbol, *PtrArbol;

// Forest structure definition.
struct Bosque{
	int NumArboles;
	TArbol Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;

// Problem respresentation
struct ListaArboles{
	int 		NumArboles;
 	float		Coste;
	float		CosteArbolesCortados;
	float		CosteArbolesRestantes;
	float		LongitudCerca;
	float		MaderaSobrante;
	int 		Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;

// static coordinates.
typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;

typedef enum {false, true} bool;

  ////////////////////////
 // Variables Globales //
////////////////////////

TBosque ArbolesEntrada;

  //////////////////////////
 // Prototipos funciones //
//////////////////////////

bool CalcularCercaOptima(PtrListaArboles Optimo, int rank, int size);
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
bool ReadInputFile(char *InPath);

float CalcularDistancia(int x1, int y1, int x2, int y2);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);

int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
int EvaluarCombinacionListaArboles(int Combinacion);

void MostrarArboles(TListaArboles CombinacionArboles);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
void OrdenarArboles();

void getMin(TListaArboles *listIn, TListaArboles *listOut, int *len, MPI_Datatype *dptr);

int main(int argc, char *argv[]) {

	char outputFileName[1000];

    int rank;
    int size;

	double tpivot1=0,tpivot2=0; //time counting
	struct timeval tim;

    MPI_Status st;

	TListaArboles Optimo;

	//Capture first token time
	gettimeofday(&tim, NULL);
	tpivot1 = tim.tv_sec+(tim.tv_usec/1000000.0);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(rank == MASTER) {
		if (argc < 2 || argc > 3) {
			printf("Error Argumentos: ./CalcArbolesParallel input [output]");
			exit(1);
   		 }

		if(argc == 3) {
			sprintf(outputFileName, "%s", argv[2]);
		} else {
			const char * defaultFile= "./DefaultFile.res";
			sprintf(outputFileName, "%s", defaultFile);
		}
	}

    // Tipo derivado POINT
    int blocklengths[2] = {1,1};
    MPI_Datatype types[2] = {MPI_INT, MPI_INT};
    MPI_Aint displacements[2];
    MPI_Datatype PointType;
    MPI_Aint intex;

    MPI_Type_extent(MPI_INT, &intex);
    displacements[0] = (MPI_Aint) 0;
    displacements[1] = intex;

    MPI_Type_struct(2, blocklengths, displacements, types, &PointType);
    MPI_Type_commit(&PointType);

    // Tipo derivado ARBOL
    int blocklengths_tree[4] = {1,2,1,1};
    MPI_Datatype types_tree[4] = {MPI_INT, PointType, MPI_INT, MPI_INT};
    MPI_Aint displacements_tree[4];
    MPI_Datatype TreeType;
    MPI_Aint pointex;

    MPI_Type_extent(PointType, &pointex);
    displacements_tree[0] = (MPI_Aint) 0;
    displacements_tree[1] = intex;
    displacements_tree[2] = pointex + intex;
    displacements_tree[3] = intex + pointex + intex;

    MPI_Type_struct(4, blocklengths_tree, displacements_tree, types_tree, &TreeType);
    MPI_Type_commit(&TreeType);

    //Tipo derivado BOSQUE
    int blocklengths_forest[2] = {1, DMaxArboles};
    MPI_Datatype types_forest[2] = {MPI_INT, TreeType};
    MPI_Aint displacements_forest[2];
    MPI_Datatype ForestType;

    displacements_forest[0] = (MPI_Aint) 0;
    displacements_forest[1] = intex;

    MPI_Type_struct(2, blocklengths_forest, displacements_forest, types_forest, &ForestType);
    MPI_Type_commit(&ForestType);

    // Tipo derivado tlist
    int blocklengths_tlist[7] = {1,1,1,1,1,1, DMaxArboles};
    MPI_Datatype types_tlist[7] = {MPI_INT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_INT};
    MPI_Aint displacements_tlist[7];
    MPI_Datatype TreeListType;
    MPI_Aint floatex;

    MPI_Type_extent(MPI_FLOAT, &floatex);
    displacements_tlist[0] = (MPI_Aint) 0;
    displacements_tlist[1] = intex;
    displacements_tlist[2] = intex + floatex;
    displacements_tlist[3] = intex + floatex * 2;
    displacements_tlist[4] = intex + floatex * 3;
    displacements_tlist[5] = intex + floatex * 4;
    displacements_tlist[6] = intex + floatex * 5;

    MPI_Type_struct(7, blocklengths_tlist, displacements_tlist, types_tlist, &TreeListType);
    MPI_Type_commit(&TreeListType);

    //Leer fichero de entrada solo una vez.
    if(rank == MASTER) {
        if (!ReadInputFile(argv[1])){
		    printf("Error lectura fichero entrada.\n");
		    exit(1);
	    }
        OrdenarArboles();
    }
    //Comunicar datos a todos los procesos.
    MPI_Bcast(&ArbolesEntrada, 1, ForestType, MASTER, MPI_COMM_WORLD);

    // Cada proceso obtendrá la combinación óptima de las que el es consciente.
    if (!CalcularCercaOptima(&Optimo, rank, size)) {
		printf("Error CalcularCercaOptima.\n");
		exit(1);
	}

	MPI_Barrier(MPI_COMM_WORLD);

  MPI_Op getMinOp;
  MPI_Op_create((MPI_User_function *) getMin, 1, &getMinOp);
  TListaArboles optimoFinal;

	if(MPI_Reduce(&Optimo, &optimoFinal, 1,
        TreeListType, getMinOp, MASTER, MPI_COMM_WORLD) != MPI_SUCCESS) {
            perror("Error reducing result.");
            exit(-1);
    }

	MPI_Barrier(MPI_COMM_WORLD);

    if(rank == MASTER) {
        printf("Optimo FINAL: Coste: %f y tala %d arboles\n",
        optimoFinal.Coste, optimoFinal.NumArboles);

        if (!GenerarFicheroSalida(optimoFinal, outputFileName)) {
            printf("Error GenerarFicheroSalida.\n");
            exit(1);
        }
    }

    MPI_Finalize();

	gettimeofday(&tim, NULL);
    tpivot2 = (tim.tv_sec+(tim.tv_usec/1000000.0));
    // printf("\n%.6lf\n", tpivot2-tpivot1);

    return 0;
}

/*
* Determina cual es la combinación de coste mínimo
*/
void getMin(TListaArboles *listIn, TListaArboles *listOut, int *len, MPI_Datatype *dptr){

	// Si el coste del elemento actual es menor que el del optimo asignado
	// asignamos este elemento como optimo.
	if (listIn->Coste < listOut->Coste) {
        *listOut = *listIn;
    }

	// Si el coste del elemento actual es menor o igual que el del optimo asignado
	// y además tala menos arboles que este, asignamos a este como optimo.
	if (listIn->Coste <= listOut->Coste) {
		if (listOut->NumArboles > listIn->NumArboles) {
        	*listOut = *listIn;
		}
	}

}

bool ReadInputFile(char *InPath) {
    FILE *FicIn;
	int a;

	FicIn=fopen(InPath,"r");
	if (FicIn==NULL) {
		perror("Lectura Fichero entrada.");
		return false;
	}
	printf("Datos Entrada:\n");

	// Leemos el nmero de arboles del bosque de entrada.
	if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles)) < 1){
		perror("Lectura arboles entrada");
		return false;
	}
	printf("\tÁrboles: %d.\n",ArbolesEntrada.NumArboles);

	// Leer atributos arboles.
	for(a=0; a < ArbolesEntrada.NumArboles; a++) {
		ArbolesEntrada.Arboles[a].IdArbol=a+1;
		// Leer x, y, Coste, Longitud.
		if (fscanf(FicIn, "%d %d %d %d",&(ArbolesEntrada.Arboles[a].Coord.x), &(ArbolesEntrada.Arboles[a].Coord.y), &(ArbolesEntrada.Arboles[a].Valor), &(ArbolesEntrada.Arboles[a].Longitud)) < 4) {
			perror("Lectura datos arbol.");
			return false;
		}
		printf("\tÁrbol %d-> (%d,%d) Coste:%d, Long:%d.\n",a+1,ArbolesEntrada.Arboles[a].Coord.x, ArbolesEntrada.Arboles[a].Coord.y, ArbolesEntrada.Arboles[a].Valor, ArbolesEntrada.Arboles[a].Longitud);
	}

	return true;
}

void OrdenarArboles() {
  int a,b;

	for(a=0; a<(ArbolesEntrada.NumArboles-1); a++)
	{
		for(b=1; b<ArbolesEntrada.NumArboles; b++)
		{
			if ( ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
				 (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y) )
			{
				TArbol aux;

				// aux=a
				aux.Coord.x = ArbolesEntrada.Arboles[a].Coord.x;
				aux.Coord.y = ArbolesEntrada.Arboles[a].Coord.y;
				aux.IdArbol = ArbolesEntrada.Arboles[a].IdArbol;
				aux.Valor = ArbolesEntrada.Arboles[a].Valor;
				aux.Longitud = ArbolesEntrada.Arboles[a].Longitud;

				// a=b
				ArbolesEntrada.Arboles[a].Coord.x = ArbolesEntrada.Arboles[b].Coord.x;
				ArbolesEntrada.Arboles[a].Coord.y = ArbolesEntrada.Arboles[b].Coord.y;
				ArbolesEntrada.Arboles[a].IdArbol = ArbolesEntrada.Arboles[b].IdArbol;
				ArbolesEntrada.Arboles[a].Valor = ArbolesEntrada.Arboles[b].Valor;
				ArbolesEntrada.Arboles[a].Longitud = ArbolesEntrada.Arboles[b].Longitud;

				// b=aux
				ArbolesEntrada.Arboles[b].Coord.x = aux.Coord.x;
				ArbolesEntrada.Arboles[b].Coord.y = aux.Coord.y;
				ArbolesEntrada.Arboles[b].IdArbol = aux.IdArbol;
				ArbolesEntrada.Arboles[b].Valor = aux.Valor;
				ArbolesEntrada.Arboles[b].Longitud = aux.Longitud;
			}
		}
	}
}

bool CalcularCercaOptima(PtrListaArboles Optimo, int rank, int size){
    int totalArboles = ArbolesEntrada.NumArboles;
    int MaxCombinaciones;

	/* C�culo Máximo Combinaciones */
	MaxCombinaciones = (int) pow(2.0, ArbolesEntrada.NumArboles);
    int combinationsPerProcess = MaxCombinaciones / size;

    int multiplier = rank+1;
    int add = 0;
    if(rank == MASTER) add = 1;
    Optimo->NumArboles = 0;
	Optimo->Coste = DMaximoCoste;

    int extra = MaxCombinaciones%size;

    if(extra != 0 && rank == size - 1){
	    CalcularCombinacionOptima(combinationsPerProcess * rank + add, combinationsPerProcess * multiplier + extra, Optimo);
    } else {
        CalcularCombinacionOptima(combinationsPerProcess * rank + add, combinationsPerProcess * multiplier, Optimo);
    }

    return true;
}

bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo){
	int MejorCombinacion=0, CosteMejorCombinacion;
	TListaArboles OptimoParcial;

	TListaArboles CombinacionArboles;
	TVectorCoordenadas CoordArboles, CercaArboles;
	int NumArboles, PuntosCerca;
	float MaderaArbolesTalados;

  #pragma omp parallel
	{
		int Combinacion, Coste;
  	CosteMejorCombinacion = Optimo->Coste;

    #pragma omp for
  	for (Combinacion=PrimeraCombinacion; Combinacion<UltimaCombinacion; Combinacion++) {
  		Coste = EvaluarCombinacionListaArboles(Combinacion);
  		if (Coste < CosteMejorCombinacion ) {
        #pragma omp critical
        {
          if (Coste < CosteMejorCombinacion ) {
            CosteMejorCombinacion = Coste;
      			MejorCombinacion = Combinacion;
          }
        }
  		}

  		if ((Combinacion%S)==0) {
  			 ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
  			 MostrarArboles(OptimoParcial);
  		}
      }
  }

	ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
	MostrarArboles(OptimoParcial);

	if (CosteMejorCombinacion == Optimo->Coste)
		return false;  // No se ha encontrado una combinacin mejor.

	// Asignar combinacin encontrada.
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, Optimo);
	Optimo->Coste = CosteMejorCombinacion;
	// Calcular estadisticas óptimo.
	NumArboles = ConvertirCombinacionToArboles(MejorCombinacion, &CombinacionArboles);
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	Optimo->LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);
	MaderaArbolesTalados = CalcularMaderaArbolesTalados(*Optimo);
	Optimo->MaderaSobrante = MaderaArbolesTalados - Optimo->LongitudCerca;
	Optimo->CosteArbolesCortados = CosteMejorCombinacion;
	Optimo->CosteArbolesRestantes = CalcularCosteCombinacion(CombinacionArboles);

	return true;
}

int EvaluarCombinacionListaArboles(int Combinacion) {
	TVectorCoordenadas CoordArboles, CercaArboles;
	TListaArboles CombinacionArboles, CombinacionArbolesTalados;
	int NumArboles, NumArbolesTalados, PuntosCerca, CosteCombinacion;
	float LongitudCerca, MaderaArbolesTalados;

	// Convertimos la combinacin al vector de arboles no talados.
	NumArboles = ConvertirCombinacionToArboles(Combinacion, &CombinacionArboles);

	// Obtener el vector de coordenadas de arboles no talados.
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);

	// Calcular la cerca
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	/* Evaluar si obtenemos suficientes �boles para construir la cerca */
	LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);

	// Evaluar la madera obtenida mediante los arboles talados.
	// Convertimos la combinacin al vector de arboles no talados.
	NumArbolesTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombinacionArbolesTalados);
    if (DDebug) printf(" %d arboles cortados: ",NumArbolesTalados);
    if (DDebug) MostrarArboles(CombinacionArbolesTalados);
    MaderaArbolesTalados = CalcularMaderaArbolesTalados(CombinacionArbolesTalados);
    if (DDebug) printf("  Madera:%4.2f  \tCerca:%4.2f ",MaderaArbolesTalados, LongitudCerca);
	if (LongitudCerca > MaderaArbolesTalados){
		// Los arboles cortados no tienen suficiente madera para construir la cerca.
        if (DDebug) printf("\tCoste:%d",DMaximoCoste);
        return DMaximoCoste;
	}

	// Evaluar el coste de los arboles talados.
	CosteCombinacion = CalcularCosteCombinacion(CombinacionArbolesTalados);
    if (DDebug) printf("\tCoste:%d",CosteCombinacion);
	return CosteCombinacion;
}

int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles) {
	int arbol=0;

	CombinacionArboles->NumArboles=0;
	CombinacionArboles->Coste=0;

	while (arbol < ArbolesEntrada.NumArboles) {
		if ((Combinacion % 2) == 0) {
			CombinacionArboles->Arboles[CombinacionArboles->NumArboles] = arbol;
			CombinacionArboles->NumArboles++;
			CombinacionArboles->Coste += ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}
	return CombinacionArboles->NumArboles;
}

int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados) {
	int arbol=0;

	CombinacionArbolesTalados->NumArboles=0;
	CombinacionArbolesTalados->Coste=0;

	while (arbol < ArbolesEntrada.NumArboles) {
		if ((Combinacion % 2) == 1) {
			CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles] = arbol;
			CombinacionArbolesTalados->NumArboles++;
			CombinacionArbolesTalados->Coste += ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}
	return CombinacionArbolesTalados->NumArboles;
}

void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas) {
	int c, arbol;

	for (c = 0; c < CombinacionArboles.NumArboles; c++) {
        arbol = CombinacionArboles.Arboles[c];
		Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
		Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
	}
}

float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca) {
	int x;
	float coste;

	for (x = 0; x < (SizeCerca-1); x++) {
		coste+= CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y, CoordenadasCerca[x+1].x, CoordenadasCerca[x+1].y);
	}

	return coste;
}

float CalcularDistancia(int x1, int y1, int x2, int y2) {
	return(sqrt(pow((double) abs(x2 - x1), 2.0) + pow((double) abs(y2 - y1), 2.0)));
}

int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles) {
	int a;
	int LongitudTotal = 0;

	for (a = 0; a < CombinacionArboles.NumArboles; a++) {
		LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;
	}

	return(LongitudTotal);
}

int CalcularCosteCombinacion(TListaArboles CombinacionArboles) {
	int a;
	int CosteTotal=0;

	for (a=0 ; a < CombinacionArboles.NumArboles; a++) {
		CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;
	}

	return(CosteTotal);
}

void MostrarArboles(TListaArboles CombinacionArboles) {
	int a;
	//for (a = 0; a < CombinacionArboles.NumArboles; a++) {
	//	printf("%d ",ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);
    //}
    //for (; a < ArbolesEntrada.NumArboles; a++) printf("  ");
}

bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut) {
	FILE *FicOut;
	int a;

	FicOut = fopen(PathFicOut, "w+");
	if (FicOut == NULL) {
		perror("Escritura fichero salida.");
		return false;
	}

	// Escribir arboles a talartalado.
	// Escribimos nmero de arboles a talar.
    char pluralize[15];
    if(Optimo.NumArboles > 1) {
        sprintf(pluralize, "%s", "árboles");
    } else {
        sprintf(pluralize, "%s", "árbol");
    }

	if (fprintf(FicOut, "Cortar %d %s: ", Optimo.NumArboles, pluralize) < 1) {
		perror("Escribir nmero de arboles a talar");
		return false;
	}

	for(a = 0; a < Optimo.NumArboles; a++) {
		// Escribir nmero arbol.
		if (fprintf(FicOut, "%d ", ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol) < 1){
			perror("Escritura nmero �bol.");
			return false;
		}
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nMadera Sobrante: \t%4.2f (%4.2f)", Optimo.MaderaSobrante,  Optimo.LongitudCerca) < 1) {
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles cortados: \t%4.2f", Optimo.CosteArbolesCortados) < 1) {
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles restantes: \t%4.2f\n",Optimo.CosteArbolesRestantes) < 1) {
		perror("Escribir coste arboles a talar.");
		return false;
	}

	return true;
}
