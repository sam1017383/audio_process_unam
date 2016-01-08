//###########################################################################
//
// TITLE:   Proyecto de procesamiento de audio.
//
//! Este proyecto:
//! - Configura interrupciones por tiempo del timer 0 para una frecuenia de muestreo a 20 KHz
//! - Captura dos señales analogicas por los pines A0 y A1
//! - Invoca una subrutina en ensamblador
//! - Envia un dato de 12 bits por el convertidor a bordo DAC
//
//###########################################################################
//! Basado en codigo de ejemplo proveido por Texas Instruments
// $TI Release: F2837xS Support Library v180 $
// $Release Date: Fri Nov  6 16:27:58 CST 2015 $
// $Copyright: Copyright (C) 2014-2015 Texas Instruments Incorporated -
//             http://www.ti.com/ ALL RIGHTS RESERVED $
//###########################################################################

// constantes de configuracion
#define REFERENCE_VDAC     1 // Convertidor DAC sin referencia externa
#define CPU_CLOCK_100      100 // CPU a 100 MHZ
#define INT_PERIOD_uS      50 // Periodo de interrupcion a 50 microS, 20,000 Hz

// constantes
#define WINDOW_LENGTH 30

// variables globales
int contador_interrupciones = 0;
int inicio = 1;
int fin = 1;

//variables para almacenar resultados de la conversion ADC
int AdcaResult0;
int window[WINDOW_LENGTH];
short newest;




#include "F28x_Project.h"     // Device Headerfile and Examples Include File
#include "F2837xS_GlobalPrototypes.h"

// Prototype statements for functions found within this file.
__interrupt void cpu_timer0_isr(void);


void ConfigureADC(void);
void SetupADCSoftware(void);
void mi_rutina_ensamblador(void);
int  capture_from_adc_a0(void);
int send_to_dac(int);
int capture_to_window(void);




// Main interruption subrutine
__interrupt void cpu_timer0_isr(void){
   CpuTimer0.InterruptCount++;
   if (CpuTimer0.InterruptCount == 20000) {
	   CpuTimer0.InterruptCount = 0;
   GpioDataRegs.GPATOGGLE.bit.GPIO12 = 1; // Toggle GPIO34 once per 500 milliseconds
   }

   //envia valor analogico al pin 24
   send_to_dac(222);
   //captura valor analogico de pin 27
   AdcaResult0 = capture_from_adc_a0();

   //envia valor analogico al pin 24
   send_to_dac(888);
   //captura valor analogico de pin 27
   AdcaResult0 = capture_from_adc_a0();

   //subrutinas de filtrado
   mi_rutina_ensamblador();



   // Acknowledge this __interrupt to receive more __interrupts from group 1
   PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}




//Write ADC configurations and power up the ADC for both ADC A and ADC B
void ConfigureADC(void){
	EALLOW;
	//write configurations
	AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4
	AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
	//Set pulse positions to late
	AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;
	//power up the ADCs
	AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;
	//delay for 1ms to allow ADC time to power up
	DELAY_US(1000);
	EDIS;
}

void SetupADCSoftware(void){
	Uint16 acqps;

	//determine minimum acquisition window (in SYSCLKS) based on resolution
	if(ADC_RESOLUTION_12BIT == AdcaRegs.ADCCTL2.bit.RESOLUTION){
		acqps = 14; //75ns
	}
	else { //resolution is 16-bit
		acqps = 63; //320ns
	}


//Select the channels to convert and end of conversion flag
    //ADCA
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin A0 =
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin A1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 1; //end of SOC1 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    EDIS;
}

send_to_dac(int data){
	//verify data is a valid 12 bit positive value
	//DAC C value is pin 24 compartido con ADCINB1
	if (data > 1 && data < 1023){
		DaccRegs.DACVALS.bit.DACVALS = data;
		return 1;
	}
	if (data >= 1023){
			DaccRegs.DACVALS.bit.DACVALS = 1023;
		}else{
			DaccRegs.DACVALS.bit.DACVALS = 0;
		}
	return 0;

}

capture_from_adc_a0(void){
	//start conversions immediately via software, ADCA
	AdcaRegs.ADCSOCFRC1.all = 0x0003; //SOC0 Y SOC1
	//wait for ADCA to complete, then acknowledge flag
	while(AdcaRegs.ADCINTFLG.bit.ADCINT1 == 0);
	AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

	// Puerto A0 es pin 27, A1 es el pin 29, del conjunto j3 tarjeta LauchPad XL
	return AdcaResultRegs.ADCRESULT0;

}

capture_to_window(void){
	//To do
	//hacer una estructura de ventana que tenga un campo de datos, valor actual, etc
	//para hacer un buffer circular con acceso organizado de la estrucuta.s
	return 0;
}



void main(void){

   InitSysCtrl();
   DINT;
   InitPieCtrl();
   IER = 0x0000;
   IFR = 0x0000;
   InitPieVectTable();
   EALLOW;  // This is needed to write to EALLOW protected registers
   PieVectTable.TIMER0_INT = &cpu_timer0_isr;
   EDIS;    // This is needed to disable write to EALLOW protected registers
   InitCpuTimers();   // For this example, only initialize the Cpu Timers
   // 100MHz CPU Freq, 50 microsecond Period (in uSeconds) 20,000 Hz
   ConfigCpuTimer(&CpuTimer0, CPU_CLOCK_100, INT_PERIOD_uS);
   CpuTimer0Regs.TCR.all = 0x4001; // Use write-only instruction to set TSS bit = 0

   // Configure GPIO12 (LED rojo) as a GPIO output pin
   EALLOW;
   GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0; // declare GPIO
   GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;  // decalre as output
   EDIS;

   // Enable CPU INT1 which is connected to CPU-Timer 0:
   IER |= M_INT1;
   // Enable TINT0 in the PIE: Group 1 __interrupt 7
   PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

   //Configure the ADCs and power them up
   ConfigureADC();
   //Setup the ADCs for software conversions
   SetupADCSoftware();

   // Enable DACOUTA
   EALLOW;
   //Use VDAC as the reference for DAC
   DaccRegs.DACCTL.bit.DACREFSEL  = REFERENCE_VDAC;
   //Enable DAC output
   DaccRegs.DACOUTEN.bit.DACOUTEN = 1;
   EDIS;

// Enable global Interrupts and higher priority real-time debug events:
   EINT;   // Enable Global __interrupt INTM
   ERTM;   // Enable Global realtime __interrupt DBGM

// Step 6. IDLE loop. Just sit and loop forever (optional):
   for(;;);
}


//===========================================================================
// No more.
//===========================================================================
