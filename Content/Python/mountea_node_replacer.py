import unreal
import os
from node_definitions import NodeReplacementManager

class MounteaNodeReplacerWidget:
    @staticmethod
    def create_widget():
        widget_class_path = '/Game/Editor/Widgets/WBP_MounteaNodeReplacer'
        if not unreal.EditorAssetLibrary.does_asset_exist(widget_class_path):
            factory = unreal.EditorUtilityWidgetBlueprint()
            package_path = '/Game/Editor/Widgets'
            asset_name = 'WBP_MounteaNodeReplacer'
            if not unreal.EditorAssetLibrary.does_directory_exist(package_path):
                unreal.EditorAssetLibrary.make_directory(package_path)
            widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                asset_name, package_path, unreal.EditorUtilityWidgetBlueprint, factory
            )
            widget_class = widget.get_editor_property('parent_class')
            widget_class.add_function('ProcessSelectedAssets')
            widget_class.add_function('ProcessAllAssets')
            unreal.EditorAssetLibrary.save_loaded_asset(widget)
            return widget
        else:
            return unreal.load_asset(widget_class_path)

def create_bluetility_widget():
    widget = MounteaNodeReplacerWidget.create_widget()
    with unreal.SlatePythonBridge() as slate:
        main_box = slate.vertical_box()
        title = slate.text_block("Mountea Node Replacer")
        main_box.add_slot(title)
        process_selected_button = slate.button(
            "Process Selected Assets",
            tool_tip="Process only selected assets in Content Browser",
            on_clicked=lambda: process_selected_assets()
        )
        main_box.add_slot(process_selected_button)
        process_all_button = slate.button(
            "Process All Assets",
            tool_tip="Process all blueprints in the project",
            on_clicked=lambda: process_all_assets()
        )
        main_box.add_slot(process_all_button)
        status_text = slate.text_block("Ready")
        main_box.add_slot(status_text)
        widget.set_content(main_box)
    return widget

def process_selected_assets():
    selected_assets = unreal.EditorUtilityLibrary.get_selected_assets()
    if not selected_assets:
        unreal.log_warning("No assets selected!")
        return
    manager = NodeReplacementManager()
    config_loaded = load_configuration(manager)
    if not config_loaded:
        return
    replacements = manager.get_replacements()
    modified_count = 0
    for asset in selected_assets:
        if isinstance(asset, unreal.Blueprint):
            if process_blueprint(asset.get_path_name(), replacements):
                modified_count += 1
    unreal.log(f"Modified {modified_count} selected blueprints")

def process_all_assets():
    manager = NodeReplacementManager()
    config_loaded = load_configuration(manager)
    if not config_loaded:
        return
    replacements = manager.get_replacements()
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    all_assets = asset_registry.get_all_assets()
    modified_count = 0
    for asset in all_assets:
        if asset.asset_class_path.get_asset_name() == "Blueprint":
            if process_blueprint(asset.object_path, replacements):
                modified_count += 1
    unreal.log(f"Modified {modified_count} blueprints")

def load_configuration(manager):
    config_paths = [
        os.path.join(unreal.Paths.project_content_dir(), "Python/node_replacements.json"),
        os.path.join(unreal.Paths.project_content_dir(), "Python/node_replacements.yaml"),
    ]
    for path in config_paths:
        if os.path.exists(path):
            if path.endswith('.json'):
                manager.load_from_json(path)
            else:
                manager.load_from_yaml(path)
            return True
    unreal.log_error("No configuration file found!")
    return False

def add_menu_extension():
    menus = unreal.ToolMenus.get()
    main_menu = menus.find_menu("LevelEditor.MainMenu")
    if main_menu:
        entry = unreal.ToolMenuEntry(
            name="Python.MounteaNodeReplacer.Widget",
            type=unreal.MultiBlockType.MENU_ENTRY,
            insert_position=unreal.ToolMenuInsert("", unreal.ToolMenuInsertType.FIRST)
        )
        entry.set_label("Open Mountea Node Replacer")
        entry.set_tool_tip("Open the Mountea Node Replacer utility widget")
        entry.set_string_command(
            unreal.ToolMenuStringCommandType.PYTHON,
            custom_string="import mountea_node_replacer; mountea_node_replacer.create_bluetility_widget()"
        )
        main_menu.add_menu_entry("File", entry)
        menus.refresh_all_widgets()

def add_content_browser_menu_extension():
    menus = unreal.ToolMenus.get()
    content_browser_menu = menus.find_menu("ContentBrowser.AssetContextMenu")
    if content_browser_menu:
        entry = unreal.ToolMenuEntry(
            name="Python.MounteaNodeReplacer.ProcessSelected",
            type=unreal.MultiBlockType.MENU_ENTRY,
            insert_position=unreal.ToolMenuInsert("", unreal.ToolMenuInsertType.FIRST)
        )
        entry.set_label("Process Selected with Node Replacer")
        entry.set_tool_tip("Process selected assets with Mountea Node Replacer")
        entry.set_string_command(
            unreal.ToolMenuStringCommandType.PYTHON,
            custom_string="import mountea_node_replacer; mountea_node_replacer.process_selected_assets()"
        )
        content_browser_menu.add_menu_entry("AssetContextExtensions", entry)
        menus.refresh_all_widgets()