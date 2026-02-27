SECTION "Plant Data", WRAM0
	dstruct PLANT, testPlant ; declare our structs

SECTION "Plant Subroutines", ROM0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	GAME SPECIFIC SUBROUTINE
;;	BLOCK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Tests
InitialiseTestPlant:
	; initialise plant variables and load/define the tiles needed
	ld a, 80
	ld [testPlant_XPos], a
	ld a, 72
	ld [testPlant_YPos], a
	ld a, 0
	ld [testPlant_GrowthStage], a ; Contains the current growth stage of the plant (0-3)
    ld a, 255
    ld [testPlant_WaterAmount], a ; The amount of water currently in the plant, (0-255, takes some per tick)
    ld a, 2
    ld [testPlant_TimerToNextStage], a ; 1 byte which contains the amount of seconds until the next check
    ld a, TICKS_PER_SECOND
    ld [testPlant_TicksUntilNextSecond], a ; contains ticks until next second

	ld de, PlantTiles
	ld bc, PlantTilesEnd - PlantTiles
	ld a, 1

	call TileLoader

    ld a, c                             ; coming back from the tile loader, get the plant tiles start

    ld [testPlant_PlantTilesStart], a

    call RedrawTestPlant

    ret


; Per tick update function, used only for the singular plant for now
UpdateTestPlant:
    call PlantUpdateTimerUpdate

    ret

PlantUpdateTimerUpdate:
    ld a, [testPlant_TicksUntilNextSecond]
	dec a
	ld [testPlant_TicksUntilNextSecond], a
	cp a, 0
	jp z, .DecrementSecond				; if the tick timer is down to zero, then jump to decrement second
	ret						            ; otherwise we're done
.DecrementSecond:
	ld a, [testPlant_TimerToNextStage]
	dec a
	ld [testPlant_TimerToNextStage], a
	; do a check for zero seconds at this point
	cp a, 0
	jp z, .TimerRanOut
	jp .ResetTicks
.TimerRanOut:
	call GrowthStageUpdate
.ResetTicks:
	ld a, TICKS_PER_SECOND
	ld [testPlant_TicksUntilNextSecond], a
    ret

GrowthStageUpdate:
    ld a, 2
    ld [testPlant_TimerToNextStage], a

    ld a, [testPlant_GrowthStage]
    inc a

    cp a, 6
    jp nc, .ResetGrowth
    ld [testPlant_GrowthStage], a
    call RedrawTestPlant
    ret 
.ResetGrowth
    ld a, 0
    ld [testPlant_GrowthStage], a
    call RedrawTestPlant
    ret

; determines which tiles to write to and replaces them with the tile correct for the state the plant is in
RedrawTestPlant:

    ld a, [testPlant_XPos]              ; load the position into bc
    ld b, a
    ld a, [testPlant_YPos]
    ld c, a

    call GetTileAddressByPixel          ; turn it into a tile address

    ld a, [testPlant_PlantTilesStart]   ; figure out exactly which tile we're working with
    ld b, a
    ; if growth stage is 0, then plain pot, otherwise stubbed pot gets drawn (+1)
    ld a, [testPlant_GrowthStage]       ; index by the growth stage
    cp a, 0
    jp nz, .GrowthStage1
.GrowthStage0
    ld a, b
    jp .LowerTilePush
.GrowthStage1
    ld a, b
    inc a
.LowerTilePush
    push af                             ; put it on the stack

    ; work on the upper half
    ld a, l                           ; go back up a row
    sub $20
    ld l, a

    ld a, [testPlant_PlantTilesStart]
    ld b, a
    ld a, [testPlant_GrowthStage]
    cp a, 0
    jp z, .GrowthStageNotAbove0
.GrowthStageAbove0
    inc a
    add a, b
    call WaitNoMode3
    ld [hl], a
    jp .LowerHalfWrite
.GrowthStageNotAbove0
    ; get the original level
    push hl
    ld a, 1
    ld c, a
    call ConvertVRAMTileMapToROMTileMap
    ld a, [hl]
    pop hl
    call WaitNoMode3
    ld [hl], a
.LowerHalfWrite
    ; write the lower half
    ld a, l
    add $20
    ld l, a
    pop af                              ; pull off of the stack
    call WaitNoMode3
    ld [hl], a                          ; write it to the tilemap

    ret