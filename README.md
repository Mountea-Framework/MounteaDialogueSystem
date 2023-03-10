# MounteaDialogueSystem
Mountea Dialogue System is an Open-source Mountea Framework Unreal Engine tool for creating (not just) complex dialogues within its own editor and validation system.

[![UE](https://img.shields.io/badge/supported-4.26%2B-green)](https://github.com/Mountea-Framework/MounteaDialogueSystem/edit/master/README.md#compatible-engine-version)
[![License](https://img.shields.io/github/license/Mountea-Framework/ActorInteractionPlugin)](https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/master/LICENSE)
[![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=flat&logo=youtube)](https://www.youtube.com/@mounteaframework)
[![Discord](https://badgen.net/discord/online-members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)
[![Discord](https://badgen.net/discord/members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)

## Discord Channel

[![Discord LINK](https://static.wikia.nocookie.net/siivagunner/images/9/9f/Discord_icon.svg/revision/latest?cb=20210814160101)](https://discord.gg/2vXWEEN)

Join support and community Discord!

## Compatible Engine Version

### Legend
Colour | Explanation
-------------- | --------------
🟩 | Supported
🟨 | WIP
🟪 | Deprecated (abaddoned)
🟥 | Partially supported ([Epic Games Policies](https://www.unrealengine.com/en-US/marketplace-guidelines#263d)) 

### Suport Table
Engine Version | Plugin Version | Support
-------------- | -------------- | ----
UE 4.26 | 0.0.0.1 | 🟩
UE 4.27 | - | 🟨
UE 5.00 | - | 🟨
UE 5.1 | - | 🟨

## Features
### Graph Editor
This plugin comes with its own Graph Editor! This way you can easily connect nodes and visualise the Dialogue flow even for the most complex graphs you can imagine.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/222853211-57874166-bf4c-4b46-9b5e-c10e513ab035.png">
</p>


#### Graph Editor Validations
We are just humans and we tend to do mistakes. This feature will try its best to track down all issues that might resolve in broken Dialogue Trees and inform you which node has what issues!
**Bonus:** Graph Validation reflects Themes! You can have Light or Dark one.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/219807417-a2fa5d61-70be-449d-bbab-d9a8e9ae4793.png">
</p>

#### Dialogue Previews
Select any node and see what is inside. No need to search where all the texts are, just click and see.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/219806723-bd0d407d-77ce-451a-a315-a01dd8f6409e.png">
</p>

#### Dialogue Decorators
Introducing *Decorators*, a powerful yet versitale tool of Blueprint based events that could be triggered per Dialogue Node! This provides a simple way to trigger cutscenes, switch cameras, end dialogues when being attacked or anything you like!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/222269825-c1e92517-7f13-4754-ad15-97d938b9c841.png">
</p>

#### Dialogue Node Details
A new feature droped in, and that is Node Style now supporting indications how many Decorators each Node implements and whether it inherits Decorators from Graph or not. This will come handy for debugging dialogues, showing that some nodes might not implement required Decorators!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/222851076-58c74403-2818-4048-9fd6-a374487aaad6.png">
</p>

## Installation

Download the plugin (either from Releases or from Marketplace). For more detailed guide take a look at Wiki page.

1. Download the branch release you are interested in
2. Instal the plugin to your Game Project (within /Plugin folder)

## Branches

* Master: A master branch, should be always relevant to most current Engine version and stable.
* Version Branch: A stable branch for specified Engine version.
* Version Brach_Dev: A development branch for specified Engine version.

## Additional Documentation and Acknowledgments

* Roadmap: -
* Documentation: -
