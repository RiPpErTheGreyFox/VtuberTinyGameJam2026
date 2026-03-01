SECTION "Plant Data", WRAM0
	dstructs NUMBER_OF_PLANTS, PLANT, testPlant ; declare our structs

    wPlantTilesStart: db

; temp variables for pulling from ram, makes not having indexed or shadow registers easier
wTempYPos: db
wTempXPos: db
wTempGrowthStage: db
wTempWaterAmount: db
wTempTimerToNextStage: db
wTempTicksUntilNextSecond: db
    

SECTION "Plant Subroutines", ROM0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	GAME SPECIFIC SUBROUTINE
;;	BLOCK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; initialises the struct of a plant at address hl
; @params: b,c - x and y position (in pixels, 8px aligned) to place the pot
; @params: hl - address of plant

InitialisePlant:
    ; initialise plant variables and load/define the tiles needed
	ld a, c
	ld [wTempYPos], a
    ld a, b
	ld [wTempXPos], a
	ld a, 0
	ld [wTempGrowthStage], a ; Contains the current growth stage of the plant (0-3)
    ld a, 10
    ld [wTempWaterAmount], a ; The amount of water currently in the plant, (0-255, takes some per tick)
    ld a, 2
    ld [wTempTimerToNextStage], a ; 1 byte which contains the amount of seconds until the next check
    ld a, TICKS_PER_SECOND
    ld [wTempTicksUntilNextSecond], a ; contains ticks until next second

    call SaveTempVariablesToHL
    call RedrawPlant
    ret

; call once to ensure the tiles are in memory
; @clobbers: a, bc, de, hl
InitialisePlantGraphics:
	ld de, PlantTiles
	ld bc, PlantTilesEnd - PlantTiles
	ld a, 1

	call TileLoader

    ld a, c                             ; coming back from the tile loader, get the plant tiles start

    ld [wPlantTilesStart], a

    ret

; loads the struct from hl into the temp variables
; @params: hl - pointer to the plant struct we're aiming for
PopulateTempVariablesFromHL:
    ld a, [hli]
    ld [wTempYPos], a
    ld a, [hli]
    ld [wTempXPos], a
    ld a, [hli]
    ld [wTempGrowthStage], a
    ld a, [hli]
    ld [wTempWaterAmount], a
    ld a, [hli]
    ld [wTempTimerToNextStage], a
    ld a, [hli]
    ld [wTempTicksUntilNextSecond], a
    ret

; stores the struct data in temp variables into hl
; @params: hl - pointer to where we're saving the struct data
SaveTempVariablesToHL:
    ld a, [wTempYPos]
    ld [hli], a
    ld a, [wTempXPos]
    ld [hli], a
    ld a, [wTempGrowthStage]
    ld [hli], a
    ld a, [wTempWaterAmount]
    ld [hli], a
    ld a, [wTempTimerToNextStage]
    ld [hli], a
    ld a, [wTempTicksUntilNextSecond]
    ld [hli], a
    ret

; Per tick update function, used only for the singular plant for now
; @params: hl - target plant
UpdatePlant:
    push hl
    call PopulateTempVariablesFromHL
    call PlantUpdateTimerUpdate
    pop hl
    call SaveTempVariablesToHL

    ret

; assumes temp variables are already loading
PlantUpdateTimerUpdate:
    ld a, [wTempTicksUntilNextSecond]
	dec a
	ld [wTempTicksUntilNextSecond], a
	cp a, 0
	jp z, .DecrementSecond				; if the tick timer is down to zero, then jump to decrement second
	ret						            ; otherwise we're done
.DecrementSecond:
	ld a, [wTempTimerToNextStage]
	dec a
	ld [wTempTimerToNextStage], a
	; do a check for zero seconds at this point
	cp a, 0
	jp z, .TimerRanOut
	jp .ResetTicks
.TimerRanOut:
	call GrowthStageUpdate
.ResetTicks:
	ld a, TICKS_PER_SECOND
	ld [wTempTicksUntilNextSecond], a
    ret

; assumes temp variables are already loading
GrowthStageUpdate:
    ld a, 2
    ld [wTempTimerToNextStage], a

    ld a, [wTempGrowthStage]
    inc a

    cp a, 6
    jp nc, .ResetGrowth
    ld [wTempGrowthStage], a
    call RedrawPlant
    ret 
.ResetGrowth
    ld a, 0
    ld [wTempGrowthStage], a
    call PlantHarvested
    call RedrawPlant
    ret

; handles everything needed when the plant is harvested
PlantHarvested:
    ; Increase the score
    ld a, [wLevelScore]
    inc a
    ld [wLevelScore], a

    ; play a sound

    ret 

; determines which tiles to write to and replaces them with the tile correct for the state the plant is in
; assumes temp variables are already loading
RedrawPlant:
    ld a, [wTempYPos]
    ld c, a
    ld a, [wTempXPos]              ; load the position into bc
    ld b, a

    call GetTileAddressByPixel          ; turn it into a tile address

    ld a, [wPlantTilesStart]   ; figure out exactly which tile we're working with
    ld b, a
    ; if growth stage is 0, then plain pot, otherwise stubbed pot gets drawn (+1)
    ld a, [wTempGrowthStage]       ; index by the growth stage
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

    ld a, [wPlantTilesStart]
    ld b, a
    ld a, [wTempGrowthStage]
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

; Finds the address of which plant is at the X/Y value 
; @params: b,c X/Y coordinate
; @return: hl address of the plant at the point
FindPlantAddressAtXY:
    ld hl, testPlant0
    ld d, NUMBER_OF_PLANTS            ; set loop counter
.loop:

    ld a, [hli]
    cp a, c                             ; check if YPos matches
    jp nz, .YEscape                     ; if not, not our guy
    ld a, [hli]
    cp a, b                             ; check if XPos matches
    jp nz, .nextItem                    ; if not, next item
    jp .foundPlant                      ; otherwise both match and this is what we're looking for
.YEscape
    inc hl

.nextItem
    ld a, sizeof_PLANT-2                ; increment memory counter
    ld a, l
    add a, sizeof_PLANT-2
    ld l, a
    dec d
    jp nz, .loop                        ; if there's still loops left, keep going

.notFound
    ld hl, $0000                        ; set the return to 0 to indicate we didn't find anything
    ret 

.foundPlant:
    dec hl                              ; reset the pointer to the start of the struct
    dec hl
    ret

; grabs the plant the player is looking at and sets it's water to 255/$FF
WaterPlant:
    ; get player XPos and YPos
	; calculate an offset from the top left to the intended position (adjust this with direction later)
	ld a, [mainCharacter_XPos]		; grab the X position of the player
	sub 8							; offset from the top left
	ld b, a							; stick it in B ready for any function calls
	ld a, [mainCharacter_YPos]		; do the same for Y
	add 8
	ld c, a

	call AdjustBoxOffsetForDirection

	; call get tile by pixel to get the tile offset
	call FindPlantAddressAtXY

    ; check if the high address on the return is $00
    ld a, h
    cp a, $00
    jp z, .return                       ; if high address is $00 then we didn't find anything

    ld a, l                             ; grab the low address
    add a, 3                            ; we want the third byte of the plant struct
    ld l, a

    ld a, $FF                           ; set water to 255
    ld [hl], a
    
.return
    ret