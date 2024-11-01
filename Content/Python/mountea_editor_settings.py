import unreal

@unreal.uclass()
class MounteaDialogueEditorSettings(unreal.EditorSettings):
    enable_node_replacer = unreal.uproperty(bool, config="Editor")
    node_replacer_config_path = unreal.uproperty(str, config="Editor")

    def __init__(self):
        super().__init__()
        self.enable_node_replacer = True
        self.node_replacer_config_path = "/MounteaDialogueSystem/Content/Config/node_replacements.json"

def register_settings():
    settings = unreal.get_default_object(MounteaDialogueEditorSettings)
    settings_container = unreal.SettingsContainer.get()
    settings_container.register_settings("Project", "Plugins", "MounteaDialogue", "Node Replacer", "Configure Mountea Node Replacer settings", settings)