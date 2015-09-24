////////////////////////////////////////////////////////////////////////////////////// 
////    Lab 2 - Analog To Digital Converter 
////        -SAMPLE CODE DOES NOT WORK-
////            - insert variables into appropriate registers
////            - set up ADC pointer (similar to port_inst setup)
//////////////////////////////////////////////////////////////////////////////////////

#include <asf.h>

void Simple_Clk_Init(void);
void enable_adc_clocks(void);
void init_adc(void);
unsigned int read_adc(void);
    // Configure the ports for the voltage divider with the potentiometer
void init_vdivider(void);

PortGroup *porta = (PortGroup *)PORT;
Adc* adc = (Adc*)(ADC);

int main (void)
{    
    Simple_Clk_Init();
    enable_adc_clocks();
    init_adc();
    init_vdivider();
        
    volatile int x;
    
    while(1)
    {
        /*
            BIN = (0xFFF)(V_in * GAIN / V_ref)
            GAIN = 1
            V_ref = 1 V
        */
        x = read_adc();    //store variable from ADC into variable "x"
		x = 0;
    }
}

// set up generic clock for ADC
void enable_adc_clocks(void)
{
    PM->APBCMASK.reg |= 1 << 16;             // PM_APBCMASK enable is in the 16 position
    
    uint32_t temp = 0x17;                 // ID for ADC 0x17 (see table 14-2)
    temp |= 0<<8;                             // Selection Generic clock generator 0
    GCLK->CLKCTRL.reg = temp;                 // Setup in the CLKCTRL register
    GCLK->CLKCTRL.reg |= 0x1u << 14;         // enable it.
}

// initialize the on-board ADC system 
void init_adc(void)
{
    adc->CTRLA.reg &= ~(1 << 1u);       //ADC block is disabled

    /*
        BIN = (0xFFF)(V_in * GAIN / V_ref)
    */
    adc->REFCTRL.reg |= 0x2;        // Select a V_DD_AN/2 (1.65) reference
    adc->REFCTRL.reg &= ~(1<<7u);   // Disable REFCOMP for speed
    adc->AVGCTRL.reg |= 0x2 << 4u;  // Include additional 4 right shifts
                                    //  to use 12-bit resolution while
                                    //  sampling 16 times
    adc->AVGCTRL.reg |= 0x4;        // Now collect 16 samples at a time
        // Total sampling time length = (SAMPLEN+1)*(Clk_ADC/2)
    adc->SAMPCTRL.reg = 0x4;    // Set sampling time to 5 adc clock cycles
    adc->CTRLB.reg |= 0x1 << 8u;    // Relative to main clock, have adc clock
                                    //  run 8 times slower
    adc->CTRLB.reg &= ~(
            (1 << 3u)   // Disable digital correction
                        //  since there is no calibration yet
                        //  Correction: -OFFSETCORR*GAINCORR
            | (1 << 1u) // Right adjust the data in the RESULT register
            | (1 << 0u) // Don't use Diff. mode since input > 0 always.
        );
    adc->CTRLB.reg |= 1 << 2u;   // Enable free running to keep sampling
    adc->INPUTCTRL.reg |= 0x1 << 27u;   // Since reference is 1/2,
                                        //  set gain to 2 to keep largest
                                        // input voltage range (expected input
                                        //  will be 0 - 3.3V)
    adc->INPUTCTRL.reg |=
        0x7 << 8    // Not using the negative for differential, so ground it.
        | 0x0       // Connect the first, positive analog input to mux
                    //  (first one is also mapped to PA02)
        ;
    
    // config PA11 to be owned by ADC Peripheral

#define ADC_IN 11
    porta->PMUX[ADC_IN].reg |= 0x1 << 4;   // Choose function B, which is analog input
    porta->PINCFG[ADC_IN].reg |= 0x7;    // Enable pull up and enable pin as input.
                                    // Also connect pin to multiplexer to
                                    //  allow analog input
    
    adc->CTRLA.reg |= 1 << 1u;            //ADC block is enabled    
}

unsigned int read_adc(void)
{

    // start the conversion
    adc->SWTRIG.reg |= 1 << 1u;
        
    while(!adc->INTFLAG.bit.RESRDY);    //wait for conversion to be available
    
    adc->SWTRIG.reg &= ~(1 << 1u); // Ensure the conversion is not started
    return adc->RESULT.reg; // Extract stored value
    
}

void init_vdivider(void){
        // Use pin 13 for the voltage source of the voltage divider.
        //  Pin 11 will be connecteed to the output of the voltage divider.
    porta->DIR.reg |= (1 << 13u);
    porta->DIR.reg &= ~(1 << 11u);
        // Enable input and enable pull-up resistor
    porta->PINCFG[11].reg = PORT_PINCFG_INEN | PORT_PINCFG_PULLEN;

    porta->OUT.reg |= (1 << 13u); // Always keep the voltage divider on
}

//Simple Clock Initialization
void Simple_Clk_Init(void)
{
    /* Various bits in the INTFLAG register can be set to one at startup.
       This will ensure that these bits are cleared */
    
    SYSCTRL->INTFLAG.reg = SYSCTRL_INTFLAG_BOD33RDY | SYSCTRL_INTFLAG_BOD33DET |
            SYSCTRL_INTFLAG_DFLLRDY;
            
    system_flash_set_waitstates(0);  //Clock_flash wait state =0

    SYSCTRL_OSC8M_Type temp = SYSCTRL->OSC8M;      /* for OSC8M initialization  */

    temp.bit.PRESC    = 0;    // no divide, i.e., set clock=8Mhz  (see page 170)
    temp.bit.ONDEMAND = 1;    //  On-demand is true
    temp.bit.RUNSTDBY = 0;    //  Standby is false
    
    SYSCTRL->OSC8M = temp;

    SYSCTRL->OSC8M.reg |= 0x1u << 1;  //SYSCTRL_OSC8M_ENABLE bit = bit-1 (page 170)
    
    PM->CPUSEL.reg = (uint32_t)0;    // CPU and BUS clocks Divide by 1  (see page 110)
    PM->APBASEL.reg = (uint32_t)0;     // APBA clock 0= Divide by 1  (see page 110)
    PM->APBBSEL.reg = (uint32_t)0;     // APBB clock 0= Divide by 1  (see page 110)
    PM->APBCSEL.reg = (uint32_t)0;     // APBB clock 0= Divide by 1  (see page 110)

    PM->APBAMASK.reg |= 01u<<3;   // Enable Generic clock controller clock (page 127)

    /* Software reset Generic clock to ensure it is re-initialized correctly */

    GCLK->CTRL.reg = 0x1u << 0;   // Reset gen. clock (see page 94)
    while (GCLK->CTRL.reg & 0x1u ) {  /* Wait for reset to complete */ }
    
    // Initialization and enable generic clock #0

    *((uint8_t*)&GCLK->GENDIV.reg) = 0;  // Select GCLK0 (page 104, Table 14-10)

    GCLK->GENDIV.reg  = 0x0100;            // Divide by 1 for GCLK #0 (page 104)

    GCLK->GENCTRL.reg = 0x030600;           // GCLK#0 enable, Source=6(OSC8M), IDC=1 (page 101)
}