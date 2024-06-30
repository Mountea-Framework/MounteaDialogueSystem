Current Version: 1.0.6.51

# Changelog

All notable changes to this project will be documented in this file.

**Version 2.0.0.51**

### Added
> - Dialogue Nodes can be created in blueprints
>   - That means you can create your own Nodes!
> - Dialogue Graph, Node and Decorator will have **Tick** function!
>   - Carefully narrated to the Active Node and Decorators only for performance saving
> - **Replication** of base Dialogue stats
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
> - **Traverse Path** is now Structure instead of TMap
>   - This is ***breaking*** change.
> - Dialogue Node and pre-defined Decorators require `FDialogueRow` DataTable
> - Dialogue Decorators don't have drop arrow in the list anymore - replaced with `Delete` button

**Version 1.0.6.51**
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

**Version 1.0.5.51**
### Added
> - Added: Added support for Gameplay Tags (beta)

### Fixed
> - Fixed: Editor Crash fix
> - Fixed: Interfaces C++ access fix
> - Fixed: OverrideParticipants Decorator Fix
> - Fixed: Linux/Mac Build fix

### Changed
> - Changed: Exposed `bDoesAutoStart` for Dialogue Nodes

**Version 1.0.0.51**
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