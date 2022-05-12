//Robert, Brian, Hugo, Leonard
//Projet - robot PID
//Fonctions demontrees : balance, chute, tourne sur place


#include "mbed.h"
#include "stdio.h"
#include <cstdint>
#include <cstdio>
//#include "cmps03.h"

#define VITESSE_NULLE 1500

#define PWM1 1300
#define PWM2 1600

#define TARGET_TX_PIN USBTX
#define TARGET_RX_PIN USBRX
#define CMPS_ADDR 0xC0 

// Create a BufferedSerial object to be used by the system I/O retarget code.
static BufferedSerial serial_port(TARGET_TX_PIN, TARGET_RX_PIN, 115200);

I2C cmps12(p9, p10);        // sda, scl

int process_variable;

char data[31]; 

//Pin du moteur 
    // Pin controle avant/arriere
DigitalOut EN(p28);

    // Pin controle resolution
DigitalOut MS1(p27);
DigitalOut MS2(p26);
DigitalOut MS3(p25);

    // Pin controle directions et changement des step 
        // Moteur 1 
DigitalOut DIR1(p29);
DigitalOut STEP1(p30);
        // Moteur 2
DigitalOut DIR2(p21);
DigitalOut STEP2(p23);

// Variables
Timeout t;
Timer compteur;

float temps, temps2, dtemps, commande,P, I, D, PID, tmps, frequence, sumx, x, bearing, bearing360, bearing180, delta_erreur, err, old_erreur;
int erreur;

using namespace std::chrono;

// Controle du moteur par interruption
void flip1() {
    STEP1 = !STEP1;
    STEP2 = !STEP2;
    tmps = (1/frequence)/2;
    t.attach(&flip1, tmps);
}

int main()
{
    MS1 = 1;
    MS2 = 1;
    MS3 = 1;

    EN = 0;

    frequence = (200 * 16);
    tmps = (1/frequence)/2;
    t.attach(&flip1, tmps);
    compteur.start(); // Commencement du compteur 

    while (true) {
        
        cmps12.write(CMPS_ADDR, 0, 1);
        cmps12.read(CMPS_ADDR, data, sizeof(data));

        // Roll
        process_variable = data[5];

        // Bousole
        bearing =  data[1];


        bearing360 = bearing * 360 / 256;

        bearing180 = bearing360 > 180 ? bearing360 - 360 : bearing180;
        bearing180 = bearing360 < 180 ? bearing360 : bearing180;

        if (process_variable >= 90)
            process_variable = process_variable - 256;

        erreur = 0 - process_variable;

        // Proportionnel
        commande = 22 * erreur;
        P = commande;

        // Delta temp
        compteur.stop();
        dtemps = duration_cast<milliseconds>(compteur.elapsed_time()).count(); //5
        compteur.reset();
        compteur.start();
        dtemps = dtemps/1000;

        // Integral
        x = erreur * dtemps;
        sumx = sumx + x;
        I = sumx * 100;

        // Derive
        delta_erreur = err - old_erreur;
        old_erreur = err;
        D = (delta_erreur / dtemps) * 20;

        PID = P + I + D;

        compteur.stop();   //9
        
        if (erreur < -5)
        {
            DIR1 = 0;
            DIR2 = 1;
            EN = 0;
            frequence = (200 * 16) * (PID / 360) * -1;
        }

        if (erreur > 5)
        {
            DIR1 = 1;
            DIR2 = 0;
            EN = 0;
            frequence = (200 * 16) * (PID / 360);
        }
        
        if (erreur >= -5 && erreur <= 5 )
        {
            EN = 0;
            DIR1 = 1;
            DIR2 = 1;
            frequence = (200 * 16) * (280.0/ 360);
        }

        if (process_variable > 70 || process_variable < -70) //Arret en chute
        {
            EN = 1;
        }

        wait_us(100000);
    }
}
