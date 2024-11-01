import re
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
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        config_path = os.path.join(current_dir, "Config", "node_replacements.json")
        
        if os.path.exists(config_path):
            manager.load_from_json(config_path)
            unreal.log(f"Loaded configuration from: {config_path}")
            return True
            
        unreal.log_error(f"No configuration file found at: {config_path}")
        return False
        
    except Exception as e:
        unreal.log_error(f"Error loading configuration: {str(e)}")
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


def process_assets(asset_paths):
    if not asset_paths:
        unreal.log_warning("No assets provided!")
        return

    manager = NodeReplacementManager()
    config_loaded = load_configuration(manager)
    if not config_loaded:
        return

    replacements = manager.get_replacements()
    modified_count = 0

    for path in asset_paths:
        asset = unreal.load_asset(path)
        if asset and isinstance(asset, unreal.Blueprint):
            if process_blueprint(path, replacements):
                modified_count += 1

    unreal.log(f"Modified {modified_count} blueprints")
    
def process_blueprint(blueprint_path, replacements):
    try:
        # Get the asset
        blueprint = unreal.load_asset(blueprint_path)
        if not blueprint:
            unreal.log_warning(f"Could not load blueprint: {blueprint_path}")
            return False

        # Get text export of the blueprint
        export_options = unreal.AssetExportTask()
        export_options.asset = blueprint
        export_options.filename = "temp_blueprint_export.txt"
        export_options.replace_identical = True
        export_options.prompt = False
        export_options.automatic = True
        export_options.as_text = True

        if unreal.Exporter.run_asset_export_task(export_options):
            # Read the exported content
            with open(export_options.filename, 'r') as f:
                content = f.read()
            
            modified = False
            new_content = content
            
            for old_template, new_template in replacements:
                # Extract the essential identifying parts from templates
                old_class = re.search(r'Class=(.*?) Name=', old_template).group(1)
                old_function = re.search(r'FunctionReference=\(MemberParent=(.*?),MemberName="(.*?)"\)', old_template)
                old_parent = old_function.group(1)
                old_name = old_function.group(2)

                # Find all node definitions in the content
                node_pattern = r'Begin Object Class=' + re.escape(old_class) + r'.*?End Object'
                nodes = re.finditer(node_pattern, content, re.DOTALL)

                for node in nodes:
                    node_text = node.group(0)
                    # Verify this is the node we want to replace
                    if (old_parent in node_text and f'MemberName="{old_name}"' in node_text):
                        # Create new node text while preserving original node's connections
                        new_node_text = create_replacement_node(node_text, old_template, new_template)
                        # Replace the old node with the new one
                        new_content = new_content.replace(node_text, new_node_text)
                        modified = True

            if modified:
                # Import modified content back
                import_options = unreal.AssetImportTask()
                import_options.filename = "temp_blueprint_export_modified.txt"
                import_options.destination_path = os.path.dirname(blueprint_path)
                import_options.replace_existing = True
                import_options.automated = True
                
                # Write modified content
                with open(import_options.filename, 'w') as f:
                    f.write(new_content)
                
                # Import back
                unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([import_options])
                unreal.log(f"Modified blueprint: {blueprint_path}")
                
                # Cleanup temp files
                if os.path.exists(export_options.filename):
                    os.remove(export_options.filename)
                if os.path.exists(import_options.filename):
                    os.remove(import_options.filename)
                    
                return True
                
    except Exception as e:
        unreal.log_error(f"Error processing blueprint {blueprint_path}: {str(e)}")
    
    return False

def create_replacement_node(original_node, old_template, new_template):
    # Preserve the node's position
    pos_x = re.search(r'NodePosX=(\d+)', original_node)
    pos_y = re.search(r'NodePosY=(\d+)', original_node)
    
    new_node = new_template
    
    if pos_x and pos_y:
        new_node = re.sub(r'NodePosX=\d+', f'NodePosX={pos_x.group(1)}', new_node)
        new_node = re.sub(r'NodePosY=\d+', f'NodePosY={pos_y.group(1)}', new_node)

    # Preserve pin connections
    pins = re.finditer(r'CustomProperties Pin.*?(?=CustomProperties Pin|End Object)', original_node, re.DOTALL)
    for pin in pins:
        pin_text = pin.group(0)
        pin_id = re.search(r'PinId=([A-F0-9]+)', pin_text)
        if pin_id:
            # Find corresponding pin in new template and update its connections
            linked_to = re.search(r'LinkedTo=\((.*?)\)', pin_text)
            if linked_to:
                new_node = update_pin_connection(new_node, pin_id.group(1), linked_to.group(1))

    return new_node

def update_pin_connection(node_text, pin_id, linked_to):
    # Find the corresponding pin in the new node and update its LinkedTo property
    pin_pattern = f'CustomProperties Pin.*?PinId={pin_id}.*?(?=CustomProperties Pin|End Object)'
    return re.sub(pin_pattern, 
                 lambda m: m.group(0).replace('LinkedTo=()', f'LinkedTo=({linked_to})'),
                 node_text,
                 flags=re.DOTALL)