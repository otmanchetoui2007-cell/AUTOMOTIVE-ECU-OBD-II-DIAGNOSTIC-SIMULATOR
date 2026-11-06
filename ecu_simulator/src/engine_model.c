#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdbool.h>
#include <math.h>
#include "Engine_model.h"
#define RPM_IDLE       800.0f
#define RPM_MAX        6000.0f

#define ACCEL_RATE     1500.0f  // RPM par seconde
#define DECEL_RATE     1000.0f  // RPM par seconde

#define ENGINE_AMBIENT_TEMP    25.0f
#define ENGINE_RPM_MAX         6000.0f

#define BASE_HEAT_RATE        0.16f
#define LOAD_HEAT_RATE        0.12f
#define COOLING_COEFFICIENT    0.0025f

/* Paramètres de simulation */
#define VEHICLE_SPEED_MAX       180.0f
#define MAX_ACCELERATION         12.0f
#define NATURAL_DECELERATION      3.0f

/* Parametres physiques du vehicule */
#define VEHICLE_SPEED_MAX    180.0f

#define VEHICLE_MASS         1500.0f
#define MAX_TRACTION_FORCE   5000.0f

#define GRAVITY              9.81f
#define ROLLING_COEFFICIENT  0.012f

#define AIR_DENSITY          1.225f
#define DRAG_AREA            0.65f

/*Parametres de simulation */
#define BATTERY_VOLTAGE_MIN       11.0f
#define BATTERY_VOLTAGE_MAX       14.5f
#define BATTERY_VOLTAGE_TARGET    14.2f

#define BATTERY_DISCHARGE_RATE    0.02f
#define BATTERY_CHARGE_RATE       0.10f

/* Battery voltage parameters */
#define BATTERY_VOLTAGE_MIN       11.0f
#define BATTERY_VOLTAGE_MAX       14.5f
#define BATTERY_VOLTAGE_TARGET    14.2f

/* Alternator parameters */
#define ALTERNATOR_MAX_CURRENT    60.0f
#define MAX_BATTERY_CHARGE_CURRENT 20.0f

/* Battery model parameters */
#define BATTERY_OCV_MIN           11.8f
#define BATTERY_OCV_MAX           12.6f

void engine_init(Engine_model *engine){
    if(engine == NULL)
     return;
    engine->coolant_temp = 20.0f;
    engine->rpm = 0;
    engine->vehicle_speed = 0.0f;
    engine->battery_voltage = 12.6f;
    engine->engine_running = false;
    engine->throttle = 0;
    engine->simulation_time = 0;
    engine->battery_voltage = 12.6f;
    engine->battery_soc = 100.0f;
    engine->battery_capacity_ah = 60.0f;
    engine->electrical_load = 15.0f;
    engine->battery_internal_resistance = 0.015f;
    engine->battery_current = 0.0f;
}

void engine_update_rpm(Engine_model *engine, float dt){
    // 1. Vérifier le pointeur
    if (engine == NULL)
        return;

    // 2. Vérifier le temps écoulé
    if (!isfinite(dt) || dt <= 0.0f)
        return;

    // 3. Vérifier la valeur du throttle
    if (!isfinite(engine->throttle))
        return;

    // 4. Limiter le throttle entre 0 et 100 %
    if (engine->throttle < 0.0f)
        engine->throttle = 0.0f;

    if (engine->throttle > 100.0f)
        engine->throttle = 100.0f;

    // 5. Si le moteur est arrêté
    if (engine->engine_running == false){
        engine->rpm = 0.0f;
        return;
    }

    // 6. Calculer le RPM cible
    float target_rpm = RPM_IDLE +
        (engine->throttle / 100.0f) * (RPM_MAX - RPM_IDLE);

    // 7. Calculer la différence entre RPM cible et actuel
    float delta_rpm = target_rpm - engine->rpm;

    // 8. Déterminer la vitesse d'accélération ou de décélération
    float max_delta;

    if (delta_rpm > 0.0f)
        max_delta = ACCEL_RATE * dt;
    else
        max_delta = DECEL_RATE * dt;

    // 9. Faire évoluer progressivement le RPM
    if (delta_rpm > max_delta)
        engine->rpm += max_delta;
    else if (delta_rpm < -max_delta)
        engine->rpm -= max_delta;
    else
        engine->rpm = target_rpm;

    // 10. Limiter le RPM dans la plage autorisée
    if (engine->rpm < RPM_IDLE)
        engine->rpm = RPM_IDLE;

    if (engine->rpm > RPM_MAX)
        engine->rpm = RPM_MAX;
}


void engine_update_temperature(Engine_model *engine, float dt){
    // 1. Vérifier le pointeur
    if (engine == NULL)
        return;

    // 2. Vérifier le temps écoulé
    if (!isfinite(dt) || dt <= 0.0f)
        return;

    // 3. Vérifier les données du moteur
    if (!isfinite(engine->coolant_temp) ||
        !isfinite(engine->rpm) ||
        !isfinite(engine->throttle))
        return;

    if (engine->rpm < 0.0f)
        return;

    // 4. Limiter le throttle entre 0 et 100 %
    float throttle = engine->throttle;

    if (throttle < 0.0f)
        throttle = 0.0f;

    if (throttle > 100.0f)
        throttle = 100.0f;

    // 5. Normaliser le RPM et le throttle
    float rpm_normalized = engine->rpm / ENGINE_RPM_MAX;

    if (rpm_normalized > 1.0f)
        rpm_normalized = 1.0f;

    float throttle_normalized = throttle / 100.0f;

    // 6. Calculer la charge simplifiée du moteur
    float engine_load = rpm_normalized * throttle_normalized;

    // 7. Calculer le taux de production de chaleur
    float heating_rate = 0.0f;

    if (engine->engine_running == true)
    {
        heating_rate = BASE_HEAT_RATE +
                       LOAD_HEAT_RATE * engine_load;
    }

    // 8. Calculer le taux de refroidissement
    float cooling_rate = COOLING_COEFFICIENT *
        (engine->coolant_temp - ENGINE_AMBIENT_TEMP);

    // 9. Calculer la variation de température
    float temperature_delta =
        (heating_rate - cooling_rate) * dt;

    // 10. Calculer la nouvelle température
    float new_temperature =
        engine->coolant_temp + temperature_delta;

    // 11. Vérifier le résultat
    if (!isfinite(new_temperature))
        return;

    // 12. Mettre à jour la température du moteur
    engine->coolant_temp = new_temperature;
}

void engine_update_speed(Engine_model *engine, float dt){
    /* 1. Verifier le pointeur */
    if (engine == NULL)
        return;

    /* 2. Verifier le temps */
    if (!isfinite(dt) || dt <= 0.0f)
        return;

    /* 3. Verifier les donnees */
    if (!isfinite(engine->vehicle_speed) ||
        !isfinite(engine->rpm) ||
        !isfinite(engine->throttle))
        return;

    if (engine->vehicle_speed < 0.0f ||
        engine->rpm < 0.0f)
        return;

    /* 4. Limiter le throttle */
    float throttle = engine->throttle;

    if (throttle < 0.0f)
        throttle = 0.0f;

    if (throttle > 100.0f)
        throttle = 100.0f;

    /* 5. Convertir la vitesse de km/h en m/s */
    float speed_ms = engine->vehicle_speed / 3.6f;

    /* 6. Calculer la force de traction */
    float traction_force = 0.0f;

    if (engine->engine_running == true)
    {
        traction_force =
            MAX_TRACTION_FORCE * (throttle / 100.0f);
    }

    /* 7. Calculer la resistance au roulement */
    float rolling_force =
        ROLLING_COEFFICIENT * VEHICLE_MASS * GRAVITY;

    /* 8. Calculer la resistance aerodynamique */
    float aerodynamic_force =
        0.5f * AIR_DENSITY * DRAG_AREA *
        speed_ms * speed_ms;

    /* 9. Calculer la resistance totale */
    float resistance_force =
        rolling_force + aerodynamic_force;

    /* 10. Calculer la force nette */
    float net_force =
        traction_force - resistance_force;

    /* 11. Calculer l'acceleration (2eme loi de Newton) */
    float acceleration = net_force / VEHICLE_MASS;

    /* 12. Calculer la variation de vitesse */
    float delta_speed = acceleration * dt;

    /* 13. Calculer la nouvelle vitesse en m/s */
    float new_speed_ms = speed_ms + delta_speed;

    if (!isfinite(new_speed_ms))
        return;

    /* 14. Limiter la vitesse */
    if (new_speed_ms < 0.0f)
        new_speed_ms = 0.0f;

    if (new_speed_ms > VEHICLE_SPEED_MAX / 3.6f)
        new_speed_ms = VEHICLE_SPEED_MAX / 3.6f;

    /* 15. Reconvertir en km/h */
    engine->vehicle_speed = new_speed_ms * 3.6f;
}

void engine_update_voltage(Engine_model *engine, float dt){
    /* 1. Verify the pointer */
    if (engine == NULL)
        return;

    /* 2. Verify elapsed time */
    if (!isfinite(dt) || dt <= 0.0f)
        return;

    /* 3. Verify battery parameters */
    if (!isfinite(engine->battery_soc) ||
        !isfinite(engine->battery_capacity_ah) ||
        !isfinite(engine->electrical_load) ||
        !isfinite(engine->battery_internal_resistance))
        return;

    if (engine->battery_capacity_ah <= 0.0f ||
        engine->electrical_load < 0.0f ||
        engine->battery_internal_resistance < 0.0f)
        return;

    /* 4. Limit battery SOC */
    float soc = engine->battery_soc;

    if (soc < 0.0f)
        soc = 0.0f;

    if (soc > 100.0f)
        soc = 100.0f;

    /* 5. Calculate battery current */
    float battery_current = 0.0f;

    if (engine->engine_running == true)
    {
        /* Engine running: alternator supplies the loads */

        if (engine->electrical_load <= ALTERNATOR_MAX_CURRENT)
        {
            float available_current =
                ALTERNATOR_MAX_CURRENT - engine->electrical_load;

            if (soc < 100.0f)
            {
                battery_current = available_current;

                if (battery_current > MAX_BATTERY_CHARGE_CURRENT)
                    battery_current = MAX_BATTERY_CHARGE_CURRENT;
            }
        }
        else
        {
            /* Alternator cannot supply all electrical loads */
            battery_current =
                -(engine->electrical_load - ALTERNATOR_MAX_CURRENT);
        }
    }
    else
    {
        /* Engine stopped: battery supplies all electrical loads */
        battery_current = -engine->electrical_load;
    }

    /* 6. Update battery SOC using coulomb counting */
    float delta_soc =
        (battery_current * dt) /
        (engine->battery_capacity_ah * 3600.0f) * 100.0f;

    float new_soc = soc + delta_soc;

    if (!isfinite(new_soc))
        return;

    if (new_soc < 0.0f)
        new_soc = 0.0f;

    if (new_soc > 100.0f)
        new_soc = 100.0f;

    /* 7. Estimate open-circuit voltage */
    float open_circuit_voltage =
        BATTERY_OCV_MIN +
        (BATTERY_OCV_MAX - BATTERY_OCV_MIN) *
        (new_soc / 100.0f);

    /* 8. Calculate battery terminal voltage */
    float new_voltage;

    if (engine->engine_running == true &&
        engine->electrical_load <= ALTERNATOR_MAX_CURRENT)
    {
        /* Alternator regulates the electrical system voltage */
        new_voltage = BATTERY_VOLTAGE_TARGET;
    }
    else
    {
        /* Battery supplies current: internal resistance causes voltage drop */
        new_voltage =
            open_circuit_voltage +
            battery_current * engine->battery_internal_resistance;
    }

    /* 9. Limit battery voltage */
    if (new_voltage < BATTERY_VOLTAGE_MIN)
        new_voltage = BATTERY_VOLTAGE_MIN;

    if (new_voltage > BATTERY_VOLTAGE_MAX)
        new_voltage = BATTERY_VOLTAGE_MAX;

    /* 10. Verify final values */
    if (!isfinite(new_voltage) ||
        !isfinite(battery_current))
        return;

    /* 11. Update the battery model */
    engine->battery_soc = new_soc;
    engine->battery_current = battery_current;
    engine->battery_voltage = new_voltage;
}
void engine_update(Engine_model *engine, float dt)
{
    if (engine == NULL || !isfinite(dt) || dt <= 0.0f)
    {
        return;
    }

    engine_update_rpm(engine, dt);
    engine_update_temperature(engine, dt);
    engine_update_speed(engine, dt);
    engine_update_voltage(engine, dt);

    engine->simulation_time += dt;
}

bool engine_validate(const Engine_model *engine)
{
    if (engine == NULL)
    {
        return false;
    }

    /* Vérification des valeurs numériques */
    if (!isfinite(engine->rpm) ||
        !isfinite(engine->coolant_temp) ||
        !isfinite(engine->vehicle_speed) ||
        !isfinite(engine->battery_voltage) ||
        !isfinite(engine->battery_soc) ||
        !isfinite(engine->battery_capacity_ah) ||
        !isfinite(engine->electrical_load) ||
        !isfinite(engine->battery_internal_resistance) ||
        !isfinite(engine->battery_current) ||
        !isfinite(engine->throttle) ||
        !isfinite(engine->simulation_time))
    {
        return false;
    }

    /* Vérification des limites physiques */
    if (engine->rpm < 0.0f || engine->rpm > 8000.0f)
        return false;

    if (engine->coolant_temp < -40.0f ||
        engine->coolant_temp > 150.0f)
        return false;

    if (engine->vehicle_speed < 0.0f ||
        engine->vehicle_speed > 300.0f)
        return false;

    if (engine->battery_voltage < 0.0f ||
        engine->battery_voltage > 20.0f)
        return false;

    if (engine->battery_soc < 0.0f ||
        engine->battery_soc > 100.0f)
        return false;

    if (engine->throttle < 0.0f ||
        engine->throttle > 100.0f)
        return false;

    if (engine->battery_capacity_ah < 0.0f ||
        engine->electrical_load < 0.0f ||
        engine->battery_internal_resistance < 0.0f ||
        engine->simulation_time < 0.0f)
        return false;

    return true;
}

