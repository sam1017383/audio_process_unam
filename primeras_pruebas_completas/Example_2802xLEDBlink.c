//Laboratorio de procesammiento digital de señales UNAM
/* Este programa configura los GPIO  de 0-7,12 como salidas digitales
 * configura la interrupcion del timer 0 ligada a la subrutina conversion_filtrado
 * en codigo se tienen  diferentes frecuencia de muestreo: 1Hz,4KHz,8KHz y 16KHz
 * dentro de la subrutina conversion_filtrado se forza la conversion de un canal ADC y
 * usando poleo el programa espera a que el ADC temine de convertir la señal
 * un la misma subturina se invoca a una subrutina en ensamblador para sumar dos variables
 * en C que se pasan por referencia */


#include "DSP28x_Project.h"     // importar cabezeras de configuracion del CPU que se utilize
//cabezeras de configuracion de perifericos
#include "f2802x_common/include/adc.h"
#include "f2802x_common/include/clk.h"
#include "f2802x_common/include/flash.h"
#include "f2802x_common/include/gpio.h"
#include "f2802x_common/include/pie.h"
#include "f2802x_common/include/pll.h"
#include "f2802x_common/include/timer.h"
#include "f2802x_common/include/wdog.h"

ADC_Handle myAdc;
CLK_Handle myClk;
FLASH_Handle myFlash;
GPIO_Handle myGpio;
PIE_Handle myPie;
TIMER_Handle myTimer;


// Declaracion de los prototipos de funciones
//Subrutina de interrupcion
interrupt void conversion_filtrado(void);
//Subrutina de ensamblador
extern void test(void);
//Declaracion de variables globales
int16 x ;
int16 senal[20];
int16 i=0;
int inicio;
int fin;



void main(void)
{

    CPU_Handle myCpu;
    PLL_Handle myPll;
    WDOG_Handle myWDog;
    
    // Configura con los valores predeterminados
    myAdc = ADC_init((void *)ADC_BASE_ADDR, sizeof(ADC_Obj));
    myClk = CLK_init((void *)CLK_BASE_ADDR, sizeof(CLK_Obj));
    myCpu = CPU_init((void *)NULL, sizeof(CPU_Obj));
    myFlash = FLASH_init((void *)FLASH_BASE_ADDR, sizeof(FLASH_Obj));
    myGpio = GPIO_init((void *)GPIO_BASE_ADDR, sizeof(GPIO_Obj));
    myPie = PIE_init((void *)PIE_BASE_ADDR, sizeof(PIE_Obj));
    myPll = PLL_init((void *)PLL_BASE_ADDR, sizeof(PLL_Obj));
    myTimer = TIMER_init((void *)TIMER0_BASE_ADDR, sizeof(TIMER_Obj));
    myWDog = WDOG_init((void *)WDOG_BASE_ADDR, sizeof(WDOG_Obj));
    
    // Inicializa el sistema
    WDOG_disable(myWDog);
    CLK_enableAdcClock(myClk);
    (*Device_cal)();
    
    //Selecciona el oscilador interno
    CLK_setOscSrc(myClk, CLK_OscSrc_Internal);
    
    // Configura el preescalador del reloj interno  PLL for x10 /2 which will yield 50Mhz = 10Mhz * 10 / 2
    PLL_setup(myPll, PLL_Multiplier_10, PLL_DivideSelect_ClkIn_by_2);
    
    // Desabilita todo el modulo de interrupciones
    PIE_disable(myPie);
    PIE_disableAllInts(myPie);
    CPU_disableGlobalInts(myCpu);
    CPU_clearIntFlags(myCpu);
    
    // If running from flash copy RAM only functions to RAM   
#ifdef _FLASH
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
#endif      

    // Setup a debug vector table and enable the PIE
    PIE_setDebugIntVectorTable(myPie);
    PIE_enable(myPie);
    //Asocia la subrutina conversion_filtrado a la interrupcion del timer 0
    PIE_registerPieIntHandler(myPie, PIE_GroupNumber_1, PIE_SubGroupNumber_7, (intVec_t)&conversion_filtrado);
    TIMER_stop(myTimer);
    //diferentes configuraciones del periodo de interrupcion
    //TIMER_setPeriod(myTimer, 50 * 1000000);   //1 Segundo
    TIMER_setPeriod(myTimer, 50 * 125);  //8K  125 micros  voz
    // TIMER_setPeriod(myTimer, 3125); // 16K   Audio
    //TIMER_setPeriod(myTimer, 1133); 40K   Musica
    TIMER_setPreScaler(myTimer, 0);
    TIMER_reload(myTimer);
    TIMER_setEmulationMode(myTimer, TIMER_EmulationMode_StopAfterNextDecrement);
    TIMER_enableInt(myTimer);
    TIMER_start(myTimer);

    // Configure GPIO 0-7 y 12 en modo proposito general
    GPIO_setMode(myGpio, GPIO_Number_0, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_1, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_2, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_3, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_4, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_5, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_6, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_7, GPIO_0_Mode_GeneralPurpose);
    GPIO_setMode(myGpio, GPIO_Number_12, GPIO_0_Mode_GeneralPurpose);

    //GPIO los configuramos como salidas
    GPIO_setDirection(myGpio, GPIO_Number_0, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_1, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_2, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_3, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_4, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_5, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_6, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_7, GPIO_Direction_Output);
    GPIO_setDirection(myGpio, GPIO_Number_12, GPIO_Direction_Output);
    
    //Ponemos en alto o bajo los puerto previamente configuramos
    GPIO_setHigh(myGpio, GPIO_Number_0);
    GPIO_setHigh(myGpio, GPIO_Number_1);
    GPIO_setLow(myGpio, GPIO_Number_2);
    GPIO_setLow(myGpio, GPIO_Number_3);
    GPIO_setHigh(myGpio, GPIO_Number_4);
    GPIO_setHigh(myGpio, GPIO_Number_5);
    GPIO_setLow(myGpio, GPIO_Number_6);
    GPIO_setLow(myGpio, GPIO_Number_7);
    GPIO_setHigh(myGpio, GPIO_Number_12);


    // Sea abilitan interrupciones del grupo 1
    CPU_enableInt(myCpu, CPU_IntNumber_1);

    // Sea abilitan interrupciones del grupo 1 subgrupo 7
    PIE_enableTimer0Int(myPie);

    // Abilitar interrupciones globales
    CPU_enableGlobalInts(myCpu);
    CPU_enableDebugInt(myCpu);

    //Configuracion del ADC
    ADC_enableBandGap(myAdc);
    ADC_enableRefBuffers(myAdc);
    ADC_powerUp(myAdc);
    ADC_enable(myAdc);
    ADC_setVoltRefSrc(myAdc, ADC_VoltageRefSrc_Int);
    //Asociamos el SOC(inicio de conversion) al canal analogico A4
    ADC_setSocChanNumber (myAdc, ADC_SocNumber_1, ADC_SocChanNumber_A4);
    ADC_setSocSampleWindow(myAdc, ADC_SocNumber_1, ADC_SocSampleWindow_7_cycles);   //Set SOC1 acquisition period to 7 ADCCLK
    ADC_setIntSrc(myAdc, ADC_IntNumber_1, ADC_IntSrc_EOC1);                 //Connect ADCINT1 to EOC1
    ADC_enableInt(myAdc, ADC_IntNumber_1);

    //ciclo principal
    for(;;){
     asm(" NOP");
     if(i> 99){
    	 i=0;
     }
    }
    
}

//Subrutina de interrupcion
interrupt void conversion_filtrado(void)
{
    //Inicializacion de variables que se pasan por referencia a la subrutina de ensamblador
    inicio=56;
    fin= 256;
    //Subrutina en ensamblador que realiza fin= inicio +fin
    test();
     // ADC
     //Inicia la conversion del canal A4
     ADC_forceConversion(myAdc, ADC_SocNumber_1);

     //Espera el fin de la conversion usando poleo
     while(ADC_getIntStatus(myAdc, ADC_IntNumber_1) == 0) {
      }

      // Limpia ADCINT1
       ADC_clearIntFlag(myAdc, ADC_IntNumber_1);

      // Se copia el valor de la conversion a la variable x
      x = ADC_readResult(myAdc, ADC_ResultNumber_1);
      //Se envia el valor de x recorrido 4 lugares al GPIO 0-7
      ((GPIO_Obj *)myGpio)->GPADAT = x >> 4;

       senal[i]= x;
       i++;

       //Limpia la bandera de interrupcion
       PIE_clearInt(myPie, PIE_GroupNumber_1);
}


//===========================================================================
// No more.
//===========================================================================
