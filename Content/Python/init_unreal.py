import os
import unreal


ROOT = os.path.join(unreal.Paths.project_dir(), "Tools", "SourceTextures")
TEXTURE_PATH = "/Game/Blockland/Textures"
MATERIAL_PATH = "/Game/Blockland/Materials"
BLOCKS = ("Grass", "Dirt", "Stone", "Wood", "Leaves", "Sand", "Crystal", "Water", "Creature")


def import_texture(filename):
    source = os.path.join(ROOT, filename)
    destination = f"{TEXTURE_PATH}/{os.path.splitext(filename)[0]}"
    if not os.path.isfile(source):
        unreal.log_error(f"Blockland: texture sorgente mancante: {source}")
        return None

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", TEXTURE_PATH)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset = unreal.EditorAssetLibrary.load_asset(destination)
    if not asset:
        unreal.log_error(f"Blockland: impossibile importare {source}")
    elif filename.endswith("_Normal.png"):
        asset.set_editor_property(
            "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP
        )
        asset.set_editor_property("srgb", False)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def make_material(name, base_texture, normal_texture):
    asset_path = f"{MATERIAL_PATH}/M_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        f"M_{name}", MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not material:
        unreal.log_error(f"Blockland: impossibile creare M_{name}.")
        return
    base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -500, 0
    )
    base.set_editor_property("texture", base_texture)
    tint = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -500, 430
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tinted_base = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -180, 0
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        base, "RGB", tinted_base, "A"
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(
        tint, "", tinted_base, "B"
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        tinted_base, "", unreal.MaterialProperty.MP_BASE_COLOR
    )

    normal = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -500, 220
    )
    normal.set_editor_property("texture", normal_texture)
    normal.set_editor_property(
        "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_Normal
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        normal, "RGB", unreal.MaterialProperty.MP_NORMAL
    )

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -250, 430
    )
    roughness.set_editor_property("r", 0.42 if name == "Water" else 0.82)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "R", unreal.MaterialProperty.MP_ROUGHNESS
    )

    if name == "Crystal":
        glow = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -180, 0
        )
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -400, 440
        )
        strength.set_editor_property("r", 0.12)
        unreal.MaterialEditingLibrary.connect_material_expressions(
            base, "RGB", glow, "A"
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(
            strength, "", glow, "B"
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        unreal.log_error(f"Blockland: impossibile salvare M_{name}.")


def build_assets():
    unreal.EditorAssetLibrary.make_directory(TEXTURE_PATH)
    unreal.EditorAssetLibrary.make_directory(MATERIAL_PATH)
    missing = []
    for name in BLOCKS:
        base = import_texture(f"T_{name}_BaseColor.png")
        normal = import_texture(f"T_{name}_Normal.png")
        if base and normal:
            make_material(name, base, normal)
        else:
            missing.append(name)
    if missing:
        unreal.log_error(f"Blockland: materiali mancanti: {', '.join(missing)}")
    else:
        unreal.log("Blockland: texture voxel e materiali pronti.")


build_assets()
