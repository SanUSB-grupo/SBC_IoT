#include "SanUSB1X.h"
#include <stdlib.h>

#define RST "AT+RST\r\n"
#define AT "AT\r\n"
#define ATE0 "ATE0\r\n"
#define ATE1 "ATE1\r\n"
#define send_info "GET /update?key=7V1BKM3ZABHFIVLI&field1="
#define recv_info "GET /channels/100236/fields/1.csv?results=1\r\n"
#define close "AT+CIPCLOSE\r\n"
#define CHANNEL 1
#define MULT_CONN 1



#define length_of(x)  (sizeof(x)/sizeof((x)[0]))

#define ESP8266_STATION 0x01
#define ESP8266_SOFTAP 0x02

#define NUM_RESPONSES 12
#define ESP8266_OK 0
#define ESP8266_READY 1
#define ESP8266_FAIL 2
#define ESP8266_NOCHANGE 3
#define ESP8266_LINKED 4
#define ESP8266_UNLINK 5
#define ESP8266_NOIP 6
#define ESP8266_REC 7
#define ESP_BUILDED 8
#define ESP_LINK_NOT 9
#define ESP_ALR_CONN 10
#define ESP_BUSY 11

//Global variables - I know, shame on me...
unsigned char input[256];   //Safe size for the buffer that will store replies.
                            //It's necessary to handle cases when the reply is greater than 256 chars.

int position = 0;           //Position in the input[] array.
float lux = 0,resultado;

unsigned const char* strings[NUM_RESPONSES] =  {"OK\r\n", "ready\r\n",
                                                "FAIL\r\n", "no change\r\n",
                                                "Linked\r\n", "Unlink\r\n",
                                                "no ip\r\n","> ",
                                                "link is builded\r\n", "link is not\r\n",
                                                "ALREAY CONNECT\r\n", "busy p..."};      //String representation for all possible/handled replies.
unsigned const char responses[NUM_RESPONSES] = {ESP8266_OK, ESP8266_READY,
                                                ESP8266_FAIL, ESP8266_NOCHANGE,
                                                ESP8266_LINKED, ESP8266_UNLINK,
                                                ESP8266_NOIP, ESP8266_REC,
                                                ESP_BUILDED, ESP_LINK_NOT,
                                                ESP_ALR_CONN, ESP_BUSY};      //Int representation.


void interrupt interrupcao();                               //To handle interruptions
bit esp8266_isStarted(void);                                //Sends AT and waits for OK.
int esp8266_lookFor(unsigned char *string);                 //After we've received the response from ESP, we can look for some usefel info.
bit esp8266_send(unsigned char* data, int channel);         //To send a given data
void reset_flags(void);                                     //Before sending any command it's wise to reset all flags.
bit esp8266_start(unsigned char* protocol, char* ip, int port, int channel);  //It accepts ip or DNS
void esp8266_close(void);                                   //Just to close connection
void test_sucessful();                                      //Blink LED in port A0
void test_failure();                                        //Blink LED in port A1
bit esp_setup_MUX(int mode);
bit esp_waitFor(char* string);                              //Waits until string is found or 10secs pass.

void thingSpeak_clear_channel();                            //Supposed to clean a channel, but thingSpeak hasn't implemented in the API
void count_half_sec();                                      //Uses timer0 to count precisely
void count_full_sec();
void count_x_sec(int x);
void esp_reset();                                           //Resets ESP8266
float get_value();
void check_alive();


void main(){
    clock_int_4MHz();
    habilita_interrupcao(recep_serial);
    habilita_canal_AD(AN0); //To enable pin A0 to read analogical data.
    taxa_serial(9600);

    esp_reset();    //Just to come clear.
    int time_to_wait;
    bool found = false;
    bool sent, received;

    bool keep_it_on = false;
    bool has_changed = true;
    int previous = 1;

    while(1){
        test_sucessful();   //Indicates PIC's alive.

        if(!entrada_pin_e3){//pressionar o botão para gravação
            Reset();
        }

        time_to_wait = 30;  //60 secs.
        
        resultado = le_AD10bits(0);
        resultado = 5*(float)resultado/1023;

        resultado = le_AD10bits(0);
        resultado = 5*(float)resultado/1023;

        if (resultado >= 2. && resultado <2.8){
            lux=(3936.4-(1249*resultado))/0.8;
        }
        if (resultado >= 2.8 && resultado <=3.8){
            lux=2057.2-494*resultado;
        }
        if (resultado > 3.8){
            lux=(900-180*resultado)/1.2;
        }
        if(resultado < 2) lux = 2000;   // For values greater than 2v

        //Sending data to TS
        do{
            sent = false;   //It'll look here until it sends something to TS.

            found = esp_setup_MUX(MULT_CONN);
            if(!found) found = esp8266_lookFor((char*)strings[ESP_BUILDED]); //In the case we receive Link is builded
            if(found){

                found = esp8266_start((char*)"TCP", (char*)"184.106.153.149", 80, CHANNEL);
                if(!found) found = esp8266_lookFor((char*)strings[ESP_ALR_CONN]); //In case we receive Already connected
                if(found){

                    unsigned char info_to_send[60];
                    sprintf(info_to_send, "%s%.2f\r\n", send_info, lux);
                    found = esp8266_send((char*)info_to_send, CHANNEL);

                    if(!found) found = esp8266_lookFor((char*)strings[ESP8266_OK]); //In case we receive OK
                    if(found){ //It was able to send
                        sent = true;
                        esp8266_close();
                    }else{  //Unable to send
                        esp_reset();
                        test_failure();
                        time_to_wait = 5;
                    }
                }else{   //Couldn't link to the website.
                    esp_reset();
                    test_failure();
                    time_to_wait = 5;
                }
            }else{  //Couldn't connect MUX=1
                esp_reset();
                test_failure();
                time_to_wait = 5;
            }
            reset_flags();
        }while(!sent);

        count_x_sec(5);
        test_sucessful();

        //Receiveing data from TS
        do{
            received = false;   //It'll loop here until it receives something from TS.
            float LDR_value = 100;

            found = esp_setup_MUX(MULT_CONN);
            if(!found) found = esp8266_lookFor((char*)strings[ESP_BUILDED]); //In case we receive Link is builded
            if(found){

                found = esp8266_start((char*)"TCP", (char*)"184.106.153.149", 80, CHANNEL);
                if(!found) found = esp8266_lookFor((char*)strings[ESP_ALR_CONN]); //In the case we receive Already connected
                if(found){

                    found = esp8266_send((char*)recv_info, CHANNEL);
                    if(!found) found = esp8266_lookFor((char*)strings[ESP8266_OK]); //In the case we receive OK
                    if(found){ //It was able to receive
                        received = true;
                        LDR_value = get_value();
                        if(LDR_value < 100){
                                keep_it_on = true;
                                if(previous == 0){
                                    has_changed = false;
                                    previous = 1;
                                }
                            }else if(LDR_value >= 100){
                                keep_it_on = false;
                                if(previous == 1){
                                    has_changed = false;
                                    previous = 0;
                                }
                            }

                            if(keep_it_on && !has_changed){
                                has_changed = true;
                                nivel_alto(pin_b7);
                            }

                            if(!keep_it_on && !has_changed){
                                has_changed = true;
                                nivel_baixo(pin_b7);
                            }
                        esp8266_close();
                    }else{  //Unable to send
                        esp_reset();
                        test_failure();
                        time_to_wait = 5;
                    }
                }else{   //Couldn't link to the website.
                    esp_reset();
                    test_failure();
                    time_to_wait = 5;
                }
            }else{  //Couldn't connect MUX=1
                esp_reset();
                test_failure();
                time_to_wait = 5;
            }
            
        }while(!received);

        count_x_sec(time_to_wait);    //Sends and receives data every one minute or less in case it fails to send
    }
}

void interrupt interrupcao(){
    if(serial_interrompeu){
        serial_interrompeu = 0;

        if(position < 256){
            input[position] = le_serial();
            position++;
        }else{
            char garbage = le_serial();
        }
    }

}

bit esp8266_isStarted(void){
    reset_flags();
    printf(AT);

    return (esp8266_lookFor((char*)strings[ESP8266_OK]) > 0)?true:false;
}

/*
 * This function is to be called after every command we send to ESP8266.
 * It should receive the parameter that will make the loop stop.
 * Otherwise, the function will return 0, which mean the response didn'nt come through.
 */
int esp8266_lookFor(unsigned char *string){
   int so_far = 0;
   int string_length = strlen(string);
   int counter = 0;
   int attempts = 5;

   while(attempts > 0){
        attempts--;
        for(int i = 0; i < position; i++){
            counter++;
            if (input[i] == string[so_far]) {
                so_far++;
                if(string_length == so_far)
                    return counter;
            }else{
               so_far = 0;
            }
        }
    }
   return 0;    //In case *string is not in input[]
}


/**
 * // Send data (AT+CIPSEND)
* Send data over a connection.
*
* This sends the AT+CIPSEND command to the ESP module.
*
* @param data The data to send
*
* @return true iff the data was sent correctly.
*/
bit esp8266_send(unsigned char* data, int channel){
    reset_flags();
    int data_length = strlen(data);

    printf("AT+CIPSEND=%d,%d\r\n", channel, data_length);
    count_half_sec();
        printf("%s", data);
        count_half_sec();

        return esp_waitFor((char*) "+IPD,");
}

void reset_flags(void){
    for(int i = 0; i < position; i++){  //Reset the input's var
        input[i] = '\0';
    }
    position = 0;
}

/**
* Open a TCP or UDP connection.
*
* This sends the AT+CIPSTART command to the ESP module.
*
* @param protocol Either ESP8266_TCP or ESP8266_UDP
* @param ip The IP or hostname to connect to; as a string
* @param port The port to connect to
*
* @return true iff the connection is opened after this.
*/
bit esp8266_start(unsigned char* protocol, char* ip, int port, int channel) {
    reset_flags();

    printf("AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n", channel, protocol, ip, port);
    count_half_sec();

    return esp_waitFor((char*) strings[ESP8266_OK]);
}

void esp8266_close(void){
    reset_flags();
    printf("AT+CIPCLOSE\r\n");
    count_half_sec();
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

void check_alive(){
    for(int i = 0; i < 5; i++){
        nivel_alto(pin_a5);
        nivel_baixo(pin_a4);
        tempo_ms(100);
        nivel_alto(pin_a4);
        nivel_baixo(pin_a5);
        tempo_ms(100);
    }
}

bit esp_setup_MUX(int mode){
    reset_flags();
    char command[14];
    sprintf(command, "AT+CIPMUX=%d\r\n", mode);

    printf(command);
    count_half_sec();

    return esp_waitFor((char*) strings[ESP8266_OK]);
}

void thingSpeak_clear_channel(){
    int attempts = 5;
    do{
        attempts--;
        esp_setup_MUX(MULT_CONN);
        esp8266_start((char*)"TCP", (char*)"api.thingspeak.com", 80, CHANNEL);

        //Setup the correct info to send in order to clean up the channl.
        esp8266_send((char*)recv_info, CHANNEL);
        count_full_sec();
    }while(attempts >= 0); //Find out what msg thingspeak gives when a field is cleaned up.
}



void count_half_sec(){
    timer0_ms(500); //Using timer, not delay.
}

void count_full_sec(){
    timer0_ms(1000);//Using timer, not delay.
}

void count_x_sec(int x){
    for(int i = 0; i < x; i++) count_full_sec();
}

/*
 * 3 ways of ending this method:
 * true: Whether string is found in input[]
 * false: Whether string is found in input[]
 * false: Whether the timer is over and esp was restarted.
 * Timer is set to 20 secs.
 */
bit esp_waitFor(unsigned char* string){
    int timerIsOver = 0;
    int max_time = 15;

    do{
        //before each attempt to find string, it's wise to wait for a bit
        count_half_sec();
        timerIsOver++;
        if(esp8266_lookFor(string) > 0){
            break;
        }
    }while(timerIsOver < max_time);  //The number after timerIsOver indicates the seconds until we give up on waiting.

    return (timerIsOver < max_time)?true:false;
}

void esp_reset(){
    printf(RST);
    count_x_sec(5);
    printf(ATE0);   //To disable ESP to show commands we type, we can only see the reply.
}

float get_value(){
    /*
     * Typical response sample:
     * +IPD,1,61:created_at,entry_id,field1\r\n2016-04-20 12:30:48 UTC,11,2.93\r\n
     * Where between the 2nd ',' and before ':' we have the size of the msg.
     */
    int num_commas = 0;

    int starting_point = esp8266_lookFor((char*) "+IPD");
    char numStr[10];
    int p = 0;  //Position of the array.

    for(int i=starting_point; i < position; i++){
        if(num_commas == 6 && input[i] != '\r' ){
            numStr[p] = input[i];
            p++;
        }else if(num_commas == 6 && input[i] == '\r') break;
        if(input[i] == ',') num_commas++;
    }

    float num = atof((char*)numStr);

    return num;
}