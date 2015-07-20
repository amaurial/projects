double getTemp(void) {	//Chip Internal Temperature Sensor
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
//  wADC = ADCW;    //only for Atmega328p
  wADC = (ADCH << 8) | ADCL;     // read the high byte and read the low byte

  // The offset of - 324.31 could be wrong. It is just an indication.
  t = (wADC - 23.0 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}
