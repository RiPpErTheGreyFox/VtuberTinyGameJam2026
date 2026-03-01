SECTION "Gameplay Specific Variables", WRAM0

wLevelBoxCount: db
wLevelFlammableBoxCount: db
wLevelRadioactiveBoxCount: db
wLevelScore: db

wSecondsRemainingInLevel: db
wTicksUntilNextSecond: db

SECTION "Animation Variables", WRAM0
wPlayerAnimationCounter: db				; counts frames before next frame of animation
wPlayerAnimationCurrentFrame: db		; keeps track of the current frame
wPlayerTileTLOffsetX: db
wPlayerTileTROffsetX: db
wPlayerTileBLOffsetX: db
wPlayerTileBROffsetX: db
wPlayerFlipped: db

SECTION "Gameplay Subroutines", ROM0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;	GAME SPECIFIC SUBROUTINE
;;	BLOCK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

InitialisePlayer:
	; initialise player variables and load/define the tiles needed
	ld a, 8
	ld [mainCharacter_XPos], a
	ld [mainCharacter_XDestination], a
	ld a, 16
	ld [mainCharacter_YPos], a
	ld [mainCharacter_YDestination], a
	ld a, 0
	ld [mainCharacter_Direction], a
	ld [mainCharacter_TileFlags], a
	ld a, 128
	ld [mainCharacter_OAMOffset], a
	ld a, 1
	ld [mainCharacter_AllowMove], a

	; initialise animation variables
	ld a, 0
	ld [wPlayerAnimationCounter], a
	ld [wPlayerAnimationCurrentFrame], a
	ld [wPlayerFlipped], a

	call UpdatePlayerFlipped
	

	ld de, PlayerSpriteData
	ld bc, PlayerSpriteDataEnd - PlayerSpriteData
	ld a, 0

	call TileLoader

	; for now, main character tiles are upper is $+2 and lower is $+7
	ld a, PLAYER_TL_FRAME1_TILE
	ld [mainCharacter_TileTL],a
	ld a, PLAYER_TR_FRAME1_TILE
	ld [mainCharacter_TileTR], a
	ld a, PLAYER_BL_FRAME1_TILE
	ld [mainCharacter_TileBL],a
	ld a, PLAYER_BR_FRAME1_TILE
	ld [mainCharacter_TileBR], a

	; get OAM offset
	ld a,  [mainCharacter_OAMOffset]	; get the offset from the start of OAM
	ld h, 0
	ld l, a								; load the offset into hl
	ld bc, $FE00						; start of OAM memory
	add hl, bc							; add the offset and the start of OAM together

	call WaitNoMode3

	; assign and update the OAM's
	ld a, [mainCharacter_YPos]
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld [hli], a
	ld a, [mainCharacter_TileTL]
	ld [hli], a
	ld a, %00000000
	ld [hli], a
	
	ld a, [mainCharacter_YPos]
	ld [hli], a
	ld a, [mainCharacter_XPos]
	add 8
	ld [hli], a
	ld a, [mainCharacter_TileTR]
	ld [hli], a
	ld a, %00000000
	ld [hli], a
	
	ld a, [mainCharacter_YPos]
	add 8
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld [hli], a
	ld a, [mainCharacter_TileBL]
	ld [hli], a
	ld a, %00000000
	ld [hli], a
	
	ld a, [mainCharacter_YPos]
	add 8
	ld [hli], a
	ld a, [mainCharacter_XPos]
	add 8
	ld [hli], a
	ld a, [mainCharacter_TileBR]
	ld [hli], a
	ld a, %00000000
	ld [hli], a

	ret

InitialiseCountdownTimer:
	ld a, DEFAULT_SECONDS_PER_LEVEL
	ld [wSecondsRemainingInLevel], a

	ld a, TICKS_PER_SECOND
	ld [wTicksUntilNextSecond], a

	ret

InitialiseScore:
	ld a, 0
	ld [wLevelScore], a

	ret

; updates and draws the timer at the top of the screen, call only once per vsync to keep time
; @clobbers: a, b, de, hl
; @returns: c - 1 for timer run out, 0 if time still remaining
UpdateCountdownTimer:
	ld a, [wTicksUntilNextSecond]
	dec a
	ld [wTicksUntilNextSecond], a
	cp a, 0
	jp z, .DecrementSecond				; if the tick timer is down to zero, then jump to decrement second
	jp .DrawTimer						; otherwise just draw it and we'll be on our way
.DecrementSecond:
	ld a, [wSecondsRemainingInLevel]
	dec a
	ld [wSecondsRemainingInLevel], a
	; do a check for zero seconds at this point
	ld c, 0								; set the return to 0
	cp a, 0
	jp z, .TimerRanOut
	jp .ResetTicks
.TimerRanOut:
	ld c, 1
.ResetTicks:
	ld a, TICKS_PER_SECOND
	ld [wTicksUntilNextSecond], a
.DrawTimer:
	ld a, [wSecondsRemainingInLevel]
	ld hl, wNumberStringData
	call NumberToString

	ld hl, wNumberStringData
	ld de, $9800 + $11
	call DrawTextTilesLoop
	ret

UpdateScoreDrawing:
	ld a, [wLevelScore]
	ld hl, wNumberStringData
	call NumberToString

	ld hl, wNumberStringData
	ld de, $9800 + $04
	call DrawTextTilesLoop
	ret

; When passed in an XY, determine if tile is passable or not
; @param b: X
; @param c: Y
; @return a: 1 if impassable otherwise 0
CheckCollisionAtXY:
	; bc is already in the registers ready to call
	call GetTileAddressByPixel
	
	call WaitNoMode3
	
	ld a, [hl]						; get the tile type at that address
	cp a, 128						; compare to 127, if > 127, carry bit will be not set
	jp nc, .Impassable				; if carry bit not set, there's already a box there, jump out

	; redefine impassable tiles when doing the next art pass
	; tiles 1, 2 and 3 in the background bank are impassable
	;cp a, 1
	;jp z, .Impassable

	;cp a, 2
	;jp z, .Impassable
	
	;cp a, 3
	;jp z, .Impassable
.Passable
	ld a, 0
	ret

.Impassable
	ld a, 1
	ret

; UpdatePlayer runs through the per frame updates required
; usually input and collision checking
UpdatePlayer:
.UpdateMovementToDestination
.CheckYDestinationPositionDifference
	; clear the allow move flag register
	ld de, 0
	; if the destination != the current position, process updates to move the player towards it
	ld a, [mainCharacter_YPos]
	ld b, a
	ld a, [mainCharacter_YDestination]
	cp a,b
	; do the move closer math here
	jp z, .YIsTheSame
	jp c, .SubFromY
	; if Y < Destination, add to Y
	; if Y > Destination, subtract from Y
.AddToY
	ld a, [mainCharacter_YPos]
	inc a
	ld [mainCharacter_YPos], a		
	jp .CheckXDestinationPositionDifference
.SubFromY			
	ld a, [mainCharacter_YPos]
	dec a
	ld [mainCharacter_YPos], a
	jp .CheckXDestinationPositionDifference
.YIsTheSame
	ld d, 1
	ld [mainCharacter_AllowMove], a
.CheckXDestinationPositionDifference
	; if the destination != the current position, process updates to move the player towards it
	ld a, [mainCharacter_XPos]
	ld b, a
	ld a, [mainCharacter_XDestination]
	cp a,b
	; do the move closer math here
	jp z, .XIsTheSame
	jp c, .SubFromX
	; if Y < Destination, add to X
	; if Y > Destination, subtract from X
.AddToX
	ld a, [mainCharacter_XPos]
	inc a
	ld [mainCharacter_XPos], a		
	jp .CheckIfAcceptingNewDestination
.SubFromX			
	ld a, [mainCharacter_XPos]
	dec a
	ld [mainCharacter_XPos], a
	jp .CheckIfAcceptingNewDestination
.XIsTheSame
	ld e, 1
	ld [mainCharacter_AllowMove], a
.CheckIfAcceptingNewDestination
	; check if both d and e flags are set
	ld a, d
	cp a, 1
	jp nz, .KeyCheckFinished
	ld a, e
	cp a, 1
	jp nz, .KeyCheckFinished
	ld a, 1
	ld [mainCharacter_AllowMove], a
	; check if debounce is active
	call CheckButtonDebounce
	cp a, 0									; check if debounce is still running
	jp z, .CheckMoveFlag
	jp .KeyCheckFinished

.CheckMoveFlag
	ld a,[mainCharacter_AllowMove]			; check if we're allowed to accept a new destination
	cp a, 1									; allowMove == 1 means accept new input
	jp nz, .KeyCheckFinished				; otherwise just skip movement input
.CheckForKeyPresses
	ld a, [wCurKeys]
	and a, PADF_UP
	jp z, .CheckDownPressed

	ld a, [mainCharacter_YDestination]
	; check for low Y clamp
	cp a, 24
	jp c, .CheckDownPressed					; if Y is 8 or lower, just jump
	; check if direction is already correct
	ld e, a
	ld a, [mainCharacter_Direction]
	cp a, 0
	ld a, e
	jp nz, .SetDirectionUp
	sub 8
	ld [mainCharacter_YDestination], a
	ld a, 0
	ld [mainCharacter_AllowMove], a
	jp .CheckDownPressed
.SetDirectionUp
	ld a, 0
	ld [mainCharacter_Direction], a
	ld a, 5
	call SetButtonDebounceRegA

.CheckDownPressed
	ld a, [wCurKeys]
	and a, PADF_DOWN
	jp z, .CheckLeftPressed

	ld a, [mainCharacter_YDestination]
	; check for high Y clamp
	cp a, 144 - 24
	jp nc, .CheckLeftPressed
	; check if direction is already correct
	ld e, a
	ld a, [mainCharacter_Direction]
	cp a, 2
	ld a, e
	jp nz, .SetDirectionDown
	add 8
	ld [mainCharacter_YDestination], a
	ld a, 0
	ld [mainCharacter_AllowMove], a
	jp .CheckLeftPressed
.SetDirectionDown
	ld a, 2
	ld [mainCharacter_Direction], a
	ld a, 5
	call SetButtonDebounceRegA

.CheckLeftPressed
	ld a, [wCurKeys]
	and a, PADF_LEFT
	jp z, .CheckRightPressed

	ld a, [mainCharacter_XDestination]
	; check for low X clamp
	cp a, 16
	jp c, .CheckRightPressed				; if Y is 8 or lower, just jump
	; check if direction is already correct
	ld e, a
	ld a, [mainCharacter_Direction]
	cp a, 3
	ld a, e
	jp nz, .SetDirectionLeft
	sub 8
	ld [mainCharacter_XDestination], a
	ld a, 0
	ld [mainCharacter_AllowMove], a
	jp .CheckRightPressed
.SetDirectionLeft
	ld a, 3
	ld [mainCharacter_Direction], a
	ld a, 5
	call SetButtonDebounceRegA

.CheckRightPressed
	ld a, [wCurKeys]
	and a, PADF_RIGHT
	jp z, .KeyCheckFinished

	ld a, [mainCharacter_XDestination]
	; check for high X clamp
	cp a, 160 - 24
	jp nc, .KeyCheckFinished
	; check if direction is already correct
	ld e, a
	ld a, [mainCharacter_Direction]
	cp a, 1
	ld a, e
	jp nz, .SetDirectionRight
	add 8
	ld [mainCharacter_XDestination], a
	ld a, 0
	ld [mainCharacter_AllowMove], a
	jp .KeyCheckFinished
.SetDirectionRight
	ld a, 1
	ld [mainCharacter_Direction], a
	ld a, 5
	call SetButtonDebounceRegA

.KeyCheckFinished

.DestinationCollisionCheck
	; take the intended destination, and if it would collide, just back out

	; do direction based collision checking 0 = N, 1 = E, 2 = S, 3 = W
	ld a, [mainCharacter_Direction]
	cp a, 0
	jp z, .NorthCheck
	cp a, 1
	jp z, .EastCheck
	cp a, 2
	jp z, .SouthCheck
	cp a, 3
	jp z, .WestCheck

	; TODO: exception handling
	jp .FinishCollisionChecks

.NorthCheck
	ld a, [mainCharacter_XDestination]
	ld b, a
	ld a, [mainCharacter_YDestination]
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	ld a, [mainCharacter_XDestination]
	add 8
	ld b, a
	ld a, [mainCharacter_YDestination]
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	jp .FinishCollisionChecks
.EastCheck
	ld a, [mainCharacter_XDestination]
	add 8
	ld b, a
	ld a, [mainCharacter_YDestination]
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	ld a, [mainCharacter_XDestination]
	add 8
	ld b, a
	ld a, [mainCharacter_YDestination]
	add 8
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	jp .FinishCollisionChecks
.SouthCheck
	ld a, [mainCharacter_XDestination]
	ld b, a
	ld a, [mainCharacter_YDestination]
	add 8
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	ld a, [mainCharacter_XDestination]
	add 8
	ld b, a
	ld a, [mainCharacter_YDestination]
	add 8
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	jp .FinishCollisionChecks
.WestCheck
	ld a, [mainCharacter_XDestination]
	ld b, a
	ld a, [mainCharacter_YDestination]
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	ld a, [mainCharacter_XDestination]
	ld b, a
	ld a, [mainCharacter_YDestination]
	add 8
	ld c, a
	call CheckCollisionAtXY

	cp a, 1
	jp z, .Collided

	jp .FinishCollisionChecks

.Collided									; just stop our character where he is
	ld a, [mainCharacter_XPos]				; grab the current position
	and %11111000							; truncate it down to the closest align 8
	ld [mainCharacter_XDestination], a
	
	ld a, [mainCharacter_YPos]
	and %11111000
	ld [mainCharacter_YDestination], a

.FinishCollisionChecks
	ld a, [mainCharacter_AllowMove]
	cp a, 1
	jp z, .ResetToFrame1					; if we're not allowed to move, assume we're moving
	call UpdatePlayerAnimation				; only call player animation update when moving
	jp .EndOfFunc
.ResetToFrame1
	ld a, $FF								; go to the last frame of the animation so that the next frame command works
	call SetPlayerAnimationFrame
.EndOfFunc
	call UpdatePlayerFlipped
	ret

; keeps track of the animation counter and updates the tile references needed
; @clobbers a
UpdatePlayerAnimation:
		; check the frame counter
	ld a, [wPlayerAnimationCounter]
	; if above CONVEYOR_BELT_ANIMATION_SPEED, reset it and swap the tiles
	cp a, PLAYER_ANIMATION_SPEED
	jp nc, .TimeToSwap
	inc a
	ld [wPlayerAnimationCounter], a
	jp EndOfAnimFunc

.TimeToSwap
	; we're moving so play the walking sound
	ld hl, sound_character_walk
	call PlaySoundHL

	ld a, 0
	ld [wPlayerAnimationCounter], a

	ld a, [wPlayerAnimationCurrentFrame]	
SetPlayerAnimationFrame:	
	; check the frame number and jump to the correct next frame (zero indexed)
	cp a, 0
	jp z, .SecondFrame
	cp a, 1
	jp z, .ThirdFrame
.FirstFrame
	ld a, PLAYER_TL_FRAME1_TILE
	ld [mainCharacter_TileTL], a
	ld a, PLAYER_TR_FRAME1_TILE
	ld [mainCharacter_TileTR], a
	ld a, PLAYER_BL_FRAME1_TILE
	ld [mainCharacter_TileBL], a
	ld a, PLAYER_BR_FRAME1_TILE
	ld [mainCharacter_TileBR], a
	ld a, 0	; zero indexed
	ld [wPlayerAnimationCurrentFrame], a
	jp .EndOfFunc
	; load in the first set of tiles
.SecondFrame
	ld a, PLAYER_TL_FRAME2_TILE
	ld [mainCharacter_TileTL], a
	ld a, PLAYER_TR_FRAME2_TILE
	ld [mainCharacter_TileTR], a
	ld a, PLAYER_BL_FRAME2_TILE
	ld [mainCharacter_TileBL], a
	ld a, PLAYER_BR_FRAME2_TILE
	ld [mainCharacter_TileBR], a
	ld a, 1	; zero indexed
	ld [wPlayerAnimationCurrentFrame], a
	jp .EndOfFunc
	; load in the second set of tiles
.ThirdFrame
	ld a, PLAYER_TL_FRAME3_TILE
	ld [mainCharacter_TileTL], a
	ld a, PLAYER_TR_FRAME3_TILE
	ld [mainCharacter_TileTR], a
	ld a, PLAYER_BL_FRAME3_TILE
	ld [mainCharacter_TileBL], a
	ld a, PLAYER_BR_FRAME3_TILE
	ld [mainCharacter_TileBR], a
	ld a, 2	; zero indexed
	ld [wPlayerAnimationCurrentFrame], a
	jp .EndOfFunc
	; load in the second set of tiles
.EndOfFunc
EndOfAnimFunc:
	ret

; sets up all the flags and offsets to set up if we're flipped or not
UpdatePlayerFlipped:
	; do direction based checking 0 = N, 1 = E, 2 = S, 3 = W
	ld a, [mainCharacter_Direction]
	cp a, 1
	jp z, .EastFlip
	cp a, 3
	jp z, .WestFlip
	jp .FlipCheck

.EastFlip
	ld a, 1
	ld [wPlayerFlipped], a
	jp .FlipCheck

.WestFlip
	ld a, 0
	ld [wPlayerFlipped], a

	; now check if the player flipped tag is set
.FlipCheck
	ld a, [wPlayerFlipped]
	cp a, 1
	jp z, .FacingRight			; if flipped, then facing right, otherwise is facing left

.FacingLeft
	ld a, PLAYER_TL_NORM_X_OFFSET
	ld [wPlayerTileTLOffsetX], a
	ld a, PLAYER_TR_NORM_X_OFFSET
	ld [wPlayerTileTROffsetX], a
	ld a, PLAYER_BL_NORM_X_OFFSET
	ld [wPlayerTileBLOffsetX], a
	ld a, PLAYER_BR_NORM_X_OFFSET
	ld [wPlayerTileBROffsetX], a
	ld a, %00000000
	ld [mainCharacter_TileFlags], a
	jp .EndOfFunc
.FacingRight
	ld a, PLAYER_TL_FLIP_X_OFFSET
	ld [wPlayerTileTLOffsetX], a
	ld a, PLAYER_TR_FLIP_X_OFFSET
	ld [wPlayerTileTROffsetX], a
	ld a, PLAYER_BL_FLIP_X_OFFSET
	ld [wPlayerTileBLOffsetX], a
	ld a, PLAYER_BR_FLIP_X_OFFSET
	ld [wPlayerTileBROffsetX], a
	ld a, %00100000
	ld [mainCharacter_TileFlags], a
	jp .EndOfFunc

.EndOfFunc
	ret
; loads in anything that may change per level
; needs the LCD off
; @clobbers a, bc, de, hl
LoadLevelSpecificData:

	; Copy tile data into VRAM
	ld de, LevelOneTiles				; load the address of the tiles into the DE register
	ld hl, $9000				; load the beginning VRAM address into HL (HL is easy to inc/dec)
	ld bc, LevelOneTilesEnd - LevelOneTiles		; load the length of Tiles into BC
	call Memcopy				; call the memcopy subroutine

	; The above tile loading will clobber the tilemap in VRAM, but for now just load the other half of tiles

	; Copy tilemap data into VRAM (functionally identical to above but pointing to tilemap data and addresses)
	ld de, LevelOneTilemap
	ld hl, $9800
	ld bc, LevelOneTilemapEnd - LevelOneTilemap
	call Memcopy				; call the memcopy subroutine

	jp .EndOfFunc
.EndOfFunc
	ret 

; loads the background, tilemap and sets up the rest of the game field
; make sure the LCD is OFF
; @clobbers a, bc, de, hl
InitialiseLevel:

	call LoadLevelSpecificData
	call TileLoaderReset

	call InitialisePlayer
	call InitialiseCursor
	call InitialiseFont
	call InitialiseCountdownTimer
	call InitialiseScore
	call InitialisePlantGraphics
	
	ld b, 32
	ld c, 48
	ld hl, testPlant0
	call InitialisePlant

	ld b, 32+32
	ld c, 48
	ld hl, testPlant1
	call InitialisePlant

	ld b, 32+64
	ld c, 48
	ld hl, testPlant2
	call InitialisePlant

	ld b, 32+96
	ld c, 48
	ld hl, testPlant3
	call InitialisePlant

	ld b, 32
	ld c, 80
	ld hl, testPlant4
	call InitialisePlant

	ld b, 32+32
	ld c, 80
	ld hl, testPlant5
	call InitialisePlant

	ld b, 32+64
	ld c, 80
	ld hl, testPlant6
	call InitialisePlant

	ld b, 32+96
	ld c, 80
	ld hl, testPlant7
	call InitialisePlant

	call SetBlankDMGPalette

	; do a single tick as part of loading
	call UpdateGameScene

	ret

; call to load tiles and initialise variables
InitialiseCursor:
	; zero out everything
	ld a, 0
	ld [boxCursor_YPos], a
	ld [boxCursor_XPos], a
	ld [boxCursor_Tile], a
	ld [boxCursor_Enabled], a
	ld a, 0							; TODO: hardcoded OAM
	ld [boxCursor_OAMOffset], a		; set the OAM offset to be clear of the player character and box object

	ld de, CursorSpriteData
	ld bc, CursorSpriteDataEnd - CursorSpriteData
	ld a, 0
	call TileLoader

	ld a, c
	ld [boxCursor_Tile], a

	ret

; call each frame to make sure the cursor is placed in the correct spot
UpdateCursor:
	; get player XPos and YPos
	; calculate an offset from the top left to the intended position (adjust this with direction later)
	ld a, [mainCharacter_XPos]		; grab the X position of the player
	sub 8							; offset from the top left
	ld b, a							; stick it in B ready for any function calls
	ld a, [mainCharacter_YPos]		; do the same for Y
	add 8
	ld c, a

	call AdjustBoxOffsetForDirection

	push bc							; save the X/Y position
	call WaitNoMode3
	; call get tile by pixel to get the tile offset
	call GetTileAddressByPixel
	pop bc
	; check if the tile index is above 128, if so then 
	; check if there's a box in hand, or if there's a box in front of us
	call WaitNoMode3
	ld a, [hl]						; get the tile type at that address
	cp a, 179						; compare to 179 and 180, if
	jp z, .EnableCursor				; if zero bit is set we've got the pot
	cp a, 180
	jp z, .EnableCursor
	
.DisableCursor
	ld a, 0
	ld [boxCursor_Enabled], a
	ld [boxCursor_XPos], a
	ld [boxCursor_YPos], a
	jp .EnabledCheck
.EnableCursor
	ld a, 1
	ld [boxCursor_Enabled], a
	ld a, b
	add 8							; do X position offset for OAM offset
	and %11111000					; mask it to snap it to the lowest align by 8 pixel position
	ld [boxCursor_XPos], a
	ld a, c
	add 16
	and %11111000
	ld [boxCursor_YPos], a
.EnabledCheck
	ld a, [boxCursor_Enabled]
	cp a, 0
	jp z, .DoNotDraw
	call EnableCursor
	jp .FinishedDrawing
.DoNotDraw
	call DisableCursor
.FinishedDrawing
	ret 

; show the cursor
EnableCursor:
	; get OAM offset
	ld a,  [boxCursor_OAMOffset]	; get the offset from the start of OAM
	ld h, 0
	ld l, a								; load the offset into hl
	ld bc, $FE00						; start of OAM memory
	add hl, bc							; add the offset and the start of OAM together

	ld a, [boxCursor_YPos]				; update the OAM with the new position
	ld [hli], a
	ld a, [boxCursor_XPos]
	ld [hli], a

	ret

; hide the cursor
DisableCursor:
	ld a, 0
	ld [boxCursor_YPos], a
	ld [boxCursor_XPos], a

	ret
	
; Function for checking and adjusting the direction based offsets for box related positions
; @param bc: x/y
; @clobbers a
; @return bc: x/y
AdjustBoxOffsetForDirection:
	; do direction based offsetting 0 = N, 1 = E, 2 = S, 3 = W
	ld a, [mainCharacter_Direction]
	cp a, 0
	jp z, .NorthCursor
	cp a, 1
	jp z, .EastCursor
	cp a, 2
	jp z, .SouthCursor
	cp a, 3
	jp z, .WestCursor

.NorthCursor						; offset the cursor to appear in the correct spot
	ld a, b							; remember that it starts at -8/+8
	add 8
	ld b, a
	ld a, c
	sub 16
	ld c, a
	jp .EndOfFunc
.EastCursor
	ld a, b				
	add 24
	ld b, a
	ld a, c
	add 0
	ld c, a
	jp .EndOfFunc
.SouthCursor	
	ld a, b				
	add 8
	ld b, a
	ld a, c
	add 8
	ld c, a
	jp .EndOfFunc
.WestCursor
	; defaults are for west cursor
	jp .EndOfFunc

.EndOfFunc
	ret


; draw function where we actually update the OAM's to make sure it's at the start of the frame
DrawPlayer:
.UpdateOAM
	; get OAM offset
	ld a,  [mainCharacter_OAMOffset]	; get the offset from the start of OAM
	ld h, 0
	ld l, a								; load the offset into hl
	ld bc, $FE00						; start of OAM memory
	add hl, bc							; add the offset and the start of OAM together

	call WaitNoMode3

	ld a, [mainCharacter_YPos]
	add 16							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld b, a
	ld a, [wPlayerTileTLOffsetX]
	add b							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_TileTL]
	ld [hli], a
	ld a, [mainCharacter_TileFlags]
	ld [hli], a

	ld a, [mainCharacter_YPos]
	add 16							; then add the metasprite offset							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld b, a
	ld a, [wPlayerTileTROffsetX]
	add b							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_TileTR]
	ld [hli], a
	ld a, [mainCharacter_TileFlags]
	ld [hli], a

	ld a, [mainCharacter_YPos]
	add 16							; make sure to set the sprite offsets
	add 8							; then add the metasprite offset
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld b, a
	ld a, [wPlayerTileBLOffsetX]
	add b							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_TileBL]
	ld [hli], a
	ld a, [mainCharacter_TileFlags]
	ld [hli], a

	ld a, [mainCharacter_YPos]
	add 16							; make sure to set the sprite offsets
	add 8							; then add the metasprite offset
	ld [hli], a
	ld a, [mainCharacter_XPos]
	ld b, a
	ld a, [wPlayerTileBROffsetX]
	add b							; make sure to set the sprite offsets
	ld [hli], a
	ld a, [mainCharacter_TileBR]
	ld [hli], a
	ld a, [mainCharacter_TileFlags]
	ld [hli], a
.UpdateOAMFinished
	ret

; Cursor Draw function
DrawCursorObject:
	ld a, [boxCursor_XPos]				; load the position into bc
	ld d, a
	ld a, [boxCursor_YPos]
	ld e, a
	ld a, [boxCursor_Enabled]
	cp a, 1
	jp z, .StartDrawing
	ld d, 0								; if it's not supposed to be enabled just set the positions to 0
	ld e, 0
.StartDrawing
	call WaitNoMode3
	; initialise the OAM
	ld hl, _OAMRAM				; grab the address for the top of the OAM
	ld a, [boxCursor_OAMOffset]
	ld b, a
	ld a, l
	add b	; add the offset
	ld l, a

	call WaitNoMode3

	ld a, [boxCursor_YPos]
	ld [hl], e
	inc hl
	ld a, [boxCursor_XPos]
	ld [hl], d
	inc hl
	ld a, [boxCursor_Tile]
	ld [hli], a
	ld a, %00000000
	ld [hli], a
.EndOfFunc
	ret

; big game scene update function, put here to make it easier to separate out all the different scene types
UpdateGameScene:
call DrawCursorObject
call DrawPlayer

call UpdateCountdownTimer
call UpdateScoreDrawing
call UpdateButtonDebounce
call UpdateKeys
call UpdatePlayer
call UpdateCursor

ld hl, testPlant0
call UpdatePlant
ld hl, testPlant1
call UpdatePlant
ld hl, testPlant2
call UpdatePlant
ld hl, testPlant3
call UpdatePlant
ld hl, testPlant4
call UpdatePlant
ld hl, testPlant5
call UpdatePlant
ld hl, testPlant6
call UpdatePlant
ld hl, testPlant7
call UpdatePlant

.ItemUsageChecking:
	ld a, [wCurKeys]
	and a, PADF_A
	jp nz, .WaterPlantCheck

	jp .EndOfFunc

.WaterPlantCheck:
	; check if the debounce is done
	call CheckButtonDebounce			; check the button debounce flag
	cp a, 1								; if a == 1, zero flag will be set
	jp z, .EndOfFunc					; skip the button
	call SetButtonDebounce				; otherwise debounce is clear and we can press button
	call WaterPlant						; make sure to reset the debounce
	jp .EndOfFunc

.EndOfFunc
	ret