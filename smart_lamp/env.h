#ifndef ENV_H
#define ENV_H

/** Initialize the AHT20 temperature and humidity module. */
void env_init();

/** Periodic update for the AHT20 temperature and humidity module. */
void env_update();

#endif  // ENV_H
