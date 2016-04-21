//Utiliza interrupcao serial para receber comandos enviados via bluetooth ou zigbee
#include "SanUSB1X.h" //
short int pisca = 0;
short int msg = 1;
unsigned char comando;

//#pragma interrupt interrupcao
void interrupt interrupcao()
{
 if (serial_interrompeu)  {
     serial_interrompeu=0;
     comando = le_serial();

         switch (comando){
             case 'l':
                pisca=0;
                nivel_alto(pin_b7); //Não imprime (printf) dentro da interrupcao
             break;

             case 'd':
                pisca=0;
                nivel_baixo(pin_b7);
             break;

             case 'p':
                pisca=1;
                nivel_alto(pin_b7);
             break;
                
             case 'e':
                pisca = 0;
                nivel_alto(pin_b7);
                msg = 1;
             break;
         }
   }
}

void main(){

    clock_int_4MHz();
    habilita_interrupcao(recep_serial);
    taxa_serial(9600);

    while(1){

        if (entrada_pin_e3==0){Reset();} //com interrupcao evitar no while, pois pode dar conflito com a gravacao

        while (pisca==1){
            inverte_saida(pin_b7);tempo_ms(300);
        }//pisca rapido

        if(msg == 1){
            msg = 0;
            sendsw((char *)"SanUSB\r\n");
        }
        //tempo_ms (1000);
        //inverte_saida(pin_b7);

    }
}

