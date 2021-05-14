#include <MKL25Z4.h>
#include "utils.h"

/*----------------------------------------------------------------------------
  Function that initializes LEDs
 *----------------------------------------------------------------------------*/
void LED_Initialize(void) {

  SIM->SCGC5    |= (1 <<  10) | (1 <<  12);  /* Enable Clock to Port B & E */
  PORTB->PCR[18] = (1 <<  8) ;               /* Pin PTB22 is GPIO */
  PORTB->PCR[19] = (1 <<  8);                /* Pin PTB21 is GPIO */
  PORTD->PCR[1] = (1 <<  8);                /* Pin PTE26  is GPIO */
  
  PTB->PDOR = (1 << 18 | 1 << 19 );          /* switch Red/Green LED off  */
  PTB->PDDR = (1 << 18 | 1 << 19 );          /* enable PTB18/19 as Output */

  PTD->PDOR = 1 << 1;            /* switch Blue LED off  */
  PTD->PDDR = 1 << 1;            /* enable PTE26 as Output */
}

/*----------------------------------------------------------------------------
  Function that toggles the red LED
 *----------------------------------------------------------------------------*/

void LEDRed_Toggle (void) {
	PTB->PTOR = 1 << 18; 	   /* Red LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that toggles the blue LED
 *----------------------------------------------------------------------------*/
void LEDBlue_Toggle (void) {
	PTD->PTOR = 1 << 1; 	   /* Blue LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that toggles the green LED
 *----------------------------------------------------------------------------*/
void LEDGreen_Toggle (void) {
	PTB->PTOR = 1 << 19; 	   /* Green LED Toggle */
}

/*----------------------------------------------------------------------------
  Function that turns on Red LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDRed_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PCOR   = 1 << 18;   /* Red LED On*/
  PTB->PSOR   = 1 << 19;   /* Breen LED Off*/
  PTD->PSOR   = 1 << 1;    /* Blue LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns on Green LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDGreen_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 18;   /* Red LED Off*/
  PTB->PCOR   = 1 << 19;   /* Green LED On*/
  PTD->PSOR   = 1 << 1;    /* Blue LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns on Blue LED & all the others off
 *----------------------------------------------------------------------------*/
void LEDBlue_On (void) {
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 19;   /* Green LED Off*/
  PTB->PSOR   = 1 << 18;   /* Red LED Off*/
  PTD->PCOR   = 1 << 1;    /* Blue LED On*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

/*----------------------------------------------------------------------------
  Function that turns all LEDs off
 *----------------------------------------------------------------------------*/
void LED_Off (void) {	
	// Save and disable interrupts (for atomic LED change)
	uint32_t m;
	m = __get_PRIMASK();
	__disable_irq();
	
  PTB->PSOR   = 1 << 19;   /* Green LED Off*/
  PTB->PSOR   = 1 << 18;   /* Red LED Off*/
  PTD->PSOR   = 1 << 1;    /* Blue LED Off*/
	
	// Restore interrupts
	__set_PRIMASK(m);
}

void delay(void){
	int j;
	for(j=0; j<1000000; j++);
}
