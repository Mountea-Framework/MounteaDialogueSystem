Current Version: 1.0.0.50

# Changelog

All notable changes to this project will be documented in this file.

**Version 0.0.0.8**
### Added
> - Added: Playing audio for Participants
> - Added: Switching Active Participant from Player to NPC
> - Added: Decorators World Context and Initialization
> - Added: Example Decorator

### Fixed
> - Fixed: Mispelled function names

### Changed
> - Changed: Icons now have shadows
> - Changed: Example Dialogue Tree

**Version 0.0.0.7**
### Added
> - Added: Node Decorators from array now can be deleted
> - Added: Node Tooltip for Graph Editor
> - Added: Evaluate Decorators for Graph
> - Added: Evaluate Decorators each Node
> - Added: Execute Decorators each Node

### Fixed
> - Fixed: Fixed Edit button opening Decorators with wrong Event name
> - Fixed: Broken Dialogue Graph Validation
> - Fixed: Broken Dialogue Node Validation
> - Fixed: Editor crash when multiple Nodes are selected in Graph Editor

### Changed
> - Changed: When creating Decorators from context menu, Tree menu is now in use
> - Changed: UI icons resized to max 128px
> - Changed: Unified Node style to be more professional
> - Changed: Removed all Player Controller code but showing Cursor and UI
> - Changed: Removed EvaluateDecorators from Blueprint Function Library

**Version 0.0.0.6**
### Added
> - Added: Node Decorators
> - Added: Context Menu updates to show Dialogue Assets
> - Added: Graph Based Decorators, Node based decorators
> - Added: Edit button to edit selected Decorator from an array
> - Added: Browse button to browse to selected Decorator from an array

### Fixed
> - Fixed: Fixed "Base", "Hidden" and "Private" categories not being hidden by default

### Changed
> - Changed: Graph Node base width from 110px to 145px
> - Changed: Graph Node displays Decorators information
> - Changed: Graph Node settings to toggle what Decorators info show

**Version 0.0.0.5**
### Added
> - Added: BPC_DialogueParticipant
> - Added: Dialogue UI
> - Added: Dialogue UI Materials
> - Added: Dialogue UI Textures
> - Added: Subtitles Settings in Project Settings
> - Added: Dialogue Participant States
> - Added: Dialogue Manager States

### Fixed
> - Fixed: Crash when tried to access Dialogue Node with no Data
> - Fixed: Diaogue Node GUID has been duplicated when copy pasted
> - Fixed: Dialogue Row GUID has been duplicated when copy pasted
> - Fixed: Dialogue Row Data has been duplicated when copy pasted

### Changed
> - Changed: Graph Cycles Detecion
> - Changed: Dialogue Structure now has Row Icon
> - Changed: Compelete Node(s) now have Dialogue Data, too, to allow "Goodby" diaogue lines etc.

**Version 0.0.0.4**
### Added
> - Added: Dialogue Manager Component
> - Added: Dialogue Context Object
> - Added: Dialogue Gameplay Settings

### Fixed
> - Fixed: -

### Changed
> - Changed: Removed all Dialogue flow functions from Dialogue Participant

**Version 0.0.0.3**
### Added
> - Added: Node Theme values in Settings
> - Added: Arrow Type values in Settings
> - Added: Node Class specific colours override

### Fixed
> - Fixed: Wiring thickness clamped
> - Fixed: Graph Validation text styling unified

### Changed
> - Changed: Removed Settings tab from Graph window
> - Changed: Graph Validation doesn't stack, only one Window for each Graph
> - Changed: Removed Settings button freom Graph toolbar
> - Changed: Node widget back to beveled shape

**Version 0.0.0.2**
### Added
> - Added: GitHub Changelog getter
> - Added: Mountea Dialogue Graph extensions: Validate Graph
> - Added: Mountea Dialogue Graph extensions Icons
> - Added: Welcome popup with Links
> - Added: Graph Validation popup with Links
> - Added: Mountea Dialogue Participant Component: component responsible for handling the Dialogues
> - Added: Mountea Dialogue Interface: abstracted source of Events and Functions for Mountea Dialogue Participants
> - Added: Graph and Node Validations for popup and in editor Validations

### Fixed
> - Fixed: Ability to Copy and Paste Start Nodes

### Changed
> - Changed: Hidden Connection Edge between nodes
> - Changed: Added Nodes Categories to separate Nodes in Graph Menu
> - Changed: Only one Start Node per Graph

**Version 0.0.0.1**
### Added 
> - Added: Context Browser menu extension: Mountea Dialogue
> - Added: Mountea Dialogue Graph Editor
> - Added: Mountea Dialogue Graph extensions: Auto Arrange

### Fixed
> - Fixed: -

### Changed
> - Changed: -