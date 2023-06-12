# MounteaDialogueSystem
Mountea Dialogue System is an Open-source Mountea Framework Unreal Engine tool for creating (not just) complex dialogues within its own editor and validation system.

[![UE](https://img.shields.io/badge/supported-4.26%2B-green)](https://github.com/Mountea-Framework/MounteaDialogueSystem/edit/master/README.md#compatible-engine-version)
[![License](https://img.shields.io/github/license/Mountea-Framework/ActorInteractionPlugin)](https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/master/LICENSE)
[![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=flat&logo=youtube)](https://www.youtube.com/@mounteaframework)
[![Discord](https://badgen.net/discord/online-members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)
[![Discord](https://badgen.net/discord/members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)

## Discord Channel
<a href="https://discord.gg/2vXWEEN"><img src="https://static.wikia.nocookie.net/siivagunner/images/9/9f/Discord_icon.svg/revision/latest?cb=20210814160101" alt="Discord LINK" width="100" height="100"></a>

Join support and community Discord!

## Tutorial videos
<a href="https://www.youtube.com/playlist?list=PLIU53wA8zZmg5eBKEcpZr7G8JBBZ4QPKq"><img src="https://img.talkandroid.com/uploads/2016/06/youtube-logo-450x450.png" alt="TUTORIAL playlist" width="100" height="100"></a>

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
UE 4.26 | [1.0.0.426](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.426) | 🟥
UE 4.27 | [1.0.0.427](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.427) | 🟥
UE 5.00 | [1.0.0.50](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.50) | 🟩
UE 5.1 | [1.0.0.51](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.51) | 🟩
UE 5.2 | [1.0.0.52](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.52) | 🟩

## Features
### Graph Editor
This plugin comes with its own Graph Editor! This way you can easily connect nodes and visualise the Dialogue flow even for the most complex graphs you can imagine.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844580-a8947def-a4b6-4866-a0ac-d013b1b9d89a.png">
</p>

#### Graph Editor Validations
We are just humans and we tend to make mistakes. Graph Validator tries its best to track down all issues in the Editor before the project is packed, therefore saving a bunch of debugging hours for all parties included.
**Bonus:** Graph Validator reflects Graph Themes!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233845041-81b0f873-5aaa-4881-8e50-f80b4e66c45a.png">
</p>

#### Graph Themes Settings
Dialogue Graph Editor settings contain a lot of settings that might help developers tweak the Dialogue Graph Editor to their liking.

#### Dialogue Previews
To save precious time, Dialogue Previews show the raw context of the Dialogue Data. What is this helpful for? Well, changing Dialogue Data automatically updates the Previews, showing what data will be played for this Node.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844783-95be9ace-f612-4448-8a4a-715969237fc1.png">
</p>

#### Dialogue Decorators
Introducing *Decorators*, a powerful yet versitale tool of Blueprint based events that could be triggered per Dialogue Node! This provides a simple way to trigger cutscenes, switch cameras, end dialogues when being attacked or anything you like!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844834-a3e7522f-b38c-49b7-9dcb-e3725f3a5208.png">
</p>

#### Dialogue Node Details
A new feature droped in, and that is Node Style now supporting indications how many Decorators each Node implements and whether it inherits Decorators from Graph or not. This will come handy for debugging dialogues, showing that some nodes might not implement required Decorators!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844886-4414e2f6-788d-48da-a643-8b275c083f09.png">
</p>

## Installation

Download the plugin (either from Releases or from Marketplace). For more detailed guide take a look at Wiki page.

1. Download the branch release you are interested in
2. Instal the plugin to your Game Project (within /Plugin folder)

## Branches

* Master: A master branch, should be always relevant to most current Engine version and stable.
* Version Branch: A stable branch for specified Engine version.
* Version Brach_Dev: A development branch for specified Engine version.
