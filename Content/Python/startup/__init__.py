import unreal

def startup_initialize():
    try:
        from mountea_node_replacer import add_menu_extension, add_content_browser_menu_extension
        add_menu_extension()
        add_content_browser_menu_extension()
        unreal.log("Mountea Node Replacer UI initialized")
    except Exception as e:
        unreal.log_error(f"Failed to initialize UI: {str(e)}")

startup_initialize()