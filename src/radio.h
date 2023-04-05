void setupRadio();

/**
 * Set radio to recive
 * 
 * @param modulation modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
 * @param setrxbw Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
 * @param frequency Set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
 * @param deviation Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
 * @param datarate Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
*/
void setRadioToRecive(
    byte modulation, 
    float setrxbw,
    float frequency,
    float deviation,
    float datarate
);
  
void rawRecive();
