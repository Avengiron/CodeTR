//			******PROJET TR 2eme annee********
//
//	Affichage de SMS sur afficheur industriel
//	Utilisation d'un modem GSM pour recevoir des SMS


//GOHORY
//GROS

// **/!\** Les fonctions avec des commentaires en anglais n'ont pas été codées par nous.

#pragma		config=0x20D4			// osc interne + Reset
#pragma		library 1
#include	"RS232.h"
#pragma		library 0
#pragma		codepage 0

///////////////////////////////////////////////////////////////////////////
// Definitions des entrées sorties (I/O)
//

//	I/O pour RESET et ON/OFF
#define		Out1	PORTA.1					//Bloc RESET
#define		Out2	PORTA.2					//Bloc ON/OFF

//  I/O Pour le max3232
#define		Tx_TTL	PORTA.3					//Tx_TTL du Max3232
#define		Rx_TTL	PORTA.5					//Rx_TTL du Max3232

//	I/O pour Visu LED et Test BP
#define     DEL1  	PORTC.1					//visu reception signal LED1
#define     DEL2 	PORTC.2					//visu reception signal LED2
#define		DEL3	PORTC.3					//visu reception signal LED3
#define		DEL4	PORTC.4					//visu reception signal LED4
#define		DEL5	PORTC.5					//LED de TEST

#define 	BP0		PORTB.0					//BP0 pour Test sur RB0, actif par niveau bas
#define 	BP1		PORTB.1					//BP1 pour test sur RB1, actif par niveau bas

//	I/O Pour le modem GSM : voir config UART0 dans RS232.h

////////////////////////////////////////////////////////////////////////////
// 							Définition des constantes
//
char bitCount, limit;
char serialData;
char chaine_envoye[20];         //tableau pour les caracteres émis par le PC
char chaine_recu [20];			//tableau issue du Telit

////////////////////////////////////////////////////////////////////////////
	
#define _4_MHz    /* pour UART soft sur tx_TTL */

// optional items
#define UseTMR0
#define USE_CONST		
#define NText 12


////////////////////////////////////////////////////////////////
//		if pour liaison RS232 pic vers DB9
//
#ifndef USE_CONST
char text( char i)
{
    skip(i);
   #pragma return[NText] = "Hello world!"
}
#else
const char text[NText] = "Hello world!";
#endif


////////////////////////////////////////////////////////////////////////////
// 							Prototypes des fonctions pour Pic
void InitPIC(void);
void InitGSM (void);
void sendData( char dout);
void Envoie_Text(const char *ptext);
void ON_GSM (void);
void getch ();
void tempoms (uns16 nbms);

//////////////////////////////////////////////////////////////////////////////
//		definition constante pour liaison RS232 pic -> PC	(9600@4M, 19200@8M)	
//		
#ifdef _4_MHz
 #define   TimeStartbit    4    //4 pour prescaler de 8
 #define   BitTimeIncr     13   //13 pour 56us par bit en 19200 (52,1us théorique => 8% err) prescaler de 8
#endif


//////////////////////////////////////////////////////////////////////////////
//
//
#ifdef _4_MHz
 #ifdef UseTMR0
   #define delayStart   limit = TMR0;
   #define delayOneBit      \
     limit += BitTimeIncr;  \
     while (limit != TMR0) \
         ;
 #else
   #define delayStart   /* empty */
   #define delayOneBit  {      \
        char ti;               \
        ti = 17;               \
        do ; while( --ti > 0); \
        nop();                 \
     }  /* total: 5 + 30*3 - 1 + 1 + 9 \
           = 104 microsec. at 4 MHz */
 #endif
#endif



//**************************************************************************
// 				Fonction principale 
//


void main(void)
{
	const char* M_Hello;
	unsigned long i,j=0;
	char fin;
	char temp=0;

	InitPIC();		//Initialisation I/O Logique et Analogique, Timer2, Pull Up ...
	InitRS232();	//Initialisation liaison RS232 disponible dans RS232.h (en headers)


	Envoie_Text("Init OK\0");
   	sendData(13);
	sendData(10);
	Out1=0;                     // Out1 correspond au reset qui ne doit
                                // pas etre active si on veux demarrer le GSM
								
	ON_GSM ();					//Fonction de simulation de l'appui du bouton ON/OFF
								//d'au moins une seconde, pour démarrer le GSM
    
	Envoie_Text("demarrage GSM OK\0");
	sendData(13);
	sendData(10);
	
	for (;;)                    // Boucle infinie
	{
				
        DEL5=1;
        DEL4=0;
        DEL3=0;
        DEL2=0;
        DEL1=0;
	
		getch();                // Fonction d'acquisition des commandes AT - 
		
		DEL1=1;	
		
		////////////////////////////////////////////////////////////////////////////////
		// Fonction envoie commande depuis PIC vers GSM
		// Fin de la trame avec le caractere CR
		// Envoie vers GSM
		// RS232_Envoie_Text_CR(chaine_envoye[i]);
        
		i=0;
		fin=0;

		do
		{	
			if (chaine_envoye[i]==CR)
				fin=1;	
			RS232_Envoie_Car(chaine_envoye[i]);
			i++;		
		}
		while (fin==0);
		
        Envoie_Text("RS232 Envoi :\0");
		sendData(13);
		sendData(10);	
	
        ////////////////////////////////////////////////////////////////////////////////
        // Attente du LF à la fin de l'echo	(2CR avant le LF)
		
		do
		{
			temp=RS232_Recu();
		}
		while (temp!=LF);
		
        Envoie_Text("RS232 Recu :\0");
		sendData(13);
		sendData(10);
		
        ////////////////////////////////////////////////////////////////////////////////
        // Attente de la reponse du GSM vers PC
		
		j=0;
		fin=0;
		
		do
		{
			temp=RS232_Recu();
			chaine_recu[j]=temp;
			if (chaine_recu[j]==LF)
				fin=1;
			j++;	
		}
		while (fin==0);


        DEL3=1;
        ////////////////////////////////////////////////////////////////////////////////
           
        Envoie_Text("Reponse GSM :\0");
		sendData(13);
		sendData(10);

        ////////////////////////////////////////////////////////////////////////////////
        // Decryptage de la reponse GSM vers PC
		
		fin=0;
		j=0;
		
		do
		{
			sendData(chaine_recu[j]);
			if (chaine_recu[j]==CR)
				fin=1;
			j++;
		}
		while (fin==0);
		
		j=0;	

		sendData(13);
		sendData(10);
        ////////////////////////////////////////////////////////////////////////////////
		
        DEL4=1;
	}
}		

/////////////////////////////////////////////////////////////////////////////////////
// Fonction de lecture des caracteres émis depuis le clavier du PC (+ stockage dans tableau)
//

void getch ()
{
	int i=0;		// Boucles de tableau
	int fin=0;		// Condition sortie lecture tableau

	Envoie_Text("Saisie des commandes\0");
	sendData(13);
	sendData(10);

	do
	{
		
	    while (Rx_TTL == 1);

 	 Start_RS232:				/* sampling once per bit */									//
   		 								//
   		TMR0 = 0;													//
   		limit = TimeStartbit;										//
  		while (limit != TMR0);										//
																	//
 		if (Rx_TTL == 1)											//	
		     goto StartBitError;									//
																	//
   		 bitCount = 8;												//
  		do  														//			
		{															//	
       		 limit += BitTimeIncr;									//	
       		 while (limit != TMR0);									//
																	//			
       		 Carry = Rx_TTL;										//
       		 serialData = rr( serialData);  /* rotate carry */		//
  		}															//
		while (-- bitCount > 0);									//
																	//
   		 limit += BitTimeIncr;										//		
   		 while (limit != TMR0);										//
   		 if (Rx_TTL == 0)											//
    	    goto StopBitError;										//

	/* ***** Remplissage du tableau ******* */ 
		chaine_envoye[i]=serialData;
		if(chaine_envoye[i]==CR)
			fin=1;
				//sendData(i+0x30);	//+0x30 pour conv ascii
		i++;
	}
	while(fin==0);
	fin=0;		// Interet ?
	
	sendData(13);
	sendData(10);
	
 	StopBitError:
 	StartBitError:
 	
}

/////////////////////////////////////////////////////////////////////////////
//						Fonction envoi de données
//							de l'UART soft

void sendData( char dout)	/* envoie un octet */
{
    Tx_TTL = 0;  /* startbit */
    delayStart
    for (bitCount = 9; ; bitCount--)  {
        delayOneBit
        if (bitCount == 0)
            break;
        Carry = 1;  /* incl. stopbit */
        dout = rr( dout);
        Tx_TTL = Carry;
    }
}
	 
///////////////////////////////////////////////////////////////////////////////////////////
// Fonction pour affichage d'un texte sur UART soft
// La chaîne de texte se termine par '0'

void Envoie_Text(const char *ptext)
{
	do
	{
		sendData(*ptext);
		ptext=ptext+1;
	}
	while (*ptext!=0);

	//sendData(13);
	//sendData(10);	
}


//////////////////////////////////////////////////////////////////////////////////////////
//Fonction de simulation de l'appui du bouton ON/OFF
//d'au moins une seconde, pour démarrer le module GSM

void ON_GSM (void) 
{
	Envoie_Text("Fonction ON/OFF\0");
	sendData(13);
	sendData(10);

	Out2=0;				//Bouton OFF
	tempoms(500);		//Attente de 1000ms

	Out2=1;				//Bouton ON
	tempoms(1200);		//Attente de 3000ms

	Out2=0;				//Bouton OFF
	tempoms(500);		//Attente de 5000ms
	
}


////////////////////////////////////////////////////////////////////////////////
//					Fonction de temporisation
//


void tempoms (uns16 nbms)			//Fonction qui permet de faire une temporisation
{
	uns16 i;				
	for(i=0; i<nbms; i++)
	{
		TMR2IF=0;					//	1 ms dans cette boucle
		TMR2=130;					//	si nbms=1000 alors
		while(TMR2IF==0);			// 	attente de 1 s
	}
}


////////////////////////////////////////////////////////////////////////////////
//					Fonction pour l'initialisation du PIC
//

void InitPIC(void)					//Par défault, Les ports sont mis entrée si ils ne servent pas
{
	OSCCON=0b.0111.0000;//8M	0b.0110.0000;			//4MHz

		/* * //// Initialisation des I/O \\\\ *  */
	ANSEL=0;						// Pas d'entrées analogiques
	ANSELH=0;
	TRISA=0b11110001;				// RA1,RA2,RA3 en sortie, le reste en entrée dont RA5
	TRISB=0b11111111;				// Tout le Port B en entrée
	TRISC=0b11000001;				// RC1 à RC6 en sortie et RC7 en entrée

	WPUB=0b00000011;				// Pull up interne sur les bits RB0 et RB1

	OPTION = 2;// 0 prescaler timer0 de 2				//2 prescaler timer0 par 8

	// Initialisation Timer2
	T2CON=0b00111100;			// Timer ON, Prescaler=1; Postscaler=8
}

//******************************* Fin du programme ****
