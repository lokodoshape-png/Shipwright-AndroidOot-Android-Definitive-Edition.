// OOT D.E.: the mouse / touch pointer for the QuestMenu (the "Diário de Quests" of the pause menu).
// On Android, SDL turns the touches into mouse events, so one tap is a left click and a drag is a held button.
#ifndef QUEST_MENU_POINTER_H
#define QUEST_MENU_POINTER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool valid;  // inside the game image, and not used by the ImGui menus
    bool down;   // left button held / finger on the screen
    float x;     // game screen coordinates: 0..320 (outside on the sides of wide screens), 0..240
    float y;
    float wheel; // mouse wheel since the last frame, positive upwards
} QuestMenuPointer;

void QuestMenuPointer_Get(QuestMenuPointer* out);

#ifdef __cplusplus
}
#endif

#endif
