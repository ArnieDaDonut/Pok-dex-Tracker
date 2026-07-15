#include "tracker_screen.h"
#include "constants/pokedex.h"
#include "constants/songs.h"
#include "data.h"
#include "gflib.h"
#include "global.h"
#include "menu.h"
#include "naming_screen.h"
#include "new_menu_helpers.h"
#include "overworld.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text_window.h"
#include "constants/items.h"
#include "item.h"
#include "region_map.h"
#include "constants/region_map_sections.h"
#include "wild_encounter.h"
#include "event_data.h"


static bool8 IsSubstring(const u8 *name, const u8 *query);
static void FilterPokemonList(void);

// ============================================================
// Constants
// ============================================================

static const u16 sFireRedAvailablePokemon[] = {
    1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  29,  30,  31,  32,
    33,  34,  35,  36,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
    50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
    65,  66,  67,  68,  72,  73,  74,  75,  76,  77,  78,  81,  82,  83,  84,
    85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
    115, 116, 117, 118, 119, 122, 123, 124, 125, 128, 129, 130, 131, 132, 133,
    134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
    149, 150, 161, 162, 165, 166, 167, 168, 169, 172, 173, 174, 175, 176, 177,
    178, 182, 186, 187, 188, 189, 193, 194, 195, 198, 201, 202, 206, 208, 211,
    212, 214, 218, 219, 220, 221, 225, 227, 230, 231, 232, 233, 236, 237, 238,
    239, 242, 243, 244, 245, 246, 247, 248, 249, 250, 360,
};

EWRAM_DATA u16 gTrackerSelectedSpecies = SPECIES_NONE;

#define TRACKER_TOTAL_POKEMON ARRAY_COUNT(sFireRedAvailablePokemon)
#define TRACKER_VISIBLE_ROWS 4
#define TRACKER_ROW_HEIGHT 30
#define TRACKER_NAME_X 34
#define TRACKER_STATUS_X 200


// Window IDs
enum { WIN_MAIN, WIN_FILTER, WIN_COUNT };

// Palette for uncaught Pokémon silhouettes (solid black)
static const u16 sSilhouettePaletteData[] = {
    RGB(0, 0, 0), // Transparent (Color 0)
    RGB(0, 0, 0), // Black for all other colors
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
    RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0), RGB(0, 0, 0),
};

static const struct SpritePalette sSilhouetteSpritePalette = {
    sSilhouettePaletteData,
    0x8888 // Unique tag for silhouette palette
};

// Custom Palette for perfectly matching the design
static const u16 sTrackerPalette[] = {
    RGB(4, 9, 9),    //  (Outer Screen Background)
    RGB(31, 31, 31), //  (Inner Window Background)
    RGB(12, 12, 12), //  (Normal Text - Medium-Dark Grey)
    RGB(24, 24, 24), //  (Shadow)
    RGB(28, 4, 4),   //  (Uncaught X)
    RGB(31, 20, 20), //  (Red Shadow)
    RGB(4, 25, 4),   //  (Caught Text)
    RGB(20, 31, 20), //  (Green Shadow)
    RGB(4, 12, 22),  //  (for location button)
    RGB(31, 31, 31), //  (for location button text)
    RGB(10, 16, 24), //  (Blue shadow)
    RGB(9, 12, 12),  //  (Teal Border)
};

// Task states
enum {
  FILTER_ALL = 0,
  FILTER_CAUGHT,
  FILTER_REQUIRED,
};

enum {
  STATE_INIT_GFX = 0,
  STATE_DRAW_HEADER,
  STATE_DRAW_LIST,
  STATE_HANDLE_INPUT,
  STATE_FILTER_MENU_OPEN,
  STATE_FILTER_MENU_INPUT,
  STATE_EXIT_FADE,
  STATE_EXIT_DONE,
  STATE_TOWN_MAP_LAUNCH
};

// ============================================================
// Data
// ============================================================

static EWRAM_DATA u16 sScrollOffset = 0;
static EWRAM_DATA u16 sSelectedRow = 0;
static EWRAM_DATA u16 sLastScrollOffset = 0xFFFF;
static EWRAM_DATA u16 sLastSelectedRow = 0xFFFF;
static EWRAM_DATA u8 sIconSpriteIds[TRACKER_VISIBLE_ROWS] = {0xFF, 0xFF, 0xFF,
                                                             0xFF};
static EWRAM_DATA u8 sWindowIds[WIN_COUNT] = {};
static EWRAM_DATA u8 sSearchBuffer[12] = {0};
static EWRAM_DATA u16 sFilteredPokemon[TRACKER_TOTAL_POKEMON] = {0};
static EWRAM_DATA u16 sFilteredCount = 0;
static EWRAM_DATA bool8 sInHeader = FALSE;
static EWRAM_DATA bool8 sKeepSearchQuery = FALSE;
static EWRAM_DATA bool8 sLastInHeader = FALSE;
static EWRAM_DATA u8 sHeaderColumn = 0;
static EWRAM_DATA u8 sFilterMode = FILTER_ALL;
static EWRAM_DATA bool8 sOnLocationButton = FALSE;
static EWRAM_DATA bool8 sLastOnLocationButton = FALSE;

static const struct BgTemplate sTrackerBgTemplates[] = {{.bg = 0,
                                                          .charBaseIndex = 0,
                                                          .mapBaseIndex = 31,
                                                          .screenSize = 0,
                                                          .paletteMode = 0,
                                                          .priority = 1,
                                                          .baseTile = 0},
                                                         {.bg = 1,
                                                          .charBaseIndex = 3,
                                                          .mapBaseIndex = 30,
                                                          .screenSize = 0,
                                                          .paletteMode = 0,
                                                          .priority = 0,
                                                          .baseTile = 0}};

static const struct WindowTemplate sTrackerWindowTemplates[] = {
  [WIN_MAIN] = {.bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 20,
        .paletteNum = 15,
        .baseBlock = 1},
  [WIN_FILTER] = {.bg = 1,
        .tilemapLeft = 14,
        .tilemapTop = 2,
        .width = 10,
        .height = 6,
        .paletteNum = 15,
        .baseBlock = 1},
  DUMMY_WIN_TEMPLATE
};
    

static const u8 sText_FilterAll_Menu[] = _("ALL");
static const u8 sText_FilterCaught_Menu[] = _("CAUGHT");
static const u8 sText_FilterRequired_Menu[] = _("REQUIRED");

static const struct MenuAction sFilterMenuActions[] = {
    {sText_FilterAll_Menu, NULL},
    {sText_FilterCaught_Menu, NULL},
    {sText_FilterRequired_Menu, NULL},
};

// Text color arrays: {background, foreground, shadow}
static const u8 sTextColor_Header[] = {1, 2, 3};    // White BG, Dark Grey Text
static const u8 sTextColor_Caught[] = {1, 6, 7};    // White BG, Green Text
static const u8 sTextColor_Uncaught[] = {1, 4, 5};  // White BG, Red Text
static const u8 sTextColor_Cursor[] = {1, 2, 3};    // White BG, Dark Grey Text
static const u8 sTextColor_Location[] = {8, 9, 10}; // Blue BG, White Text

// ============================================================
// Forward declarations
// ============================================================

static void Task_TrackerScreen(u8 taskId);
static void TrackerScreen_VBlankCB(void);
static void TrackerScreen_DrawHeader(void);
static void TrackerScreen_DrawList(void);
static void TrackerScreen_DrawRow(u16 dexNum, u8 row, bool8 isSelected);
static u16 TrackerScreen_GetCaughtCount(void);
static void CB2_TrackerScreen(void);

// ============================================================
// Main callback — sets up everything
// ============================================================

static void CB2_ReturnFromSearchNamingScreen(void) {
  sScrollOffset = 0;
  sSelectedRow = 0;
  sLastScrollOffset = 0xFFFF;
  sLastSelectedRow = 0xFFFF;
  CB2_InitTrackerScreen();
}
static void CB2_TrackerScreen(void) {
  RunTasks();
  AnimateSprites();
  BuildOamBuffer();
  UpdatePaletteFade();
}

void CB2_InitTrackerScreen(void) {
  int i;

  if (!sKeepSearchQuery) {
    memset(sSearchBuffer, EOS, sizeof(sSearchBuffer));
    sInHeader = FALSE;
    sHeaderColumn = 0;
    sFilterMode = FILTER_ALL;
    sScrollOffset = 0;
    sSelectedRow = 0;
    gTrackerSelectedSpecies = SPECIES_NONE;
  }
  sKeepSearchQuery = FALSE;
  FilterPokemonList();

  // Reset state
  
  sLastScrollOffset = 0xFFFF;
  sLastSelectedRow = 0xFFFF;

  // Standard cleanup
  SetVBlankCallback(NULL);
  SetHBlankCallback(NULL);

  // Clear VRAM, OAM, palette
  DmaFill16(3, 0, VRAM, VRAM_SIZE);
  DmaFill32(3, 0, OAM, OAM_SIZE);
  DmaFill16(3, 0, PLTT, PLTT_SIZE);

  // Reset sprite data
  ScanlineEffect_Stop();
  ResetTasks();
  ResetSpriteData();
  FreeAllSpritePalettes();

  // Set up BG
  ResetBgsAndClearDma3BusyFlags(0);
  InitBgsFromTemplates(0, sTrackerBgTemplates,
                       ARRAY_COUNT(sTrackerBgTemplates));

  // Init windows
  InitWindows(sTrackerWindowTemplates);
  DeactivateAllTextPrinters();

  LoadMonIconPalettes();
  LoadSpritePalette(&sSilhouetteSpritePalette);

  // Load our custom palette for the window and the background
  LoadPalette(sTrackerPalette, BG_PLTT_ID(15), sizeof(sTrackerPalette));
  LoadPalette(&sTrackerPalette[0], BG_PLTT_ID(0), 2); // Set outer background

  // Clear windows
  sWindowIds[WIN_MAIN] = WIN_MAIN;

  // Set up BG colors
  SetGpuReg(REG_OFFSET_BG0HOFS, 0);
  SetGpuReg(REG_OFFSET_BG0VOFS, 0);
  SetGpuReg(REG_OFFSET_BG1HOFS, 0);
  SetGpuReg(REG_OFFSET_BG1VOFS, 0);

  // Set up display
  SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
  ShowBg(0);
  ShowBg(1);

  // Initialize sprite IDs
  for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
    sIconSpriteIds[i] = 0xFF;
  }

  // Create task
  CreateTask(Task_TrackerScreen, 0);

  // Set VBlank
  SetVBlankCallback(TrackerScreen_VBlankCB);

  // Set main callback
  SetMainCallback2(CB2_TrackerScreen);

  // Start fade in
  BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
}

static void TrackerScreen_VBlankCB(void) {
  LoadOam();
  ProcessSpriteCopyRequests();
  TransferPlttBuffer();
}

// ============================================================
// Main task state machine
// ============================================================
static void Task_RestoreLocationButton(u8 taskId){
  gTasks[taskId].data[0]--;
  if(gTasks[taskId].data[0] <= 0) {
    sLastScrollOffset = 0xFFFF;
    TrackerScreen_DrawList();
    DestroyTask(taskId);
  }
}


struct SeviiStaticLocation{
  u16 species;
  u8 mapSecId;
};

static const struct SeviiStaticLocation sSeviiStaticLocations[] = {
  {SPECIES_LUGIA, MAPSEC_NAVEL_ROCK},
  {SPECIES_HO_OH, MAPSEC_NAVEL_ROCK},
  {SPECIES_DEOXYS, MAPSEC_BIRTH_ISLAND},
};

static bool32 IsTrackerSpeciesInEncounterTable(const struct WildPokemonInfo *info, u16 species, s32 count){
  s32 i;
  if(info != NULL){
    for(i = 0; i < count; i++){
      if(info->wildPokemon[i].species == species)
      return TRUE;
    }
  }
  return FALSE;
}

static bool32 IsTrackerSpeciesOnMap(const struct WildPokemonHeader *header, u16 species){
  if (IsTrackerSpeciesInEncounterTable(header->landMonsInfo, species, 12))
  return TRUE;
  if(IsTrackerSpeciesInEncounterTable(header->waterMonsInfo, species, 5))
  return TRUE;
  if(IsTrackerSpeciesInEncounterTable(header->fishingMonsInfo, species, 10))
  return TRUE;
  if(IsTrackerSpeciesInEncounterTable(header->rockSmashMonsInfo, species, 5))
  return TRUE;

  return FALSE;

}

static bool8 IsSpeciesSeviiOnly(u16 species){
  s32 i;
  bool8 foundAny = FALSE;
  if(species == SPECIES_NONE)
  return FALSE;

  for(i = 0; i < ARRAY_COUNT(sSeviiStaticLocations); i++){
    if(sSeviiStaticLocations[i].species == species){
       if(sSeviiStaticLocations[i].mapSecId < SEVII_MAPSEC_START)
       return FALSE;

       foundAny = TRUE;
    }
  }

  for(i = 0; gWildMonHeaders[i].mapGroup != 0xFF; i++){
    if(IsTrackerSpeciesOnMap(&gWildMonHeaders[i], species)){
      u8 mapSecId = Overworld_GetMapHeaderByGroupAndId(gWildMonHeaders[i].mapGroup, gWildMonHeaders[i].mapNum)->regionMapSectionId;

      if(mapSecId < SEVII_MAPSEC_START)
      return FALSE;

      foundAny = TRUE;
    }
  }
  return foundAny;
}

static void Task_TrackerScreen(u8 taskId) {
  s16 *data = gTasks[taskId].data;
  int i;

  switch (data[0]) {
  case STATE_INIT_GFX:
    // Fill BG1 with tile 0 (which is now completely transparent since charBaseIndex is 3)
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 32, 32);

    PutWindowTilemap(WIN_MAIN);
    CopyWindowToVram(WIN_MAIN, COPYWIN_FULL);

    CopyBgTilemapBufferToVram(0);
    CopyBgTilemapBufferToVram(1);

    data[0] = STATE_DRAW_HEADER;
    break;

  case STATE_DRAW_HEADER:
    TrackerScreen_DrawHeader();
    data[0] = STATE_DRAW_LIST;
    break;

  case STATE_DRAW_LIST:
    TrackerScreen_DrawList();
    data[0] = STATE_HANDLE_INPUT;
    break;

  case STATE_HANDLE_INPUT:
    if (gPaletteFade.active)
      break;

    if (JOY_NEW(B_BUTTON)) {
      PlaySE(SE_SELECT);
      BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
      data[0] = STATE_EXIT_FADE;
      break;
    }

    if (JOY_NEW(A_BUTTON)) {
      if (sInHeader) {
        if (sHeaderColumn == 0) {
          PlaySE(SE_SELECT);
          sKeepSearchQuery = TRUE;
          for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
            if (sIconSpriteIds[i] != 0xFF) {
              DestroyMonIcon(&gSprites[sIconSpriteIds[i]]);
              sIconSpriteIds[i] = 0xFF;
            }
          }
          FreeMonIconPalettes();
          FreeSpritePaletteByTag(0x8888);

          DoNamingScreen(NAMING_SCREEN_NICKNAME, sSearchBuffer, SPECIES_NONE, 0,
                         0, CB2_ReturnFromSearchNamingScreen);
          break;
        } else {
          PlaySE(SE_SELECT);
          data[0] = STATE_FILTER_MENU_OPEN;
          break;
        }
      } else{
        if(sOnLocationButton) {
          u16 species = NationalPokedexNumToSpecies(sFilteredPokemon[sSelectedRow]);
          if(!CheckBagHasItem(ITEM_TOWN_MAP, 1)){
            u8 newRow = sSelectedRow - sScrollOffset;
            u8 y = 20 + (newRow * TRACKER_ROW_HEIGHT);
            static const u8 sText_NotYet[] = _("NOT YET!");
            static const u8 sTextColor_NotYet[] = {8, 4, 3};
            u16 textWidth = GetStringWidth(FONT_NORMAL, sText_NotYet, 0);
            u16 xOffset = 120 + (60 - textWidth)/2;
            u8 restoreTaskId;
            PlaySE(SE_BOO);
            FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(8), 120, y + 8, 60, 16);
            AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, xOffset, y + 8, sTextColor_NotYet, 0xFF, sText_NotYet);
            CopyWindowToVram(WIN_MAIN, COPYWIN_FULL);
            restoreTaskId = FindTaskIdByFunc(Task_RestoreLocationButton);
            if (restoreTaskId != 0xFF) {
              DestroyTask(restoreTaskId);
            }
            restoreTaskId = CreateTask(Task_RestoreLocationButton, 8);
            gTasks[restoreTaskId].data[0] = 45;

          } else if (IsSpeciesSeviiOnly(species) && !FlagGet(FLAG_SYS_SEVII_MAP_123) && !FlagGet(FLAG_SYS_SEVII_MAP_4567)){
            u8 newRow = sSelectedRow - sScrollOffset;
            u8 y = 20 + (newRow * TRACKER_ROW_HEIGHT);
            static const u8 sText_NoMap[] = _("SEVII ONLY!");
            static const u8 sTextColor_NoMap[] = {8, 4, 3};
            u16 textWidth = GetStringWidth(FONT_SMALL, sText_NoMap, 0);
            u16 xOffset = 120 + (60 - textWidth)/2;
            u8 restoreTaskId;
            PlaySE(SE_BOO);
            
            FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(8), 120, y + 8, 60, 16);
            AddTextPrinterParameterized3(WIN_MAIN, FONT_SMALL, xOffset, y + 8, sTextColor_NoMap, 0xFF, sText_NoMap);
            CopyWindowToVram(WIN_MAIN, COPYWIN_FULL);

            restoreTaskId = FindTaskIdByFunc(Task_RestoreLocationButton);
            if(restoreTaskId != 0xFF){
              DestroyTask(restoreTaskId);
            }
            restoreTaskId = CreateTask(Task_RestoreLocationButton, 8);
            gTasks[restoreTaskId].data[0] = 90;
          } else{
            PlaySE(SE_SELECT);
            gTrackerSelectedSpecies = species;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            data[0] = STATE_TOWN_MAP_LAUNCH;
            break;
          }
        }
      }
    }

    if (JOY_REPT(DPAD_UP)) {
      if (sInHeader) {

      } else if (sSelectedRow > 0) {
        PlaySE(SE_SELECT);
        sSelectedRow--;
        if (sSelectedRow < sScrollOffset)
          sScrollOffset = sSelectedRow;
        TrackerScreen_DrawList();
      } else if (sScrollOffset > 0) {
        PlaySE(SE_SELECT);
        sScrollOffset--;
        TrackerScreen_DrawList();
      } else {
        PlaySE(SE_SELECT);
        sInHeader = TRUE;
        sOnLocationButton = FALSE;
        TrackerScreen_DrawList();
      }
    }

    else if (JOY_REPT(DPAD_DOWN)) {
      if (sInHeader) {
        if (sFilteredCount > 0) {
          PlaySE(SE_SELECT);
          sInHeader = FALSE;
          sSelectedRow = 0;
          sScrollOffset = 0;
          sOnLocationButton = FALSE;
          TrackerScreen_DrawList();
        }
      } else if (sSelectedRow < sFilteredCount - 1) {
        PlaySE(SE_SELECT);
        sSelectedRow++;
        if (sSelectedRow >= sScrollOffset + TRACKER_VISIBLE_ROWS)
          sScrollOffset = sSelectedRow - TRACKER_VISIBLE_ROWS + 1;
        TrackerScreen_DrawList();
      }
    } else if (JOY_NEW(DPAD_LEFT)) {
      if(sInHeader) {
        if(sHeaderColumn == 1){
          PlaySE(SE_SELECT);
          sHeaderColumn = 0;
          TrackerScreen_DrawList();
        }
      } else {
        if(sOnLocationButton) {
          PlaySE(SE_SELECT);
          sOnLocationButton = FALSE;
          TrackerScreen_DrawList();
        }
      }
    } else if (JOY_NEW(DPAD_RIGHT)) {
      if (sInHeader) {
        if (sHeaderColumn == 0) {
          PlaySE(SE_SELECT);
          sHeaderColumn = 1;
          TrackerScreen_DrawList();
        }
      } else{
        if(!sOnLocationButton && sFilteredCount > 0){
          PlaySE(SE_SELECT);
          sOnLocationButton = TRUE;
          TrackerScreen_DrawList();
        }
      }
    } else if (JOY_NEW(L_BUTTON)) {
      // Page up
      PlaySE(SE_SELECT);
      if (sSelectedRow >= TRACKER_VISIBLE_ROWS) {
        sSelectedRow -= TRACKER_VISIBLE_ROWS;
        if (sScrollOffset >= TRACKER_VISIBLE_ROWS)
          sScrollOffset -= TRACKER_VISIBLE_ROWS;
        else
          sScrollOffset = 0;
      } else {
        sSelectedRow = 0;
        sScrollOffset = 0;
      }
      TrackerScreen_DrawList();
    } else if (JOY_NEW(R_BUTTON)) {
      // Page down
      PlaySE(SE_SELECT);
      if (sSelectedRow + TRACKER_VISIBLE_ROWS < TRACKER_TOTAL_POKEMON) {
        sSelectedRow += TRACKER_VISIBLE_ROWS;
        sScrollOffset += TRACKER_VISIBLE_ROWS;
        if (sScrollOffset + TRACKER_VISIBLE_ROWS > TRACKER_TOTAL_POKEMON)
          sScrollOffset = TRACKER_TOTAL_POKEMON - TRACKER_VISIBLE_ROWS;
      } else {
        sSelectedRow = TRACKER_TOTAL_POKEMON - 1;
        sScrollOffset = TRACKER_TOTAL_POKEMON - TRACKER_VISIBLE_ROWS;
      }
      TrackerScreen_DrawList();
    }
    break;

  case STATE_FILTER_MENU_OPEN:
    FillWindowPixelBuffer(WIN_FILTER, PIXEL_FILL(1));
    PutWindowTilemap(WIN_FILTER);

    for (i = 0; i < ARRAY_COUNT(sFilterMenuActions); i++) {
      AddTextPrinterParameterized3(WIN_FILTER, FONT_NORMAL, 8, 2 + (i * 16), sTextColor_Header, 0xFF, sFilterMenuActions[i].text);
    }
    Menu_InitCursor(WIN_FILTER, FONT_NORMAL, 0, 2, 16, 3, sFilterMode);
    CopyWindowToVram(WIN_FILTER, COPYWIN_FULL);
    ScheduleBgCopyTilemapToVram(1);
    data[0] = STATE_FILTER_MENU_INPUT;
    break;

  case STATE_FILTER_MENU_INPUT: {
    s8 input = Menu_ProcessInput();
    switch (input) {
    case MENU_NOTHING_CHOSEN:
      break;
    case MENU_B_PRESSED:
      PlaySE(SE_SELECT);
      ClearStdWindowAndFrameToTransparent(WIN_FILTER, TRUE);
      data[0] = STATE_HANDLE_INPUT;
      TrackerScreen_DrawList();
      break;
    default:
      sFilterMode = input;
      ClearStdWindowAndFrameToTransparent(WIN_FILTER, TRUE);
      FilterPokemonList();
      sScrollOffset = 0;
      sSelectedRow = 0;
      sLastScrollOffset = 0xFFFF;
      sLastSelectedRow = 0xFFFF;
      data[0] = STATE_DRAW_LIST;
      break;
    }
  }
    break;

  case STATE_EXIT_FADE:
    if (!gPaletteFade.active)
      data[0] = STATE_EXIT_DONE;
    break;

  case STATE_EXIT_DONE: {
    int i;
    // Clean up windows
    ClearStdWindowAndFrameToTransparent(WIN_MAIN, TRUE);
    for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
      if (sIconSpriteIds[i] != 0xFF) {
        DestroySprite(&gSprites[sIconSpriteIds[i]]);
      }
    }
    FreeMonIconPalettes();
    FreeSpritePaletteByTag(0x8888);
    FreeAllWindowBuffers();
    DestroyTask(taskId);
    SetMainCallback2(gMain.savedCallback);
    break;
  }

  case STATE_TOWN_MAP_LAUNCH: {
    int i;
    if (!gPaletteFade.active) {
      for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
        if (sIconSpriteIds[i] != 0xFF) {
          DestroyMonIcon(&gSprites[sIconSpriteIds[i]]);
          sIconSpriteIds[i] = 0xFF;
        }
      }
      FreeMonIconPalettes();
      FreeSpritePaletteByTag(0x8888);
      FreeAllWindowBuffers();
      sKeepSearchQuery = TRUE;
      InitRegionMapWithExitCB(REGIONMAP_TYPE_NORMAL, CB2_InitTrackerScreen);
      DestroyTask(taskId);
    }
    break;
  }
  }
}

// ============================================================
// Drawing functions
// ============================================================

static bool8 IsSubstring(const u8 *name, const u8 *query) {
  int i, j;
  if (query[0] == EOS)
    return TRUE;
  for (i = 0; name[i] != EOS; i++) {
    for (j = 0; query[j] != EOS; j++) {
      u8 c1, c2;
      if (name[i + j] == EOS)
        break;
      c1 = name[i + j];
      c2 = query[j];

      if (c1 >= 0xBB && c1 <= 0xD4)
        c1 += 0x1A;
      if (c2 >= 0xBB && c2 <= 0xD4)
        c2 += 0x1A;

      if (c1 != c2)
        break;
    }

    if (query[j] == EOS)
      return TRUE;
  }
  return FALSE;
}

static void FilterPokemonList(void) {
  int i;
  sFilteredCount = 0;

  for (i = 0; i < ARRAY_COUNT(sFireRedAvailablePokemon); i++) {
    u16 dexNum = sFireRedAvailablePokemon[i];
    u16 species = NationalPokedexNumToSpecies(dexNum);
    if (species != SPECIES_NONE) {
      if (sSearchBuffer[0] == EOS ||
          IsSubstring(gSpeciesNames[species], sSearchBuffer)) {
        bool8 isCaught = GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT);
        if (sFilterMode == FILTER_ALL ||
            (sFilterMode == FILTER_CAUGHT && isCaught) ||
            (sFilterMode == FILTER_REQUIRED && !isCaught)) {
          sFilteredPokemon[sFilteredCount++] = dexNum;
        }
      }
    }
  }
}

static u16 TrackerScreen_GetCaughtCount(void) {
  u16 count = 0;
  u16 i;

  for (i = 0; i < TRACKER_TOTAL_POKEMON; i++) {
    u16 dexNum = sFireRedAvailablePokemon[i];
    u16 species = NationalPokedexNumToSpecies(dexNum);
    if (species != SPECIES_NONE && GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT))
      count++;
  }
  return count;
}

static void TrackerScreen_DrawHeader(void) {
  u8 text[32];
  u8 *ptr;
  u16 caughtCount = 0;
  u16 requiredCount;
  static const u8 sText_Search[] = _("SEARCH:");
  static const u8 sText_Filter[] = _("FILTER");
  static const u8 sText_Status[] = _("STATUS");
  static const u8 sText_Caught[] = _("CAUGHT: ");
  static const u8 sText_Required[] = _(" / REQUIRED: ");

  // Clear main window
  FillWindowPixelBuffer(WIN_MAIN, PIXEL_FILL(1));

  // Draw borders (Teal - Color 11)
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 0, 0, 240, 2);
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 0, 158, 240, 2);
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 0, 0, 2, 160);
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 238, 0, 2, 160);
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 0, 20, 240, 2);
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(11), 0, 138, 240, 2);

  // Header Columns
  ptr = StringCopy(text, sText_Search);
  if (sSearchBuffer[0] != EOS) {
    StringCopy(ptr, sSearchBuffer);
  }
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 16, 4, sTextColor_Header,
                               0xFF, text);
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 128, 4, sTextColor_Header,
                               0xFF, sText_Filter);
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 192, 4, sTextColor_Header,
                               0xFF, sText_Status);

  // Calculate caught
  caughtCount = TrackerScreen_GetCaughtCount();
  requiredCount = TRACKER_TOTAL_POKEMON - caughtCount;


  // Bottom text: CAUGHT: X / REQUIRED: Y
  ptr = StringCopy(text, sText_Caught);
  ptr = ConvertIntToDecimalStringN(ptr, caughtCount, STR_CONV_MODE_LEFT_ALIGN, 3);
  ptr = StringCopy(ptr, sText_Required);
  ptr = ConvertIntToDecimalStringN(ptr, requiredCount, STR_CONV_MODE_LEFT_ALIGN, 3);
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 16, 142, sTextColor_Header, 0xFF, text);
  
}

static void TrackerScreen_DrawList(void) {
  u8 i;
  u16 maxOffset;
  u16 dexNum;

  // Clamp scroll
  if (sFilteredCount > TRACKER_VISIBLE_ROWS)
    maxOffset = sFilteredCount - TRACKER_VISIBLE_ROWS;
  else
    maxOffset = 0;

  if (sScrollOffset > maxOffset)
    sScrollOffset = maxOffset;

  if (sLastScrollOffset != sScrollOffset) {
    // 1. Redraw entire canvas from scratch (extremely fast with buffer clear)
    TrackerScreen_DrawHeader();

    // 2. Sprite Shifting Logic
    for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
      if (sIconSpriteIds[i] != 0xFF) {
        DestroyMonIcon(&gSprites[sIconSpriteIds[i]]);
        sIconSpriteIds[i] = 0xFF;
      }
    }
    sLastScrollOffset = sScrollOffset;
    sLastSelectedRow = sSelectedRow;

    // 3. Draw all rows
    if (sFilteredCount == 0) {
      static const u8 sText_NoMatching[] = _("No matching Pokemon found.");
      AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 32, 60,
                                   sTextColor_Header, 0xFF, sText_NoMatching);
    } else {
      for (i = 0; i < TRACKER_VISIBLE_ROWS; i++) {
        if (sScrollOffset + i >= sFilteredCount)
          break;
        dexNum = sFilteredPokemon[sScrollOffset + i];
        TrackerScreen_DrawRow(
            dexNum, i, !sInHeader && (sScrollOffset + i) == sSelectedRow);
      }
    }
    if (sInHeader) {
      // Clear header cursors
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 4, 4, 12, 12);
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 116, 4, 12, 12);

      if (sHeaderColumn == 0) {
        AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 4, 4,
                                     sTextColor_Cursor, 0xFF,
                                     (const u8[]){CHAR_RIGHT_ARROW, EOS});
      } else {
        AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 116, 4,
                                     sTextColor_Cursor, 0xFF,
                                     (const u8[]){CHAR_RIGHT_ARROW, EOS});
      }
    }
  } else {
    // 4. WE DIDN'T SCROLL! Only update the cursor for the changed rows to save
    // CPU.
    if (sInHeader) {
      // Clear old list cursor if we transitioned to the header
      if (!sLastInHeader && sLastSelectedRow >= sScrollOffset &&
          sLastSelectedRow < sScrollOffset + TRACKER_VISIBLE_ROWS) {
        u8 oldRow = sLastSelectedRow - sScrollOffset;
        u8 oldY = 20 + (oldRow * TRACKER_ROW_HEIGHT);
        FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 8, oldY + 8, 16, 16);
      }
      // Clear header cursors
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 4, 4, 12, 12);
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 116, 4, 12, 12);

      // Draw active header cursor
      if (sHeaderColumn == 0) {
        AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 4, 4,
                                     sTextColor_Cursor, 0xFF,
                                     (const u8[]){CHAR_RIGHT_ARROW, EOS});
      } else {
        AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 116, 4,
                                     sTextColor_Cursor, 0xFF,
                                     (const u8[]){CHAR_RIGHT_ARROW, EOS});
      }
    } else {
      // Clear header cursors
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 4, 4, 12, 12);
      FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 116, 4, 12, 12);

      if (sSelectedRow != sLastSelectedRow || sLastInHeader != sInHeader || sOnLocationButton!= sLastOnLocationButton) {
        // Erase old cursor (only if the last state was not in the header)
        if (!sLastInHeader && sLastSelectedRow >= sScrollOffset && sLastSelectedRow < sScrollOffset + TRACKER_VISIBLE_ROWS) {
          u8 oldRow = sLastSelectedRow - sScrollOffset;
          u8 oldY = 20 + (oldRow * TRACKER_ROW_HEIGHT);
          FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 8, oldY + 8, 16, 16);
          FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 108, oldY + 8, 12, 16);
        }

        // Draw new cursor
        if (sSelectedRow >= sScrollOffset &&
            sSelectedRow < sScrollOffset + TRACKER_VISIBLE_ROWS) {
          u8 newRow = sSelectedRow - sScrollOffset;
          u8 newY = 20 + (newRow * TRACKER_ROW_HEIGHT);
          if (sOnLocationButton) {
            AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 108, newY + 8,
                                         sTextColor_Cursor, 0xFF,
                                         (const u8[]){CHAR_RIGHT_ARROW, EOS});
          } else {
            AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 8, newY + 8,
                                         sTextColor_Cursor, 0xFF,
                                         (const u8[]){CHAR_RIGHT_ARROW, EOS});
          }
        }
      }
      sLastSelectedRow = sSelectedRow;
      sLastOnLocationButton = sOnLocationButton;
    }
  }

  // Draw scroll indicators
  if (sScrollOffset > 0) {
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 220, 22,
                                 sTextColor_Header, 0xFF,
                                 (const u8[]){CHAR_UP_ARROW, EOS});
  } else {
    FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 220, 22, 8, 16);
  }

  if (sScrollOffset + TRACKER_VISIBLE_ROWS < sFilteredCount) {
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 220, 124,
                                 sTextColor_Header, 0xFF,
                                 (const u8[]){CHAR_DOWN_ARROW, EOS});
  } else {
    FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(1), 220, 124, 8, 16);
  }

  sLastInHeader = sInHeader;

  PutWindowTilemap(WIN_MAIN);
  CopyWindowToVram(WIN_MAIN, COPYWIN_FULL);
}

static void TrackerScreen_DrawRow(u16 dexNum, u8 row, bool8 isSelected) {
  u8 text[32];
  u16 species;
  bool8 isCaught;
  u8 y = 20 + (row * TRACKER_ROW_HEIGHT); // Screen Y (starts at Y=20)
  const u8 *statusColor;
  u16 textWidth;
  u16 xOffset;
  static const u8 sText_Location[] = _("LOCATION");

  species = NationalPokedexNumToSpecies(dexNum);
  if (species == SPECIES_NONE)
    return;

  isCaught = GetSetPokedexFlag(dexNum, FLAG_GET_CAUGHT);
  statusColor = isCaught ? sTextColor_Caught : sTextColor_Uncaught;

  // 1. Draw cursor indicator
  if (isSelected) {
    if(sOnLocationButton) {
      AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 108, y + 8, sTextColor_Cursor, 0xFF, (const u8[]){CHAR_RIGHT_ARROW, EOS});
    } else{
      AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 8, y + 8, sTextColor_Cursor, 0xFF, (const u8[]){CHAR_RIGHT_ARROW, EOS});
    }
  }

  // 2. Draw Sprite Icon (using absolute screen coordinates)
  if (sIconSpriteIds[row] == 0xFF) {
    sIconSpriteIds[row] =
        CreateMonIcon(species, SpriteCallbackDummy, 30, y + 8, 0, 0, TRUE);

    // Force priority to 0 so it stays in front of the white window (BG0,
    // priority 1)
    gSprites[sIconSpriteIds[row]].oam.priority = 0;

    // If not caught, override the sprite's palette slot with the silhouette
    // palette slot
    if (!isCaught) {
      u8 palIndex = IndexOfSpritePaletteTag(0x8888);
      if (palIndex != 0xFF) {
        gSprites[sIconSpriteIds[row]].oam.paletteNum = palIndex;
      }
    }
  }

  // 3. Draw Pokémon name
  GetSpeciesName(text, species);
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 50, y + 8,
                               sTextColor_Header, 0xFF, text);

  // 4. Draw LOCATION button
  textWidth = GetStringWidth(FONT_NORMAL, sText_Location, 0);
  xOffset = 120 + (60 - textWidth)/2;
  FillWindowPixelRect(WIN_MAIN, PIXEL_FILL(8), 120, y + 8, 60, 16);
  AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, xOffset, y + 8, sTextColor_Location, 0xFF, sText_Location);

  // 5. Draw caught status
  if (isCaught) {
    static const u8 sCaughtText[] = _("O");
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 196, y + 8, statusColor,
                                 0xFF, sCaughtText);
  } else {
    static const u8 sNotCaughtText[] = _("X");
    AddTextPrinterParameterized3(WIN_MAIN, FONT_NORMAL, 196, y + 8, statusColor,
                                 0xFF, sNotCaughtText);
  }
}