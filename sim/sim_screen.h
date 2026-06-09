#pragma once

// Speglar de fysiska skärmmåtten från
// scout_screen/components/display/display.h så att UI-koden i ui.c kan
// vara en exakt kopia av enhetens. På enheten kommer SCREEN_W/SCREEN_H
// från display.h — håll dessa värden i synk med den filen.
#define SCREEN_W 1024
#define SCREEN_H 600
