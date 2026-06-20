/*
 * sound.c - Haiku port stub
 * Sound disabled; SDL audio replaced with no-ops.
 * Port to Haiku Media Kit can be done later.
 */

int soundDisabled = 1;

void soundInit() {}
void soundEnd()  {}
void soundPlay(int nb) { (void)nb; }
