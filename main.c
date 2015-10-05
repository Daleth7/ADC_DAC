#include <asf.h>

#define ADC_IN 11   // Use pin 11 for analog input from voltage divider
#define POT_SRC 13  // Pin POT_SRC supplies voltage to voltage divider circuit
#define DAC_OUT 2   // Use pin 2 to output generated waveform to the speaker

    /**********   Start type aliasing   **********/
#include <stdint.h>

#define INT32   int32_t
#define UINT32  uint32_t
#define UINT16  uint16_t
#define UINT8   uint8_t

#define BOOLEAN__     UINT8
#define TRUE__        1
#define FALSE__       0

    // Short macro functions for inlining common expressions
#define IS_NULL(P) (P == NULL)
    /**********   End type aliasing   **********/

    /**********   Start function prototypes   **********/
void run_volt_meter(void);
void run_tone_generator(void);

void configure_modules(void);
void Simple_Clk_Init(void);
void enable_adc_clk(void);
void enable_dac_clk(void);
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
void configure_keypad_ports(void);
void display_dig(
    UINT32 add_delay, UINT8 num, UINT8 select,
    BOOLEAN__ show_dot, BOOLEAN__ show_sign
);
    // Check for any input (key press) and provide debouncing functionality.
    //  0 - 15 denotes key on keypad from right to left then bottom to top
void check_key(UINT8* row_dest, UINT8* col_dest);
    // Debounce key presses for validation. Return the column bits.
UINT8 debounce_keypress(void);
    // Find least significant ON bit
UINT32 find_lsob(UINT32);
    /**********   End function prototypes   **********/

    /**********   Start global variables   **********/
static Port* port;
static PortGroup *porta;
static PortGroup *portb;
static Adc* adc;
static Dac* dac;

#define SAMP_POP 1000
static UINT16 sin_samps[SAMP_POP] = {
	1008,  1008,  1007,  1007,  1007,  1007,  1007,  1007,  1007,  1007, 
	1007,  1006,  1006,  1006,  1006,  1005,  1005,  1005,  1004,  1004, 
	1004,  1003,  1003,  1002,  1002,  1001,  1001,  1000,  1000,  999, 
	999,  998,  997,  997,  996,  995,  995,  994,  993,  993, 
	992,  991,  990,  989,  989,  988,  987,  986,  985,  984, 
	983,  982,  981,  980,  979,  978,  977,  976,  975,  974, 
	972,  971,  970,  969,  968,  966,  965,  964,  963,  961, 
	960,  959,  957,  956,  954,  953,  952,  950,  949,  947, 
	946,  944,  943,  941,  940,  938,  936,  935,  933,  931, 
	930,  928,  926,  925,  923,  921,  919,  918,  916,  914, 
	912,  910,  908,  907,  905,  903,  901,  899,  897,  895, 
	893,  891,  889,  887,  885,  883,  881,  879,  876,  874, 
	872,  870,  868,  866,  863,  861,  859,  857,  855,  852, 
	850,  848,  845,  843,  841,  838,  836,  834,  831,  829, 
	826,  824,  822,  819,  817,  814,  812,  809,  807,  804, 
	802,  799,  797,  794,  791,  789,  786,  784,  781,  778, 
	776,  773,  770,  768,  765,  762,  760,  757,  754,  751, 
	749,  746,  743,  740,  738,  735,  732,  729,  726,  724, 
	721,  718,  715,  712,  709,  706,  704,  701,  698,  695, 
	692,  689,  686,  683,  680,  677,  674,  671,  668,  665, 
	662,  659,  656,  653,  650,  647,  644,  641,  638,  635, 
	632,  629,  626,  623,  620,  617,  614,  611,  608,  605, 
	602,  599,  595,  592,  589,  586,  583,  580,  577,  574, 
	571,  568,  564,  561,  558,  555,  552,  549,  546,  543, 
	539,  536,  533,  530,  527,  524,  521,  517,  514,  511, 
	508,  505,  502,  499,  495,  492,  489,  486,  483,  480, 
	477,  474,  470,  467,  464,  461,  458,  455,  452,  449, 
	445,  442,  439,  436,  433,  430,  427,  424,  421,  417, 
	414,  411,  408,  405,  402,  399,  396,  393,  390,  387, 
	384,  381,  378,  375,  372,  369,  366,  363,  360,  357, 
	354,  351,  348,  345,  342,  339,  336,  333,  330,  327, 
	324,  321,  318,  315,  313,  310,  307,  304,  301,  298, 
	295,  292,  290,  287,  284,  281,  278,  276,  273,  270, 
	267,  265,  262,  259,  256,  254,  251,  248,  246,  243, 
	240,  238,  235,  232,  230,  227,  225,  222,  220,  217, 
	214,  212,  209,  207,  204,  202,  199,  197,  194,  192, 
	190,  187,  185,  182,  180,  178,  175,  173,  171,  168, 
	166,  164,  162,  159,  157,  155,  153,  150,  148,  146, 
	144,  142,  140,  138,  135,  133,  131,  129,  127,  125, 
	123,  121,  119,  117,  115,  113,  111,  110,  108,  106, 
	104,  102,  100,  98,  97,  95,  93,  91,  90,  88, 
	86,  85,  83,  81,  80,  78,  76,  75,  73,  72, 
	70,  69,  67,  66,  64,  63,  62,  60,  59,  57, 
	56,  55,  53,  52,  51,  50,  48,  47,  46,  45, 
	44,  42,  41,  40,  39,  38,  37,  36,  35,  34, 
	33,  32,  31,  30,  29,  28,  27,  27,  26,  25, 
	24,  23,  23,  22,  21,  21,  20,  19,  19,  18, 
	17,  17,  16,  16,  15,  15,  14,  14,  13,  13, 
	12,  12,  12,  11,  11,  11,  10,  10,  10,  10, 
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9, 
	9,  9,  9,  9,  9,  9,  9,  9,  9,  9, 
	9,  10,  10,  10,  10,  11,  11,  11,  12,  12, 
	12,  13,  13,  14,  14,  15,  15,  16,  16,  17, 
	17,  18,  19,  19,  20,  21,  21,  22,  23,  23, 
	24,  25,  26,  27,  27,  28,  29,  30,  31,  32, 
	33,  34,  35,  36,  37,  38,  39,  40,  41,  42, 
	44,  45,  46,  47,  48,  50,  51,  52,  53,  55, 
	56,  57,  59,  60,  62,  63,  64,  66,  67,  69, 
	70,  72,  73,  75,  76,  78,  80,  81,  83,  85, 
	86,  88,  90,  91,  93,  95,  97,  98,  100,  102, 
	104,  106,  108,  110,  111,  113,  115,  117,  119,  121, 
	123,  125,  127,  129,  131,  133,  135,  138,  140,  142, 
	144,  146,  148,  150,  153,  155,  157,  159,  162,  164, 
	166,  168,  171,  173,  175,  178,  180,  182,  185,  187, 
	190,  192,  194,  197,  199,  202,  204,  207,  209,  212, 
	214,  217,  220,  222,  225,  227,  230,  232,  235,  238, 
	240,  243,  246,  248,  251,  254,  256,  259,  262,  265, 
	267,  270,  273,  276,  278,  281,  284,  287,  290,  292, 
	295,  298,  301,  304,  307,  310,  313,  315,  318,  321, 
	324,  327,  330,  333,  336,  339,  342,  345,  348,  351, 
	354,  357,  360,  363,  366,  369,  372,  375,  378,  381, 
	384,  387,  390,  393,  396,  399,  402,  405,  408,  411, 
	414,  417,  421,  424,  427,  430,  433,  436,  439,  442, 
	445,  449,  452,  455,  458,  461,  464,  467,  470,  474, 
	477,  480,  483,  486,  489,  492,  495,  499,  502,  505, 
	508,  511,  514,  517,  521,  524,  527,  530,  533,  536, 
	539,  543,  546,  549,  552,  555,  558,  561,  564,  568, 
	571,  574,  577,  580,  583,  586,  589,  592,  595,  599, 
	602,  605,  608,  611,  614,  617,  620,  623,  626,  629, 
	632,  635,  638,  641,  644,  647,  650,  653,  656,  659, 
	662,  665,  668,  671,  674,  677,  680,  683,  686,  689, 
	692,  695,  698,  701,  704,  706,  709,  712,  715,  718, 
	721,  724,  726,  729,  732,  735,  738,  740,  743,  746, 
	749,  751,  754,  757,  760,  762,  765,  768,  770,  773, 
	776,  778,  781,  784,  786,  789,  791,  794,  797,  799, 
	802,  804,  807,  809,  812,  814,  817,  819,  822,  824, 
	826,  829,  831,  834,  836,  838,  841,  843,  845,  848, 
	850,  852,  855,  857,  859,  861,  863,  866,  868,  870, 
	872,  874,  876,  879,  881,  883,  885,  887,  889,  891, 
	893,  895,  897,  899,  901,  903,  905,  907,  908,  910, 
	912,  914,  916,  918,  919,  921,  923,  925,  926,  928, 
	930,  931,  933,  935,  936,  938,  940,  941,  943,  944, 
	946,  947,  949,  950,  952,  953,  954,  956,  957,  959, 
	960,  961,  963,  964,  965,  966,  968,  969,  970,  971, 
	972,  974,  975,  976,  977,  978,  979,  980,  981,  982, 
	983,  984,  985,  986,  987,  988,  989,  989,  990,  991, 
	992,  993,  993,  994,  995,  995,  996,  997,  997,  998, 
	999,  999,  1000,  1000,  1001,  1001,  1002,  1002,  1003,  1003, 
	1004,  1004,  1004,  1005,  1005,  1005,  1006,  1006,  1006,  1006, 
	1007,  1007,  1007,  1007,  1007,  1007,  1007,  1007,  1007,  1008
	};
    /**********   End global variables   **********/

int main (void)
{    
	delay_init();
    Simple_Clk_Init();
    configure_modules();
    configure_ssd_ports();
    configure_keypad_ports();
    enable_adc_clk();
    enable_dac_clk();
    init_vdivider();
    init_adc();
    init_dac();

    while(TRUE__){
        run_volt_meter();
        run_tone_generator();
    }

    return 0;
}

void run_volt_meter(void){
    disable_dac();
    enable_adc();

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

    UINT8 col = 0x0;
    
    while(TRUE__)
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

        check_key(NULL, &col);
        if(col == 0xD)  break;
    }
}

void run_tone_generator(void){
    disable_adc();
    enable_dac();

    static UINT16 sample = 0;

    UINT8 samp_arr[4] = {2, 3, 0x10, 0x10};
    UINT8 sample_speed = 32;
    unsigned row_sel = 0;
    UINT8 row = 0x0, col = 0x0;

    while(TRUE__){
        while(dac->STATUS.reg & DAC_STATUS_SYNCBUSY);

        dac->DATA.reg = sin_samps[sample];
        sample = (sample + sample_speed)%SAMP_POP;

        display_dig(1, samp_arr[row_sel], row_sel, FALSE__, FALSE__);
        row_sel = (row_sel+1u)%4u;

        check_key(&row, &col);
        if(col == 0xB)  break;
        else {
            /*
            Future implementation will include change in frequency
            */
        }
    }
}

void configure_modules(void){
    port = (Port*)(PORT);
    porta = (PortGroup *)(&port->Group[0]);
    portb = (PortGroup*)(&port->Group[1]);
    adc = (Adc*)(ADC);
    dac = (Dac*)(DAC);
}

void enable_adc_clk(void){
    PM->APBCMASK.reg |= 1 << 16;             // PM_APBCMASK enable is in the 16 position
    
    uint32_t temp = 0x17;                 // ID for ADC 0x17 (see table 14-2)
    temp |= 0<<8;                             // Selection Generic clock generator 0
    GCLK->CLKCTRL.reg = temp;                 // Setup in the CLKCTRL register
    GCLK->CLKCTRL.reg |= 0x1u << 14;         // enable it.
}

void enable_dac_clk(void){
    PM->APBCMASK.reg |= 1 << 18;             // PM_APBCMASK enable is in the 16 position
    
    uint32_t temp = 0x1A;                 // ID for ADC 0x17 (see table 14-2)
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

void configure_keypad_ports(void){
        // Controls power to the keypad and SSDs. 0000 1111 0000
    porta->DIR.reg |= 0x000000F0;


        // For reading input from keypad. 1111 0000 0000 0000 0000 
        //  Active high logic for input.
    porta->DIR.reg &= ~0x000F0000;
    unsigned short i;
    for(i = 16; i < 20; ++i){
            // Enable input (bit 1).
            //  Note that the pins are externally pulled low,
            //  so disable pull up.
        porta->PINCFG[i].reg = PORT_PINCFG_INEN;
    }
}

void check_key(UINT8* row_dest, UINT8* col_dest){
    static UINT8 cur_row = 0u;
    // Provide power to one specific row
    porta->OUT.reg |= 0x000000F0;
    porta->OUT.reg &= ~(1u << (4u + cur_row));
        // Extract the four bits we're interested in from
        //   the keypad.
    if(!IS_NULL(col_dest))  *col_dest = debounce_keypress();
    if(!IS_NULL(row_dest))  *row_dest = cur_row;

        // Prepare for the next row. If we were on the last
        //  row, cycle back to the first row.
        // Take modulous to retrieve current value of cur_row when
        //  we were not on the last row. If that is the case,
        //  reset row_bit to 0.
    cur_row = (cur_row+1u)%4u;
}

UINT8 debounce_keypress(void){
    // Triggered the instant the first key press is detected
    //  Returns the resulting hex number

    UINT8 toreturn = (porta->IN.reg >> 16u) & 0xF;

        // Check if more than one button in a row was pressed.
        //  If so, checking for glitches is no longer important.
    UINT32 counter = 0x0;
    BOOLEAN__ already_on = FALSE__;
    for(; counter < 4; ++counter){
        if(already_on & (toreturn >> counter))  return toreturn;
        else    already_on = (toreturn >> counter) & 0x1;
    }

    if(!toreturn)   return 0x0;
#define MAX_JITTER  5
#define MAX_JITTER2 1000
#define RELEASE_LIM 7500

    // First, read up to MAX_JITTER times to swallow spikes as button is
    //  pressed. If no key press was detected in this time, the noise is
    //  not from a button press.
    for(counter = 0x0; counter < MAX_JITTER; ++counter){
        if(!((porta->IN.reg >> 16u) & 0xF))    return 0x0;
    }

    // Now swallow the spikes as the button is released. Do not exit
    //  until the spikes are no longer detected after MAX_JITTER reads.
    //  If the user is holding down the button, release manually based
    //  on RELEASE_LIM.
    volatile UINT32 release = 0x0;
    for(
        counter = 0x0;
        counter < MAX_JITTER2 && release < RELEASE_LIM;
        ++counter, ++release
    ){
        if((porta->IN.reg >> 16u) & 0xF)    counter = 0x0;
    }

    return toreturn;

#undef MAX_JITTER
#undef RELEASE_LIM
}

UINT32 find_lsob(UINT32 target){
    UINT32 toreturn = 0u;
    while(!(target & 0x1)){
        ++toreturn;
        target >>= 1;
    }
    return toreturn;
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