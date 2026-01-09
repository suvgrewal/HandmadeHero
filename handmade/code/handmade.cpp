#include "handmade.h"



internal void
RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
    // TODO: See what optimizer does if passing by value

    // Bytes to move
    uint8* Row = (uint8*)Buffer->Memory;

    for (int Y = 0;
        Y < Buffer->Height;
        Y++)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0;
            X < Buffer->Width;
            X++)
        {
            uint8 Blue = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Red = 0;

            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void
GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	// GameOutputSound(SampleCountToOutput, SoundBuffer);
    
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}