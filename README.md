[![Documentation](https://img.shields.io/badge/documentation-github?style=flat&logo=GitHub&labelColor=5a5a5a&color=98c510)](https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Getting-Started)
[![Marketplace](https://img.shields.io/badge/marketplace-epicgames?style=flat&logo=UnrealEngine&labelColor=5a5a5a&color=98c510)](https://www.unrealengine.com/marketplace/en-US/product/ea38ae1f87b24807a66fdf4fa65ef521)
![UE](https://img.shields.io/badge/supported-4.26%2B-green)
[![license](https://img.shields.io/badge/license-Apache%20License%20++-99c711?labelColor=555555&style=flat&link=https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/master/LICENSE)](https://github.com/Mountea-Framework/MounteaDialogueSystem/blob/master/LICENSE)
[![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=flat&logo=youtube)](https://www.youtube.com/@mounteaframework)
[![Discord](https://badgen.net/discord/online-members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)
[![Discord](https://badgen.net/discord/members/2vXWEEN?label=Discord&logo=discord&logoColor=ffffff&color=7389D8)](https://discord.com/invite/2vXWEEN)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/345048cfc8794b4191a400c84703100e)](https://app.codacy.com/gh/Mountea-Framework/MounteaDialogueSystem/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

# MounteaDialogueSystem
Mountea Dialogue System is an Open-source Mountea Framework Unreal Engine tool for creating (not just) complex dialogues within its own editor and validation system.

> [!IMPORTANT]
> **PLAYEBALE DEMO**
> 
> Feel free to download the playable demo and take a look at what this plugin (in combination with its Dialogue sibling) can do in a matter of a few clicks!
> <p align="center" width="100%">    <a href="https://drive.google.com/file/d/1XS6meFqXbDM_YfbjQijcco6q1O1wl9-J/view?usp=drive_link">        <img width="20%" src="https://github.com/Mountea-Framework/MounteaInteractionSystem/assets/37410226/da51eced-84e4-4c96-a9ff-cd5a03055d64">    </a></p>

> [!IMPORTANT]
> **DISCORD CHANNEL**
> 
> <a href="https://discord.gg/2vXWEEN"><img src="https://anbhelp.zendesk.com/hc/article_attachments/360021176052/discord.gif" alt="Discord LINK" width="100"></a>

> [!IMPORTANT]
> **DOCUMENTATION**
>
> Online [Documentation](https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Getting-Started) including a Quick Start guide which will help you set up the plugin in a few easy-to-follow steps.
> 
> <a href="https://discord.gg/2vXWEEN"><img src="https://media.tenor.com/mTz7WCbH92oAAAAj/book-transparent.gif" alt="DOCUMENTATION" width="100"></a>

> [!IMPORTANT]
> **TUTORIAL VIDEOS**
>
> <a href="https://www.youtube.com/playlist?list=PLIU53wA8zZmg5eBKEcpZr7G8JBBZ4QPKq"><img src="https://cdnl.iconscout.com/lottie/premium/thumb/youtube-8622218-6842833.gif" alt="TUTORIAL playlist" width="150"></a>

## Compatible Engine Version

### Legend
Colour | Explanation
-------------- | --------------
🟩 | Supported
🟨 | WIP
🟪 | Deprecated (abandoned)
🟥 | Partially supported ([Epic Games Policies](https://www.unrealengine.com/en-US/marketplace-guidelines#263d)) 

### Support Table
Engine Version | Plugin Version | Support
-------------- | -------------- | ----
UE 4.26 | [1.0.0.426](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.426) | 🟪
UE 4.27 | [1.0.0.427](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.427) | 🟪
UE 5.00 | [1.0.0.50](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/1.0.0.50) | 🟪
UE 5.1 | [3.0.0.51](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/3.0.0.51) | 🟥
UE 5.2 | [3.0.0.52](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/3.0.0.52) | 🟩
UE 5.3 | [3.0.0.53](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/3.0.0.53) | 🟩
UE 5.4 | [3.0.0.54](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/3.0.0.54) | 🟩
UE 5.5 | [3.0.0.55](https://github.com/Mountea-Framework/MounteaDialogueSystem/releases/tag/3.0.0.55) | 🟩

## Features
### Graph Editor
This plugin comes with its own Graph Editor! This way you can easily connect nodes and visualise the Dialogue flow even for the most complex graphs you can imagine.
<p align="center" width="100%">
    <img width="65%" src="https://github.com/user-attachments/assets/7d8f16ee-7336-4215-a9e7-d1e538c9080f">
</p>

#### Graph Editor Validations
We are just humans and we tend to make mistakes. Graph Validator tries its best to track down all issues in the Editor before the project is packed, therefore saving a bunch of debugging hours for all parties included. Validation system is clever enough to provide contextual information which part of the Dialogue is not correct.
**Bonus:** Graph Validator reflects Graph Themes!
<p align="center" width="100%">
    <img width="65%" src="https://github.com/user-attachments/assets/77d1b87c-dad7-42c0-a2d7-c84ccfaec415">
</p>

#### Graph Themes Settings
Dialogue Graph Editor settings contain a lot of settings that might help developers tweak the Dialogue Graph Editor to their liking.

#### Dialogue Previews
To save precious time, Dialogue Previews show the raw context of the Dialogue Data. What is this helpful for? Well, changing Dialogue Data automatically updates the Previews, showing what data will be played for this Node.
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844783-95be9ace-f612-4448-8a4a-715969237fc1.png">
</p>

#### Dialogue Graph Preview (Runtime)
Inspect Graph progress in runtime per-participant per-PIE instance.
<p align="center" width="100%">
    <img width="65%" src="https://github.com/user-attachments/assets/79c316a0-08c5-4d0d-87cc-858e9580fa8a">
</p>

#### Dialogue Decorators
Introducing *Decorators*, a powerful yet versatile tool of Blueprint-based events that could be triggered per Dialogue Node! This provides a simple way to trigger cutscenes, switch cameras, end dialogues when being attacked or anything you like!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844834-a3e7522f-b38c-49b7-9dcb-e3725f3a5208.png">
</p>

#### Dialogue Node Details
A new feature dropped in, and that is Node Style now supporting indications of how many Decorators each Node implements and whether it inherits Decorators from Graph or not. This will come handy for debugging dialogues, showing that some nodes might not implement required Decorators!
<p align="center" width="100%">
    <img width="65%" src="https://user-images.githubusercontent.com/37410226/233844886-4414e2f6-788d-48da-a643-8b275c083f09.png">
</p>

#### Editor Runtime Debug
A piece of helpful information is displayed in the Editor and Debug Builds alike.
<p align="center" width="100%">
    <img width="65%" src="https://github.com/Mountea-Framework/MounteaDialogueSystem/assets/37410226/00d8004c-bbef-4c76-b1df-2b024a4e857b">
</p>

## Online Editor Tool
We're thrilled to announce a game-changing addition to our toolkit: the new standalone dialogue builder! You can now easily check it out [HERE](https://mountea-framework.github.io/MounteaDialoguer/)! ✨

**Dialoguer** is a powerful tool that allows you to create, manage, and refine dialogues like never before. This tool isn't just built for Unreal Engine - it's designed to export dialogues for any engine and media format. Seamlessly import your dialogues into Unreal and export them back for other platforms, making your creative process more flexible and efficient than ever!
<p align="center" width="100%">
    <img width="65%" src="https://github.com/user-attachments/assets/3ef3c8ca-325c-45e9-a68c-0e3d436dd97c">
</p>

## Installation

Download the plugin (either from Releases or from Marketplace). For a more detailed guide take a look at the Wiki page.

1. Download the branch release you are interested in
2. Instal the plugin to your Game Project (within /Plugin folder)

## Branches

* Master: A master branch, should always be relevant to the most current Engine version and stable.
* Version Branch: A stable branch for specified Engine version.
* Version Brach_Dev: A development branch for a specified Engine version.

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=Mountea-Framework/MounteaDialogueSystem&type=Date)](https://star-history.com/#Mountea-Framework/MounteaDialogueSystem&Date)

