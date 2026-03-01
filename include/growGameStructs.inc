INCLUDE "include/structs.inc"

    struct CURSOR
        bytes 1, YPos ; 1 byte
        bytes 1, XPos ; 1 byte
        bytes 1, Tile ; 1 byte tile index for drawing/type checking, will be referenced in the update sub
        bytes 1, OAMOffset ; 1 byte which OAM object this will be using to draw to
        bytes 1, Enabled ; 1 byte that will be treated as a bool
    end_struct

    struct PLAYER
        bytes 1, YPos ; 1 byte
        bytes 1, XPos ; 1 byte
        bytes 1, XDestination ; 1 byte
        bytes 1, YDestination ; 1 byte
        bytes 1, TileTL ; 1 byte tile index for drawing, will be referenced in the update sub
        bytes 1, TileTR ; 1 byte
        bytes 1, TileBL ; 1 byte
        bytes 1, TileBR ; 1 byte
        bytes 1, Direction ; 1 byte
        bytes 1, OAMOffset ; 1 byte which OAM object this will be using to draw to
        bytes 1, TileFlags ; 1 byte used to store the OAM flags
        bytes 1, AllowMove ; 1 byte treat as a bool
        bytes 1, ItemHeld ; 1 byte for ItemIndex, 255 for empty hand
    end_struct

    struct PLANT
        bytes 1, YPos ; 1 byte
        bytes 1, XPos ; 1 byte
        bytes 1, GrowthStage ; Contains the current growth stage of the plant (0-4)
        bytes 1, WaterAmount ; The amount of water currently in the plant, (0-255, takes some per tick)
        bytes 1, TimerToNextStage ; 1 byte which contains the amount of seconds until the next check
        bytes 1, TicksUntilNextSecond ; contains ticks until next second
    end_struct

    struct ITEM
        bytes 1, YPos ; 1 byte
        bytes 1, XPos ; 1 byte
        bytes 1, Tile ; 1 byte tile index for drawing/type checking, will be referenced in the update sub
        bytes 1, OAMOffset ; 1 byte which OAM object this will be using to draw to
        bytes 1, ItemType ; 0 = Watering Can
    end_struct