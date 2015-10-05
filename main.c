#include <asf.h>

#define ADC_IN 11   // Use pin 11 for analog input from voltage divider
#define POT_SRC 13  // Pin POT_SRC supplies voltage to voltage divider circuit
#define DAC_OUT 2   // Use pin 2 to output generated waveform to the speaker

    /**********   Start type aliasing   **********/
#include <stdint.h>

#define INT32   int32_t
#define UINT32  uint32_t
#define UINT8   uint8_t

#define BOOLEAN__     UINT8
#define TRUE__        1
#define FALSE__       0
    /**********   End type aliasing   **********/

void run_volt_meter(void);
void run_tone_generator(void);

void configure_modules(void);
void Simple_Clk_Init(void);
void enable_analog_clks(void);
void init_adc(void);    // Internally enables adc
void init_dac(void);    // Internally enables dac
void enable_adc(void);
void disable_adc(void);
void enable_dac(void);
void disable_dac(void);
unsigned int read_adc(void);
    // Configure the ports for the voltage divider with the potentiometer
void init_vdivider(void);

void configure_ssd_ports(void);
void display_dig(
    UINT32 add_delay, UINT8 num, UINT8 select,
    BOOLEAN__ show_dot, BOOLEAN__ show_sign
);

Port* port;
PortGroup *porta;
PortGroup *portb;
Adc* adc;
Dac* dac;

int main (void)
{    
    configure_modules();
	delay_init();
    Simple_Clk_Init();
    configure_ssd_ports();

    run_volt_meter();

    return 0;
}

void run_volt_meter(void){
    enable_analog_clks();
    init_vdivider();
    init_adc();
        
    UINT32 x = 0;
    unsigned row_sel = 0;
    const UINT32
        min_pos = 100, max_pos = 1000*min_pos,
        bin_max = 65535, volt_max = 330,
        volt_max_prec = volt_max*min_pos,
        show_dec_pos = 1000*min_pos
        ;
    UINT32 dec_position = min_pos;
    const UINT32 offset = 2*min_pos;  // Manual offset for value correction
    
    while(1)
    {
        /*
            BIN = (0xFFF)(V_in * GAIN / V_ref)
            GAIN = 1/2
            V_ref = (1/2)(3.3) V

			V_in = BIN/0xFFF * 3.3V
        */
        x = read_adc();    //store variable from ADC into variable "x"

            // Map from 0 - 2^16-1 to 0 - 3.3 V
        x = (x*volt_max_prec)/bin_max;
        if(x > offset)  x -= offset;

            // Display one digit at a time
        display_dig(
            100, ((x%dec_position)*10)/dec_position, row_sel,
            dec_position == show_dec_pos, FALSE__
            );
		dec_position *= 10;
        if(dec_position > max_pos) dec_position = min_pos;
        row_sel = (row_sel+1u)%4u;
    }
}

void run_tone_generator(void){
    
}

void configure_modules(void){
    port = (Port*)(PORT);
    porta = (PortGroup *)(&port->Group[0]);
    portb = (PortGroup*)(&port->Group[1]);
    adc = (Adc*)(ADC);
    dac = (Dac*)(DAC);
}

// set up generic clock for ADC
void enable_analog_clks(void){
    PM->APBCMASK.reg |= 1 << 16;             // PM_APBCMASK enable is in the 16 position
    
    uint32_t temp = 0x17;                 // ID for ADC 0x17 (see table 14-2)
    temp |= 0<<8;                             // Selection Generic clock generator 0
    GCLK->CLKCTRL.reg = temp;                 // Setup in the CLKCTRL register
    GCLK->CLKCTRL.reg |= 0x1u << 14;         // enable it.
}

// initialize the on-board ADC system 
void init_adc(void){
    disable_adc();

    /*
        BIN = (0xFFF)(V_in * GAIN / V_ref)
    */
    adc->REFCTRL.reg |= 0x2;        // Select a V_DD_AN/2 (1.65) reference
    adc->AVGCTRL.reg |= 0x8;        // Now collect 256 samples at a time.
                                    //  Theoretical result has 20-bit precision.
                                    //  ADC will automatically right shift
                                    //  6 times, so result has 16-bit precision.
        // Total sampling time length = (SAMPLEN+1)*(Clk_ADC/2)
    adc->SAMPCTRL.reg = 0x5;    // Set sampling time to 3 adc clock cycle?
    adc->CTRLB.reg |= 0x2 << 8u;    // Relative to main clock, have adc clock
                                    //  run 8 times slower
    adc->CTRLB.reg |= 0x1 << 4; // For averaging more than 2 samples,
                                //  change RESSEL (0x1 for 16-bit)
    adc->INPUTCTRL.reg |= 0xF << 24u;   // Since reference is 1/2,
                                        //  set gain to 1/2 to keep largest
                                        // input voltage range (expected input
                                        //  will be 0 - 3.3V)
    adc->INPUTCTRL.reg |=
        0x18 << 8    // Not using the negative for differential, so ground it.
        | 0x13       // Map the adc to analog pin 19, which is mapped to ADC_IN
        ;
    
    // config PA11 to be owned by ADC Peripheral

    porta->PMUX[(ADC_IN - 1)/2].reg |= 0x1 << 4;    // Choose function B, which is analog input
                                            // Shift by 4 to enter into the odd register
    porta->PINCFG[ADC_IN].reg |= 0x1;   // Connect pin to multiplexer to
                                        //  allow analog input
    
    enable_adc();
}

void init_dac(void){
	porta->PINCFG[DAC_OUT].reg |= 0x1; // Enable multiplexing
	porta->PMUX[DAC_OUT/2].reg |= 0x1; // Set to function B, which include DAC

        // With each register change, wait for synchronization

	while (dac->STATUS.reg & DAC_STATUS_SYNCBUSY);

    dac->CTRLB.reg |= 0x1 << 6; // Set reference to be V_dd_ana

	while (dac->STATUS.reg & DAC_STATUS_SYNCBUSY);

    enable_dac();

	dac->CTRLB.reg |= 0x1;  // Enable DAC output to Vout

	while (dac->STATUS.reg & DAC_STATUS_SYNCBUSY);
}

void enable_adc(void){
    adc->CTRLA.reg |= 0x2;  //ADC block is enabled   
}

void disable_adc(void){
    adc->CTRLA.reg &= ~0x2;  //ADC block is disabled   
}

void enable_dac(void){
    dac->CTRLA.reg |= 0x2; // Enable DAC

	while (dac->STATUS.reg & DAC_STATUS_SYNCBUSY);  // Synchronize clock
}

void disable_dac(void){
    dac->CTRLA.reg &= ~0x2; // Disable DAC
}

unsigned int read_adc(void){

    // start the conversion
    adc->SWTRIG.reg |= 1 << 1u;
        
    while(!adc->INTFLAG.bit.RESRDY);    //wait for conversion to be available
    
    return adc->RESULT.reg; // Extract stored value
    
}

void init_vdivider(void){
        // Use pin POT_SRC for the voltage source of the voltage divider.
    porta->DIR.reg |= (1 << POT_SRC);

    porta->OUT.reg |= (1 << POT_SRC); // Always keep the voltage divider on
}

void configure_ssd_ports(void){
        // Controls power to the keypad and SSDs. 0000 1111 0000
    porta->DIR.reg |= 0x000000F0;

    portb->DIR.reg
            // Controls which segment turn on. 0000 1111 1111
        |= 0x000000FF
            // Controls sign indicator. 0010 0000 0000
        |  0x00000200
        ;
    // Set high drive strength ?
    unsigned short i = 0;
    for(; i < 8; ++i){
        porta->PINCFG[i].reg |= (1<<6);
    }
        // Turn off extra dots
    portb->OUT.reg &= ~0x10;
}

void display_dig(
    UINT32 add_delay, UINT8 num, UINT8 select,
    BOOLEAN__ show_dot, BOOLEAN__ show_sign
){
        // Active low logic
    portb->OUT.reg |= 0x000000FF;
        // Provide power to one specific SSD
    porta->OUT.reg |= 0x000000F0;
    porta->OUT.reg &= ~(1 << (select + 4u));
    switch(num){
                //  GEF DCBA
        case 0: // 0100 0000
            portb->OUT.reg &= ~0xBF;
            break;
        case 1: // 0111 1001
            portb->OUT.reg &= ~0x86;
            break;
        case 2: // 0010 0100
            portb->OUT.reg &= ~0xDB;
            break;
        case 3: // 0011 0000
            portb->OUT.reg &= ~0xCF;
            break;
        case 4: // 0001 1001
            portb->OUT.reg &= ~0xE6;
            break;
        case 5: // 0001 0010
            portb->OUT.reg &= ~0xED;
            break;
        case 6: // 0000 0010
            portb->OUT.reg &= ~0xFD;
            break;
        case 7: // 0111 1000
            portb->OUT.reg &= ~0x87;
            break;
        case 8: // 0000 0000
            portb->OUT.reg &= ~0x7F;
            break;
        case 9: // 0001 0000
            portb->OUT.reg &= ~0xEF;
            break;
        case 10: // 0000 1000
            portb->OUT.reg &= ~0xF7;
            break;
        case 11: // 0000 0011
            portb->OUT.reg &= ~0xFC;
            break;
        case 12: // 0100 0110
            portb->OUT.reg &= ~0xB9;
            break;
        case 13: // 0010 0001
            portb->OUT.reg &= ~0xDE;
            break;
        case 14: // 0000 0110
            portb->OUT.reg &= ~0xF9;
            break;
        case 15: // 0000 1110
            portb->OUT.reg &= ~0xF1;
            break;
        default:    // Non-hexadecimal digit or negative
            portb->OUT.reg |= 0xFF;
            show_dot = FALSE__;
            break;
    }
    if(show_dot)    portb->OUT.reg &= ~0x00000080;
    else            portb->OUT.reg |=  0x00000080;
    if(show_sign)   portb->OUT.reg &= ~0x00000200;
    else            portb->OUT.reg |=  0x00000200;
    delay_us(add_delay);
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