#include "SanUSB1X.h"

float lux = 0,resultado;

void interrupt interrupcao();
void test_sucessful();
void test_failure();

void main(){

    clock_int_4MHz();
    habilita_interrupcao(recep_serial); //To receive any data from serial.
    habilita_canal_AD(AN0); //To enable pin A0 to read analogical data.
    taxa_serial(9600);      //To communicate with device hooked in the serial.

    while(1){
        if (entrada_pin_e3==0)
            {Reset();
        } 

        resultado = le_AD10bits(0);
        resultado = 5*(float)resultado/1023;
        
        if (resultado >= 2. && resultado <2.8){
            lux=(3936.4-(1249*resultado))/0.8;
            printf("Reta 1: ");
        }
        if (resultado >= 2.8 && resultado <=3.8){
            printf("Reta 2: ");
            lux=2057.2-494*resultado;
        }
        if (resultado > 3.8){
            printf("Reta 3: ");
            lux=(900-180*resultado)/1.2;
        }

        printf("Luminosidade = %.2f\r\n", lux);

        timer0_ms(2000);
        test_sucessful();   //To indicate PIC is alive.
    }
}
void interrupt interrupcao(){
 if (serial_interrompeu)  {
     serial_interrompeu=0;
     char comando = le_serial();
   }
}

void test_sucessful(){
    for(int i = 0; i < 5; i++){
        nivel_alto(pin_a4);
        tempo_ms(100);
        nivel_baixo(pin_a4);
        tempo_ms(100);
    }
}

void test_failure(){
    for(int i = 0; i < 5; i++){
        nivel_alto(pin_a5);
        tempo_ms(100);
        nivel_baixo(pin_a5);
        tempo_ms(100);
    }
}
