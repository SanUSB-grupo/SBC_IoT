#include "SanUSB1X.h"
//#include <stdlib.h>

//Biblioteca
//Conversor de tensão LM324
//Traic e optotriac


void main(){
    clock_int_4MHz();
    habilita_interrupcao(recep_serial);
    taxa_serial(9600);//115200

    int enviar = 1;
    int reseted = 0;
    int count = 1;
    int tempo = 1;

    //Variables:
    //char AT[] = "AT\r\n";
    char CIPMUX[] = "AT+CIPMUX=1\r\n";
//    	AT+CIPSTART=4,"TCP","184.106.153.149",80
    char CIPSTART[] = "AT+CIPSTART=4,\"TCP\",\"184.106.153.149\",80\r\n";
    char CIPSEND[] ="AT+CIPSEND=4,44\r\n";
    char info[] = "GET /update?key=7V1BKM3ZABHFIVLI&field1=";//35\r\n";
    char close[] = "AT+CIPCLOSE\r\n";



    while(1){

        if(!entrada_pin_e3){//pressionar o botão para gravação
            Reset();
        }
        if(enviar == 1){
            if(reseted == 0){
                reseted = 1;

                //sendrw((char*) "AT+RST\r\n");
                nivel_alto(pin_b6);
                tempo_ms(500);
                nivel_baixo(pin_b6);
                tempo_ms(500);

                sendrw((char*)CIPMUX);
                tempo_ms(500);

                nivel_alto(pin_b5);
                tempo_ms(500);
                nivel_baixo(pin_b5);
                tempo_ms(500);
            }
//            sendrw((char*)CIPSTART);

            sendrw((char*)"AT+CIPSTART=4,");
            sendrw((char*)"\"TCP\",");
            //sendrw((char*)",");
            sendrw((char*)"\"184.106.153.149\"");
            sendrw((char*)",");
            sendnum(80);
            sendrw((char*)"\r\n");

            tempo_ms(3000);
            sendrw((char*)CIPSEND);
            tempo_ms(500);

            sendrw((char*)info);
            sendnum(count);
            count = rand() % 100;
            sendrw((char*)"\r\n");
            tempo_ms(3000);

            sendrw((char*)close);
            tempo_ms(500);

            nivel_alto(pin_b4);
            tempo_ms(500);
            nivel_baixo(pin_b4);
            tempo_ms(500);
        }
        if(enviar == 0){
            sendrw((char*)"AT\r\n");
            nivel_alto(pin_b6);
            tempo_ms(500);
            nivel_baixo(pin_b6);
            tempo_ms(500);
        }

        tempo = rand()%13;
        tempo_ms(10000*tempo);//10s * generated number. It´s from 10s to 2min.
    }
}

void interrupt interrupcao(){
}