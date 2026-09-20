import subprocess
import sys
import os

def compile_shader(slangc_bin, input_path, output_path, stage, entry):
    cmd = [
        slangc_bin,
        input_path,
        "-profile",
        "spirv_1_0",
        "-target",
        "spirv",
        "-capability",
        "GLSL_330",
        "-o",
        output_path,
        "-stage",
        stage,
        "-entry",
        entry,
        "-emit-spirv-via-glsl",
    ]
    print(f"{input_path} ({stage}) -> {output_path}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"> Error compiling {input_path} ({stage}):\n{result.stderr}")
        sys.exit(1)

def compile_shaders(slangc_bin, src, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for file in sorted(os.listdir(src)):
        if file.endswith(".slang"):
            base = os.path.splitext(file)[0]
            input_path = os.path.join(src, file)
            output_vert = os.path.join(out_dir, f"{base}v.spv")
            output_frag = os.path.join(out_dir, f"{base}f.spv")

            compile_shader(slangc_bin, input_path, output_vert, "vertex", "vs_main")
            compile_shader(slangc_bin, input_path, output_frag, "fragment", "fs_main")

if __name__ == "__main__":
    slangc = sys.argv[1] if len(sys.argv) > 1 else "slangc"
    compile_shaders(slangc, "app/res/shaders/src", "app/res/shaders/bin")
    print("All shaders compiled successfully.")
