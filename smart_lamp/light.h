#ifndef LIGHT_H
#define LIGHT_H

/** Initialize the BH1750 ambient light sensor module. */
void light_init();

/** Periodic update for the BH1750 ambient light sensor module. */
void light_update();

#endif  // LIGHT_H
