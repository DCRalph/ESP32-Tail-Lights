# ESP32 Tail Lights Menu System

This document describes the menu system implementation for the ESP32 Tail Lights project, which provides intuitive navigation and group management using 2 WS2812B status LEDs and 4 buttons.

## Hardware Requirements

- 2x WS2812B Status LEDs (statusLed1 and statusLed2)
- 4x Buttons:
  - **Boot** (BtnBoot) - Primary navigation and mode switching
  - **Previous** (BtnPrev) - Navigate backward through options
  - **Select** (BtnSel) - Modify values/confirm selections
  - **Next** (BtnNext) - Navigate forward through options

## Menu States

The system has three main states:

### 1. Normal Mode (Default)

- **LED Display**: Shows current application mode
  - **Green**: NORMAL mode
  - **Magenta**: TEST mode
  - **Blue**: REMOTE mode
  - **Red**: OFF mode
- **Button Functions**:
  - **Boot (1 click)**: Cycle through application modes
  - **Boot (2 clicks)**: Enter Group Info mode

### 2. Group Info Mode

- **LED Display**: Shows group status
  - **Solid Cyan**: Group master
  - **Blinking Cyan**: Group member
  - **Yellow Flash**: Not in group (searching)
- **Duration**: Automatically returns to Normal Mode after 3 seconds
- **Button Functions**:
  - **Boot (2 clicks)**: Enter Group Menu

### 3. Group Menu Mode

- **LED Display**: Different patterns for each menu option
- **Button Functions**:
  - **Boot (1 click)**: Execute selected action
  - **Boot (2 clicks)**: Return to Normal Mode
  - **Next**: Navigate to next menu option
  - **Prev**: Navigate to previous menu option
  - **Sel**: Modify values (when editing)

## Group Menu Options

### 1. Create Group

- **LED Pattern**: Green pulsing
- **Action**: Creates a new group with this device as master
- **Confirmation**: Green flash (3 times)

### 2. Join Group

- **LED Pattern**: Blue pulsing
- **Action**: Enter group ID editing mode
- **Editing Mode**:
  - **LED Pattern**: Alternating blue/white
  - **Navigation**: Next/Prev to move between digits
  - **Modification**: Sel to increment digit value
  - **Confirm**: Boot to join entered group ID
- **Confirmation**: Blue flash (3 times)

### 3. Leave Group

- **LED Pattern**: Red pulsing
- **Action**: Leave current group
- **Confirmation**: Red flash (3 times)

### 4. Auto-Join Toggle

- **LED Pattern**: Purple/White alternating
- **Action**: Toggle automatic group joining
- **Confirmation**: Purple flash (auto-join ON) or White flash (auto-join OFF)

### 5. Back to Normal

- **LED Pattern**: Orange blinking
- **Action**: Return to Normal Mode

## Usage Examples

### Creating a Group

1. **Boot (2 clicks)** → Enter Group Info mode
2. **Boot (2 clicks)** → Enter Group Menu
3. **Next/Prev** → Navigate to "Create Group" (green pulsing)
4. **Boot (1 click)** → Execute (creates group, green flash confirmation)

### Joining a Specific Group

1. **Boot (2 clicks)** → Enter Group Info mode
2. **Boot (2 clicks)** → Enter Group Menu
3. **Next/Prev** → Navigate to "Join Group" (blue pulsing)
4. **Boot (1 click)** → Enter editing mode (alternating blue/white)
5. **Next/Prev** → Move between digits
6. **Sel** → Increment current digit
7. **Boot (1 click)** → Confirm join (blue flash confirmation)

### Leaving a Group

1. **Boot (2 clicks)** → Enter Group Info mode
2. **Boot (2 clicks)** → Enter Group Menu
3. **Next/Prev** → Navigate to "Leave Group" (red pulsing)
4. **Boot (1 click)** → Execute (leaves group, red flash confirmation)

## Technical Implementation

### Menu State Management

- Menu state is stored in `Application::menuContext`
- State transitions are handled in `Application::handleMenuNavigation()`
- LED displays are managed by dedicated display functions

### Timing and Performance

- Group Info mode auto-returns after 3 seconds
- Sync LED updates are controlled and limited to 50ms intervals
- Only updates sync LED during Normal Mode to avoid interference

### Group ID Input

- Supports up to 8-digit group IDs
- Digit editing with wrap-around (0-9)
- Visual feedback shows current editing position

### Sync Integration

- Integrates with existing SyncManager for group operations
- Manages `updateSyncedLED()` calls to avoid conflicts
- Provides visual feedback for all sync operations

## Status LED Reference

| State                  | LED Pattern              | Meaning                       |
| ---------------------- | ------------------------ | ----------------------------- |
| Normal Mode            | Solid colors             | Application mode indicator    |
| Group Info - Master    | Solid cyan               | Device is group master        |
| Group Info - Member    | Blinking cyan            | Device is group member        |
| Group Info - Searching | Yellow flash             | Not in group, searching       |
| Create Group           | Green pulsing            | Menu option: Create group     |
| Join Group             | Blue pulsing             | Menu option: Join group       |
| Join Group (Editing)   | Blue/White alternating   | Editing group ID              |
| Leave Group            | Red pulsing              | Menu option: Leave group      |
| Auto-Join Toggle       | Purple/White alternating | Menu option: Toggle auto-join |
| Back to Normal         | Orange blinking          | Menu option: Return to normal |

## Error Handling

- Invalid group IDs (0) are rejected
- Menu timeouts prevent getting stuck in menu states
- Button click counters are properly reset to prevent accumulation
- Sync operations are only performed when appropriate

## Future Enhancements

- Add group discovery with visual list navigation
- Implement group member count display
- Add network signal strength indication
- Support for group naming/identification
- Advanced sync timing visualization
