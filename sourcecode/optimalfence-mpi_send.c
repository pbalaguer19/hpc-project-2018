#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <stddef.h>
#include "ConvexHull.h"

#define DMaxArboles 	32
#define DMaximoCoste 999999
#define S 10000
#define DDebug 0
#define ROOT 0


  //////////////////////////
 // Estructuras de datos //
//////////////////////////

// Struct que sera enviada als processos.
typedef struct tupla_s {
		int ini;	// Inici de combinacions
		int end;	// Final de combinacions
} tupla;


// Definicin estructura arbol entrada (Conjunto Arboles).
struct  Arbol
{
	int	  IdArbol;
	Point Coord;			// Posicin Arbol
	int Valor;				// Valor / Coste Arbol.
	int Longitud;			// Cantidad madera Arbol
};
typedef struct Arbol TArbol, *PtrArbol;


// Definicin estructura Bosque entrada (Conjunto Arboles).
struct Bosque
{
	int 		NumArboles;
	TArbol 	Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;


// Combinacin .
struct ListaArboles
{
	int 		NumArboles;
 	float		Coste;
	float		CosteArbolesCortados;
	float		CosteArbolesRestantes;
	float		LongitudCerca;
	float		MaderaSobrante;
	int 		Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;


// Vector estaico Coordenadas.
typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;

typedef enum {false, true} bool;


  ////////////////////////
 // Variables Globales //
////////////////////////

TBosque ArbolesEntrada;
int size, rank;
MPI_Datatype mpi_tupla_type;


  //////////////////////////
 // Prototipos funciones //
//////////////////////////

void crear_mpi_struct();
void CalculoCombinacionesNoRoot(int ini, int end, PtrListaArboles Optimo);
bool LeerFicheroEntrada(char *PathFicIn);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
bool CalcularCercaOptima(PtrListaArboles Optimo);
void OrdenarArboles();
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo);
int EvaluarCombinacionListaArboles(int Combinacion);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);
float CalcularDistancia(int x1, int y1, int x2, int y2);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
void MostrarArboles(TListaArboles CombinacionArboles);



int main(int argc, char *argv[]){

	double tpivot1=0,tpivot2=0; //time counting
	struct timeval tim;

	//Capture first token time
	gettimeofday(&tim, NULL);
	tpivot1 = tim.tv_sec+(tim.tv_usec/1000000.0);

	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	TListaArboles Optimo;

	if (argc<2 || argc>3)
		printf("Error Argumentos");

	if (!LeerFicheroEntrada(argv[1])){
		printf("Error lectura fichero entrada.\n");
		exit(1);
	}

	crear_mpi_struct();

	if (rank == ROOT){
		if (!CalcularCercaOptima(&Optimo)){
			printf("Error CalcularCercaOptima.\n");
			exit(1);
		}

		if (argc==2){
			if (!GenerarFicheroSalida(Optimo, "./Cerca.res")){
				printf("Error GenerarFicheroSalida.\n");
				exit(1);
			}
		}
		else{
			if (!GenerarFicheroSalida(Optimo, argv[2])){
				printf("Error GenerarFicheroSalida.\n");
				exit(1);
			}
		}
	}else{
		tupla recv;
		PtrListaArboles opt;

		MPI_Recv(&recv, 1, mpi_tupla_type, ROOT, 22, MPI_COMM_WORLD, &status);
		CalculoCombinacionesNoRoot(recv.ini, recv.end, &Optimo);
	}
	MPI_Finalize();

	gettimeofday(&tim, NULL);	
    tpivot2 = (tim.tv_sec+(tim.tv_usec/1000000.0));
    // printf("\n%.6lf\n", tpivot2-tpivot1);

	exit(0);
}

void crear_mpi_struct(){
	/* Creactio del tipus tupla */
	const int 	 nitems=2;
	int          blocklengths[2] = {1, 1};
	MPI_Datatype types[4] = {MPI_INT, MPI_INT};
	MPI_Aint     offsets[2];

	offsets[0] = offsetof(tupla, ini);
	offsets[1] = offsetof(tupla, end);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_tupla_type);
	MPI_Type_commit(&mpi_tupla_type);
}

void CalculoCombinacionesNoRoot(int ini, int end, PtrListaArboles Optimo){
	OrdenarArboles();

	Optimo->NumArboles = 0;
	Optimo->Coste = DMaximoCoste;
	CalcularCombinacionOptima(ini, end, Optimo);
}

bool LeerFicheroEntrada(char *PathFicIn){
	FILE *FicIn;
	int a;

	FicIn=fopen(PathFicIn,"r");
	if (FicIn==NULL){
		perror("Lectura Fichero entrada.");
		return false;
	}
	if(rank == ROOT) printf("Datos Entrada:\n");

	// Leemos el nmero de arboles del bosque de entrada.
	if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles))<1){
		perror("Lectura arboles entrada");
		return false;
	}

	if(rank == ROOT) printf("\tÁrboles: %d.\n",ArbolesEntrada.NumArboles);

	// Leer atributos arboles.
	for(a=0;a<ArbolesEntrada.NumArboles;a++){
		ArbolesEntrada.Arboles[a].IdArbol=a+1;
		// Leer x, y, Coste, Longitud.
		if (fscanf(FicIn, "%d %d %d %d",&(ArbolesEntrada.Arboles[a].Coord.x), &(ArbolesEntrada.Arboles[a].Coord.y), &(ArbolesEntrada.Arboles[a].Valor), &(ArbolesEntrada.Arboles[a].Longitud))<4){
			perror("Lectura datos arbol.");
			return false;
		}
		if(rank == ROOT) printf("\tÁrbol %d-> (%d,%d) Coste:%d, Long:%d.\n",a+1,ArbolesEntrada.Arboles[a].Coord.x, ArbolesEntrada.Arboles[a].Coord.y, ArbolesEntrada.Arboles[a].Valor, ArbolesEntrada.Arboles[a].Longitud);
	}
	if (rank == ROOT) printf("\n");
	return true;
}


bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut){
	FILE *FicOut;
	int a;

	FicOut=fopen(PathFicOut,"w+");
	if (FicOut==NULL){
		perror("Escritura fichero salida.");
		return false;
	}

	// Escribir arboles a talartalado.
	// Escribimos nmero de arboles a talar.
	if (fprintf(FicOut, "Cortar %d árbol/es: ", Optimo.NumArboles)<1){
		perror("Escribir nmero de arboles a talar");
		return false;
	}

	for(a=0;a<Optimo.NumArboles;a++){
		// Escribir nmero arbol.
		if (fprintf(FicOut, "%d ",ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol)<1){
			perror("Escritura nmero Arbol.");
			return false;
		}
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nMadera Sobrante: \t%4.2f (%4.2f)", Optimo.MaderaSobrante,  Optimo.LongitudCerca)<1){
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles cortados: \t%4.2f.", Optimo.CosteArbolesCortados)<1){
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor árboles restantes: \t%4.2f\n", Optimo.CosteArbolesRestantes)<1){
		perror("Escribir coste arboles a talar.");
		return false;
	}
	return true;
}


bool CalcularCercaOptima(PtrListaArboles Optimo){
	int MaxCombinaciones, ini, end, avg, rest, i;

	/* Caculo Máximo Combinaciones */
	MaxCombinaciones = (int) pow(2.0,ArbolesEntrada.NumArboles);

	// Ordenar Arboles por segun coordenadas crecientes de x,y
	OrdenarArboles();
	tupla send;

	/* Caculo optimo */
	Optimo->NumArboles = 0;
	Optimo->Coste = DMaximoCoste;

	avg = MaxCombinaciones / size;
	rest = MaxCombinaciones % size; //Anira de 0 a size-1
	end = avg;

	for(i=1; i<size; i++){
		ini = end + 1;
		end = ini + avg - 1;
		if (rest != 0){
			end ++;
			rest --;
		}
		send.ini = ini;
		send.end = end;
		MPI_Send(&send, 1, mpi_tupla_type, i, 22, MPI_COMM_WORLD);
		//Podria ser un MPI_Scatter amb un array de tuplas.
	}
	ini = 1;
	end = avg;
	CalcularCombinacionOptima(ini, end, Optimo);

	return true;
}



void OrdenarArboles(){
  int a,b;

	for(a=0; a<(ArbolesEntrada.NumArboles-1); a++){
		for(b=1; b<ArbolesEntrada.NumArboles; b++){
			if ( ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
				 (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y) ){

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


// Calcula la combinacin ptima entre el rango de combinaciones PrimeraCombinacion-UltimaCombinacion.
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo){
	int Combinacion, MejorCombinacion=0, CosteMejorCombinacion, i;
	int Coste, parcial[1], *recivebuf_coste, *recivebuf_combinaciones;
	TListaArboles OptimoParcial;

	TListaArboles CombinacionArboles;
	TVectorCoordenadas CoordArboles, CercaArboles;
	int NumArboles, PuntosCerca;
	float MaderaArbolesTalados;

	if(rank == ROOT){
		recivebuf_coste = (int *) malloc(sizeof(int) * size);
		recivebuf_combinaciones = (int *) malloc(sizeof(int) * size);
	}

  printf("Rank: %d. Analizo las combinaciones %d a %d \n", rank, PrimeraCombinacion, UltimaCombinacion);
	CosteMejorCombinacion = Optimo->Coste;
	for (Combinacion=PrimeraCombinacion; Combinacion<UltimaCombinacion + 1; Combinacion++){
		Coste = EvaluarCombinacionListaArboles(Combinacion);
		if ( Coste < CosteMejorCombinacion ){
			CosteMejorCombinacion = Coste;
			MejorCombinacion = Combinacion;
		}
	}

	printf("Rank: %d. La meva millor combinacio es %d i costa %d\n", rank, MejorCombinacion, CosteMejorCombinacion);
	parcial[0] = CosteMejorCombinacion;
	MPI_Gather(parcial, 1, MPI_INT, recivebuf_coste, 1, MPI_INT, ROOT, MPI_COMM_WORLD);
	parcial[0] = MejorCombinacion;
	MPI_Gather(parcial, 1, MPI_INT, recivebuf_combinaciones, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

	if(rank == ROOT){
		CosteMejorCombinacion = Optimo->Coste;
		for(i = 0; i < size; i++){
			if(CosteMejorCombinacion > recivebuf_coste[i]){
				CosteMejorCombinacion = recivebuf_coste[i];
				MejorCombinacion = recivebuf_combinaciones[i];
			}
		}

		ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
		printf("\n\rOPTIM: Combinacio numero %d\nCost: %d\n%d Arboles talados: ", MejorCombinacion, CosteMejorCombinacion, OptimoParcial.NumArboles);
		MostrarArboles(OptimoParcial);
		printf("\n");

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
	}
	return true;
}


int EvaluarCombinacionListaArboles(int Combinacion){
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

	// Evaluar si obtenemos suficientes arboles para construir la cerca
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


int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles){
	int arbol=0;

	CombinacionArboles->NumArboles=0;
	CombinacionArboles->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles){
		if ((Combinacion%2)==0){
			CombinacionArboles->Arboles[CombinacionArboles->NumArboles]=arbol;
			CombinacionArboles->NumArboles++;
			CombinacionArboles->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArboles->NumArboles;
}


int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados){
	int arbol=0;

	CombinacionArbolesTalados->NumArboles=0;
	CombinacionArbolesTalados->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles){
		if ((Combinacion%2)==1){
			CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles]=arbol;
			CombinacionArbolesTalados->NumArboles++;
			CombinacionArbolesTalados->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArbolesTalados->NumArboles;
}


void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas){
	int c, arbol;

	for (c=0;c<CombinacionArboles.NumArboles;c++){
    arbol=CombinacionArboles.Arboles[c];
		Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
		Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
	}
}


float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca){
	int x;
	float coste;

	for (x=0; x<(SizeCerca-1); x++)
		coste+= CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y, CoordenadasCerca[x+1].x, CoordenadasCerca[x+1].y);

	return coste;
}


float CalcularDistancia(int x1, int y1, int x2, int y2){
	return(sqrt(pow((double)abs(x2-x1),2.0)+pow((double)abs(y2-y1),2.0)));
}


int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles){
	int a;
	int LongitudTotal=0;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;

	return(LongitudTotal);
}


int CalcularCosteCombinacion(TListaArboles CombinacionArboles){
	int a;
	int CosteTotal=0;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;

	return(CosteTotal);
}


void MostrarArboles(TListaArboles CombinacionArboles){
	int a;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		printf("%d ",ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);

  for (;a<ArbolesEntrada.NumArboles;a++)
    printf("  ");
}
