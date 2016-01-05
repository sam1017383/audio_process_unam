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



// variables globales
int contador_interrupciones = 0;
int inicio = 1;
int fin = 1;

//variables para almacenar resultados de la conversion ADC
int AdcaResult0;
int AdcaResult1;
int AdcbResult0;
int AdcbResult1;

// constantes de configuracion
#define REFERENCE_VDAC     1 // Convertidor DAC sin referencia externa
#define CPU_CLOCK_100      100 // CPU a 100 MHZ
#define INT_PERIOD_uS      50 // Periodo de interrupcion a 50 microS, 20,000 Hz



#include "F28x_Project.h"     // Device Headerfile and Examples Include File
#include "F2837xS_GlobalPrototypes.h"

// Prototype statements for functions found within this file.
__interrupt void cpu_timer0_isr(void);


void ConfigureADC(void);
void SetupADCSoftware(void);
extern void mi_rutina_ensamblador(void);

void main(void)
{

   InitSysCtrl();
// InitGpio();  // Skipped for this example
   DINT;
   InitPieCtrl();

// Disable CPU __interrupts and clear all CPU __interrupt flags:
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


__interrupt void cpu_timer0_isr(void){
   DaccRegs.DACVALS.bit.DACVALS = 0;
   DaccRegs.DACVALS.bit.DACVALS = 1000;
   DaccRegs.DACVALS.bit.DACVALS = 2000;
   DaccRegs.DACVALS.bit.DACVALS = 4000;
   CpuTimer0.InterruptCount++;
   mi_rutina_ensamblador();
   if (contador_interrupciones == 20000) {
      contador_interrupciones = 0;
   GpioDataRegs.GPATOGGLE.bit.GPIO12 = 1; // Toggle GPIO34 once per 500 milliseconds
   }
   //start conversions immediately via software, ADCA
   AdcaRegs.ADCSOCFRC1.all = 0x0003; //SOC0 and SOC1
   //wait for ADCA to complete, then acknowledge flag
   while(AdcaRegs.ADCINTFLG.bit.ADCINT1 == 0);
   AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;

   //store results
   AdcaResult0 = AdcaResultRegs.ADCRESULT0;
   AdcaResult1 = AdcaResultRegs.ADCRESULT1;

   //at this point, conversion results are stored in
   //AdcaResult0, AdcaResult1, AdcbResult0, and AdcbResult1

   //software breakpoint, hit run again to get updated conversions
   asm("   ESTOP0");

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
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 0;  //SOC0 will convert pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = 1;  //SOC1 will convert pin A1
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps; //sample window is acqps + 1 SYSCLK cycles
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 1; //end of SOC1 will set INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;   //enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; //make sure INT1 flag is cleared
    EDIS;
}
//===========================================================================
// No more.
//===========================================================================
