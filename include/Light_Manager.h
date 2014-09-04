#ifndef  LIGHT_MANAGER_H
#define  LIGHT_MANAGER_H

#include <avr/io.h>
#include <avr/cpufunc.h>

/*
 * light manager structure
 */
struct _Light_Manager {
    char currPattern;
    volatile uint16_t* output_a;
    volatile uint16_t* output_b;
    int16_t counter;
    int16_t patternSpeed;
};
typedef struct _Light_Manager LightManager;

// the manager singleton
extern LightManager _manager_instance;

/*
 * setup lighting manager
 * params are a manager and channel output registers
 */
void LightManager_init(volatile uint16_t *, volatile uint16_t *);

/* 
 * mark time in light manager
 */
void LightManager_tick(void);

/* 
 * implement the currently selected pattern
 */
void LightManager_setPattern(char);

#endif
