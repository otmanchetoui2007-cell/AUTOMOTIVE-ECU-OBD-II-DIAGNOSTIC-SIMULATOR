#ifndef ENGINE_MODEL_H
#define ENGINE_MODEL_H

#include <stdbool.h>
#include <stdint.h>
 

typedef struct {
    /* Configuration et état du moteur */
    float rpm;//nbr tours de moteur par seconde 
    float coolant_temp;//temp de lique de refroidissement 
    float vehicle_speed;//v de vehicule lui meme 
    /* Battery parameters */
    float battery_voltage;
    float battery_soc;
    float battery_capacity_ah;
    float electrical_load;
    float battery_internal_resistance;
    float battery_current;
    
    /* Commandes de simulation */
    float throttle;
    bool engine_running;

    /* Temps de simulation */
    float simulation_time;
}Engine_model;

//inialisation de ecu moteur 
void engine_init(Engine_model *engine);
//mise à jour de rpm
void engine_update_rpm(Engine_model *engine , float dt);
//mise à jour de temperature 
void engine_update_temperature(Engine_model *engine , float dt);
//mise à jour de vitesse
void engine_update_speed(Engine_model *engine , float dt );
//mise à jour de tension
void engine_update_voltage(Engine_model *engine , float dt);
//mise à jour de module complete de  ecu moteur 
void engine_update(Engine_model *engine , float dt);
//Vérifie la cohérence des valeurs, wach dakaxi ma39ol ou non 
bool engine_validate( const Engine_model *engine);

#endif /* ENGINE_MODEL_H */

