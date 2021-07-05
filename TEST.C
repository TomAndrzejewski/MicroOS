/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"
#include "errno.h"
#include "limits.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        5       /* Number of identical tasks                          */
#define  TASK_READING_PRIOR				5
#define  TASK_PROCESSING_PRIOR			6
#define  TASK_DELTA_PRIOR				7
#define  TASK_DISPLAYING_PRIOR			8
#define  TASK_SEND_PRIOR				9
#define  TASK_QUEUE_PRIOR				10
#define  TASK_MAILBOX_PRIOR				15
#define  TASK_SEMAPHORE_PRIOR			20
#define  CHAR_LIMIT						10		/* Limit znakow w jednej linii */




/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_STK		TaskReadingStk[TASK_STK_SIZE];
OS_STK		TaskProcessingStk[TASK_STK_SIZE];
OS_STK		TaskDeltaStk[TASK_STK_SIZE];
OS_STK		TaskDisplayingStk[TASK_STK_SIZE];
OS_STK		TaskSendStk[TASK_STK_SIZE];
OS_STK		TaskQueueStk[N_TASKS][TASK_STK_SIZE];
OS_STK		TaskMailboxStk[N_TASKS][TASK_STK_SIZE];
OS_STK		TaskSemaphoreStk[N_TASKS][TASK_STK_SIZE];

	typedef struct {
		INT32U load;
		INT32U counter;
		INT8U task_number;
		INT8U delta_flag;
		INT8U key_flag;
		INT16S key;
	}Task_Data;
	
	typedef struct {
		INT32U load;
		INT8U task_number;
		INT32U msg_counter;
	}Load_Data;

INT8U 		TaskData[15];
INT32U 		global_load = 1;

OS_EVENT     *RandomSem;

OS_MEM *Mem;
INT32U Memory[100];
OS_MEM *Mem2;
Load_Data Memory2[100];

OS_EVENT *Mailbox;					/* Instancja skrzynki */
OS_EVENT *Mailbox_Send;				/* Instancja skrzynki do wysylania obciazenia pomiedzy zadaniem przetwarzajacym a wysylajacym */
OS_EVENT *Mailbox_Load[5];					/* Instancje skrzynek do zadan obciazajacych*/

void *Messages[200];					/* Tablica adresów przypisanych kolejce */
OS_EVENT *Queue;					/* Instancja kolejki */
void *Messages_Load[200];					/* Tablica adresów przypisanych kolejce */
OS_EVENT *Queue_Load;						/* Instancja kolejki do zadan obciazajacych*/





/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
		void  TaskReading(void *data);
		void  TaskProcessing(void *data);
		void  TaskDelta(void *data);
		void  TaskDisplaying(void *data);
		void  TaskSend(void *data);
		void  TaskQueue(void *data);
		void  TaskMailbox(void *data);
		void  TaskSemaphore(void *data);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
	INT8U *err;
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

    RandomSem   = OSSemCreate(1);                          /* Random number semaphore                  */
	Mem = OSMemCreate(Memory, 100, sizeof(INT32U), err);
	Mem2 = OSMemCreate(Memory2, 100, sizeof(Load_Data), err);

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    INT16S     key;
	INT8U i;
	
    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
	
	Mailbox = OSMboxCreate((void *)0);						/* Zainicjowanie obiektu skrzynki */
	Mailbox_Send = OSMboxCreate((void *)0);
	for(i = 0; i < 5; i++)
		Mailbox_Load[i] = OSMboxCreate((void *)0);
	
	Queue = OSQCreate(&Messages[0], 200);					/* Zainicjowanie obiektu kolejki */
	Queue_Load = OSQCreate(&Messages_Load[0], 200);
	
    TaskStartCreateTasks();                                /* Create all the application tasks         */

    for (;;) {
        TaskStartDisp();                                  /* Update the display                       */

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDispInit (void)
{
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr( 0,  0, "                         uC/OS-II, The Real-Time Kernel                         ", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr( 0,  1, "                     Tomasz Andrzejewski, Tomasz Budzynski                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "                                Grupa 1, sekcja 3                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "Tasks:           Counter:             Load:               Delta:      ERROR:    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, " Q  1                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, " Q  2                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, " Q  3                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, " Q  4                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, " Q  5                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, " M  6                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, " M  7                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, " M  8                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, " M  9                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, " M 10                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, " S 11                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, " S 12                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, " S 13                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, " S 14                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, " S 15                                                                           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "#Tasks          :                    Set Load:                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "#Task switch/sec:       CPU Usage:    [promile]  Frequency:    [Hz]             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY + DISP_BLINK);
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDisp (void)
{
    char   s[80];


    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(17, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

#if OS_TASK_STAT_EN > 0
    sprintf(s, "%4d", OSCPUUsage * 10);                                 /* Display CPU usage in %               */
    PC_DispStr(34, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5d", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(17, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    switch (_8087) {                                               /* Display whether FPU present          */
        case 0:
             PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 1:
             PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 2:
             PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 3:
             PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
    INT8U  i = 0;

				/* Tworzenie zadan do obslugi klawiatury */
	OSTaskCreate(TaskReading, (void *)0, &TaskReadingStk[TASK_STK_SIZE - 1], TASK_READING_PRIOR);
	OSTaskCreate(TaskDisplaying, (void *)0, &TaskDisplayingStk[TASK_STK_SIZE - 1], TASK_DISPLAYING_PRIOR);
	OSTaskCreate(TaskProcessing, (void *)0, &TaskProcessingStk[TASK_STK_SIZE - 1], TASK_PROCESSING_PRIOR);
	OSTaskCreate(TaskSend, (void *)0, &TaskSendStk[TASK_STK_SIZE - 1], TASK_SEND_PRIOR);
	
	OSTaskCreate(TaskDelta, (void *)0, &TaskDeltaStk[TASK_STK_SIZE - 1], TASK_DELTA_PRIOR);				//Zadanie wysylajace impuls co 1 sekunde, aby obliczac delte
	
				
	for (i = 1; i < 16; i++)					//Tworzenie tablicy z identyfikatorami zadan
		TaskData[i - 1] = i;
	
	for(i = 0; i < 5; i++)
		OSTaskCreate(TaskQueue, (void *)&TaskData[i], &TaskQueueStk[i][TASK_STK_SIZE - 1], TASK_QUEUE_PRIOR + i);		//Tworzenie zadan obciazajacych z kolejka
	
	for(i = 5; i < 10; i++)
		OSTaskCreate(TaskMailbox, (void *)&TaskData[i], &TaskMailboxStk[i - 5][TASK_STK_SIZE - 1], TASK_MAILBOX_PRIOR + i - 5);			//Tworzenie zadan obciazajacych z mailboxem
	
	for(i = 10; i < 15; i++)
		OSTaskCreate(TaskSemaphore, (void *)&TaskData[i], &TaskSemaphoreStk[i - 10][TASK_STK_SIZE - 1], TASK_SEMAPHORE_PRIOR + i - 10);	//Tworzenie zadan obciazajacych z semaforem

}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void  TaskReading(void *pdata)
{
	Task_Data key_data;						/* Zmienna zawierajaca kod ASCII wcisnietego klawisza z klawiatury*/
	key_data.counter = 0;
	key_data.load = 0;
	key_data.task_number = 0;
	key_data.key_flag = 1;
	key_data.delta_flag = 0;
	pdata = pdata;					/* Przypisanie antywarningowe */
	
	
	for (;;){
		if (PC_GetKey(&key_data.key))				/* Jesli klawisz na klawiaturze zostal wcisniety */
			OSMboxPost(Mailbox, &key_data);		/* Wyslij kod ASCII tego klawisza do skrzynki*/
		OSTimeDly(1);					/* Opoznienie o jeden tick systemowy */
	}
}

void TaskProcessing(void *pdata)
{
	Task_Data *key_data;					/* Wskaznik, ktory wskazuje na adres przechowywanej wiadomosci z kolejki */
	INT8U *err;						/* Zmienna zapisujaca kod bledu odbioru danych ze skrzynki */
	INT32U load = 0;				// Lokalne obciazenie
	int i = 0, j = 0;				// Zmienne iteracyjne
	char buffer[CHAR_LIMIT] = {' '};// Buffer na wpisywana z klawiatury liczbe
	pdata = pdata;					/* Przypisanie antywarningowe */
	
	
	for (;;){
		key_data = OSMboxPend(Mailbox, 0, err);		/* Odebranie danych ze skrzynki */
		if(*err == OS_NO_ERR){
			OSQPost(Queue, key_data);			/* Wyslanie odebranych danych do kolejki */
				switch(key_data->key){
					case(0x08):										/* Jesli wprowadzono backspace, cofnij sie o jeden indeks i wyzeruj ostatnia wartosc w buforze */
						i--;
						buffer[i] = ' ';
					break;
					case(0x0D):										/* Jesli wprowadzono enter, wyslij odczytana liczbe do zadan obciazajacych */
						load = strtoul(&buffer, NULL, 10);		// Konwersja z string na INT32U
						if((load == ULONG_MAX) && (errno == ERANGE))		//Jeœli strtoul() zwrocila przekroczenie zasiegu INT32U
							PC_DispStr(0, 4, "ERROR: WRITTEN NUMBER WAS TOO LARGE!", DISP_FGND_RED + DISP_BGND_LIGHT_GRAY);
						OSMboxPost(Mailbox_Send, &load);			//Wysyla wartosc obciazenia do zadania rozsylajacego
						for(j = 0; j <= i; j++)						//Zerowanie bufora
							buffer[j] = ' ';
						i = 0;
					break;
					default:					/* Jesli wprowadzono zwykly znak (nie enter lub backspace) zapisz go do bufora*/
						if(i < CHAR_LIMIT){
							buffer[i] = key_data->key;
							i++;
						}
					break;
				}
		}
	}
}

void TaskSend(void *pdata)
{
	INT32U *reciever;					// Wskaznik, do ktorego przypisujemy wartosc obciazenia wyslanego z zadania przetwarzajacego
	INT32U *cleaner;					// Wskaznik odpowiedzialny za czyszczenia mailboxa w przypadku, gdyby byl pelny
	INT8U *err_reciever;
	INT8U *err_mem_mailbox;				// Zmienna przechowujaca kod bledu z dyn. alokacji pamieci dla mailboxa
	INT8U *err_mem2_queue;				// Zmienna przechowujaca kod bledu z dyn. alokacji pamieci dla kolejki
	INT8U *err_sem;						// Zmienna przechowujaca kod bledu semafora
	INT8U err_mailbox;					// Zmienna przechowujaca kod bledu z wysylania wiadomosci przez mailbox
	INT8U err_queue;
	INT32U msg_counter = 0;				// Licznik serii wiadomosci wysylanych do kolejek, pozwala identyfikowac kolejnosc otrzymywania wiadomosci
	INT8U i = 0;						// Zmienne iterujace
	INT32U *ptr_load[5];				// Tablica wskaznikow, do ktorych przypisujemy adresy uzyskane przy dynamicznej alokacji pamieci podczas wysylania do mailboxow
	Load_Data *ptr_load_data[5];		// Tablica wskaznikow, do ktorych przypisujemy adresy uzyskane przy dynamicznej alokacji pamieci podczas wysylania do kolejek
	pdata = pdata;						// Przypisanie antywarninigowe
	
	for(;;){
		reciever = OSMboxPend(Mailbox_Send, 0, err_reciever);		//Odbieranie przetworzonego obciazenia
		if(*err_reciever == OS_NO_ERR){						//Jesli odebrano jakas wiadomosc
			OSSemPend(RandomSem, 0, err_sem);								// Blokowanie semafora dla ochrony zmiennej globalnej
			global_load = *reciever;										// Przypisanie obciazenia do zmiennej globalnej
			OSSemPost(RandomSem);											// Zwolnienie semafora
			
			msg_counter++;													// Zwiekszenie licznika serii wiadomosci
			for(i = 0; i < 5; i++){											// Wyslanie do pieciu zadan obciazajacych obciazenie poprzez kolejke
				ptr_load_data[i] = OSMemGet(Mem2, err_mem2_queue);			// Przypisanie do wskaznika adres na dynamicznie zaalokowana pamiec
				if(ptr_load_data != NULL){									// Jesli do wskaznika udalo sie przypisac adres
					ptr_load_data[i]->load = *reciever;						// Przypisanie otrzymanego obciazenia do pola zawierajacego obciazenie wysylanego do zadania z kolejka
					ptr_load_data[i]->task_number = i + 1;					// Wpisanie dla jakiego zadania ma byc ta wiadomosc
					ptr_load_data[i]->msg_counter = msg_counter;			// Wpisanie licznika serii wiadomosci, dzieki czemu wiadomo w jakiej kolejnosci zadania maja czytac z kolejki
					err_queue = OSQPost(Queue_Load, ptr_load_data[i]);					// Wyslanie wypelnionej struktury
					if(err_queue == OS_Q_FULL){
						PC_DispStr(69, 6 + i, "DATA LOST!", DISP_FGND_RED + DISP_BGND_LIGHT_GRAY);
						OSMemPut(Mem2, ptr_load_data[i]);
					}
				}
			}
			
			for(i = 0; i < 5; i++){												// Wysylanie obciazenia do mailboxow
				ptr_load[i] = OSMemGet(Mem, err_mem_mailbox);					// Zapisanie dyn. zaalokowanego adresu do wskaznika
				if(ptr_load[i] != NULL){										// Jesli udalo zapisac sie adres do wskaznika
					*ptr_load[i] = *reciever;									// Wpisanie wartosci obciazenia do wskaznika
					err_mailbox = OSMboxPost(Mailbox_Load[i], ptr_load[i]);		// Wstawienie wiadomosci do mailboxa
					if(err_mailbox == OS_MBOX_FULL){							// Jesli mailbox jest pelny
						cleaner = OSMboxAccept(Mailbox_Load[i]);				// Wyczysz mailbox
						OSMemPut(Mem, cleaner);											// Wyzeruj wskaznik czyszczacy mailboxa
						OSMboxPost(Mailbox_Load[i], ptr_load[i]);				// Wstaw nowa wiadomosc w mailboxa
						PC_DispStr(69, 11 + i, "DATA LOST!", DISP_FGND_RED + DISP_BGND_LIGHT_GRAY);
					}
				}
			}
		}
	}
}
	
void TaskDelta(void *pdata)
{
									// Zadanie to wysyla co sekunde impuls do zadania wyswietlajacego, aby zaktualizowal wartosci delt
	Task_Data flag;
	pdata = pdata;
	flag.counter = 0;
	flag.load = 0;
	flag.task_number = 0;
	flag.key_flag = 0;
	flag.delta_flag = 1;
	
	for(;;){
		OSQPost(Queue, &flag);
		OSTimeDlyHMSM(0, 0, 1, 0);
	}	
}	
	
void TaskDisplaying(void *pdata)
{
	Task_Data *reciever;					/* Wskaznik, ktory wskazuje na adres przechowywanej wiadomosci ze skrzynki */
	Task_Data *data_load[15];				// Wskazniki na struktury, ktore przechowuja dane o zadaniach obciazajacych
	Task_Data *data_bufor;					// Wskaznik na strukture, odbierajacy wiadomosc z kolejki danych z zadan obciazajacych
	INT8U *err_reciever;
	INT8U *err_disp;
	INT8U *err_data;
	INT8U *err_errorDisp;
	INT8U *error_to_display;				// Wskaznik na kod bledu, ktory ma byc wyswietlony na ekranie
	INT16U column = 0, char_counter = 0;	/* Zmienne sterujace dlugoscia i rozmieszczeniem wypisywanego tekstu */
	INT8U i = 0, j = 0;						/* Zmienna iterujaca */
	INT32U delta_counter[2][15];			// Wartosci przy pomocy ktorych obliczamy delty
	INT32U delta;
	char array[20] = {0};					/* Tablica znakow zapisanych w buforze */
	char s[30];
	char s1[30];
	char s2[30];
	pdata = pdata;							/* Przypisanie antywarningowe */

	for(i = 0; i < 15; i++){				// Zerowanie wartosci pomocniczych przy obliczaniu delty
		for(j = 0; j < 2; j++)
			delta_counter[0][i] = 0;
	}
	
	PC_DispStr(46, 22, " 1", DISP_FGND_YELLOW + DISP_BGND_BLUE);				// Startowa wartosc obciazenia
	PC_DispStr(59, 23, " 167", DISP_FGND_YELLOW + DISP_BGND_BLUE);				// Czestotliwosc pracy systemu
	
	for(;;){
		reciever = OSQPend(Queue, 0, err_reciever);									/* Pobranie znaku z kolejki */
		if(*err_reciever == OS_NO_ERR){												// Jesli odebrano znak
			switch(reciever->key_flag){
				case(1):															// Jesli key_flag == 1, to do wyswietlenia jest wpisany znak z klawiatury
					switch(reciever->key){
						case(0x0D):														/* Jesli wcisniety klawisz to ENTER */
							for(column = 0; column < CHAR_LIMIT + 1; column++){		/* Wyczysc wszystkie linie */
								PC_DispChar(column, 2, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);
								PC_DispChar(46 + column, 22, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);
							}
							
							for(column = 0; column < char_counter + 1; column++){	/* Skopiuj pierwsza linie do drugiej */
								if(column == 0)
									PC_DispChar(46 + column, 22, NULL, DISP_FGND_YELLOW + DISP_BGND_BLUE);
								PC_DispChar(46 + column, 22, array[column - 1], DISP_FGND_YELLOW + DISP_BGND_BLUE);
							}
							char_counter = 0;
						break;
						case(0x08):														/* Jesli wcisniety klawisz to BACKSPACE */
							if(char_counter != 0){			/* Gdy w pierwszej linijce jest znak */
								char_counter--;				/* Indeksem char_counter wroc na pozycje ostatniego znaku */
								PC_DispChar(char_counter, 2, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);	/* Nadpisz NULLem ostatni znak */
							}
						break;
						case(0x1B):														/* Jesli wcisniety klawisz to ESCAPE */
							PC_DOSReturn();							/* Powroc do DOSa */
						break;
						default:														/* Wcisnieto zwykly klawisz */
							if(char_counter < CHAR_LIMIT){												/* Gdy linijka nie jest pelna */
								PC_DispChar(char_counter, 2, reciever->key, DISP_FGND_WHITE + DISP_BGND_BLUE);	/* Napisz znak */
								array[char_counter] = reciever->key;												/* Zapisz wypisany znak do bufora */
								char_counter++;															/* Przesun sie o jedna kolumne w prawo */
							}
						break;
					}
				break;
				case(0):																		// Jesli key_flag == 0, to dane dotycza zadan obciazajacych
					switch(reciever->delta_flag){
						case(0):																// Jesli delta_flag == 0, to odebrano dane z zadania obciazajacego
							i = reciever->task_number - 1;
							delta_counter[1][i] = reciever->counter;
							for(j = 0; j < CHAR_LIMIT; j++){					// Czyszczenie linii z danymi zadan
								PC_DispChar(17 + j, reciever->task_number + 5, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);
								PC_DispChar(38 + j, reciever->task_number + 5, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);
							}
							sprintf(s, "%lu", reciever->counter);
							PC_DispStr(17, reciever->task_number + 5, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	// Wyswietlanie licznika
							sprintf(s1, "%lu", reciever->load);
							PC_DispStr(38, reciever->task_number + 5, s1, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);	// Wyswietlanie obciazenia
						break;
						case(1):																// Jesli delta_flag == 1, to odebrano sygnal do policzenia delty
							for(i = 0; i < 15; i++){								// Dla kazdego zadania:
								delta = delta_counter[1][i] - delta_counter[0][i];	// Licz delte
								delta_counter[0][i] = delta_counter[1][i];
								for(j = 0; j < 3; j++)								// Wyczysc stara wartosc delty na ekranie
									PC_DispChar(58 + j, i + 6, NULL, DISP_FGND_WHITE + DISP_BGND_LIGHT_GRAY);
								sprintf(s2, "%lu", delta);
								PC_DispStr(58, i + 6, s2, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);			// Wypisz nowa wartosc delty
							}
						break;
						default:
						break;
					}
				break;
				default:
				break;
			}
		}
	}
}

void TaskQueue(void *pdata)
{
	INT8U err_MemPut;						// Zapisuje kod bledu zwracania adresu uzyskanego z dyn. alokacji pamieci
	INT32U i = 0;							/* Zmienna iterujaca w petli obciazajacej */
	INT32U msg_counter = 0;					// Licznik serii wiadomosci
	INT32U incrementer = 0;					/* Zmienna do imitacji algorytmu w petli obciazajacej */
	Task_Data task_data;					/* Struktura z danymi przesylanymi do wyswietlenia */
	Load_Data *recieved_load;				// Odebrana struktura z wartoscia obciazenia oraz innymi danymi potrzebnymi do indentyfikacji dla jakiego zadania jest konkrenta wiadomosc
	
									//Wpisywanie domyslnych danych dla kazdego zadania
	task_data.task_number = *(INT8U *)pdata;
	task_data.counter = 0;
	task_data.load = 1;
	task_data.delta_flag = 0;
	task_data.key_flag = 0;
	
	for(;;){
		for(i = 0; i < 5; i++){														// 5 razy przeszukaj kolejke
			recieved_load = OSQAccept(Queue_Load);									/* Odebranie obciazenia z kolejki */
			if(recieved_load != NULL){												/* Jesli odebrano wiadomosc */
				if((recieved_load->task_number == task_data.task_number) && (recieved_load->msg_counter == msg_counter + 1)){	/* Jesli odebrano wiadomosc dla tego zadania*/
					msg_counter++;															// Zwieksz licznik serii wiadomosci
					task_data.load = recieved_load->load;									/* Zapisz do zmiennej obciazajacej wartosc, ktora wyslalismy */
					err_MemPut = OSMemPut(Mem2, recieved_load);								// Zwroc pamiec
					
					break;
				}
				else{															/* Jesli odebrano wiadomosc nie dla tego zadania */
					OSQPost(Queue_Load, recieved_load);					/* Odeslij ja do kolejki */
				}
			}
		}
		task_data.counter++;											// Inkrementacja licznika wykonania zadania
		OSQPost(Queue, &task_data);								// Wyslanie danych o zadaniu
		for(i = 0; i < task_data.load; i++)								// Petla obciazajaca
			incrementer++;
		OSTimeDly(1);
	}
}

void TaskMailbox(void *pdata)
{
	INT8U err_MemPut;						// Zapisuje kod bledu zwracania adresu uzyskanego z dyn. alokacji pamieci
	INT32U i = 0;							/* Zmienna iterujaca w petli obciazajacej */
	INT32U incrementer = 0;					/* Zmienna do imitacji algorytmu w petli obciazajacej */
	INT32U *recieved_load;					// Wskaznik do ktorego przypisujemy otrzymana wartosc obciazenia
	Task_Data task_data;					/* Struktura z danymi przesylanymi do wyswietlenia */
	
									//Wpisywanie domyslnych danych dla kazdego zadania
	task_data.task_number = *(INT8U *)pdata;
	task_data.counter = 0;
	task_data.load = 1;
	task_data.delta_flag = 0;
	task_data.key_flag = 0;
	
	for(;;){
		recieved_load = OSMboxAccept(Mailbox_Load[task_data.task_number - 6]);					// Odebranie wiadomosci z odpowiedniego mailboxa (rozny dla kazdego zadania)
		if(recieved_load != NULL){												// Jesli odebrano wiadomosc
			task_data.load = *recieved_load;									// Zapisz odebrane obciazenie
			err_MemPut = OSMemPut(Mem, recieved_load);							// Zwolnij pamiec
		}
		
		task_data.counter++;											// Inkrementacja licznika wykonania zadania
		OSQPost(Queue, &task_data);								// Wyslanie danych o zadaniu
		for(i = 0; i < task_data.load; i++)								// Petla obciazajaca
			incrementer++;
		OSTimeDly(1);
	}
}

void TaskSemaphore(void *pdata)
{
	INT16U value;							// Zmienna, do ktorej OSSemAccept wpisuje, czy semafor jest wolny czy zajety
	INT32U i = 0;							/* Zmienna iterujaca w petli obciazajacej */
	INT32U incrementer = 0;					/* Zmienna do imitacji algorytmu w petli obciazajacej */
	Task_Data task_data;					/* Struktura z danymi przesylanymi do wyswietlenia */
	
									//Wpisywanie domyslnych danych dla kazdego zadania
	task_data.task_number = *(INT8U *)pdata;
	task_data.counter = 0;
	task_data.load = 1;
	task_data.delta_flag = 0;
	task_data.key_flag = 0;
	
	for(;;){
		value = OSSemAccept(RandomSem);							// Sprawdzenie czy semafor jest wolny czy zajety
		if(value != 0){											// Gdy semafor jest wolny
			task_data.load = global_load;						// Zapisz wartosc obciazenia ze zmiennej globalnej do lokalnej
			OSSemPost(RandomSem);								// Zwolnij semafor
		}
		
		task_data.counter++;											// Inkrementacja licznika wykonania zadania
		OSQPost(Queue, &task_data);								// Wyslanie danych o zadaniu
		for(i = 0; i < task_data.load; i++)								// Petla obciazajaca
			incrementer++;
		OSTimeDly(1);
	}
}



