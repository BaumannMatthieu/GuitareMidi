#define FASTADC 0

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int start;
float freq = 0.f;

int velocity = 100;//velocity of MIDI notes, must be between 0 and 127
//higher velocity usually makes MIDI instruments louder
 
int noteON = 144;//144 = 10010000 in binary, note on command
int noteOFF = 128;//128 = 10000000 in binary, note off command

int previous_note = 0;
boolean play = false;

//send MIDI message
void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}

void setup() {
#if FASTADC
 // set prescale to 16
 sbi(ADCSRA,ADPS2) ;
 cbi(ADCSRA,ADPS1) ;
 cbi(ADCSRA,ADPS0) ;
#endif

 Serial.begin(115200) ;
 start = millis();
 
 Serial.println("Guitare Midi sampling : ");
}

#define BUFFER_SIZE 300
#define WINDOW_SIZE 75

#define MID_VALUE 510
#define THRESHOLD_VALUE 20

int buffer[BUFFER_SIZE] = {0};

int num_record = 0;

char frequency_to_midi(float f) {
    return 69.0f + 12.f*(log(f/440.0f)/log(2.0f)); 
}

void loop() {
   int maximum_value = 0;
 
     /* Echantillonage de size_buffer valeurs du signal à une frequence d'échantillonnage de 9009 Hz. (prescale = 7) 
      * Cout en temps égal à approximativement 40 ms pour BUFFER_SIZE = 350.
      */
     for(int i = 0; i < BUFFER_SIZE; i++) {
          buffer[i] = analogRead(0) - MID_VALUE;
          //Serial.println(buffer[i]);
          if(abs(buffer[i]) > maximum_value) {
              maximum_value = abs(buffer[i]); 
          }
     }
     
     if(maximum_value >= THRESHOLD_VALUE) {
         // Différence de signaux
         unsigned long long cumul_difference = 0;
         
         unsigned int first_min = 0;
         float prec_dd, current_dd;

         for(int tau = 0; tau < BUFFER_SIZE - WINDOW_SIZE; tau++) {
              unsigned long d = 0;
              float dd = 0.f;
              //unsigned long long prime = 0;
             for(int j = 1; j <= WINDOW_SIZE; j++) {
                 d += abs((buffer[j] - buffer[j + tau]));
             }
             
             // Calcul of cumulative mean normalized difference function
             cumul_difference += d;
             if(tau == 0) {
                 dd = 1.f;
                 prec_dd = dd;
                 current_dd = dd;
             } else {
                 float current_mean_difference = cumul_difference/tau;
                 // calcul de l'eqm normalisé selon la méthode décrite dans l'algorithme de YIN
                 dd = float(d)/current_mean_difference;
             }
             
             /* détection des deux minimaux successifs afin de calculer la fréquence reconnue */
             if(dd <= 0.50f && num_record >= BUFFER_SIZE - WINDOW_SIZE) {
                  if(prec_dd > current_dd && current_dd < dd) {
                      if(first_min == 0) {
                          first_min = tau;
                      } else {
                          if(tau >= (first_min + 7)) {
                            // fréquence reconnue
                            freq = 9009.f/(tau - first_min);
                            play = true;
                            num_record = 0;
                            //Serial.println(freq);
                            break;
                          }
                      }
                  }
             }

             prec_dd = current_dd;
             current_dd = dd;
             num_record++;
         }
         // on envoie en Serial la note convertie en midi ainsi que les ordre de jouer la note (noteON)
         if(freq != 0.f && play) {
             int note = int(frequency_to_midi(freq));
             if(note != previous_note) {
                 if(previous_note != 0) {
                     // on dit qu'on ne veut plus jouer les notes jouées precedemment. On clean le buffer.
                     for(int n = 0; n < 128; n++) {
                         MIDImessage(noteOFF, n, velocity); //turn note off
                     }
                 }
                 delay(50);
                 MIDImessage(noteON, note, velocity); //turn note on
                 
             }
             previous_note = note;
             start = millis();
         }   
    }
    play = false; 
     
}


