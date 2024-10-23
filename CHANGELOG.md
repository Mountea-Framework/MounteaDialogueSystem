Current Version: 2.0.3.54

# Changelog

All notable changes to this project will be documented in this file.

**Version 2.0.4**
### Added
> - Graph Scoped Decorators
>   - Enables decorators to be scoped to run only when Graph starts
>   - Helps prevent unintended effects across multiple nodes
> - Option to Enable logging channels
>   - Allows selective activation of logging channels for better debugging
>   - NetMode logging information
>      - Additional logging details to track network replication states
> - Execution order to Nodes
>   - Nodes now have explicit execution order control
>   - Ensures predictable flow in complex dialogue graphs
> - New Delay Node `VERY EXPERIMENTAL`
>   - Introduces a pause in Graph execution
>   - Currently experimental, use with caution!
> - Hidden variables for StartNode, ReturnToNode, and DelayNode in graph editor
>   - Hidden in editor to reduce visual clutter
> - Toolbar Button updated with Dialogue buttons separated from Interaction ones
> - Decorators now have `Blacklisted Node`, showing an error message if attached to such
>   - Prevents decorators from attaching to incompatible nodes

### Fixed
> - Decorators evaluation (First Time Only)
>   - Fixed issue with decorators not evaluating correctly for `OnlyOnce` type of Decorators
> - Graph Validation
>   - Improved validation to catch more potential issues
>   - Graph errors are now shown in PIE, too
> - `IsValid` function to validate DialogueRow
>   - Function now properly checks DialogueRow for validity
> - OptionsContainer not propagating OptionClass properly
>   - Now correctly propagates the `OptionClass`
> - `PIE` bug where engine crashed when hovered over Node in Graph (while in breakpoint)
>   - Resolved crash occurring during debugging in Play-In-Editor (PIE)
> - `PIE` bug where Node title was not visible in Graph (while in breakpoint)
>   - Node titles now remain visible during debugging in PIE

### Updated
> - Graph connection visual
>   - Improved visuals for connections between nodes.
> - Sorting algorithm to respect Execution Order
>   - Algorithm updated to ensure nodes follow the correct execution sequence.
> - Enabled (almost) 0s dialogue durations
>   - Allows rapid transitions with very short or nearly zero-duration dialogues.

**Version 2.0.3**
### Added 
> - Typewriter Effect
>   - Effect is in `Dialogue Row`
>   - Effect can be turned on/off using Decorator
> - `Import/Export` logic `(THIS FEATURE MIGHT BE UNSTABLE)`
>   - Every Dialogue can be export as `mnteadlg` file
>   - Dialogue can be import to `Mountea Dialoguer` tool and reimported back again
>   - New config file in Project Settings contains all import info for reimports
> - Right-click menu now contains `Dialogue Data table shortcut`
> - Z-Order value in Dialogue Widget
>   - Manager has Getter/Setter functions for updating the Z-Order
> - Auto Gameplay Tags download
>   - This way default Gameplay Tags will always be present
> - Added helper function `GetObjectByClass` which can input Object, Class and will return Object of the selected class (instead of casting)
> - Added custom K2Nodes visuals

### Fixed
> - Fixed `WBP_DialoguerRow_WithIcon` missing reference 
> - Fixed functions documentation to mention Player `State` instead of Controller
> - Fixed dialogue initialization crash with Saved Node
>   - If Participant has Saved Node which is from different Graph than the current one, then Dialogue ignores Saved Node and continues from start
> - Fixed Decorators initialization
>   - Ensured all Decorators have Manager and Participant before validation
> - Fixed broken Input Actions and Mappings

### Changed
> - `Traversed Path` now contains Graph ID
> - Right-click menu updated
>   - Menu is now on top
> - `AllowOnlyFirstTime` decorator will result in failed validation if attached to `first` dialogue node after start node
> - Toolbar Ribbon buttons reworked
>   - Dropdown menu now offers more links with better descriptions
> - Updated welcome screen and validation screen with updated icons
> - `InitializeDialogue` function from Function Library has been renamed to `Advanced Dialogue Initialization`
> - All functions are now categorized by `Class`
>   - Manager function are in `Mountea|Dialogue|Manager` section etc.

**Version 2.0.0**
### Added
> - Dialogue Nodes can be created in blueprints
>   - That means you can create your own Nodes!
> - Dialogue Graph, Node and Decorator will have ***Tick*** function!
>   - Carefully narrated to the Active Node and Decorators only for performance saving
> - ***Replication*** of base Dialogue stats
>   - States, Progress etc.
> - Dialogue UI will be slightly changed to allow custom Widgets
>   - All UI is Interface based now, so calling Interface functions instead of class-based functions to allow swapping WBPs
> - Default Gameplay Tags
> - Row Duration `Auto` is now editable from Config (default is `8`)
>   - This value defines how many seconds it takes to wait per 100 words
>     - 300 words long text would wait `( 300 * 8 ) / 100`
> - `Invoke Dialogue UI` function is now exposed to Blueprints
> - Typewriter effect will be added for UI
>   - UI will show text character by character
> - Added new Editor/Debug Build logging to Console and Screen
> - Added `bIsStoppingRow` variable to Dialogue Row Data to enable dialogue rows from being executed
> - Replace regex in Text so you can now use formatted texts with possibility to have parts replace
>   - Example "Oh, isn't that the famous {name} who alone won the cup?"
>     - `{name}` can be fed to regex replacement function with any input

### Fixed
> - That annoying fix where the graph crashes if using Gameplay Tags

### Changed
> - ***Traverse Path*** is now Structure instead of TMap
>   - This is ***breaking*** change.
> - Dialogue Node and pre-defined Decorators require `FDialogueRow` DataTable
> - Dialogue Decorators don't have drop arrow in the list anymore - replaced with `Delete` button

**Version 1.0.6**
### Added
> - Added: Advanced Wiring Style for Nodes
> - Added: More robust logs for `InitializeDialogue`
> - Added: New Dialogue Row Data Duration Mode `Manual` to allow Manual skipping of Row Datas
> - Added: New and nicer Menu Selector

### Fixed
> - Fixed: Editor Crash fix for Auto Arrange
> - Fixed: Dialogue Row UI now support Class/ID coloring settings
> - Fixed: Missing `Additional Data` class Description

### Changed
> - Changed: Additional Dialogue Data is now `DataAsset` instead of Class Picker
> - Changed: Lead Node now can point to multiple Answer Nodes
> - Changed: Answer Node now can point to multiple Lead Nodes

**Version 1.0.5**
### Added
> - Added: Added support for Gameplay Tags (beta)

### Fixed
> - Fixed: Editor Crash fix
> - Fixed: Interfaces C++ access fix
> - Fixed: OverrideParticipants Decorator Fix
> - Fixed: Linux/Mac Build fix

### Changed
> - Changed: Exposed `bDoesAutoStart` for Dialogue Nodes

**Version 1.0.0**
### Added
> - Added: Mountea Dialogue Graph
> - Added: Mountea Dialogue Node
> - Added: Mountea Dialogue Graph Editor
> - Added: Mountea Dialogue Graph Validator
> - Added: Mountea Dialogue Graph Arranger Alpha
> - Added: Mountea Dialogue Decorators

### Fixed
> - Fixed: -

### Changed
> - Changed: -